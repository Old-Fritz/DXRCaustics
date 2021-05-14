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

#include <atlbase.h>
#include "DXSampleHelper.h"

#include "CompiledShaders/RayGenerationShaderLib.h"
#include "CompiledShaders/RayGenerationShaderSSRLib.h"
#include "CompiledShaders/HitShaderLib.h"
#include "CompiledShaders/MissShaderLib.h"
#include "CompiledShaders/DiffuseHitShaderLib.h"
#include "CompiledShaders/RayGenerationShadowsLib.h"
#include "CompiledShaders/MissShadowsLib.h"

#include "Shaders/RaytracingHlslCompat.h"
#include "Shaders/ModelViewerRayTracing.h"

using namespace GameCore;
using namespace Math;
using namespace Graphics;

constexpr UINT MaxRayRecursion = 2;
constexpr UINT c_NumCameraPositions = 5;

__declspec(align(16)) struct HitShaderConstants
{
    Vector3 sunDirection;
    Vector3 sunLight;
    Vector3 ambientLight;
    float ShadowTexelSize[4];
    Matrix4 modelToShadow;
    UINT32 IsReflection;
    UINT32 UseShadowRays;
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
    SEN_Hit,
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
    NumTypes
};

struct RaytracingDispatchRayInputs
{
    RaytracingDispatchRayInputs() {}
    RaytracingDispatchRayInputs(
        ID3D12Device5& /*device*/,
        ID3D12StateObject* pPSO,
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
    CComPtr<ID3D12StateObject> m_pPSO;
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
    virtual void Raytrace(class GraphicsContext&);

    void SetCameraToPredefinedPosition(int cameraPosition);

private:
    void InitializeRaytracingStateObjects(const ModelH3D& model, UINT numMeshes);
    
    void InitStaticSamplers(D3D12_STATIC_SAMPLER_DESC* descs);
    void InitGlobalRootSignature();
    void InitLocalRootSignature();

    void InitSubObjectsConfig(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, UINT& nodeMask, D3D12_RAYTRACING_PIPELINE_CONFIG& pipelineConfig);
    void InitSubObjectsTraceShaders(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, ShaderExport exports[SEN_NumExports]);

    void InitHitGroups(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, D3D12_RAYTRACING_SHADER_CONFIG& shaderConfig, 
        D3D12_HIT_GROUP_DESC& hitGroupDesc, LPCWSTR hitGroupNames[HG_NumGroups], ShaderExport exports[SEN_NumExports]);
    void InitRayTraceInputs(std::function<void(ID3D12StateObject*, byte*)> GetShaderTable, D3D12_STATE_OBJECT_DESC& stateObject, 
        std::vector<byte>& pHitShaderTable, UINT shaderRecordSizeInBytes, ShaderExport exports[SEN_NumExports]);

    void CreateTLAS(const ModelH3D& model, UINT numMeshes);

    void RaytraceDiffuse(GraphicsContext& context, const Math::Camera& camera, ColorBuffer& colorTarget);
    void RaytraceShadows(GraphicsContext& context, const Math::Camera& camera, ColorBuffer& colorTarget, DepthBuffer& depth);
    void RaytraceReflections(GraphicsContext& context, const Math::Camera& camera, ColorBuffer& colorTarget, DepthBuffer& depth, ColorBuffer& normals);

    Camera m_Camera;
    std::unique_ptr<FlyingFPSCamera> m_CameraController;
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

    ID3D12DescriptorHeap& GetDescriptorHeap() { return *m_pDescriptorHeap; }

    void AllocateDescriptor(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, _Out_ UINT& descriptorHeapIndex)
    {
        descriptorHeapIndex = m_descriptorsAllocated;
        cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCpuBase, descriptorHeapIndex, m_descriptorSize);
        m_descriptorsAllocated++;
    }

    UINT AllocateBufferSrv(_In_ ID3D12Resource& resource)
    {
        UINT descriptorHeapIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        AllocateDescriptor(cpuHandle, descriptorHeapIndex);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device.CreateShaderResourceView(&resource, &srvDesc, cpuHandle);
        return descriptorHeapIndex;
    }

    UINT AllocateBufferUav(_In_ ID3D12Resource& resource)
    {
        UINT descriptorHeapIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        AllocateDescriptor(cpuHandle, descriptorHeapIndex);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;

        m_device.CreateUnorderedAccessView(&resource, nullptr, &uavDesc, cpuHandle);
        return descriptorHeapIndex;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT descriptorIndex)
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, m_descriptorSize);
    }
private:
    ID3D12Device& m_device;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
    UINT m_descriptorsAllocated = 0;
    UINT m_descriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE m_descriptorHeapCpuBase;
};

enum RaytracingMode
{
    RTM_OFF,
    RTM_TRAVERSAL,
    RTM_SSR,
    RTM_SHADOWS,
    RTM_DIFFUSE_WITH_SHADOWMAPS,
    RTM_DIFFUSE_WITH_SHADOWRAYS,
    RTM_REFLECTIONS,
};
extern EnumVar rayTracingMode;

extern const char* rayTracingModes[];

extern std::unique_ptr<DescriptorHeapStack> g_pRaytracingDescriptorHeap;

extern ByteAddressBuffer          g_hitConstantBuffer;
extern ByteAddressBuffer          g_dynamicConstantBuffer;

extern D3D12_GPU_DESCRIPTOR_HANDLE g_GpuSceneMaterialSrvs[27];
extern D3D12_CPU_DESCRIPTOR_HANDLE g_SceneMeshInfo;

extern D3D12_GPU_DESCRIPTOR_HANDLE g_OutputUAV;
extern D3D12_GPU_DESCRIPTOR_HANDLE g_DepthAndNormalsTable;
extern D3D12_GPU_DESCRIPTOR_HANDLE g_SceneSrvs;

extern std::vector<CComPtr<ID3D12Resource>>   g_bvh_bottomLevelAccelerationStructures;
extern CComPtr<ID3D12Resource>   g_bvh_topLevelAccelerationStructure;

extern DynamicCB           g_dynamicCb;
extern CComPtr<ID3D12RootSignature> g_GlobalRaytracingRootSignature;
extern CComPtr<ID3D12RootSignature> g_LocalRaytracingRootSignature;

extern StructuredBuffer    g_hitShaderMeshInfoBuffer;

extern RaytracingDispatchRayInputs g_RaytracingInputs[RaytracingTypes::NumTypes];
extern D3D12_CPU_DESCRIPTOR_HANDLE g_bvh_attributeSrvs[34];



extern ByteAddressBuffer   g_bvh_bottomLevelAccelerationStructure;

extern CComPtr<ID3D12Device5> g_pRaytracingDevice;