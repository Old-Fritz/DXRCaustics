//#pragma once

#ifndef COMMON_RT_H_INCLUDED
#define COMMON_RT_H_INCLUDED

#include "Common.hlsli"

// materials
#define Slot_BaseColorTexture					6
#define Slot_MetallicRoughnessTexture			7
#define Slot_OcclusionTexture					8
#define Slot_EmissiveTexture					9
#define Slot_NormalTexture						10

#define Slot_BaseColorSampler					0
#define Slot_MetallicRoughnessSampler			1
#define Slot_OcclusionSampler					2
#define Slot_EmissiveSampler					3
#define Slot_NormalSampler						4

// lighting 
#define Slot_RadianceIBLTexture					11
#define Slot_IrradianceIBLTexture				12
#define Slot_TexSSAO							13
#define Slot_TexSunShadow						14

// light grid
#define Slot_LightBuffer						15
#define Slot_LightShadowArrayTex				16
#define Slot_LightGrid							17
#define Slot_LightGridBitMask					18

// GBuffer
#define Slot_GBDepth							20
#define Slot_GBBaseColor						21
#define Slot_GBMetallicRoughness				22
#define Slot_GBOcclusion						23
#define Slot_GBEmissive							24
#define Slot_GBNormal							25

#define Slot_ArrayGBDepth						26
#define Slot_ArrayGBBaseColor					27
#define Slot_ArrayGBMetallicRoughness			28
#define Slot_ArrayGBOcclusion					29
#define Slot_ArrayGBEmissive					30
#define Slot_ArrayGBNormal						31

#define MIP_LEVEL 1

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