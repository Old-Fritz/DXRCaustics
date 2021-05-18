#ifndef SHADOWS_H_INCLUDED
#define SHADOWS_H_INCLUDED

float SampleShadow(float3 ShadowCoord, Texture2D<float> texShadow)
{
	return texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy, ShadowCoord.z);
}

float GetShadow(float3 ShadowCoord, Texture2D<float> texShadow)
{
	float result = SampleShadow(ShadowCoord, texShadow);

	return result * result;
}

float GetSampledShadow(float3 ShadowCoord, float texelSize, Texture2D<float> texShadow)
{
	const float Dilation = 2.0 * texelSize;
	float d1 = Dilation * 0.125;
	float d2 = Dilation * 0.875;
	float d3 = Dilation * 0.625;
	float d4 = Dilation * 0.375;
	float result = (
		SampleShadow(ShadowCoord, texShadow) * 2.0 +
		SampleShadow(ShadowCoord + float3(-d2,  d1, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3(-d1, -d2, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3( d2, -d1, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3( d1,  d2, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3(-d4,  d3, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3(-d3, -d4, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3( d4, -d3, 0.0), texShadow) +
		SampleShadow(ShadowCoord + float3( d3,  d4, 0.0), texShadow)
		) / 10.0;

	return result * result;
}

#ifdef RAY_TRACING
float SampleShadowRT(float3 shadowDirection, float3 shadowOrigin, float maxDistance)
{
	float shadow = 1.0;

	RayDesc rayDesc;
	rayDesc.Origin = shadowOrigin;
	rayDesc.Direction = shadowDirection;
	rayDesc.TMin = 0.01f;
	rayDesc.TMax = maxDistance - 0.01f;

	RayPayload shadowPayload;
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
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d2,  d1, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d1, -d2, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3( d2, -d1, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3( d1,  d2, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d4,  d3, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3(-d3, -d4, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3( d4, -d3, 0.0), maxDistance) +
		SampleShadowRT(shadowDirection, shadowOrigin + float3( d3,  d4, 0.0), maxDistance)
		) / 10.0;

	return result * result;
}
#endif

#endif // SHADOWS_H_INCLUDED
