
#ifndef RES_MATERIAL_TEXTURES_H_INCLUDED
#define RES_MATERIAL_TEXTURES_H_INCLUDED


#include "Common.hlsli"

// local gbuffer textures
Texture2D<float4> g_BaseColorTexture			: T_REG(Slot_BaseColorTexture);
Texture2D<float3> g_MetallicRoughnessTexture	: T_REG(Slot_MetallicRoughnessTexture);
Texture2D<float1> g_OcclusionTexture			: T_REG(Slot_OcclusionTexture);
Texture2D<float3> g_EmissiveTexture				: T_REG(Slot_EmissiveTexture);
Texture2D<float3> g_NormalTexture				: T_REG(Slot_NormalTexture);

SamplerState g_BaseColorSampler					: T_REG(Slot_BaseColorSampler);
SamplerState g_MetallicRoughnessSampler			: T_REG(Slot_MetallicRoughnessSampler);
SamplerState g_OcclusionSampler					: T_REG(Slot_OcclusionSampler);
SamplerState g_EmissiveSampler					: T_REG(Slot_EmissiveSampler);
SamplerState g_NormalSampler					: T_REG(Slot_NormalSampler);


#endif // RES_MATERIAL_TEXTURES_H_INCLUDED