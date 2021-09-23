#pragma once

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

#define NOMINMAX

#include "d3d12.h"
#include "d3d12video.h"
#include <d3d12.h>
#include "dxgi1_3.h"
#include "d3dx12.h"
#include "GameCore.h"
#include "CameraController.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "Display.h"
#include "TemporalEffects.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "PostEffects.h"
#include "ParticleEffectManager.h"
#include "SSAO.h"
#include "FXAA.h"
#include "SystemTime.h"
#include "TextRenderer.h"
#include "ShadowCamera.h"
#include "ParticleEffectManager.h"
#include "GameInput.h"
#include "SponzaRenderer.h"
#include "ModelH3D.h"
#include "Renderer.h"
#include "Model.h"
#include "ModelLoader.h"
#include "MeshConvert.h"
#include "LightManager.h"
#include <atlbase.h>
#include "DXSampleHelper.h"


#include "CompiledShaders/RGS_Shadows.h"
#include "CompiledShaders/MS_Shadows.h"

#include "CompiledShaders/RGS_SSR.h"
#include "CompiledShaders/RGS_Diffuse.h"
#include "CompiledShaders/CHS_Diffuse.h"

#include "CompiledShaders/CHS_Default.h"
#include "CompiledShaders/AHS_Default.h"
#include "CompiledShaders/MS_Default.h"

#include "CompiledShaders/RGS_Backward.h"
#include "CompiledShaders/CHS_Backward.h"
#include "CompiledShaders/AHS_Backward.h"
#include "CompiledShaders/MS_Backward.h"

#include "CompiledShaders/RGS_Caustic.h"
#include "CompiledShaders/CHS_Caustic.h"
#include "CompiledShaders/AHS_Caustic.h"
#include  "CompiledShaders/MS_Caustic.h"

#include "Shaders/RaytracingHlslCompat.h"

#define IID_PPV_ARGS_(ptr) IID_PPV_ARGS(ptr.GetAddressOf())
#define MAX_LIGHTS 20

using namespace GameCore;
using namespace Math;
using namespace Graphics;

constexpr UINT MaxRayRecursion = 16;
constexpr UINT c_NumCameraPositions = 2;
constexpr UINT MaxMaterials = 100;

__declspec(align(256)) struct HitShaderConstants
{
	Matrix4			ViewProjMatrix;
	Matrix4			SunShadowMatrix;
	Vector4			ViewerPos;
	Vector4			SunDirection; 
	Vector4			SunIntensity;
	Vector4			AmbientIntensity;
	Vector4			ShadowTexelSize;
	Vector4			InvTileDim;
	UintVector4		TileCount;
	UintVector4		FirstLightIndex;

	float			ModelScale;
	float			IBLRange;
	float			IBLBias;
	uint			AdditiveRecurrenceSequenceIndexBasis;

	float2			AdditiveRecurrenceSequenceAlpha;
	uint			IsReflection;
	uint			UseShadowRays;
};

struct ShaderExport
{
	LPCWSTR exportName;
	D3D12_STATE_SUBOBJECT* pSubObject;
	D3D12_EXPORT_DESC exportDesc;
	D3D12_DXIL_LIBRARY_DESC dxilLibDesc;
};

enum ShaderExportNames
{
	SEN_RayGen = 0,
	SEN_AnyHit,
	SEN_ClosestHit,
	SEN_Miss,
	SEN_NumExports
};

enum HitGroups
{
	HG_HitGroup = 0,
	HG_NumGroups
};

enum RaytracingTypes
{
	Primarybarycentric = 0,
	Reflectionbarycentric,
	Shadows,
	DiffuseHitShader,
	Reflection,
	Backward,
	Caustic,
	NumTypes
};

struct RaytracingDispatchRayInputs
{
	RaytracingDispatchRayInputs() {}
	RaytracingDispatchRayInputs(
		ID3D12Device5& /*device*/,
		ComPtr<ID3D12StateObject> pPSO,
		void* pHitGroupShaderTable,
		UINT HitGroupStride,
		UINT HitGroupTableSize,
		LPCWSTR rayGenExportName,
		LPCWSTR missExportName) : m_pPSO(pPSO)
	{
		const UINT shaderTableSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		ID3D12StateObjectProperties* stateObjectProperties = nullptr;
		ThrowIfFailed(pPSO->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
		void* pRayGenShaderData = stateObjectProperties->GetShaderIdentifier(rayGenExportName);
		void* pMissShaderData = stateObjectProperties->GetShaderIdentifier(missExportName);

		m_HitGroupStride = HitGroupStride;

		// MiniEngine requires that all initial data be aligned to 16 bytes
		UINT alignment = 16;
		std::vector<BYTE> alignedShaderTableData(shaderTableSize + alignment - 1);
		BYTE* pAlignedShaderTableData = alignedShaderTableData.data() + ((UINT64)alignedShaderTableData.data() % alignment);
		memcpy(pAlignedShaderTableData, pRayGenShaderData, shaderTableSize);
		m_RayGenShaderTable.Create(L"Ray Gen Shader Table", 1, shaderTableSize, alignedShaderTableData.data());

		memcpy(pAlignedShaderTableData, pMissShaderData, shaderTableSize);
		m_MissShaderTable.Create(L"Miss Shader Table", 1, shaderTableSize, alignedShaderTableData.data());

		m_HitShaderTable.Create(L"Hit Shader Table", 1, HitGroupTableSize, pHitGroupShaderTable);
	}

	D3D12_DISPATCH_RAYS_DESC GetDispatchRayDesc(UINT DispatchWidth, UINT DispatchHeight)
	{
		D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};

		dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = m_RayGenShaderTable.GetGpuVirtualAddress();
		dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = m_RayGenShaderTable.GetBufferSize();
		dispatchRaysDesc.HitGroupTable.StartAddress = m_HitShaderTable.GetGpuVirtualAddress();
		dispatchRaysDesc.HitGroupTable.SizeInBytes = m_HitShaderTable.GetBufferSize();
		dispatchRaysDesc.HitGroupTable.StrideInBytes = m_HitGroupStride;
		dispatchRaysDesc.MissShaderTable.StartAddress = m_MissShaderTable.GetGpuVirtualAddress();
		dispatchRaysDesc.MissShaderTable.SizeInBytes = m_MissShaderTable.GetBufferSize();
		dispatchRaysDesc.MissShaderTable.StrideInBytes = dispatchRaysDesc.MissShaderTable.SizeInBytes; // Only one entry
		dispatchRaysDesc.Width = DispatchWidth;
		dispatchRaysDesc.Height = DispatchHeight;
		dispatchRaysDesc.Depth = 1;
		return dispatchRaysDesc;
	}

	UINT m_HitGroupStride;
	ComPtr<ID3D12StateObject> m_pPSO;
	ByteAddressBuffer   m_RayGenShaderTable;
	ByteAddressBuffer   m_MissShaderTable;
	ByteAddressBuffer   m_HitShaderTable;
};

struct MaterialRootConstant
{
	UINT MaterialID;
};
class RTModelViewer : public GameCore::IGameApp
{
public:

	RTModelViewer(void) {}

	virtual void Startup(void) override;
	virtual void Cleanup(void) override;

	virtual void Update(float deltaT) override;

	virtual void RenderScene(void) override;
	virtual void RenderUI(class GraphicsContext&) override;

	void SetCameraToPredefinedPosition(int cameraPosition);

	// update
	void UpdateLight();
	void SaveLightsInFile();
	void LoadLightsFromFile();

	// scene render
	void UpdateGlobalConstants(GlobalConstants& globals);
	void RenderLightShadows(GraphicsContext& gfxContext, GlobalConstants& globals);
	void RenderCausticMaps(GraphicsContext& gfxContext, GlobalConstants& globals);
	void RenderSunShadow(GraphicsContext& gfxContext, GlobalConstants& globals);

	void RenderZPass(GraphicsContext& gfxContext, Renderer::MeshSorter& sorter, GlobalConstants& globals);
	void RenderColor(GraphicsContext& gfxContext, Renderer::MeshSorter& sorter, GlobalConstants& globals);
	void RenderGBuffer(GraphicsContext& gfxContext, Renderer::MeshSorter& sorter, GlobalConstants& globals);
	void RenderDeferred(GraphicsContext& gfxContext, GlobalConstants& globals);

	void RenderPostProces(GraphicsContext& gfxContext);

	void RenderRaytrace(GraphicsContext& gfxContext, const GlobalConstants& globalConstants);

private:
	// Viewer startup
	void SetupPredefinedCameraPositions();

	// RT Model
	void InitializeRTModel();
	void InitializeRTMeshInfo();
	void InitializeRTViews();

	// RT PSO
	void InitializeRaytracingStateObjects();
	
	void InitStaticSamplers(D3D12_STATIC_SAMPLER_DESC* descs);
	void InitGlobalRootSignature();
	void InitLocalRootSignature();

	void InitSubObjectsConfig(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, UINT& nodeMask, D3D12_RAYTRACING_PIPELINE_CONFIG& pipelineConfig);
	void InitSubObjectsTraceShaders(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, ShaderExport exports[SEN_NumExports]);

	void InitHitGroups(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, D3D12_RAYTRACING_SHADER_CONFIG& shaderConfig, 
		D3D12_HIT_GROUP_DESC& hitGroupDesc, LPCWSTR hitGroupNames[HG_NumGroups], ShaderExport exports[SEN_NumExports]);
	void InitRayTraceInputs(std::function<void(ComPtr<ID3D12StateObject>, byte*)> GetShaderTable, D3D12_STATE_OBJECT_DESC& stateObject, 
		std::vector<byte>& pHitShaderTable, UINT shaderRecordSizeInBytes, ShaderExport exports[SEN_NumExports]);

	// RT TLAS
	void CreateTLAS();

	// RT Render
	void RaytraceDiffuse(CommandContext& context, const GlobalConstants& globalConstants,
		const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer);
	void RaytraceShadows(CommandContext& context, const GlobalConstants& globalConstants,
		const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer);
	void RaytraceReflections(CommandContext& context, const GlobalConstants& globalConstants,
		const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer);
	void RaytraceBackward(CommandContext& context, const GlobalConstants& globalConstants,
		const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer);
	void RaytraceCaustic(CommandContext& context, const GlobalConstants& globalConstants,
		const Math::Camera& camera, ColorBuffer& colorTarget, GeometryBuffer& GBuffer);

	Camera m_Camera;
	std::unique_ptr<CameraController> m_CameraController;
	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;

	struct CameraPosition
	{
		Vector3 position;
		float heading;
		float pitch;
	};

	CameraPosition m_CameraPosArray[c_NumCameraPositions];
	UINT m_CameraPosArrayCurrentPosition;

	ModelInstance m_ModelInst;
	ShadowCamera m_SunShadowCamera;

	TextureRef m_BlueNoiseRGBA;

};


// Returns bool whether the device supports DirectX Raytracing tier.
inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
{
	ComPtr<ID3D12Device> testDevice;
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

	return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
		&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
		&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}



class DescriptorHeapStack
{
public:
	DescriptorHeapStack(ID3D12Device& device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT NodeMask) :
		m_device(device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = NodeMask;
		device.CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap));

		m_descriptorSize = device.GetDescriptorHandleIncrementSize(type);
		m_descriptorHeapCpuBase = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	ID3D12DescriptorHeap& GetDescriptorHeap() { return *(m_pDescriptorHeap.Get()); }

	void AllocateDescriptor(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, _Out_ UINT& descriptorHeapIndex)
	{
		descriptorHeapIndex = m_descriptorsAllocated;
		cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCpuBase, descriptorHeapIndex, m_descriptorSize);
		m_descriptorsAllocated++;
	}

	UINT AllocateBufferSrv(_In_  ComPtr<ID3D12Resource>resource)
	{
		UINT descriptorHeapIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		AllocateDescriptor(cpuHandle, descriptorHeapIndex);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.NumElements = (UINT)(resource->GetDesc().Width / sizeof(UINT32));
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_device.CreateShaderResourceView(resource.Get(), &srvDesc, cpuHandle);
		return descriptorHeapIndex;
	}

	UINT AllocateBufferUav(_In_ ComPtr<ID3D12Resource> resource)
	{
		UINT descriptorHeapIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		AllocateDescriptor(cpuHandle, descriptorHeapIndex);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements = (UINT)(resource->GetDesc().Width / sizeof(UINT32));
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;

		m_device.CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, cpuHandle);
		return descriptorHeapIndex;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT descriptorIndex)
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, m_descriptorSize);
	}
private:
	ID3D12Device& m_device;
	ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
	UINT m_descriptorsAllocated = 0;
	UINT m_descriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptorHeapCpuBase;
};

struct LightSource
{
	float		LightRadius;
	float		LightIntensity;
	Vector3		Pos;
	Vector3		Color;
	Vector3		ConeDir;
	float		ConeInner;
	float		ConeOuter;
};

enum RaytracingMode
{
	RTM_OFF,
	RTM_OFF_WITH_CAUSTICS,
	RTM_BACKWARD_WITH_CAUSTICS,
	RTM_CAUSTIC,
	RTM_BACKWARD,
	RTM_DIFFUSE_WITH_SHADOWMAPS,
	RTM_REFLECTIONS,
	RTM_SSR,
	RTM_TRAVERSAL,
	RTM_DIFFUSE_WITH_SHADOWRAYS,
	RTM_SHADOWS
};
extern EnumVar								rayTracingMode;

extern const char*							rayTracingModes[];

extern std::unique_ptr<DescriptorHeapStack>	g_pRaytracingDescriptorHeap;

extern ByteAddressBuffer					g_hitConstantBuffer;
extern ByteAddressBuffer					g_dynamicConstantBuffer;

extern D3D12_GPU_DESCRIPTOR_HANDLE			g_GpuSceneMaterialSrvs[MaxMaterials];
extern D3D12_CPU_DESCRIPTOR_HANDLE			g_SceneMeshInfo;

extern D3D12_GPU_DESCRIPTOR_HANDLE			g_OutputUAV;
extern D3D12_GPU_DESCRIPTOR_HANDLE			g_LightingSrvs;
extern D3D12_GPU_DESCRIPTOR_HANDLE			g_SceneSrvs;

extern std::vector<ComPtr<ID3D12Resource>>	g_bvh_bottomLevelAccelerationStructures;
extern ComPtr<ID3D12Resource>				g_bvh_topLevelAccelerationStructure;

extern DynamicCB							g_dynamicCb;
extern ComPtr<ID3D12RootSignature>			g_GlobalRaytracingRootSignature;
extern ComPtr<ID3D12RootSignature>			g_LocalRaytracingRootSignature;

extern StructuredBuffer						g_hitShaderMeshInfoBuffer;

extern RaytracingDispatchRayInputs			g_RaytracingInputs[RaytracingTypes::NumTypes];
extern D3D12_CPU_DESCRIPTOR_HANDLE			g_bvh_attributeSrvs[34];

extern LightSource							g_LightSource[MAX_LIGHTS];


extern ByteAddressBuffer g_bvh_bottomLevelAccelerationStructure;

extern ComPtr<ID3D12Device5> g_pRaytracingDevice;


void ChangeIBLSet(EngineVar::ActionType);
void ChangeIBLBias(EngineVar::ActionType);

extern DynamicEnumVar g_IBLSet;
extern std::vector<std::pair<TextureRef, TextureRef>> g_IBLTextures;
extern NumVar g_IBLBias;


extern ExpVar g_SunLightIntensity;
extern ExpVar g_AmbientIntensity;
extern NumVar g_SunOrientation;
extern NumVar g_SunInclination;

extern TextureRef* g_BlueNoiseRGBA;

extern NumVar g_RTAdditiveRecurrenceSequenceAlphaX;
extern NumVar g_RTAdditiveRecurrenceSequenceAlphaY;
extern ExpVar g_RTAdditiveRecurrenceSequenceIndexLimit;



extern NumVar g_CausticRaysPerPixel;
extern ExpVar g_CausticPowerScale;
extern NumVar g_CausticMaxRayRecursion;

extern NumVar g_SelectedLightSource;
extern NumVar g_LightsCount;
