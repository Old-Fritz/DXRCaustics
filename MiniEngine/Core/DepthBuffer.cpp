//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "DepthBuffer.h"
#include "GraphicsCore.h"
#include "EsramAllocator.h"
#include "DescriptorHeap.h"

using namespace Graphics;

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
	Create(Name, Width, Height, 1, Format, VidMemPtr);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ResourceDesc.SampleDesc.Count = Samples;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Graphics::g_Device, Format);
}

void DepthBuffer::CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format,
	D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ResourceDesc.SampleDesc.Count = 1;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Graphics::g_Device, Format, ArrayCount);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, EsramAllocator& Allocator )
{
	Create(Name, Width, Height, 1, Format, Allocator);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, EsramAllocator& )
{
	Create(Name, Width, Height, Samples, Format);
}

void DepthBuffer::CreateDerivedViews( ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize)
{
	ID3D12Resource* Resource = m_pResource.Get();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = GetDSVFormat(Format);
	DXGI_FORMAT stencilReadFormat = GetStencilFormat(Format);
	m_hDSVs.resize(ArraySize);

	for (UINT i = 0; i < ArraySize; ++i)
	{
		m_hDSVs[i][0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hDSVs[i][1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hDSVs[i][2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hDSVs[i][3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;

		if (m_hDSVs[i][0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hDSVs[i][0] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			m_hDSVs[i][1] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		if (ArraySize > 1)
		{
			if (Resource->GetDesc().SampleDesc.Count == 1)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.MipSlice = 0;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.ArraySize = 1;
			}
			else
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
				dsvDesc.Texture2DMSArray.FirstArraySlice = i;
				dsvDesc.Texture2DMSArray.ArraySize = 1;
			}
		}
		else
		{
			if (Resource->GetDesc().SampleDesc.Count == 1)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
			}
			else
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			}
		}

		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVs[i][0]); // !!

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVs[i][1]); // !!

		if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
		{
			if (m_hDSVs[i][2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			{
				m_hDSVs[i][2] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
				m_hDSVs[i][3] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			}

			dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
			Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVs[i][2]); //

			dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
			Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVs[i][3]); //
		}
		else
		{
			m_hDSVs[i][2] = m_hDSVs[i][0];
			m_hDSVs[i][3] = m_hDSVs[i][1];
		}
	}
	

	if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_hDepthSRV = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = GetDepthFormat(Format);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	}
	else if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2DMS)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2DARRAY)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.MipLevels = 1;
		SRVDesc.Texture2DArray.ArraySize = ArraySize;
		SRVDesc.Texture2DArray.FirstArraySlice = 0;
	}
	else if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2DMS)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		SRVDesc.Texture2DMSArray.ArraySize = ArraySize;
		SRVDesc.Texture2DMSArray.FirstArraySlice = 0;
	}
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Device->CreateShaderResourceView( Resource, &SRVDesc, m_hDepthSRV );

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hStencilSRV = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SRVDesc.Format = stencilReadFormat;
		Device->CreateShaderResourceView( Resource, &SRVDesc, m_hStencilSRV );
	}
}
