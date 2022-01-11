

#ifndef RES_ARRAY_G_BUFFER_H_INCLUDED
#define RES_ARRAY_G_BUFFER_H_INCLUDED

#include "Common.hlsli"

Texture2DArray<float>					g_ArrayGBDepth							: T_REG(Slot_ArrayGBDepth);
Texture2DArray<float4>					g_ArrayGBBaseColor						: T_REG(Slot_ArrayGBBaseColor);
Texture2DArray<float2>					g_ArrayGBMetallicRoughness				: T_REG(Slot_ArrayGBMetallicRoughness);
Texture2DArray<float1>					g_ArrayGBOcclusion						: T_REG(Slot_ArrayGBOcclusion);
Texture2DArray<float3>					g_ArrayGBEmissive						: T_REG(Slot_ArrayGBEmissive);
Texture2DArray<float4>					g_ArrayGBNormal							: T_REG(Slot_ArrayGBNormal);


#endif //	RES_ARRAY_G_BUFFER_H_INCLUDED
