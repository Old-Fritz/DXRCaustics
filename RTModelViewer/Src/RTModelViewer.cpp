//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "RTModelViewer.h"

std::unique_ptr<DescriptorHeapStack> g_pRaytracingDescriptorHeap;

ByteAddressBuffer		  g_hitConstantBuffer;
ByteAddressBuffer		  g_dynamicConstantBuffer;

D3D12_GPU_DESCRIPTOR_HANDLE g_GpuSceneMaterialSrvs[MaxMaterials];
D3D12_CPU_DESCRIPTOR_HANDLE g_SceneMeshInfo;

D3D12_GPU_DESCRIPTOR_HANDLE g_OutputUAV;
D3D12_GPU_DESCRIPTOR_HANDLE g_GBufferSRV[GBT_Num];
D3D12_GPU_DESCRIPTOR_HANDLE g_LightingSrvs;
D3D12_GPU_DESCRIPTOR_HANDLE g_SceneSrvs;

std::vector<ComPtr<ID3D12Resource>>   g_bvh_bottomLevelAccelerationStructures;
ComPtr<ID3D12Resource>   g_bvh_topLevelAccelerationStructure;

DynamicCB		   g_dynamicCb;
ComPtr<ID3D12RootSignature> g_GlobalRaytracingRootSignature;
ComPtr<ID3D12RootSignature> g_LocalRaytracingRootSignature;

StructuredBuffer	g_hitShaderMeshInfoBuffer;

RaytracingDispatchRayInputs g_RaytracingInputs[RaytracingTypes::NumTypes];
D3D12_CPU_DESCRIPTOR_HANDLE g_bvh_attributeSrvs[34];

ComPtr<ID3D12Device5> g_pRaytracingDevice;

std::vector<std::pair<TextureRef, TextureRef>> g_IBLTextures;
TextureRef* g_BlueNoiseRGBA;

NumVar g_RTAdditiveRecurrenceSequenceOffset("App/Raytracing/AdditiveRecurrenceSequenceOffset", 0, -1000, 1000, 1);
NumVar g_RTAdditiveRecurrenceSequenceAlphaX("App/Raytracing/AdditiveRecurrenceSequenceAlphaX", 0.698539816339744830961566084581988f, 0, 1.0f, 0.0152799f);
NumVar g_RTAdditiveRecurrenceSequenceAlphaY("App/Raytracing/AdditiveRecurrenceSequenceAlphaY", 0.6961803398874989484820458683436564f, 0, 1.0f, 0.0143562f);
//NumVar g_RTAdditiveRecurrenceSequenceAlphaX("App/Raytracing/AdditiveRecurrenceSequenceAlphaX", 0.018539816339744830961566084581988f, 0, 1.0f, 0.0152799f);
//NumVar g_RTAdditiveRecurrenceSequenceAlphaY("App/Raytracing/AdditiveRecurrenceSequenceAlphaY", 0.0161803398874989484820458683436564f, 0, 1.0f, 0.0143562f);
ExpVar g_RTAdditiveRecurrenceSequenceIndexLimit("App/Raytracing/AdditiveRecurrenceSequenceIndexLimit", 16777216, 0, 16777216*2, 1);



NumVar	g_RTReflectionsSampleCount("App/Raytracing/Refl/SampleCount", 4, 1, 128, 1);


DynamicEnumVar g_IBLSet("App/Lighting/Environment", ChangeIBLSet);
NumVar g_IBLBias("App/Lighting/Gloss Reduction", 6.0f, 0.0f, 10.0f, 0.25f, ChangeIBLBias);

ExpVar g_SunLightIntensity(	"App/Lighting/Sun Light Intensity", 0.9f,	-16.0f, 16.0f, 0.1f);
ExpVar g_AmbientIntensity(	"App/Lighting/Ambient Intensity",	0.15f,	-16.0f, 16.0f, 0.1f);

NumVar g_SunOrientation("App/Lighting/Sun Orientation", -0.5f, -100.0f, 100.0f, 0.1f);
NumVar g_SunInclination("App/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f);

NumVar g_ModelScale("App/Raytracing/ModelScale", 100.0f, 1.0f, 1000.0f);

NumVar g_CausticRaysPerPixel("App/Raytracing/Caustic/RaysPerPixel", 0.75f, 0.25f, 16, 0.25f);
ExpVar g_CausticPowerScale("App/Raytracing/Caustic/PowerScale", 1, -16.0f, 16.0f, 0.1f);
NumVar g_CausticMaxRayRecursion("App/Raytracing/Caustic/MaxRaRecursion", 3, 1, 16, 1);
BoolVar g_RTUseExperimentalCheckerboard("App/Raytracing/Caustic/Checkerboard", false);

BoolVar g_RTUseFeature1("App/Raytracing/Caustic/Feature1", false);
BoolVar g_RTUseFeature2("App/Raytracing/Caustic/Feature2", false);
BoolVar g_RTUseFeature3("App/Raytracing/Caustic/Feature3", false);
BoolVar g_RTUseFeature4("App/Raytracing/Caustic/Feature4", false);


BoolVar g_GlobalLight("App/Lighting/GlobalLight", true);

void ChangeIBLSet(EngineVar::ActionType)
{
	int setIdx = g_IBLSet - 1;
	if (setIdx < 0)
	{
		Renderer::SetIBLTextures(nullptr, nullptr);
	}
	else
	{
		auto texturePair = g_IBLTextures[setIdx];
		Renderer::SetIBLTextures(texturePair.first, texturePair.second);
	}
}

void ChangeIBLBias(EngineVar::ActionType)
{
	Renderer::SetIBLBias(g_IBLBias);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
#if _DEBUG
	ComPtr<ID3D12Debug> debugInterface;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
	{
		debugInterface->EnableDebugLayer();
	}
#endif

	ComPtr<ID3D12Device> pDevice;
	ComPtr<IDXGIAdapter1> pAdapter;
	ComPtr<IDXGIFactory2> pFactory;
	CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
	bool validDeviceFound = false;
	for (uint32_t Idx = 0; !validDeviceFound && DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (IsDirectXRaytracingSupported(pAdapter.Get()))
		{
			validDeviceFound = true;
		}
		pAdapter = nullptr;
	}

	//s_EnableVSync.Decrement();
	//TargetResolution = k720p;
	//g_DisplayWidth = 1280;
	//g_DisplayHeight = 720;
	GameCore::RunApplication(RTModelViewer(), L"RTModelViewer", hInstance, nCmdShow); 
	return 0;
}
