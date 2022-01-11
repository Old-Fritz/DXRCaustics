#ifndef RES_COMPUTE_H_INCLUDED
#define RES_COMPUTE_H_INCLUDED

#include "Common.hlsli"

struct DeferredConstants
{
	row_major float4x4 m_cameraToWorld;
	float2 m_resolution;
	uint2 padding;
};

// output
RWTexture2D<float4> g_screenOutput : register(u0);

cbuffer DeferredCB : register(b0)
{
	DeferredConstants g_deferredCB;
}



float4x4 Res::MainCameraToWorld()
{
	return g_deferredCB.m_cameraToWorld;
}
uint2 Res::ScreenResolution()
{
	return g_deferredCB.m_resolution;
}
RWTexture2D<float4> Res::GetScreenOutput()
{
	return g_screenOutput;
}

uint2 Res::MapsArrayResolution()
{
	return 0;
}

uint Res::LightIndex()
{
	return 0;
}



#endif