#ifndef RES_PIXEL_H_INCLUDED
#define RES_PIXEL_H_INCLUDED

#include "Common.hlsli"





#ifdef USE_GBUFFER
struct MRT
{
	float4 m_baseColor			: SV_Target0;
	float2 m_metallicRoughness	: SV_Target1;
	float  m_occlusion			: SV_Target2;
	float3 m_emissive			: SV_Target3;
	float3 m_normal				: SV_Target4;
};
#endif



uint Res::LightIndex()
{
	return 0;
}
uint2 Res::MapsArrayResolution()
{
	return 0;
}
float4x4 Res::MainCameraToWorld()
{
	return 0;
}
uint2 Res::ScreenResolution()
{
	return 0;
}

#endif