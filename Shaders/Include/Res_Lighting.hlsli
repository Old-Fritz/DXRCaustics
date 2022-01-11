
#ifndef RES_LIGHTING_H_INCLUDED
#define RES_LIGHTING_H_INCLUDED

#include "Common.hlsli"

TextureCube<float3> g_RadianceIBLTexture		: T_REG(Slot_RadianceIBLTexture);
TextureCube<float3> g_IrradianceIBLTexture		: T_REG(Slot_IrradianceIBLTexture);
Texture2D<float>	g_TexSSAO					: T_REG(Slot_TexSSAO);
Texture2D<float>	g_TexSunShadow				: T_REG(Slot_TexSunShadow);


#endif // RES_LIGHTING_H_INCLUDED