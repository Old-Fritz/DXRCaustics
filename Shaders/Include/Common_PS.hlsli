//#pragma once

#ifndef COMMON_PS_H_INCLUDED
#define COMMON_PS_H_INCLUDED

#include "Common.hlsli"

// materials
#define Slot_BaseColorTexture				0
#define Slot_MetallicRoughnessTexture		1
#define Slot_OcclusionTexture				2
#define Slot_EmissiveTexture				3
#define Slot_NormalTexture					4

#define Slot_BaseColorSampler				0
#define Slot_MetallicRoughnessSampler		1
#define Slot_OcclusionSampler				2
#define Slot_EmissiveSampler				3
#define Slot_NormalSampler					4

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

// GBuffer
#define Slot_ArrayGBDepth					20
#define Slot_ArrayGBBaseColor				21
#define Slot_ArrayGBMetallicRoughness		22
#define Slot_ArrayGBOcclusion				23
#define Slot_ArrayGBEmissive				24
#define Slot_ArrayGBNormal					25

#define Slot_GBDepth						26
#define Slot_GBBaseColor					27
#define Slot_GBMetallicRoughness			28
#define Slot_GBOcclusion					29
#define Slot_GBEmissive						30
#define Slot_GBNormal						31


#ifdef HLSL_2021
template<typename T>
T SampleTex(SamplerState samp, Texture2D<T> tex, TexCoords coords)
{
	return tex.Sample(samp, coords.xy);
}
#else
float4 SampleTex(SamplerState samp, Texture2D<float4> tex, TexCoords coords)
{
	return tex.Sample(samp, coords.xy);
}
float3 SampleTex(SamplerState samp, Texture2D<float3> tex, TexCoords coords)
{
	return tex.Sample(samp, coords.xy);
}
float2 SampleTex(SamplerState samp, Texture2D<float2> tex, TexCoords coords)
{
	return tex.Sample(samp, coords.xy);
}
float1 SampleTex(SamplerState samp, Texture2D<float1> tex, TexCoords coords)
{
	return tex.Sample(samp, coords.xy);
}
#endif


#endif // COMMON_PS_H_INCLUDED