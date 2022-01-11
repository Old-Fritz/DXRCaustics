#include "RTModelViewer.h"

const char* rayTracingModes[] = {
	"No RT",
	"Caustic",
	"Caustic\/Refl",		//"Backward Rays With caustics",
	"Caustic Debug",
	"Refl",				//"Backward Rays",
	"Diffuse",
	"Refl Max",
	"Refl Bary",
	"Bary Rays",		// ???
	"Shadows",
	"Shadows Debug",
};
EnumVar rayTracingMode("RayTraceMode", RTM_CAUSTIC, _countof(rayTracingModes), rayTracingModes);

void UpdateDynamicConstants(DynamicCB& inputs, const Math::Camera& camera, ColorBuffer& colorTarget)
{
	auto m0 = camera.GetViewProjMatrix();
	auto m1 = Transpose(Invert(m0));
	memcpy(&inputs.cameraToWorld, &m1, sizeof(inputs.cameraToWorld));
	memcpy(&inputs.worldCameraPosition, &camera.GetPosition(), sizeof(inputs.worldCameraPosition));
	inputs.resolution.x = (float)colorTarget.GetWidth();
	inputs.resolution.y = (float)colorTarget.GetHeight();


	inputs.causticMaxRayRecursion	= (UINT)g_CausticMaxRayRecursion;
	inputs.causticRaysPerPixel		= g_CausticRaysPerPixel;
	inputs.causticPowerScale		= g_CausticPowerScale;
	inputs.ModelScale				= g_ModelScale;


	inputs.AdditiveRecurrenceSequenceIndexBasis = g_RTAdditiveRecurrenceSequenceOffset + GetFrameCount() % (uint32_t)g_RTAdditiveRecurrenceSequenceIndexLimit;;
	inputs.AdditiveRecurrenceSequenceAlpha = { g_RTAdditiveRecurrenceSequenceAlphaX,  g_RTAdditiveRecurrenceSequenceAlphaY };

	inputs.IsReflection		= false;
	inputs.UseShadowRays	= false;
	inputs.ReflSampleCount = g_RTReflectionsSampleCount;
	inputs.UseExperimentalCheckerboard = g_RTUseExperimentalCheckerboard;

	inputs.Feature1 = g_RTUseFeature1;
	inputs.Feature2 = g_RTUseFeature2;
	inputs.Feature3 = g_RTUseFeature3;
	inputs.Feature4 = g_RTUseFeature4;
}

void SetupResources(CommandContext& context, const DynamicCB& dynamicCB, const GlobalConstants& hitShaderConstants, ColorBuffer& colorTarget, GeometryBuffer& GBuffer, Tlas& tlas)
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
	// main depth
	context.TransitionResource(g_SceneGBuffer.GetDepthBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	context.FlushResourceBarriers();

	// setup signature
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

	pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature.Get());
	pCommandList->SetComputeRootDescriptorTable(0, g_SceneSrvs); // t1-4
	pCommandList->SetComputeRootConstantBufferView(1, g_dynamicConstantBuffer.GetGpuVirtualAddress()); // b0
	pCommandList->SetComputeRootConstantBufferView(2, g_hitConstantBuffer.GetGpuVirtualAddress()); // b1
	pCommandList->SetComputeRootDescriptorTable(3, g_LightingSrvs); // t10-15 
	pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV); // u2
	pCommandList->SetComputeRootDescriptorTable(5, g_GBufferSRV[0]);//GBuffer.GetRTHandle()); // t20-t31
	pCommandList->SetComputeRootShaderResourceView(6, tlas.GetGPUVirtualAddress()); // t0
}



void Raytracebarycentrics(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer, Tlas& tlas)
{
	ScopedTimer _p0(L"Raytracing barycentrics", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	UpdateDynamicConstants(inputs, camera, colorTarget);

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, tlas);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Primarybarycentric].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
	pCommandList->SetPipelineState1(g_RaytracingInputs[Primarybarycentric].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RaytracebarycentricsSSR(CommandContext& context, const GlobalConstants& globalConstants, const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer, Tlas& tlas)
{
	ScopedTimer _p0(L"Raytracing SSR barycentrics", context);

	// Prepare constants
	DynamicCB inputs = g_dynamicCb;
	UpdateDynamicConstants(inputs, camera, colorTarget);

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, tlas);
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
	UpdateDynamicConstants(inputs, camera, colorTarget);

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, m_tlas);
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
	UpdateDynamicConstants(inputs, camera, colorTarget);
	inputs.UseShadowRays = rayTracingMode == RTM_DIFFUSE_SHADOWS;

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, m_tlas);
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
	UpdateDynamicConstants(inputs, camera, colorTarget);
	inputs.IsReflection = true;

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, m_tlas);
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
	UpdateDynamicConstants(inputs, camera, colorTarget);
	inputs.UseShadowRays = false;

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, m_tlas);
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
	UpdateDynamicConstants(inputs, camera, colorTarget);

	SetupResources(context, inputs, globalConstants, colorTarget, GBuffer, m_tlas);
	ComPtr<ID3D12GraphicsCommandList4> pCommandList;
	context.GetCommandList()->QueryInterface(IID_PPV_ARGS_(pCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[Caustic].GetDispatchRayDesc(512,512);
	dispatchRaysDesc.Depth = (UINT)g_LightsCount;
	pCommandList->SetPipelineState1(g_RaytracingInputs[Caustic].m_pPSO.Get());
	pCommandList->DispatchRays(&dispatchRaysDesc);
}

void RTModelViewer::RenderRaytrace(GraphicsContext& gfxContext, const GlobalConstants& globalConstants)
{
	ScopedTimer _prof(L"Raytrace", gfxContext);

	gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	switch (rayTracingMode)
	{
	case RTM_BARY_RAYS:
		Raytracebarycentrics(gfxContext.GetComputeContext(), globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer, m_tlas);
		break;

	case RTM_REFL_BARY:
		RaytracebarycentricsSSR(gfxContext.GetComputeContext(), globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer, m_tlas);
		break;

	case RTM_SHADOWS_DEBUG:
		RaytraceShadows(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_DIFFUSE:
	case RTM_DIFFUSE_SHADOWS:
		RaytraceDiffuse(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_REFL_MAX:
		RaytraceReflections(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;

	case RTM_REFL:
		RaytraceBackward(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		break;
	case RTM_CAUSTIC_REFL:
		RaytraceBackward(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
	case RTM_CAUSTIC:
	case RTM_CAUSTIC_DEBUG:
		//RaytraceCaustic(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, g_SceneGBuffer);
		RaytraceCaustic(gfxContext, globalConstants, m_Camera, g_SceneColorBuffer, Lighting::m_LightGBufferArray);
		break;
	}

	// Clear the gfxContext's descriptor heap since ray tracing changes this underneath the sheets
	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nullptr);
}
