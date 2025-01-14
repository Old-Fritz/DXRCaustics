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

#include "DepthBuffer.h"
#define SHADOW_FORMAT DXGI_FORMAT_D32_FLOAT
//#define SHADOW_FORMAT DXGI_FORMAT_D16_UNORM

class EsramAllocator;

class GraphicsContext;

class ShadowBuffer : public DepthBuffer
{
public:
	ShadowBuffer() {}
		
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height,
		D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, EsramAllocator& Allocator );

	void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
		D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return GetDepthSRV(); }

	void BeginRendering( GraphicsContext& context );
	void EndRendering( GraphicsContext& context );

private:
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Scissor;
};
