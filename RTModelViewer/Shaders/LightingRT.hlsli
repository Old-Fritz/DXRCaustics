#include "../../MiniEngine/Model/Shaders/LightingPBR.hlsli"


float GetDirectionalShadowRT(float3 shadowDirection, float3 shadowOrigin)
{
	float shadow = 1.0;

	RayDesc rayDesc = { shadowOrigin,
		0.1f,
		shadowDirection,
		FLT_MAX };
	RayPayload shadowPayload;
	shadowPayload.SkipShading = true;
	shadowPayload.RayHitT = FLT_MAX;
	TraceRay(g_accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, rayDesc, shadowPayload);
	if (shadowPayload.RayHitT < FLT_MAX)
	{
		shadow = 0.0;
	}


	return shadow;
}

float3 ApplyDirectionalLightRT(
	SurfaceProperties Surface,
	float3 L,
	float3 c_light,
	float3  shadowOrigin    // pos
)
{
	float shadow = GetDirectionalShadowRT(L, shadowOrigin);

	return shadow * ApplyLightCommonPBR(Surface, L, c_light);
}


float3 ApplyConeShadowedLightRT(SurfaceProperties Surface, float3 c_light,
	float3	worldPos,		// World-space fragment position
	float3	lightPos,		// World-space light position
	float	lightRadiusSq,
	float3	coneDir,
	float2	coneAngles
)
{
	float3 lightDir = lightPos - worldPos;
	float lightDistSq = dot(lightDir, lightDir);
	float invLightDist = rsqrt(lightDistSq);
	float lightDist = 1.0f / invLightDist;
	lightDir *= invLightDist;

	// modify 1/d^2 * R^2 to fall off at a fixed radius
	// (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
	float distanceFalloff = lightRadiusSq * (invLightDist * invLightDist);
	distanceFalloff = max(0, distanceFalloff - rsqrt(distanceFalloff));

	float coneFalloff = dot(-lightDir, coneDir);
	coneFalloff = saturate((coneFalloff - coneAngles.y) * coneAngles.x);

	float shadow = 1.0;

	if (coneFalloff && distanceFalloff)
	{
		RayDesc rayDesc = { worldPos,
			0.01f,
			lightDir,
			lightDist - 0.01f };
		RayPayload shadowPayload;
		shadowPayload.SkipShading = true;
		shadowPayload.RayHitT = lightDist;
		TraceRay(g_accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, rayDesc, shadowPayload);
		if (shadowPayload.RayHitT < lightDist)
		{
			shadow = 0.0;
		}
	}

	return shadow * (coneFalloff * distanceFalloff) *
		ApplyLightCommonPBR(Surface, lightDir, c_light);
}