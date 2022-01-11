#ifndef LIGHT_SHADOWS_H_INCLUDED
#define LIGHT_SHADOWS_H_INCLUDED



#include "Common.hlsli"

//#include "Common.hlsli"
//#include "Light_Common.hlsli"


//#include "Common.hlsli"

#ifndef HLSL_2021
float SampleShadow(float3 shadowCoord, Texture2D<float> texShadow)
{
	return texShadow.SampleCmpLevelZero(shadowSampler, shadowCoord.xy, saturate(shadowCoord.z + 0.01f));
}

float SampleShadow(uint lightIndex, float3 shadowCoord, Texture2DArray<float> texShadow)
{
	return texShadow.SampleCmpLevelZero(shadowSampler, float3(shadowCoord.xy, lightIndex), saturate(shadowCoord.z + 0.01f));
}

float GetShadow(float3 shadowCoord, Texture2D<float> texShadow)
{
	float result = SampleShadow(shadowCoord, texShadow);

	return result * result;
}
float GetShadow(uint lightIndex, float3 shadowCoord, Texture2DArray<float> texShadow)
{
	float result = SampleShadow(lightIndex, shadowCoord, texShadow);

	return result * result;
}

float GetSampledShadow(float3 shadowCoord, float texelSize, Texture2D<float> texShadow)
{
	const float Dilation = 2.0 * texelSize;
	float d1 = Dilation * 0.125;
	float d2 = Dilation * 0.875;
	float d3 = Dilation * 0.625;
	float d4 = Dilation * 0.375;
	float result = (
		SampleShadow(shadowCoord, texShadow) * 2.0 +
		SampleShadow(shadowCoord + (float3)float4(-d2,  d1, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4(-d1, -d2, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4( d2, -d1, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4( d1,  d2, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4(-d4,  d3, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4(-d3, -d4, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4( d4, -d3, 0, 0), texShadow) +
		SampleShadow(shadowCoord + (float3)float4( d3,  d4, 0, 0), texShadow)
		) / 10.0;

	return result * result;
}
float GetSampledShadow(float3 shadowCoord, Texture2D<float> texShadow)
{
	return GetSampledShadow(shadowCoord, g_globalCB.ShadowTexelSize.x, texShadow);
}

float GetSampledShadow(uint lightIndex, float3 shadowCoord, float texelSize, Texture2DArray<float> texShadow)
{
	const float Dilation = 2.0 * texelSize;
	float d1 = Dilation * 0.125;
	float d2 = Dilation * 0.875;
	float d3 = Dilation * 0.625;
	float d4 = Dilation * 0.375;
	float result = (
		SampleShadow(lightIndex, shadowCoord, texShadow) * 2.0 +
		SampleShadow(lightIndex, shadowCoord + float3(-d2,	d1, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3(-d1, -d2, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3( d2, -d1, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3( d1,  d2, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3(-d4,  d3, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3(-d3, -d4, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3( d4, -d3, 0.0), texShadow) +
		SampleShadow(lightIndex, shadowCoord + float3( d3,  d4, 0.0), texShadow)
		) / 10.0;

	return result * result;
}
float GetSampledShadow(uint lightIndex, float3 shadowCoord, Texture2DArray<float> texShadow)
{
	return GetSampledShadow(lightIndex, shadowCoord, 1.0 / 512.0f, texShadow); //g_globalCB.ShadowTexelSize.x, texShadow);

	//return GetSampledShadow(lightIndex, shadowCoord, g_globalCB.ShadowTexelSize.x, texShadow);
}
#else

namespace ShadowsInternal
{
	template<typename Coords, typename Resource>
	float SampleShadow(Coords shadowCoord, float z, Resource texShadow)
	{
		return texShadow.SampleCmpLevelZero(shadowSampler, shadowCoord, saturate(z + 0.01f));
	}
	
	template<typename Coords, typename Resource>
	float GetShadow(Coords shadowCoord, float z, Resource texShadow)
	{
		float result = SampleShadow(shadowCoord, z, texShadow);
	
		return result * result;
	}
	
	template<typename Coords, typename Resource>
	float GetSampledShadow(Coords shadowCoord, float texelSize, float z, Resource texShadow)
	{
		const float Dilation = 2.0 * texelSize;
		const float d1 = Dilation * 0.125;
		const float d2 = Dilation * 0.875;
		const float d3 = Dilation * 0.625;
		const float d4 = Dilation * 0.375;
		float result = (
			SampleShadow(shadowCoord							, z, texShadow) * 2.0 +
			SampleShadow(shadowCoord + (Coords)float4(-d2,  d1, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4(-d1, -d2, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4( d2, -d1, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4( d1,  d2, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4(-d4,  d3, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4(-d3, -d4, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4( d4, -d3, 0, 0), z, texShadow) +
			SampleShadow(shadowCoord + (Coords)float4( d3,  d4, 0, 0), z, texShadow)
			) / 10.0;
	
		return result * result;
	}
}
#endif

float Shadows::SampleShadow(		float2 coord, float z,					Texture2D<float> tex)			{ return ShadowsInternal::SampleShadow(	coord, z, tex); }
float Shadows::GetShadow(			float2 coord, float z,					Texture2D<float> tex)			{ return ShadowsInternal::GetShadow(	coord, z, tex); }
float Shadows::GetSampledShadow(	float2 coord, float texel,	float z,	Texture2D<float> tex)			{ return ShadowsInternal::GetSampledShadow(coord, texel, z, tex); }

float Shadows::SampleShadow(		float3 coord, float z,					Texture2DArray<float> tex)		{ return ShadowsInternal::SampleShadow(	coord, z, tex); }
float Shadows::GetShadow(			float3 coord, float z,					Texture2DArray<float> tex)		{ return ShadowsInternal::GetShadow(	coord, z, tex); }
float Shadows::GetSampledShadow(	float3 coord, float texel,	float z,	Texture2DArray<float> tex)		{ return ShadowsInternal::GetSampledShadow(coord, texel, z, tex); }

float Shadows::GetDefaultArrayTexelSize()	{ return 1.0 / 512.0f; }
float Shadows::GetDefaultTexelSize()		{ return g_globalCB.ShadowTexelSize.x; }

float Shadows::GetDefaultSampledShadow(float2 shadowCoord, float z, Texture2D<float> texShadow)
{
	return ShadowsInternal::GetSampledShadow(shadowCoord, GetDefaultTexelSize(), z, texShadow);
}
float Shadows::GetDefaultSampledShadow(float3 shadowCoord, float z, Texture2DArray<float> texShadow)
{
	return ShadowsInternal::GetSampledShadow(shadowCoord, GetDefaultArrayTexelSize(), z, texShadow);
}


float3 Shadows::CalcShadowCoord(float4x4 shadowMat, float3 worldPos)
{
	float4 shadowCoord = mul(shadowMat, float4(worldPos, 1.0));
	
	return shadowCoord.xyz / shadowCoord.w;
}





// rt experiments
#if 0
float SampleShadowRT(float3 shadowDirection, float3 shadowOrigin, float maxDistance)
{
	float shadow = 1.0;

	RayDesc rayDesc;
	rayDesc.Origin = shadowOrigin;
	rayDesc.Direction = shadowDirection;
	rayDesc.TMin = 0.01f;
	rayDesc.TMax = maxDistance - 0.01f;

	BasePayload shadowPayload;
	shadowPayload.SkipShading = true;
	shadowPayload.RayHitT = maxDistance;

	TraceRay(g_accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, rayDesc, shadowPayload);

	if (shadowPayload.RayHitT < maxDistance)
	{
		shadow = 0.0;
	}

	return shadow;
}

float GetShadowRT(float3 shadowDirection, float3 shadowOrigin, float maxDistance)
{
	float result = SampleShadowRT(shadowDirection, shadowOrigin, maxDistance);

	return result * result;
}

float GetSampledShadowRT(float3 shadowDirection, float3 shadowOrigin, float texelSize, float maxDistance)
{
	float3 lightPosition = shadowOrigin + shadowDirection;

	const float Dilation = 2.0 * texelSize;
	float d1 = Dilation * 0.125;
	float d2 = Dilation * 0.875;
	float d3 = Dilation * 0.625;
	float d4 = Dilation * 0.375;
	float result = (
		SampleShadowRT(shadowDirection, shadowOrigin, maxDistance) * 2.0 +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d2, d1, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d1, -d2, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(d2, -d1, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(d1, d2, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d4, d3, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d3, -d4, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(d4, -d3, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(d3, d4, 0.0), maxDistance)
		) / 10.0;

	return result * result;
}
#endif

#endif // LIGHT_SHADOWS_H_INCLUDED
