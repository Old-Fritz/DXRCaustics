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
		colorAccum += ApplyDirectionalLightPBR(Surface, SunDirection.xyz, SunIntensity.xyz, shadowCoord, lightSunShadow);
	}
}

void ApplySSAO(inout SurfaceProperties Surface, uint2 pixelPos)
{
	float ssao = lightSSAO[pixelPos];

	Surface.c_diff *= ssao;
	Surface.c_spec *= ssao;
}

void AccumulateAmbient(inout float3 colorAccum, SurfaceProperties Surface)
{
	// Add IBL
	colorAccum += Diffuse_IBL(Surface);
	colorAccum += Specular_IBL(Surface);

	//colorAccum += Surface.c_diff * AmbientIntensity.xyz;
}

void AccumulateLights(inout float3 colorAccum, SurfaceProperties Surface, float3 worldPos, uint2 pixelPos)
{
	ApplySSAO(Surface, pixelPos);

	AccumulateAmbient(colorAccum, Surface);

	ShadeSunLight(colorAccum, Surface, worldPos);
	ShadeLightsPBR(colorAccum, pixelPos, Surface, worldPos);
}

void AccumulateLights(inout float3 colorAccum, SurfaceProperties Surface, float3 worldPos)
{
	AccumulateAmbient(colorAccum, Surface);

	ShadeSunLight(colorAccum, Surface, worldPos);
	ShadeLightsPBR(colorAccum, Surface, worldPos);
}

#endif