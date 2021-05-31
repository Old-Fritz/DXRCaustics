#include "RTModelViewer.h"

const char* rayTracingModes[] = {
	"Off",
	"Off with cautic",
	"Backward Rays With caustics",
	"Caustic"	
	"Backward Rays",
	"Diffuse&ShadowMaps",
	"Reflection Rays",
	"Refl Bary",
	"Bary Rays",
	"Shadow Rays",
	"Diffuse&ShadowRays",
};
EnumVar rayTracingMode("App/Raytracing/RayTraceMode", RTM_BACKWARD_WITH_CAUSTICS, _countof(rayTracingModes), rayTracingModes);

void UpdateHitShaderConstants(HitShaderConstants& hitShaderConstants, const GlobalConstants& globalConstants)
{
	hitShaderConstants.ViewProjMatrix = globalConstants.ViewProjMatrix;
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
	hitShaderConstants.IBLRange = globalConstants.IBLRange;
	hitShaderConstants.IBLBias = globalConstants.IBLBias;
	hitShaderConstants.AdditiveRecurrenceSequenceAlpha = { g_RTAdditiveRecurrenceSequenceAlphaX,  g_RTAdditiveRecurrenceSequenceAlphaY };
	hitShaderConstants.AdditiveRecurrenceSequenceIndexBasis = GetFrameCount() % (uint32_t)g_RTAdditiveRecurrenceSequenceIndexLimit;
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

void SetupResources(CommandContext& context, DynamicCB& dynamicCB, HitShaderConstants& hitShaderConstants, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));
	context.WriteBuffer(g_dynamicConstantBuffer, 0, &dynamicCB, sizeof(dynamicCB));

	// resource transition
	// cb (1, 2)
	context.TransitionResource(g_hitConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	context.TransitionResource(g_dynamicConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	// lighting (3)
	context.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(Lighting::m_LightBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#ifdef USE_LIGHT_GBUFFER
	Lighting::m_LightGBufferArray.Setup(context, GBSet::Depth | GBSet::NonPixel);
#else
	context.TransitionResource(Lighting::m_LightShadowArray, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif
	context.TransitionResource(Lighting::m_LightGrid, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	context.TransitionResource(Lighting::m_LightGridBitMask, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// outputUAV (4)
	context.TransitionResource(colorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// gbuffer (5)
	GBuffer.Setup(context, GBSet::AllRTs | GBSet::Depth | GBSet::NonPixel);

	context.FlushResourceBarriers();

	// setup signature
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature.Get());
	pCommandList->SetComputeRootDescriptorTable(0, g_SceneSrvs); // t1-4
	pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress()); // b0
	pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer.GetGpuVirtualAddress()); // b1
	pCommandList->SetComputeRootDescriptorTable(3, g_LightingSrvs); // t10-15 
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV); // u2
	pCommandList->SetComputeRootDescriptorTable(5, GBuffer.GetRTHandle()); // t16-t21
	pCommandList->SetComputeRootShaderResourceView(6, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress()); // t0
}



void Raytracebarycentrics(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"Raytracing barycentrics", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Primarybarycentric].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[Primarybarycentric].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RaytracebarycentricsSSR(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"Raytracing SSR barycentrics", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Reflectionbarycentric].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[Reflectionbarycentric].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceShadows(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"Raytracing Shadows", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Shadows].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[Shadows].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceDiffuse(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"RaytracingWithHitShader", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);
	hitShaderConstants.UseShadowRays = rayTracingMode == RTM_DIFFUSE_WITH_SHADOWRAYS;

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[DiffuseHitShader].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[DiffuseHitShader].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceReflections(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"RaytracingWithHitShader", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);
	hitShaderConstants.IsReflection = true;

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Reflection].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[Reflection].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceBackward(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"RaytracingBackward", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);
	hitShaderConstants.UseShadowRays = false;

	//hitShaderConstants.IsReflection = true;

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Backward].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[Backward].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RaytraceCaustic(CommandContext& context, const GlobalConstants& globalConstants,
	const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer)
{
	ScopedTimer _p0(L"RaytracingCaustic", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	HitShaderConstants hitShaderConstants = {};
	UpdateDynamicConstants(inputs, camera, colorTarget);
	UpdateHitShaderConstants(hitShaderConstants, globalConstants);
	//hitShaderConstants.UseShadowRays = rayTracingMode == RTM_BACKWARD_WITH_SHADOWRAYS;
	//hitShaderConstants.IsReflection = true;
	inputs.resolution = { 512.0f, 512.0f };

	SetupResources(context, inputs, hitShaderConstants, colorTarget, GBuffer);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Caustic].GetDispatchRayDesc(512,512);
	pCommandList->SetPipelineState1(g_RaytracingInputs[Caustic].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RenderRaytrace(GraphicsContext& gfxContext, const GlobalConstants& globalConstants)
{
	ScopedTimer _prof(L"Raytrace", gfxContext);

	gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	switch (rayTracingMode)
	{
	case RTM_TRAVERSAL:
		Raytracebarycentrics(gfxContext.GetComputeContext(), globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_SSR:
		RaytracebarycentricsSSR(gfxContext.GetComputeContext(), globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_SHADOWS:
		RaytraceShadows(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_DIFFUSE_WITH_SHADOWMAPS:
	case RTM_DIFFUSE_WITH_SHADOWRAYS:
		RaytraceDiffuse(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_REFLECTIONS:
		RaytraceReflections(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_BACKWARD:
		RaytraceBackward(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;
	case RTM_BACKWARD_WITH_CAUSTICS:
		RaytraceBackward(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
	case RTM_OFF_WITH_CAUSTICS:
	case RTM_CAUSTIC:
		//RaytraceCaustic(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		RaytraceCaustic(gfxContext, globalConstants, Lighting::m_LightShadowCamera[0], g_SceneColorBuffer, g_SceneGBuffer);
		break;
	}

	// Clear the gfxContext's descriptor heap since ray tracing changes this underneath the sheets
	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nullptr);
}
