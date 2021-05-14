#include "ModelViewer.h"

static void InitializeSceneInfo(const ModelH3D& model)
{
    //
    // Mesh info
    //
    std::vector<RayTraceMeshInfo>   meshInfoData(model.m_Header.meshCount);
    for (UINT i = 0; i < model.m_Header.meshCount; ++i)
    {
        meshInfoData[i].m_indexOffsetBytes = model.m_pMesh[i].indexDataByteOffset;
        meshInfoData[i].m_uvAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[ModelH3D::attrib_texcoord0].offset;
        meshInfoData[i].m_normalAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[ModelH3D::attrib_normal].offset;
        meshInfoData[i].m_positionAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[ModelH3D::attrib_position].offset;
        meshInfoData[i].m_tangentAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[ModelH3D::attrib_tangent].offset;
        meshInfoData[i].m_bitangentAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[ModelH3D::attrib_bitangent].offset;
        meshInfoData[i].m_attributeStrideBytes = model.m_pMesh[i].vertexStride;
        meshInfoData[i].m_materialInstanceId = model.m_pMesh[i].materialIndex;
        ASSERT(meshInfoData[i].m_materialInstanceId < 27);
    }

    g_hitShaderMeshInfoBuffer.Create(L"RayTraceMeshInfo",
        (UINT)meshInfoData.size(),
        sizeof(meshInfoData[0]),
        meshInfoData.data());

    g_SceneMeshInfo = g_hitShaderMeshInfoBuffer.GetSRV();
}

static void InitializeViews(const ModelH3D& model)
{
    D3D12_CPU_DESCRIPTOR_HANDLE uavHandle;
    UINT uavDescriptorIndex;
    g_pRaytracingDescriptorHeap->AllocateDescriptor(uavHandle, uavDescriptorIndex);
    Graphics::g_Device->CopyDescriptorsSimple(1, uavHandle, g_SceneColorBuffer.GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_OutputUAV = g_pRaytracingDescriptorHeap->GetGpuHandle(uavDescriptorIndex);

    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
        UINT srvDescriptorIndex;
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneDepthBuffer.GetDepthSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        g_DepthAndNormalsTable = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

        UINT unused;
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneNormalBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
        UINT srvDescriptorIndex;
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneMeshInfo, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        g_SceneSrvs = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

        UINT unused;
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Sponza::GetModel().CreateIndexBufferSRV(srvHandle);

        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Sponza::GetModel().CreateVertexBufferSRV(srvHandle);

        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_ShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SSAOFullScreen.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        for (UINT i = 0; i < model.m_Header.materialCount; i++)
        {
            const TextureRef* textures = model.GetMaterialTextures(i);

            UINT slot;
            g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, slot);       // Diffuse
            Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, textures[0].GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);     // Normal
            Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, textures[2].GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            g_GpuSceneMaterialSrvs[i] = g_pRaytracingDescriptorHeap->GetGpuHandle(slot);
        }
    }
}

void RTModelViewer::Startup(void)
{
    MotionBlur::Enable = false;//true;
    TemporalEffects::EnableTAA = false;//true;
    FXAA::Enable = false;
    PostEffects::EnableHDR = false;//true;
    PostEffects::EnableAdaptation = false;//true;
    SSAO::Enable = true;

    Renderer::Initialize();

    Sponza::Startup(m_Camera);

    m_Camera.SetZRange(1.0f, 10000.0f);
    m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));


    ThrowIfFailed(g_Device->QueryInterface(IID_PPV_ARGS(&g_pRaytracingDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");

    g_pRaytracingDescriptorHeap = std::unique_ptr<DescriptorHeapStack>(
        new DescriptorHeapStack(*g_Device, 200, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0));

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1;
    g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));

    g_hitConstantBuffer.Create(L"Hit Constant Buffer", 1, sizeof(HitShaderConstants));
    g_dynamicConstantBuffer.Create(L"Dynamic Constant Buffer", 1, sizeof(DynamicCB));

    const ModelH3D& model = Sponza::GetModel();

    InitializeSceneInfo(model);
    InitializeViews(model);

    UINT numMeshes = model.m_Header.meshCount;

    // RT Stuff
    CreateTLAS(model, numMeshes);
    InitializeRaytracingStateObjects(model, numMeshes);

    m_CameraPosArrayCurrentPosition = 0;

    // Lion's head
    m_CameraPosArray[0].position = Vector3(-1100.0f, 170.0f, -30.0f);
    m_CameraPosArray[0].heading = 1.5707f;
    m_CameraPosArray[0].pitch = 0.0f;

    // View of columns
    m_CameraPosArray[1].position = Vector3(299.0f, 208.0f, -202.0f);
    m_CameraPosArray[1].heading = -3.1111f;
    m_CameraPosArray[1].pitch = 0.5953f;

    // Bottom-up view from the floor
    m_CameraPosArray[2].position = Vector3(-1237.61f, 80.60f, -26.02f);
    m_CameraPosArray[2].heading = -1.5707f;
    m_CameraPosArray[2].pitch = 0.268f;

    // Top-down view from the second floor
    m_CameraPosArray[3].position = Vector3(-977.90f, 595.05f, -194.97f);
    m_CameraPosArray[3].heading = -2.077f;
    m_CameraPosArray[3].pitch = -0.450f;

    // View of corridors on the second floor
    m_CameraPosArray[4].position = Vector3(-1463.0f, 600.0f, 394.52f);
    m_CameraPosArray[4].heading = -1.236f;
    m_CameraPosArray[4].pitch = 0.0f;
}

void RTModelViewer::Cleanup(void)
{
    Sponza::Cleanup();
    Renderer::Shutdown();
}