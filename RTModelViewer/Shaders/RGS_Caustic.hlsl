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

	LightData lightData = lightBuffer[0];

	float2 coneAngles = lightData.coneAngles;
	float coneFalloff = cos(radius * acos(coneAngles.y));
	coneFalloff = saturate((coneFalloff - coneAngles.y) * coneAngles.x);

	float maxDist = sqrt(lightData.radiusSq);


	float3 origin, direction;
	GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);

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