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
// Author(s):	Alex Nankervis
//

// outdated warning about for-loop variable scope
#pragma warning (disable: 3078)

cbuffer ConstantsCS : register(b0)
{
	row_major float4x4 CameraToWorld;
	float2 Resolution;
	uint2 padding;
}

cbuffer GlobalConstants : register(b1)
{
	column_major float4x4 ViewProj;
	column_major float4x4 SunShadowMatrix;
	float3 ViewerPos;
	float3 SunDirection;
	float3 SunIntensity;
	float3 AmbientIntensity;
	float4 ShadowTexelSize;
	float4 InvTileDim;
	uint4 TileCount;
	uint4 FirstLightIndex;
	uint FrameIndexMod2;
	float IBLRange;
	float IBLBias;
}

// output
RWTexture2D<float4>						g_screenOutput				: register(u0);


// GBuffer (16 - 21)
Texture2D<float>						g_GBDepth					: register(t0);
Texture2D<float4>						g_GBBaseColor				: register(t1);
Texture2D<float2>						g_GBMetallicRoughness		: register(t2);
Texture2D<float1>						g_GBOcclusion				: register(t3);
Texture2D<float3>						g_GBEmissive				: register(t4);
Texture2D<float4>						g_GBNormal					: register(t5);


// lighting
TextureCube<float3>						radianceIBLTexture			: register(t10);
TextureCube<float3>						irradianceIBLTexture		: register(t11);
Texture2D<float>						texSSAO						: register(t12);
Texture2D<float>						texSunShadow				: register(t13);

//SamplerState defaultSampler : register(s10);
//SamplerComparisonState shadowSampler : register(s11);
//SamplerState cubeMapSampler : register(s12);

#include "Lighting.hlsli"
#include "LightingPBR.hlsli"


#define _RootSig \
	"RootFlags(0), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"DescriptorTable(SRV(t0, numDescriptors = 6))," \
	"DescriptorTable(SRV(t10, numDescriptors = 8))," \
	"DescriptorTable(UAV(u0, numDescriptors = 1))," \
	"StaticSampler(s10, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(s11, visibility = SHADER_VISIBILITY_ALL," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"comparisonFunc = COMPARISON_GREATER_EQUAL," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)," \
	"StaticSampler(s12, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)"


[RootSignature(_RootSig)]
[numthreads(8, 8, 1)]
void main(uint2 DTid :SV_DispatchThreadID)
	//uint2 Gid : SV_GroupID,
	//uint2 GTid : SV_GroupThreadID,
	//uint GI : SV_GroupIndex)
{
	uint2 pixelPos = DTid;

	//float2 xy = DTid;


	// Load and modulate textures
	float sceneDepth =					g_GBDepth[pixelPos];					//				g_GBDepth.Load(int3(xy, 0));
	float4 baseColor =					g_GBBaseColor[pixelPos];						//				g_GBBaseColor.Load(int3(xy, 0));
	float2 metallicRoughness =			g_GBMetallicRoughness[pixelPos];						//				g_GBMetallicRoughness.Load(int3(xy, 0));
	float occlusion =					g_GBOcclusion[pixelPos];						//				g_GBOcclusion.Load(int3(xy, 0));
	float3 emissive =					g_GBEmissive[pixelPos];						//				g_GBEmissive.Load(int3(xy, 0));
	float3 normal =						g_GBNormal[pixelPos].xyz;						//				g_GBNormal.Load(int3(xy, 0));


	// Unproject into the world position using depth
	float2 screenPos = (pixelPos / Resolution);
	screenPos.y = 1.0 - screenPos.y;
	screenPos = screenPos * 2 - 1;
	float4 unprojected = mul(CameraToWorld, float4((float2)screenPos, sceneDepth, 1));
	float3 worldPos = unprojected.xyz / unprojected.w;
	float3 shadowCoord = mul(SunShadowMatrix, float4(worldPos, 1.0f)).xyz;

	SurfaceProperties Surface;
	Surface.N = normal;
	Surface.V = normalize(ViewerPos - worldPos);
	Surface.NdotV = saturate(dot(Surface.N, Surface.V));
	Surface.c_diff = baseColor.rgb * (1 - kDielectricSpecular) * (1 - metallicRoughness.x) * occlusion;
	Surface.c_spec = lerp(kDielectricSpecular, baseColor.rgb, metallicRoughness.x) * occlusion;
	Surface.roughness = metallicRoughness.y;
	Surface.alpha = metallicRoughness.y * metallicRoughness.y;
	Surface.alphaSqr = Surface.alpha * Surface.alpha;

	// Begin accumulating light starting with emissive
	float3 colorAccum = emissive;

	float ssao = texSSAO[pixelPos];

	Surface.c_diff *= ssao;
	Surface.c_spec *= ssao;

#if 1
	{
		//colorAccum += Surface.c_diff * AmbientIntensity;

		// Add IBL
		colorAccum += Diffuse_IBL(Surface) * AmbientIntensity;
		colorAccum += Specular_IBL(Surface) * AmbientIntensity;

		colorAccum += ApplyDirectionalLightPBR(Surface, SunDirection, SunIntensity, shadowCoord, texSunShadow);
		ShadeLightsPBR(colorAccum, pixelPos, Surface, worldPos);
	}
#else
	{
		// Old-school ambient light
		colorAccum += Surface.c_diff * AmbientIntensity;

		float3 viewDir = Surface.V;
		float gloss = 128.0;


		colorAccum += ApplyDirectionalLight(Surface.c_diff,
			Surface.c_spec,
			Surface.alpha,
			gloss,
			normal,
			viewDir,
			SunDirection,
			SunIntensity,
			shadowCoord,
			texSunShadow);

		ShadeLights(colorAccum,
			pixelPos,
			Surface.c_diff,
			Surface.c_spec,
			Surface.alpha,
			gloss,
			normal,
			viewDir,
			worldPos
		);
	}
#endif
	g_screenOutput[pixelPos] = float4(colorAccum, baseColor.a);
	//return float4(colorAccum, baseColor.a);;
}