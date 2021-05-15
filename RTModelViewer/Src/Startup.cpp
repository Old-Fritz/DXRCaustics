#include "RTModelViewer.h"

static void InitializeSceneInfo(const ModelInstance& model)
{
    //
    // Mesh info
    //
    std::vector<RayTraceMeshInfo>   meshInfoData(model.GetModel()->m_NumMeshes);
    MeshIterator iterator = model.GetModel()->GetMeshIterator();
    for (UINT i = 0; i < model.GetModel()->m_NumMeshes; ++i)
    {
        const Mesh* mesh = iterator.GetMesh(i);

        meshInfoData[i].m_indexOffsetBytes = mesh->ibOffset;
        meshInfoData[i].m_uvAttributeOffsetBytes = mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kTexcoord0];
        meshInfoData[i].m_normalAttributeOffsetBytes = mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kNormal];
        meshInfoData[i].m_positionAttributeOffsetBytes = mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kPosition];
        meshInfoData[i].m_tangentAttributeOffsetBytes = mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kTangent];
        meshInfoData[i].m_attributeStrideBytes = mesh->vbStride;
        meshInfoData[i].m_materialInstanceId = mesh->materialCBV;
        ASSERT(meshInfoData[i].m_materialInstanceId < MaxMaterials);
    }

    g_hitShaderMeshInfoBuffer.Create(L"RayTraceMeshInfo",
        (UINT)meshInfoData.size(),
        sizeof(meshInfoData[0]),
        meshInfoData.data());

    g_SceneMeshInfo = g_hitShaderMeshInfoBuffer.GetSRV();
}

static void InitializeViews(const ModelInstance& model)
{
    // Global: Output UAV
    D3D12_CPU_DESCRIPTOR_HANDLE uavHandle;
    UINT uavDescriptorIndex;
    g_pRaytracingDescriptorHeap->AllocateDescriptor(uavHandle, uavDescriptorIndex);
    Graphics::g_Device->CopyDescriptorsSimple(1, uavHandle, g_SceneColorBuffer.GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_OutputUAV = g_pRaytracingDescriptorHeap->GetGpuHandle(uavDescriptorIndex);

    // Global: Depth and normals
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

    // Global: scene srv
    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
        UINT srvDescriptorIndex;
        // mesh info
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneMeshInfo, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        g_SceneSrvs = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

        // IB
        UINT unused;
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        model.GetModel()->CreateIndexBufferSRV(srvHandle);

        // VB
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        model.GetModel()->CreateVertexBufferSRV(srvHandle);

        // Shadows
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_ShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // SSAO
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SSAOFullScreen.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // materail constants
        g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
        Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, model.GetModel()->m_MaterialConstants.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    }
    // Local
    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;

        for (UINT i = 0; i < model.GetModel()->m_NumMaterials; i++)
        {
            //const TextureRef* textures = model.GetModel()->textures.data();

            UINT slot;
            for (int t = 0; t < kNumTextures; ++t)
            {
                g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, slot);       // Diffuse
                Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, model.GetModel()->m_SourceTextures[i * kNumTextures + t], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

            g_GpuSceneMaterialSrvs[i] = g_pRaytracingDescriptorHeap->GetGpuHandle(slot - kNumTextures + 1);
        }
    }
}

#include <direct.h> // for _getcwd() to check data root path

void LoadIBLTextures()
{
    char CWD[256];
    _getcwd(CWD, 256);

    Utility::Printf("Loading IBL environment maps\n");

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(L"Textures/*_diffuseIBL.dds", &ffd);

    g_IBLSet.AddEnum(L"None");

    if (hFind != INVALID_HANDLE_VALUE) do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::wstring diffuseFile = ffd.cFileName;
        std::wstring baseFile = diffuseFile;
        baseFile.resize(baseFile.rfind(L"_diffuseIBL.dds"));
        std::wstring specularFile = baseFile + L"_specularIBL.dds";

        TextureRef diffuseTex = TextureManager::LoadDDSFromFile(L"Textures/" + diffuseFile);
        if (diffuseTex.IsValid())
        {
            TextureRef specularTex = TextureManager::LoadDDSFromFile(L"Textures/" + specularFile);
            if (specularTex.IsValid())
            {
                g_IBLSet.AddEnum(baseFile);
                g_IBLTextures.push_back(std::make_pair(diffuseTex, specularTex));
            }
        }
    }     while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    Utility::Printf("Found %u IBL environment map sets\n", g_IBLTextures.size());

    if (g_IBLTextures.size() > 0)
        g_IBLSet.Increment();
}

void RTModelViewer::Startup(void)
{
    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = true;
    FXAA::Enable = false;
    PostEffects::EnableHDR = true;
    PostEffects::EnableAdaptation = true;
    SSAO::Enable = true;

    Renderer::Initialize();

    LoadIBLTextures();

    std::wstring gltfFileName;

    bool forceRebuild = false;
    uint32_t rebuildValue;
    if (CommandLineArgs::GetInteger(L"rebuild", rebuildValue))
        forceRebuild = rebuildValue != 0;

    forceRebuild = true;

    if (CommandLineArgs::GetString(L"model", gltfFileName) == false)
    {
        m_ModelInst = Renderer::LoadModel(L"../Data/Sponza/sponza2.gltf", forceRebuild);
        //m_ModelInst.Resize(100.0f * m_ModelInst.GetRadius());
        OrientedBox obb = m_ModelInst.GetBoundingBox();
        float modelRadius = Length(obb.GetDimensions()) * 0.5f;
        const Vector3 eye = obb.GetCenter() + Vector3(modelRadius * 0.5f, 0.0f, 0.0f);
        m_Camera.SetEyeAtUp(eye, Vector3(kZero), Vector3(kYUnitVector));
    }
    else
    {
        m_ModelInst = Renderer::LoadModel(
            gltfFileName,
            forceRebuild);
        m_ModelInst.LoopAllAnimations();
        //m_ModelInst.Resize(10.0f);

        MotionBlur::Enable = false;
    }

    m_Camera.SetZRange(1.0f, 10000.0f);
    if (gltfFileName.size() == 0)
        m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));
    else
        m_CameraController.reset(new OrbitCamera(m_Camera, m_ModelInst.GetBoundingSphere(), Vector3(kYUnitVector)));


    // RT Specific

    ThrowIfFailed(g_Device->QueryInterface(IID_PPV_ARGS(&g_pRaytracingDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");

    g_pRaytracingDescriptorHeap = std::unique_ptr<DescriptorHeapStack>(
        new DescriptorHeapStack(*g_Device, 200, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0));

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1;
    g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));

    g_hitConstantBuffer.Create(L"Hit Constant Buffer", 1, sizeof(HitShaderConstants));
    g_dynamicConstantBuffer.Create(L"Dynamic Constant Buffer", 1, sizeof(DynamicCB));

    InitializeSceneInfo(m_ModelInst);
    InitializeViews(m_ModelInst);

    UINT numMeshes = m_ModelInst.GetModel()->m_NumMeshes;

    // RT init
    CreateTLAS(m_ModelInst, numMeshes);
    InitializeRaytracingStateObjects(m_ModelInst, numMeshes);

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
    m_ModelInst = nullptr;

    g_IBLTextures.clear();

    Renderer::Shutdown();
}