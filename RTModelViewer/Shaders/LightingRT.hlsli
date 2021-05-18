#ifndef LIGHTING_RT_H_INCLUDED
#define LIGHTING_RT_H_INCLUDED


// Shading
void ShadeSunLight(inout float3 colorAccum, SurfaceProperties Surface, float3 worldPos)
{
	if (UseShadowRays)
	{
		colorAccum += ApplyDirectionalLightRT(Surface, SunDirection.xyz, SunIntensity.xyz, worldPos);
	}
	else
	{
		float3 shadowCoord = mul(SunShadowMatrix, float4(worldPos, 1.0f)).xyz;
		colorAccum += ApplyDirectionalLightPBR(Surface, SunDirection.xyz, SunIntensity.xyz, shadowCoord, texShadow);
	}
}

void ApplySSAO(inout SurfaceProperties Surface, uint2 pixelPos)
{
	float ssao = texSSAO[pixelPos];

	Surface.c_diff *= ssao;
	Surface.c_spec *= ssao;
}

void AccumulateLights(inout float3 colorAccum, SurfaceProperties Surface, float3 worldPos, uint2 pixelPos)
{
	ApplySSAO(Surface, pixelPos);

	colorAccum += Surface.c_diff * AmbientIntensity.xyz;

	ShadeSunLight(colorAccum, Surface, worldPos);
	ShadeLightsPBR(colorAccum, pixelPos, Surface, worldPos);
}

void AccumulateLights(inout float3 colorAccum, SurfaceProperties Surface, float3 worldPos)
{
	colorAccum += Surface.c_diff * AmbientIntensity.xyz;

	ShadeSunLight(colorAccum, Surface, worldPos);
	ShadeLightsPBR(colorAccum, Surface, worldPos);
}

#endif