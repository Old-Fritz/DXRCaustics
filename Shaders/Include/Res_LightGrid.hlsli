
#ifndef RES_LIGHT_GRID_H_INCLUDED
#define RES_LIGHT_GRID_H_INCLUDED

#include "Common.hlsli"

StructuredBuffer<Lights::LightData>		g_LightBuffer				: T_REG(Slot_LightBuffer);
Texture2DArray<float>					g_LightShadowArrayTex		: T_REG(Slot_LightShadowArrayTex);
ByteAddressBuffer						g_LightGrid					: T_REG(Slot_LightGrid);
ByteAddressBuffer						g_LightGridBitMask			: T_REG(Slot_LightGridBitMask);


// enable to amortize latency of vector read in exchange for additional VGPRs being held
#define LIGHT_GRID_PRELOADING

// configured for 32 sphere lights, 64 cone lights, and 32 cone shadowed lights
#define POINT_LIGHT_GROUPS			1
#define SPOT_LIGHT_GROUPS			2
#define SHADOWED_SPOT_LIGHT_GROUPS	1
#define POINT_LIGHT_GROUPS_TAIL			POINT_LIGHT_GROUPS
#define SPOT_LIGHT_GROUPS_TAIL				POINT_LIGHT_GROUPS_TAIL + SPOT_LIGHT_GROUPS
#define SHADOWED_SPOT_LIGHT_GROUPS_TAIL	SPOT_LIGHT_GROUPS_TAIL + SHADOWED_SPOT_LIGHT_GROUPS



Lights::LightData Res::GetLightData(uint lightIndex)			{ return g_LightBuffer[lightIndex]; }
float4x4 Res::ArrayCameraToWorld(uint lightIndex)				{ return GetLightData(lightIndex).m_cameraToWorld; }


uint4 Res::PreLoadLightGrid(uint tileIndex)
{
	return g_LightGridBitMask.Load4(tileIndex * 16);
}

uint Res::GetGroupBits(uint groupIndex, uint tileIndex, uint lightBitMaskGroups[4])
{
#ifdef LIGHT_GRID_PRELOADING
	return lightBitMaskGroups[groupIndex];
#else
	return g_LightGridBitMask.Load(tileIndex * 16 + groupIndex * 4);
#endif
}


#endif // RES_LIGHTING_H_INCLUDED