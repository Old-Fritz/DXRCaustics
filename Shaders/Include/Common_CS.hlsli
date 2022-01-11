//#pragma once

#ifndef COMMON_CS_H_INCLUDED
#define COMMON_CS_H_INCLUDED

#include "Common.hlsli"

// GBuffer 
#define Slot_GBDepth						0
#define Slot_GBBaseColor					1
#define Slot_GBMetallicRoughness			2
#define Slot_GBOcclusion					3
#define Slot_GBEmissive						4
#define Slot_GBNormal						5

// lighting
#define Slot_RadianceIBLTexture				10
#define Slot_IrradianceIBLTexture			11
#define Slot_TexSSAO						12
#define Slot_TexSunShadow					13

// Light grid
#define Slot_LightBuffer					14
#define Slot_LightShadowArrayTex			15
#define Slot_LightGrid						16
#define Slot_LightGridBitMask				17

#define Slot_ArrayGBDepth					20
#define Slot_ArrayGBBaseColor				21
#define Slot_ArrayGBMetallicRoughness		22
#define Slot_ArrayGBOcclusion				23
#define Slot_ArrayGBEmissive				24
#define Slot_ArrayGBNormal					25

// materials
#define Slot_BaseColorTexture				26
#define Slot_MetallicRoughnessTexture		27
#define Slot_OcclusionTexture				28
#define Slot_EmissiveTexture				29
#define Slot_NormalTexture					30

#define Slot_BaseColorSampler				0
#define Slot_MetallicRoughnessSampler		1
#define Slot_OcclusionSampler				2
#define Slot_EmissiveSampler				3
#define Slot_NormalSampler					4


//#define USE_SAMPLE_GRAD
#define MIP_LEVEL 0

// sample textures
#ifdef HLSL_2021
template<typename T> 
T SampleTex(SamplerState samplerState, Texture2D<T> tex, TexCoords coords)
{
#ifdef USE_SAMPLE_GRAD
	return tex.SampleGrad(samplerState, coords.xy, coords.ddxUV, coords.ddyUV);
#else
	return tex.SampleLevel(samplerState, coords.xy, MIP_LEVEL);
#endif
}
#else
float4 SampleTex(SamplerState samplerState, Texture2D<float4> tex, TexCoords coords)
{
#ifdef USE_SAMPLE_GRAD
	return tex.SampleGrad(samplerState, coords.xy, coords.ddxUV, coords.ddyUV);
#else
	return tex.SampleLevel(samplerState, coords.xy, MIP_LEVEL);
#endif
}
float3 SampleTex(SamplerState samplerState, Texture2D<float3> tex, TexCoords coords)
{
#ifdef USE_SAMPLE_GRAD
	return tex.SampleGrad(samplerState, coords.xy, coords.ddxUV, coords.ddyUV);
#else
	return tex.SampleLevel(samplerState, coords.xy, MIP_LEVEL);
#endif
}
float2 SampleTex(SamplerState samplerState, Texture2D<float2> tex, TexCoords coords)
{
#ifdef USE_SAMPLE_GRAD
	return tex.SampleGrad(samplerState, coords.xy, coords.ddxUV, coords.ddyUV);
#else
	return tex.SampleLevel(samplerState, coords.xy, MIP_LEVEL);
#endif
}
float1 SampleTex(SamplerState samplerState, Texture2D<float1> tex, TexCoords coords)
{
#ifdef USE_SAMPLE_GRAD
	return tex.SampleGrad(samplerState, coords.xy, coords.ddxUV, coords.ddyUV);
#else
	return tex.SampleLevel(samplerState, coords.xy, MIP_LEVEL);
#endif
}
#endif


#endif // COMMON_PS_H_INCLUDED