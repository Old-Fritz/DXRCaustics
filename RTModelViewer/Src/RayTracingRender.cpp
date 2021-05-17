#include "RTModelViewer.h"

const char* rayTracingModes[] = {
	"Off",
	"Bary Rays",
	"Refl Bary",
	"Shadow Rays",
	"Diffuse&ShadowMaps",
	"Diffuse&ShadowRays",
	"Reflection Rays"
};
EnumVar rayTracingMode("Viewer/Raytracing/RayTraceMode", RTM_OFF, _countof(rayTracingModes), rayTracingModes);

void UpdateHitShaderConstants(HitShaderConstants& hitShaderConstants, const GlobalConstants& globalConstants)
{
	hitShaderConstants.SunShadowMatrix = globalConstants.SunShadowMatrix;
	hitShaderConstants.ViewerPos = globalConstants.CameraPos;
	hitShaderConstants.SunDirection = globalConstants.SunDirection;
	hitShaderConstants.SunIntensity = globalConstants.SunIntensity;
	hitShaderConstants.AmbientIntensity = globalConstants.AmbientIntensity;
	hitShaderConstants.ShadowTexelSize = globalConstants.ShadowTexelSize;
	hitShaderConstants.InvTileDim = globalConstants.InvTileDim;
	hitShaderConstants.TileCount = globalConstants.TileCount;
	hitShaderConstants.FirstLightIndex = globalConstants.FirstLightIndex;
	hitShaderConstants.ModelScale = g_ModelScale;
	hitShaderConstants.IsReflection = false;
	hitShaderConstants.UseShadowRays = false;
}

void UpdateDynamicConstants(DynamicCB& inputs, const Math::Camera& camera, ColorBuffer& colorTarget)
{

	auto m0 = camera.GetViewProjMatrix();
	auto m1 = Transpose(Invert(m0));
	memcpy(&inputs.cameraToWorld, &m1, sizeof(inputs.cameraToWorld));
	memcpy(&inputs.worldCameraPosition, &camera.GetPosition(), sizeof(inputs.worldCameraPosition));
	inputs.resolution.x = (float)colorTarget.GetWidth();
	inputs.resolution.y = (float)colorTarget.GetHeight();
}


void Raytracebarycentrics(
	CommandContext& context, 
	const GlobalConstants& globalConstants,
	const Math::Camera& camera,
	ColorBuffer& colorTarget)
{
	ScopedTimer _p0(L"Raytracing barycentrics", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);

	context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));
	context.WriteBuffer(g_dynamicConstantBuffer, 0, &inputs, sizeof(inputs));

	context.TransitionResource(g_hitConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(g_dynamicConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(colorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	context.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.FlushResourceBarriers();

	ID3D12GraphicsCommandList* pCommandList = context.GetCommandList();

	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature);
	pCommandList->SetComputeRootDescriptorTable(0, g_SceneSrvs);
	pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV);
	pRaytracingCommandList->SetComputeRootShaderResourceView(7, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Primarybarycentric].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pRaytracingCommandList->SetPipelineState1(g_RaytracingInputs[Primarybarycentric].m_pPSO);
	pRaytracingCommandList->DispatchRays(&dispatchRaysDesc);
}

void RaytracebarycentricsSSR(
	CommandContext& context,
	const GlobalConstants& globalConstants,
	const Math::Camera& camera,
	ColorBuffer& colorTarget,
	DepthBuffer& depth,
	ColorBuffer& normals)
{
	ScopedTimer _p0(L"Raytracing SSR barycentrics", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);

	context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));

	ComputeContext& ctx = context.GetComputeContext();
	ID3D12GraphicsCommandList* pCommandList = context.GetCommandList();

	ctx.WriteBuffer(g_dynamicConstantBuffer, 0, &inputs, sizeof(inputs));
	ctx.TransitionResource(g_dynamicConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	ctx.TransitionResource(g_hitConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	ctx.TransitionResource(normals, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(colorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	ctx.FlushResourceBarriers();

	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature);
	pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV);
	pCommandList->SetComputeRootDescriptorTable(3, g_LightingSrvs);
	pRaytracingCommandList->SetComputeRootShaderResourceView(7, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Reflectionbarycentric].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pRaytracingCommandList->SetPipelineState1(g_RaytracingInputs[Reflectionbarycentric].m_pPSO);
	pRaytracingCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceShadows(
	GraphicsContext& context,
	const GlobalConstants& globalConstants,
	const Math::Camera& camera,
	ColorBuffer& colorTarget,
	DepthBuffer& depth)
{
	ScopedTimer _p0(L"Raytracing Shadows", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);

	context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));

	ComputeContext& ctx = context.GetComputeContext();
	ID3D12GraphicsCommandList* pCommandList = context.GetCommandList();

	ctx.WriteBuffer(g_dynamicConstantBuffer, 0, &inputs, sizeof(inputs));
	ctx.TransitionResource(g_dynamicConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	ctx.TransitionResource(g_SceneNormalBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(g_hitConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	ctx.TransitionResource(depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ctx.TransitionResource(colorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	ctx.FlushResourceBarriers();

	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature);
	pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer->GetGPUVirtualAddress());
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV);
	pCommandList->SetComputeRootDescriptorTable(3, g_LightingSrvs);
	pRaytracingCommandList->SetComputeRootShaderResourceView(7, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Shadows].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pRaytracingCommandList->SetPipelineState1(g_RaytracingInputs[Shadows].m_pPSO);
	pRaytracingCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceDiffuse(
	GraphicsContext& context,
	const GlobalConstants& globalConstants,
	const Math::Camera& camera,
	ColorBuffer& colorTarget)
{
	ScopedTimer _p0(L"RaytracingWithHitShader", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);
	hitShaderConstants.UseShadowRays = rayTracingMode == RTM_DIFFUSE_WITH_SHADOWRAYS;

	context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));
	context.WriteBuffer(g_dynamicConstantBuffer, 0, &inputs, sizeof(inputs));

	context.TransitionResource(g_dynamicConstantBuffer,			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(g_SSAOFullScreen,				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(g_hitConstantBuffer,				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(g_ShadowBuffer,					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(colorTarget,						D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// lighting
	context.TransitionResource(Lighting::m_LightBuffer,			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(Lighting::m_LightShadowArray,	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(Lighting::m_LightGrid,			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(Lighting::m_LightGridBitMask,	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.FlushResourceBarriers();

	ID3D12GraphicsCommandList* pCommandList = context.GetCommandList();

	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature);
	pCommandList->SetComputeRootDescriptorTable(0, g_SceneSrvs);
	pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootDescriptorTable(3, g_LightingSrvs);
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV);
	pRaytracingCommandList->SetComputeRootShaderResourceView(7, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[DiffuseHitShader].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pRaytracingCommandList->SetPipelineState1(g_RaytracingInputs[DiffuseHitShader].m_pPSO);
	pRaytracingCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceReflections(
	GraphicsContext& context,
	const GlobalConstants& globalConstants,
	const Math::Camera& camera,
	ColorBuffer& colorTarget,
	DepthBuffer& depth,
	ColorBuffer& normals)
{
	ScopedTimer _p0(L"RaytracingWithHitShader", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);
	hitShaderConstants.IsReflection = true;

	context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));
	context.WriteBuffer(g_dynamicConstantBuffer, 0, &inputs, sizeof(inputs));

	context.TransitionResource(g_dynamicConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(normals, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(g_hitConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(colorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	context.FlushResourceBarriers();

	ID3D12GraphicsCommandList* pCommandList = context.GetCommandList();

	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature);
	pCommandList->SetComputeRootDescriptorTable(0, g_SceneSrvs);
	pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetComputeRootDescriptorTable(3, g_LightingSrvs);
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV);
	pRaytracingCommandList->SetComputeRootShaderResourceView(7, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Reflection].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pRaytracingCommandList->SetPipelineState1(g_RaytracingInputs[Reflection].m_pPSO);
	pRaytracingCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RenderRaytrace(GraphicsContext& gfxContext, const GlobalConstants& globalConstants)
{
	ScopedTimer _prof(L"Raytrace", gfxContext);

	gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	switch (rayTracingMode)
	{
	case RTM_TRAVERSAL:
		Raytracebarycentrics(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer);
		break;

	case RTM_SSR:
		RaytracebarycentricsSSR(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer, g_SceneNormalBuffer);
		break;

	case RTM_SHADOWS:
		RaytraceShadows(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer);
		break;

	case RTM_DIFFUSE_WITH_SHADOWMAPS:
	case RTM_DIFFUSE_WITH_SHADOWRAYS:
		RaytraceDiffuse(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer);
		break;

	case RTM_REFLECTIONS:
		RaytraceReflections(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer, g_SceneNormalBuffer);
		break;
	}

	// Clear the gfxContext's descriptor heap since ray tracing changes this underneath the sheets
	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nullptr);
}
