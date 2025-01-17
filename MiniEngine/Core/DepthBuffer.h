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

#pragma once

#include "PixelBuffer.h"

class EsramAllocator;

class DepthBuffer : public PixelBuffer
{
public:
	DepthBuffer( float ClearDepth = 0.0f, uint8_t ClearStencil = 0 )
		: m_ClearDepth(ClearDepth), m_ClearStencil(ClearStencil) 
	{
		//m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		//m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		//m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		//m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	// Create a depth buffer.  If an address is supplied, memory will not be allocated.
	// The vmem address allows you to alias buffers (which can be especially useful for
	// reusing ESRAM across a frame.)
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );

	// Create a depth buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		EsramAllocator& Allocator );

	void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
		D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
		EsramAllocator& Allocator );

	void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format,
		D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const { return m_hDSVs[m_currentIndex][0]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() const { return m_hDSVs[m_currentIndex][1]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() const { return m_hDSVs[m_currentIndex][2]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() const { return m_hDSVs[m_currentIndex][3]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() const { return m_hDepthSRV; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() const { return m_hStencilSRV; }

	float GetClearDepth() const { return m_ClearDepth; }
	uint8_t GetClearStencil() const { return m_ClearStencil; }

	void SetArrayIndex(uint32_t index) { m_currentIndex = index; }

protected:

	struct HandleDSV
	{
		D3D12_CPU_DESCRIPTOR_HANDLE m_handles[4];

		HandleDSV()
		{
			m_handles[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_handles[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_handles[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_handles[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE& operator[](const int index) const
		{
			return m_handles[index];
		}
		D3D12_CPU_DESCRIPTOR_HANDLE& operator[](const int index)
		{
			return m_handles[index];
		}
	};

	void CreateDerivedViews( ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize = 1);

	float m_ClearDepth;
	uint8_t m_ClearStencil;
	std::vector<HandleDSV> m_hDSVs;
	uint32_t m_currentIndex = 0;
	//D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4];
	D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
};
