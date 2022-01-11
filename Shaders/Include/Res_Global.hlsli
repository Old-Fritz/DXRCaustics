

#ifndef RES_GLOBAL_H_INCLUDED
#define RES_GLOBAL_H_INCLUDED

//#include "Common_Header.hlsli"
#include "Common.hlsli"

//#include "Common.hlsli"


// Common (static) samplers
SamplerState			defaultSampler	: register(s10);
SamplerComparisonState	shadowSampler	: register(s11);
SamplerState			cubeMapSampler	: register(s12);

cbuffer GlobalCB : register(b1)
{
	GlobalConstants g_globalCB;
}

#if !defined(STAGES_VS)


float4x4	Res::MainViewProjMatrix()			{ return g_globalCB.ViewProjMatrix; }
float3		Res::ViewerPos()					{ return g_globalCB.ViewerPos; }
uint		Res::FrameIndexMod2()				{ return g_globalCB.FrameIndexMod2; }

#endif



#endif