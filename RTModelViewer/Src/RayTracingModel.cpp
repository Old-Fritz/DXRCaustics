#include "RTModelViewer.h"

void RTModelViewer::InitializeRTModel()
{

	ThrowIfFailed(g_Device->QueryInterface(IID_PPV_ARGS(&g_pRaytracingDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");

	g_pRaytracingDescriptorHeap = std::unique_ptr<DescriptorHeapStack>(
		new DescriptorHeapStack(*g_Device, 2000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0));

	D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1;
	g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));

	g_hitConstantBuffer.Create(L"Hit Constant Buffer", 1, sizeof(GlobalConstants));
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
	std::vector<MeshLayoutInfo>   meshInfoData(m_ModelInst.GetModel()->m_NumMeshes);
	MeshIterator iterator = m_ModelInst.GetModel()->GetMeshIterator();
	for (UINT i = 0; i < m_ModelInst.GetModel()->m_NumMeshes; ++i)
	{
		const Mesh* mesh = iterator.GetMesh(i);

		meshInfoData[i].m_indexOffsetBytes				= mesh->ibOffset;
		meshInfoData[i].m_uvAttributeOffsetBytes		= mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kTexcoord0];
		meshInfoData[i].m_normalAttributeOffsetBytes	= mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kNormal];
		meshInfoData[i].m_positionAttributeOffsetBytes	= mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kPosition];
		meshInfoData[i].m_tangentAttributeOffsetBytes	= mesh->vbOffset + mesh->streamOffsets[glTF::Primitive::kTangent];
		meshInfoData[i].m_attributeStrideBytes			= mesh->vbStride;
		meshInfoData[i].m_materialInstanceId			= mesh->materialCBV;
		ASSERT(meshInfoData[i].m_materialInstanceId < MaxMaterials);
	}

	g_hitShaderMeshInfoBuffer.Create(L"MeshLayoutInfo",
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
		// Shadows t13
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
		UINT srvDescriptorIndex;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_IBLTextures[0].second.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_LightingSrvs = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

		// SSAO t12
		UINT unused;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_IBLTextures[0].first.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SSAOFullScreen.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_ShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// light buffer t14
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		// light shadow array tex t15
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
#ifdef USE_LIGHT_GBUFFER
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightGBufferArray.GetDepthBuffer().GetDepthSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
#else
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightShadowArray.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
#endif

		// light grid t16
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightGrid.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
		// light grid bit mask t17
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Lighting::m_LightGridBitMask.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;

		// blue noise t18
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_BlueNoiseRGBA->GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;


	}

	// Global: scene srv
	{
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
		UINT srvDescriptorIndex;
		// mesh info t1
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneMeshInfo, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_SceneSrvs = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

		// mesh data t2
		UINT unused;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		m_ModelInst.GetModel()->CreateMeshDataSRV(srvHandle);

		// materail constants t3
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, m_ModelInst.GetModel()->m_MaterialConstants.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// mesh constants t4
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, m_ModelInst.GetMeshConstantsGPU().GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneGBuffer.GetDepthBuffer().GetDepthSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Global : GBuffer
	{

		GeometryBuffer* GBuffers[GBT_Num] = { &g_SceneGBuffer,  &Lighting::m_LightGBufferArray };

		for (int gBufInd = 0; gBufInd < GBT_Num; ++gBufInd)
		{
			GeometryBuffer* GBuffer = GBuffers[gBufInd];

			D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
			UINT srvDescriptorIndex;
			// depth t20
			g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
			Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, GBuffer->GetDepthBuffer().GetDepthSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			g_GBufferSRV[gBufInd] = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);
			GBuffer->SetRTHandle(g_GBufferSRV[gBufInd]);

			UINT unused;
			for (uint32_t targetInd = 0; targetInd < uint32_t(GBTarget::NumTargets); ++targetInd)
			{
				g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
				Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, GBuffer->GetColorBuffer((GBTarget)targetInd).GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
	}

	// Local
	{
		// srv textures
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