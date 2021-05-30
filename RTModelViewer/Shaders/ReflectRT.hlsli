#ifndef REFLECT_RT_H_INCLUDED
#define REFLECT_RT_H_INCLUDED


float3 GetReflectedDirection(SurfaceProperties Surface, float3 viewDirection, inout RandomHandle rh)
{
	float2 eps = GetNextRandom(rh);

	float3 H = ImportanceSamplingGGX(eps, Surface.alpha, Surface.N);

	return reflect(-viewDirection, H);
}


//#define VALIDATE_NORMAL
void AccumulateReflection(inout float3 colorAccum, SurfaceProperties Surface, float3 primaryRayDirection, float3 worldPos)
{
	//float2 reflectence = gBuf.metallicRoughness.xy;
	//if (reflectence.x == 0.0)
	//	return;

	float3 sampleAccum = 0;
	uint sampleCount = 1;

	RandomHandle rh = RandomInit(sampleCount);

	for (uint i = 0; i < sampleCount; i++)
	{
		// R
		float3 direction = GetReflectedDirection(Surface, primaryRayDirection, rh);
		float3 origin = worldPos - primaryRayDirection * 0.1f;	 // Lift off the surface a bit

		RayDesc rayDesc = { origin,
			0.0f,
			direction,
			FLT_MAX };

		BackwardRayPayload payload;
		//payload.SkipShading = false;
		//payload.RayHitT = FLT_MAX;
		payload.Color = 0;
		TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);

		float3 color = UnPackDec4(payload.Color).xyz;

		// Diffuse & specular factors
		float3 diffuse, specular;
		CalcReflectionFactors(Surface, direction, diffuse, specular);

		sampleAccum += color * specular;
	}

	colorAccum += sampleAccum / sampleCount;
}

#endif