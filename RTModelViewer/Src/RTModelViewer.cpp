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
D3D12_GPU_DESCRIPTOR_HANDLE g_DepthAndNormalsTable;
D3D12_GPU_DESCRIPTOR_HANDLE g_SceneSrvs;

std::vector<CComPtr<ID3D12Resource>>   g_bvh_bottomLevelAccelerationStructures;
CComPtr<ID3D12Resource>   g_bvh_topLevelAccelerationStructure;

DynamicCB		   g_dynamicCb;
CComPtr<ID3D12RootSignature> g_GlobalRaytracingRootSignature;
CComPtr<ID3D12RootSignature> g_LocalRaytracingRootSignature;

StructuredBuffer	g_hitShaderMeshInfoBuffer;

RaytracingDispatchRayInputs g_RaytracingInputs[RaytracingTypes::NumTypes];
D3D12_CPU_DESCRIPTOR_HANDLE g_bvh_attributeSrvs[34];

CComPtr<ID3D12Device5> g_pRaytracingDevice;


DynamicEnumVar g_IBLSet("Viewer/Lighting/Environment", ChangeIBLSet);
std::vector<std::pair<TextureRef, TextureRef>> g_IBLTextures;
NumVar g_IBLBias("Viewer/Lighting/Gloss Reduction", 2.0f, 0.0f, 10.0f, 1.0f, ChangeIBLBias);

ExpVar g_SunLightIntensity("Viewer/Lighting/Sun Light Intensity", 4.0f, 0.0f, 16.0f, 0.1f);
NumVar g_SunOrientation("Viewer/Lighting/Sun Orientation", -0.5f, -100.0f, 100.0f, 0.1f);
NumVar g_SunInclination("Viewer/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f);

NumVar g_ModelScale("RayTracong/ModelScale", 100.0f, 1.0f, 1000.0f);

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
	CComPtr<ID3D12Debug> debugInterface;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
	{
		debugInterface->EnableDebugLayer();
	}
#endif

	CComPtr<ID3D12Device> pDevice;
	CComPtr<IDXGIAdapter1> pAdapter;
	CComPtr<IDXGIFactory2> pFactory;
	CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
	bool validDeviceFound = false;
	for (uint32_t Idx = 0; !validDeviceFound && DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (IsDirectXRaytracingSupported(pAdapter))
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
