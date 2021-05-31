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

#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ShadowBuffer.h"
#include "GpuBuffer.h"
#include "GraphicsCore.h"
#define FLAG(x) 1<<(x)
namespace Graphics
{
	enum class GBTarget
	{
		BaseColor,
		MetallicRoughness,
		Occlusion,
		Emissive,
		Normal,
		NumTargets
	};
	enum class GBSet
	{
		None =					0,

		// resources
		Normal =				FLAG((uint32_t)GBTarget::Normal),
		MetallicRoughness =		FLAG((uint32_t)GBTarget::MetallicRoughness),
		Occlusion =				FLAG((uint32_t)GBTarget::Occlusion),
		BaseColor =				FLAG((uint32_t)GBTarget::BaseColor),
		Emissive =				FLAG((uint32_t)GBTarget::Emissive),
		Depth =					FLAG((uint32_t)GBTarget::NumTargets),
		AllRTs = Normal | MetallicRoughness | Occlusion | Emissive | BaseColor,
		
		// states
		DSWrite =					FLAG((uint32_t)GBTarget::NumTargets + 1),
		DSRead =					FLAG((uint32_t)GBTarget::NumTargets + 2),
		Pixel =						FLAG((uint32_t)GBTarget::NumTargets + 3),
		NonPixel =					FLAG((uint32_t)GBTarget::NumTargets + 4),
		RTWrite =					FLAG((uint32_t)GBTarget::NumTargets + 5),

		// action
		Setup =					FLAG((uint32_t)GBTarget::NumTargets + 6),
		Clear =					FLAG((uint32_t)GBTarget::NumTargets + 7),

	};
	DEFINE_ENUM_FLAG_OPERATORS(GBSet);

	class GeometryBuffer
	{
	public:

		void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT dsFormat);
		void Destroy();

		void Setup(GraphicsContext& context, GBSet flags);
		void Setup(CommandContext& context, GBSet flags);

		void Bind(GraphicsContext& context, uint32_t rootIndex, uint32_t offset, GBSet flags);
		void Bind(ComputeContext& context, uint32_t rootIndex, uint32_t offset, GBSet flags);


		uint32_t GetWidth();
		uint32_t GetHeight();

		DepthBuffer& GetDepthBuffer();
		ColorBuffer& GetColorBuffer(GBTarget target);

		D3D12_GPU_DESCRIPTOR_HANDLE GetRTHandle();
		void SetRTHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle);

	public:
		static constexpr DXGI_FORMAT c_GBufferFormats[(uint32_t)GBTarget::NumTargets] =
		{
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16_FLOAT,
			DXGI_FORMAT_R16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		};

		static constexpr wchar_t* const c_GBufferNames[(uint32_t)GBTarget::NumTargets] =
		{
			L"BaseColor",
			L"MetallicRoughness",
			L"Occlusion",
			L"Emissive",
			L"Normal"
		};
	private:
		D3D12_GPU_DESCRIPTOR_HANDLE m_RTDescriptorHandle;

		DepthBuffer m_DepthBuffer;  // D32_FLOAT_S8_UINT

		ColorBuffer m_ColorBuffers[(uint32_t)GBTarget::NumTargets];
	};

	extern GeometryBuffer g_SceneGBuffer;
	extern ColorBuffer g_SceneColorBuffer;  // R11G11B10_FLOAT
	extern ColorBuffer g_PostEffectsBuffer; // R32_UINT (to support Read-Modify-Write with a UAV)
	extern ColorBuffer g_OverlayBuffer;	 // R8G8B8A8_UNORM
	extern ColorBuffer g_HorizontalBuffer;  // For separable (bicubic) upsampling

	extern ColorBuffer g_VelocityBuffer;	// R10G10B10  (3D velocity)
	extern ShadowBuffer g_ShadowBuffer;

	extern ColorBuffer g_SSAOFullScreen;	// R8_UNORM
	extern ColorBuffer g_LinearDepth[2];	// Normalized planar distance (0 at eye, 1 at far plane) computed from the SceneDepthBuffer
	extern ColorBuffer g_MinMaxDepth8;		// Min and max depth values of 8x8 tiles
	extern ColorBuffer g_MinMaxDepth16;		// Min and max depth values of 16x16 tiles
	extern ColorBuffer g_MinMaxDepth32;		// Min and max depth values of 16x16 tiles
	extern ColorBuffer g_DepthDownsize1;
	extern ColorBuffer g_DepthDownsize2;
	extern ColorBuffer g_DepthDownsize3;
	extern ColorBuffer g_DepthDownsize4;
	extern ColorBuffer g_DepthTiled1;
	extern ColorBuffer g_DepthTiled2;
	extern ColorBuffer g_DepthTiled3;
	extern ColorBuffer g_DepthTiled4;
	extern ColorBuffer g_AOMerged1;
	extern ColorBuffer g_AOMerged2;
	extern ColorBuffer g_AOMerged3;
	extern ColorBuffer g_AOMerged4;
	extern ColorBuffer g_AOSmooth1;
	extern ColorBuffer g_AOSmooth2;
	extern ColorBuffer g_AOSmooth3;
	extern ColorBuffer g_AOHighQuality1;
	extern ColorBuffer g_AOHighQuality2;
	extern ColorBuffer g_AOHighQuality3;
	extern ColorBuffer g_AOHighQuality4;

	extern ColorBuffer g_DoFTileClass[2];
	extern ColorBuffer g_DoFPresortBuffer;
	extern ColorBuffer g_DoFPrefilter;
	extern ColorBuffer g_DoFBlurColor[2];
	extern ColorBuffer g_DoFBlurAlpha[2];
	extern StructuredBuffer g_DoFWorkQueue;
	extern StructuredBuffer g_DoFFastQueue;
	extern StructuredBuffer g_DoFFixupQueue;

	extern ColorBuffer g_MotionPrepBuffer;		// R10G10B10A2
	extern ColorBuffer g_LumaBuffer;
	extern ColorBuffer g_TemporalColor[2];
	extern ColorBuffer g_TemporalMinBound;
	extern ColorBuffer g_TemporalMaxBound;

	extern ColorBuffer g_aBloomUAV1[2];		// 640x384 (1/3)
	extern ColorBuffer g_aBloomUAV2[2];		// 320x192 (1/6)  
	extern ColorBuffer g_aBloomUAV3[2];		// 160x96  (1/12)
	extern ColorBuffer g_aBloomUAV4[2];		// 80x48   (1/24)
	extern ColorBuffer g_aBloomUAV5[2];		// 40x24   (1/48)
	extern ColorBuffer g_LumaLR;
	extern ByteAddressBuffer g_Histogram;
	extern ByteAddressBuffer g_FXAAWorkQueue;
	extern TypedBuffer g_FXAAColorQueue;

	void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight );
	void ResizeDisplayDependentBuffers(uint32_t NativeWidth, uint32_t NativeHeight);
	void DestroyRenderingBuffers();

} // namespace Graphics
