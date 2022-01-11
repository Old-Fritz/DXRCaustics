
#ifndef RES_G_BUFFER_H_INCLUDED
#define RES_G_BUFFER_H_INCLUDED


#include "Common.hlsli"

Texture2D<float>						g_GBDepth					: T_REG(Slot_GBDepth);
Texture2D<float4>						g_GBBaseColor				: T_REG(Slot_GBBaseColor);
Texture2D<float2>						g_GBMetallicRoughness		: T_REG(Slot_GBMetallicRoughness);
Texture2D<float1>						g_GBOcclusion				: T_REG(Slot_GBOcclusion);
Texture2D<float3>						g_GBEmissive				: T_REG(Slot_GBEmissive);
Texture2D<float4>						g_GBNormal					: T_REG(Slot_GBNormal);


Texture2D<float> Res::MainDepth() { return g_GBDepth; }

#endif // RES_G_BUFFER_H_INCLUDED