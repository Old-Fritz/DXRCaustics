#include "RTModelViewer.h"


RTGeometryTriangle::RTGeometryTriangle() : m_desc()
{}
RTGeometryTriangle::RTGeometryTriangle(const Model* pModel, const Mesh* pMesh)
{
	Fill(pModel, pMesh);
}

void RTGeometryTriangle::Fill(const Model* pModel, const Mesh* pMesh)
{
	m_isOpaque = !(pMesh->psoFlags & PSOFlags::kAlphaTest) && !(pMesh->psoFlags & PSOFlags::kAlphaBlend);
	m_isDoubleSide = (pMesh->psoFlags & PSOFlags::kTwoSided);

	m_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	m_desc.Flags = m_isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

	D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = m_desc.Triangles;
	trianglesDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	trianglesDesc.VertexCount = pMesh->vbSize / pMesh->vbStride;
	trianglesDesc.VertexBuffer.StartAddress = pModel->m_DataBuffer.GetGpuVirtualAddress() + pMesh->vbOffset;
	trianglesDesc.IndexBuffer = pModel->m_DataBuffer.GetGpuVirtualAddress() + pMesh->ibOffset;
	trianglesDesc.VertexBuffer.StrideInBytes = pMesh->vbStride;
	trianglesDesc.IndexCount = pMesh->ibSize / ((DXGI_FORMAT)pMesh->ibFormat == DXGI_FORMAT_R32_UINT ? 4 : 2);
	trianglesDesc.IndexFormat = (DXGI_FORMAT)pMesh->ibFormat;
	trianglesDesc.Transform3x4 = 0;
}

D3D12_RAYTRACING_GEOMETRY_DESC* RTGeometryTriangle::GetDescPtr()
{
	return &m_desc;
}

RTGeometryAABB::RTGeometryAABB() : m_desc()
{}

void RTGeometryAABB::Fill()
{
	m_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	m_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	//
	//D3D12_RAYTRACING_GEOMETRY_AABBS_DESC& aabbDesc = m_desc.AABBs;
	////D3D12_RAYTRACING_AABB& m_desc.
	//
	//
	//
	//m_isOpaque = !(pMesh->psoFlags & PSOFlags::kAlphaTest) && !(pMesh->psoFlags & PSOFlags::kAlphaBlend);
	//m_isDoubleSide = (pMesh->psoFlags & PSOFlags::kTwoSided);
	//
	//m_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	//m_desc.Flags = m_isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	//
	//D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = m_desc.Triangles;
	//trianglesDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	//trianglesDesc.VertexCount = pMesh->vbSize / pMesh->vbStride;
	//trianglesDesc.VertexBuffer.StartAddress = pModel->m_DataBuffer.GetGpuVirtualAddress() + pMesh->vbOffset;
	//trianglesDesc.IndexBuffer = pModel->m_DataBuffer.GetGpuVirtualAddress() + pMesh->ibOffset;
	//trianglesDesc.VertexBuffer.StrideInBytes = pMesh->vbStride;
	//trianglesDesc.IndexCount = pMesh->ibSize / ((DXGI_FORMAT)pMesh->ibFormat == DXGI_FORMAT_R32_UINT ? 4 : 2);
	//trianglesDesc.IndexFormat = (DXGI_FORMAT)pMesh->ibFormat;
	//trianglesDesc.Transform3x4 = 0;
}

D3D12_RAYTRACING_GEOMETRY_DESC* RTGeometryAABB::GetDescPtr()
{
	return &m_desc;
}

void Blas::AddGeometry(const Model* pModel, const Mesh* pMesh)
{
	m_geometry.emplace_back(pModel, pMesh);
	m_geometryDescPtrs.push_back(m_geometry.back().GetDescPtr());
	m_isDoubleSide |= m_geometry.back().m_isDoubleSide;
}

void Blas::PrepareDesc()
	{
		m_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		m_desc.Inputs.NumDescs = (UINT)m_geometry.size();
		m_desc.Inputs.ppGeometryDescs = m_geometryDescPtrs.data();
		m_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		m_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelprebuildInfo;
		g_pRaytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&m_desc.Inputs, &bottomLevelprebuildInfo);

		m_resultSize = bottomLevelprebuildInfo.ResultDataMaxSizeInBytes;
		m_scratchSize = bottomLevelprebuildInfo.ScratchDataSizeInBytes;
	}

void Blas::PrepareBuffers()
	{
		m_scratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)m_scratchSize, 1);

		auto defaultHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto bottomLevelDesc = CD3DX12_RESOURCE_DESC::Buffer(m_resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		g_Device->CreateCommittedResource(
			&defaultHeapDesc,
			D3D12_HEAP_FLAG_NONE,
			&bottomLevelDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&m_resultBuffer));

		m_desc.ScratchAccelerationStructureData = m_scratchBuffer.GetGpuVirtualAddress();
		m_desc.DestAccelerationStructureData = m_resultBuffer->GetGPUVirtualAddress();
	}

void Blas::Build(ID3D12GraphicsCommandList4* pList)
	{
		pList->BuildRaytracingAccelerationStructure(&m_desc, 0, nullptr);
	}

D3D12_GPU_VIRTUAL_ADDRESS Blas::GetGPUVirtualAddress()
	{
		return m_resultBuffer->GetGPUVirtualAddress();
	}

void Tlas::AddInstance(Blas& blas)
{
	m_instanceDescs.emplace_back();
	D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = m_instanceDescs.back();

	// Identity matrix
	ZeroMemory(instanceDesc.Transform, sizeof(instanceDesc.Transform));
	instanceDesc.Transform[0][0] = 1.0f * g_ModelScale;
	instanceDesc.Transform[1][1] = 1.0f * g_ModelScale;
	instanceDesc.Transform[2][2] = 1.0f * g_ModelScale;

	instanceDesc.AccelerationStructure = blas.GetGPUVirtualAddress();
	instanceDesc.Flags = blas.m_isDoubleSide ? D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE : D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceMask = 1;
	instanceDesc.InstanceContributionToHitGroupIndex = m_instanceDescs.size() - 1;
}

void Tlas::PrepareDesc()
{
	m_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	m_desc.Inputs.NumDescs = (UINT)m_instanceDescs.size();
	m_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	m_desc.Inputs.pGeometryDescs = nullptr;
	m_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo;
	g_pRaytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&m_desc.Inputs, &topLevelPrebuildInfo);

	m_resultSize = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
	m_scratchSize = topLevelPrebuildInfo.ScratchDataSizeInBytes;
}

void Tlas::PrepareBuffers()
{
	m_instanceDataBuffer.Create(L"Instance Data Buffer", (UINT)m_instanceDescs.size(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC), m_instanceDescs.data());
	m_desc.Inputs.InstanceDescs = m_instanceDataBuffer->GetGPUVirtualAddress();

	m_scratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)m_scratchSize, 1);

	auto defaultHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bottomLevelDesc = CD3DX12_RESOURCE_DESC::Buffer(m_resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	g_Device->CreateCommittedResource(
		&defaultHeapDesc,
		D3D12_HEAP_FLAG_NONE,
		&bottomLevelDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(&m_resultBuffer));

	m_desc.ScratchAccelerationStructureData = m_scratchBuffer.GetGpuVirtualAddress();
	m_desc.DestAccelerationStructureData = m_resultBuffer->GetGPUVirtualAddress();
}

void Tlas::Build(ID3D12GraphicsCommandList4* pList)
{
	pList->BuildRaytracingAccelerationStructure(&m_desc, 0, nullptr);
}

D3D12_GPU_VIRTUAL_ADDRESS Tlas::GetGPUVirtualAddress()
{
	return m_resultBuffer->GetGPUVirtualAddress();
}


void RTModelViewer::CreateTLAS()
{
	auto iterator = m_ModelInst.GetModel()->GetMeshIterator();
	for (UINT i = 0; i < m_ModelInst.GetModel()->m_NumMeshes; i++)
	{
		auto pMesh = iterator.GetMesh(i);

		// create new blas
		m_blases.emplace_back();

		auto& blas = m_blases.back();
		blas.AddGeometry(m_ModelInst.GetModel().get(), pMesh);

		// prepare blas to build
		blas.PrepareDesc();
		blas.PrepareBuffers();

		// collect blas
		m_tlas.AddInstance(blas);
	}

	// prepare tlas to build
	m_tlas.PrepareDesc();
	m_tlas.PrepareBuffers();
	
	// build ases
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Create Acceleration Structure");
	ID3D12GraphicsCommandList* pCommandList = gfxContext.GetCommandList();

	ComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	pCommandList->QueryInterface(IID_PPV_ARGS_(pRaytracingCommandList));

	ID3D12DescriptorHeap* descriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
	pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
	for (UINT i = 0; i < m_blases.size(); i++)
	{
		m_blases[i].Build(pRaytracingCommandList.Get());
	}
	pCommandList->ResourceBarrier(1, &uavBarrier);

	m_tlas.Build(pRaytracingCommandList.Get());

	gfxContext.Finish(true);
}