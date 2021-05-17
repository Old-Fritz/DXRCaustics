#include "RTModelViewer.h"

void RTModelViewer::InitializeRTModel()
{

	ThrowIfFailed(g_Device->QueryInterface(IID_PPV_ARGS(&g_pRaytracingDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");

	g_pRaytracingDescriptorHeap = std::unique_ptr<DescriptorHeapStack>(
		new DescriptorHeapStack(*g_Device, 200, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0));

	D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1;
	g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));

	g_hitConstantBuffer.Create(L"Hit Constant Buffer", 1, sizeof(HitShaderConstants));
	g_dynamicConstantBuffer.Create(L"Dynamic Constant Buffer", 1, sizeof(DynamicCB));

	InitializeRTMeshInfo();

	InitializeRTViews();

	CreateTLAS();

	InitializeRaytracingStateObjects();
}

void RTModelViewer::InitializeRTMeshInfo()
{
	//
	// Mesh info
	//
	std::vector<RayTraceMeshInfo>   meshInfoData(m_ModelInst.GetModel()->m_NumMeshes);
	MeshIterator iterator = m_ModelInst.GetModel()->GetMeshIterator();
	for (UINT i = 0; i < m_ModelInst.GetModel()->m_NumMeshes; ++i)
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

void RTModelViewer::InitializeRTViews()
{
	// Global: Output UAV
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle;
	UINT uavDescriptorIndex;
	g_pRaytracingDescriptorHeap->AllocateDescriptor(uavHandle, uavDescriptorIndex);
	Graphics::g_Device->CopyDescriptorsSimple(1, uavHandle, g_SceneColorBuffer.GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_OutputUAV = g_pRaytracingDescriptorHeap->GetGpuHandle(uavDescriptorIndex);

	// Global: Lighting buffers
	{
		// depth
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
		UINT srvDescriptorIndex;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneDepthBuffer.GetDepthSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_LightingSrvs = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

		// normals
		UINT unused;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneNormalBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// light buffer
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		// light shadow array tex
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightShadowArray.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// light grid
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightGrid.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
		// light grid bit mask
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightGridBitMask.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;
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
		m_ModelInst.GetModel()->CreateIndexBufferSRV(srvHandle);

		// VB
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		m_ModelInst.GetModel()->CreateVertexBufferSRV(srvHandle);

		// Shadows
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_ShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// SSAO
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SSAOFullScreen.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// materail constants
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, m_ModelInst.GetModel()->m_MaterialConstants.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Local
	{
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;

		for (UINT i = 0; i < m_ModelInst.GetModel()->m_NumMaterials; i++)
		{
			UINT slot;
			for (int t = 0; t < kNumTextures; ++t)
			{
				g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, slot);
				Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, m_ModelInst.GetModel()->m_SourceTextures[i * kNumTextures + t], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}

			g_GpuSceneMaterialSrvs[i] = g_pRaytracingDescriptorHeap->GetGpuHandle(slot - kNumTextures + 1);
		}
	}
}