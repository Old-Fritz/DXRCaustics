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
#include "ShadowBuffer.h"
#include "EsramAllocator.h"
#include "CommandContext.h"


void ShadowBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
	DepthBuffer::Create( Name, Width, Height, SHADOW_FORMAT, VidMemPtr );

	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = (float)Width;
	m_Viewport.Height = (float)Height;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	// Prevent drawing to the boundary pixels so that we don't have to worry about shadows stretching
	m_Scissor.left = 1;
	m_Scissor.top = 1;
	m_Scissor.right = (LONG)Width - 2;
	m_Scissor.bottom = (LONG)Height - 2;
}

void ShadowBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, EsramAllocator& Allocator )
{
	DepthBuffer::Create( Name, Width, Height, SHADOW_FORMAT, Allocator );

	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = (float)Width;
	m_Viewport.Height = (float)Height;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	// Prevent drawing to the boundary pixels so that we don't have to worry about shadows stretching
	m_Scissor.left = 1;
	m_Scissor.top = 1;
	m_Scissor.right = (LONG)Width - 2;
	m_Scissor.bottom = (LONG)Height - 2;
}


void ShadowBuffer::CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
	D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	DepthBuffer::CreateArray(Name, Width, Height, ArrayCount, SHADOW_FORMAT, VidMemPtr);

	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = (float)Width;
	m_Viewport.Height = (float)Height;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	//m_Scissor.left = 0;
	//m_Scissor.top = 0;
	//m_Scissor.right = (LONG)Width;
	//m_Scissor.bottom = (LONG)Height;

	// Prevent drawing to the boundary pixels so that we don't have to worry about shadows stretching
	m_Scissor.left = 1;
	m_Scissor.top = 1;
	m_Scissor.right = (LONG)Width - 2;
	m_Scissor.bottom = (LONG)Height - 2;
}

void ShadowBuffer::BeginRendering( GraphicsContext& Context )
{
	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	Context.ClearDepth(*this);
	Context.SetDepthStencilTarget(GetDSV());
	Context.SetViewportAndScissor(m_Viewport, m_Scissor);
}

void ShadowBuffer::EndRendering( GraphicsContext& Context )
{
	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
