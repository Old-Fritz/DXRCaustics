#define HLSL
#include "RayTracingInclude.hlsli"


#if 1
[shader("raygeneration")]
void RayGen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	float2 readGBufferAt = DTid.xy + 0.5;
	LightData lightData = lightBuffer[DispatchRaysIndex().z];

	  // ---------------------------------------------- //
	 // ------------ SURFACE CREATION ---------------- //
	// ---------------------------------------------- //

	RandomHandle rh = RandomInit(g_dynamic.causticRaysPerPixel + 3);
	float2 xy = GetNextRandom(rh); // gen random epsilon to calc posability
	if (g_dynamic.causticRaysPerPixel < 1 && xy.y > g_dynamic.causticRaysPerPixel)
	{
		return;
	}





	float3 worldPos = GetScreenSpaceIntersectionPoint(readGBufferAt, lightData.cameraToWorld);
	float3 primaryRayDirection = normalize(lightData.pos - worldPos); // normalize(ViewerPos.xyz - worldPos)

	GBuffer gBuf = ExtractScreenSpaceGBuffer(readGBufferAt);
	SurfaceProperties Surface = BuildSurface(gBuf, primaryRayDirection);
	
	  // ---------------------------------------------- //
	 // ----------------- REFLEC --------------------- //
	// ---------------------------------------------- //


	float invRadius = rsqrt(lightData.radiusSq);
	float lightRadius = 1 / invRadius;

	float dist = length(lightData.pos - worldPos);
	float maxDist = lightRadius - dist;


	//float distanceFalloff = max(0, lightData.radiusSq / (dist * dist) - invRadius * dist);
	float3 lightColor = lightData.color;

	float radius = length(((float2)DispatchRaysIndex().xy / (float2)DispatchRaysDimensions().xy) * 2 - 1);
	if (radius >= 1 || maxDist <= 0)
	{
		return;
	}

	const float PI_2 = 1.57079632679f;

	float2 coneAngles = lightData.coneAngles;
	float coneFalloff = cos(radius * acos(coneAngles.y));
	coneFalloff = saturate((coneFalloff - coneAngles.y) * coneAngles.x);


	float3 origin = worldPos - primaryRayDirection * 2.0f;	 // Lift off the surface a bit

	/// gen ray
	for (int i = 0; i < g_dynamic.causticRaysPerPixel; ++i)
	{
		float3 direction = GetReflectedDirection(Surface, primaryRayDirection, rh);


		SetSurfaceView(Surface, direction);

		float3 diffuse, specular;
		CalcReflectionFactors(Surface, primaryRayDirection, diffuse, specular);

		RayDesc rayDesc = { origin,
			0.0f,
			direction,
			maxDist
		};

		CausticRayPayload payload;
		payload.SkipShading = false;
		payload.RayHitT = maxDist;
		payload.Color = lightColor * specular * coneFalloff / g_dynamic.causticRaysPerPixel;
		payload.Count = 1;
		TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);
	}


	

	//float3 origin, direction;
	//GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);
	//
	//RayDesc rayDesc = { origin,
	//	0.0f,
	//	direction,
	//	maxDist
	//};
	//
	//CausticRayPayload payload;
	//payload.SkipShading = false;
	//payload.RayHitT = 0;
	//payload.Color = lightData.color * coneFalloff;
	//payload.Count = 0;
	//TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);
	


	// ---------------------------------------------- //
   // ----------------- OUTPUT --------------------- //
  // ---------------------------------------------- //

}




/*

#define HLSL
#include "RayTracingInclude.hlsli"

[shader("raygeneration")]
void RayGen()
{
	float3 origin, direction;
	GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);

	RayDesc rayDesc = { origin,
		0.0f,
		direction,
		FLT_MAX };
	CausticRayPayload payload;
	payload.SkipShading = false;
	payload.RayHitT = FLT_MAX;
	payload.Color = float3(1, 1, 1);
	payload.Count = 0;
	TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);
}

*/

#else

#define HLSL
#include "RayTracingInclude.hlsli"

[shader("raygeneration")]
void RayGen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	float2 readGBufferAt = DTid.xy;

	// ---------------------------------------------- //
   // ------------ SURFACE CREATION ---------------- //
  // ---------------------------------------------- //

	//float3 worldPos = GetScreenSpaceIntersectionPoint(readGBufferAt);
	//float3 primaryRayDirection = normalize(ViewerPos.xyz - worldPos); // normalize(ViewerPos.xyz - worldPos)
	//
	//GBuffer gBuf = ExtractScreenSpaceGBuffer(readGBufferAt);
	//SurfaceProperties Surface = BuildSurface(gBuf, primaryRayDirection);

	// ---------------------------------------------- //
   // ----------------- SHADING -------------------- //
  // ---------------------------------------------- //

	 // Begin accumulating light starting with emissive
	//float3 colorAccum = gBuf.emissive;

	//AccumulateLights(colorAccum, Surface, worldPos, DTid);

	// ---------------------------------------------- //
   // ----------------- REFLECTIONS ---------------- //
  // ---------------------------------------------- //



	//RandomHandle rh = RandomInit(1);
	//
	//float3 direction = GetReflectedDirection(Surface, primaryRayDirection, rh);
	//float3 origin = worldPos - primaryRayDirection * 0.1f;	 // Lift off the surface a bit

	float radius = length(((float2)DispatchRaysIndex().xy / (float2)DispatchRaysDimensions().xy) * 2 - 1);
	if (radius >= 1)
	{
		return;
	}

	const float PI_2 = 1.57079632679f;

	LightData lightData = lightBuffer[DispatchRaysIndex().z];

	float2 coneAngles = lightData.coneAngles;
	float coneFalloff = cos(radius * acos(coneAngles.y));
	coneFalloff = saturate((coneFalloff - coneAngles.y) * coneAngles.x);

	float maxDist = sqrt(lightData.radiusSq);


	float3 origin, direction;
	GenerateLightCameraRay(DispatchRaysIndex().xy, origin, direction);

	RayDesc rayDesc = { origin,
		0.0f,
		direction,
		maxDist
	};

	CausticRayPayload payload;
	payload.SkipShading = false;
	payload.RayHitT = 0;
	payload.Color = lightData.color * coneFalloff;
	payload.Count = 0;
	TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);


	//AccumulateReflection(colorAccum, Surface, primaryRayDirection, worldPos);


	// ---------------------------------------------- //
   // ----------------- OUTPUT --------------------- //
  // ---------------------------------------------- //

	//float2 xy = GetProjectWorldCoords(worldPos);
	//xy.y = g_dynamic.resolution.y - xy.y;
	//xy.x = g_dynamic.resolution.x - xy.x;
	//g_screenOutput[xy] += float4(xy, 0, 1);
	//float2 dims = float2(1920, 1080);
	//
	//float4 p_xy = mul((ViewProjMatrix), float4(worldPos, 1.0));
	//
	//p_xy /= p_xy.w;
	//p_xy.y = -p_xy.y;
	//p_xy.xy = (p_xy.xy + 1) / 2 * dims + 0.5;
	//
	//g_screenOutput[p_xy.xy] += float4((p_xy.xy + 1) / 2, 0, 1);

	//g_screenOutput[DispatchRaysIndex().xy] = float4(colorAccum, 1.0);
}


#endif
/*

#define HLSL
#include "RayTracingInclude.hlsli"

[shader("raygeneration")]
void RayGen()
{
	float3 origin, direction;
	GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);

	RayDesc rayDesc = { origin,
		0.0f,
		direction,
		FLT_MAX };
	CausticRayPayload payload;
	payload.SkipShading = false;
	payload.RayHitT = FLT_MAX;
	payload.Color = float3(1, 1, 1);
	payload.Count = 0;
	TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);
}

*/