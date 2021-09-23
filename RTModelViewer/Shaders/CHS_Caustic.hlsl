

#define HLSL
#include "RayTracingInclude.hlsli"

//uint GetPhotonIndex()
//{
//	uint3 dims = DispatchRaysDimensions();
//	uint3 ind = DispatchRaysIndex();
//
//	return ind.x + dims.x * ind.y + dims.x * dims.y * ind.z;
//}
//
//void StorePhoton(float color, float position)
//{
//	Photon photon;
//	photon.position = position;
//	photon.color = PackDec4(color);
//
//	causticPhotonMap[GetPhotonIndex()] = photon;
//}

/*
void AccumLight(float3 color, uint2 pos)
{
	// multiply rays can write in same point simultanuesly
	uint3 iColor = uint3 color * 65 536;
	InterlockedAdd(cuasticLightBuffer[pos].x, iColor.x);
	InterlockedAdd(cuasticLightBuffer[pos].y, iColor.y);
	InterlockedAdd(cuasticLightBuffer[pos].z, iColor.z);
}

void TraceDirectCaustic(float3 rayColor, float3 worldPos)
{
	float4x4 ViewProjMat;// = inverse(g_dynamic.cameraToWorld);
	float3 CameraPos;
	uint2 ScreenRes;
	float3 screenPos = mul(ViewProjMat, float4(screenPos, sceneDepth, 1));

	// check if point in view space
	if (screenPos.xy >= 0.0f && screenPos.xy <= 1.0f && screenPos.z > 0.0f)
	{
		float3 rayDirection = ViewerPos.xyz - worldPos;
		float rayLength = length(rayDirection);
		rayDirection /= rayLength;

		RayDesc rayDesc;
		rayDesc.Origin = worldPos;
		rayDesc.Direction = normalize(ViewerPos.xyz - worldPos);
		rayDesc.TMin = 0.01f;
		rayDesc.TMax = rayLength - 0.01f;

		CausticRayPayload shadowPayload;
		shadowPayload.SkipShading = true;
		shadowPayload.RayHitT = rayLength;

		TraceRay(g_accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, rayDesc, shadowPayload);

		if (shadowPayload.RayHitT >= rayLength)
		{
			AccumLight(rayColor, ScreenRes * screenPos.xy);
		}

	}

}

void TraceReflectedCaustic(float3 rayColor, float3 reflectedDirection, float roughness, float3 worldPos)
{
	RayDesc rayDesc;
	rayDesc.Origin = worldPos;
	rayDesc.Direction = reflectedDirection;
	rayDesc.TMin = 0.01f;
	rayDesc.TMax = FLT_MAX;

	CausticRayPayload payload;
	payload.SkipShading = false;
	payload.color = rayColor;

	TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);
}

void TraceTransmittedCaustic(float3 rayColor, float3 transmitanceDirection, float3 worldPos)
{
	RayDesc rayDesc;
	rayDesc.Origin = worldPos;
	rayDesc.Direction = transmitanceDirection;
	rayDesc.TMin = 0.01f;
	rayDesc.TMax = FLT_MAX;

	CausticRayPayload payload;
	payload.SkipShading = false;
	payload.color = rayColor;

	TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payload);
}

void RussianRouletteCaustic(Surface surface, float3 rayColor, float3 diffuse, float3 specular, float occlusion, float3 worldPos)
{

	// depending on surface ray might be absorbed, transmitted or reflected (diffuse or specular)
	float3 reflectedTotal = diffuse + specular;
	float reflectedSum = reflectedTotal.x + reflectedTotal.y + reflectedTotal.z;
	float diffSum = diffuse.x + diffuse.y + diffuse.z;

	// probabilities
	float reflectionProbability = max(max(reflectedTotal.x, reflectedTotal.y), reflectedTotal.z);
	float diffuseProbability = diffSum / (specSum + diffSum) * reflectionProbability;
	float specularProbability = reflectionProbability - diffuseProbability;
	float absorbProbability = occlusion;

	//float epsiolon = RandNext(); // must be [0,1]

	if (diffuseProbability > epsilon)
	{
		if (diffuseProbability > epsilon && epsiolon <= reflectionProbability) // specular reflection
		{
			TraceReflectedCaustic(rayColor * specular / specularProbability, surface.V, surface.roughness, worldPos);
		}
		else if (epsilon <= absorbProbability) // absorbation
		{
			// End Trace
		}
		else // transmission
		{
			TraceTransmittedCaustic(rayColor, WorldRayDirection(), worldPos);
		}
	}
	else // if (epsiolon <= diffuseProbability) // diffuse reflection
	{
		// Should go last because waits for ray payload
		TraceDirectCaustic(rayColor * diffuse / diffuseProbability, worldPos);
		//StorePhoton(rayColor * diffuse, worldPos);
	}
}

void FullTraceCaustic(Surface surface, float3 rayColor, float3 diffuse, float3 specular, float occlusion, float3 worldPos)
{

	float3 diffuseColor = rayColor * diffuse;
	float3 specularColor = rayColor * diffuse;
	float3 transmittedColor = rayColor * 1.0f - occlusion;

	if (transmittedColor > 0)
	{
		TraceTransmittedCaustic(transmittedColor, WorldRayDirection(), worldPos);
	}
	if (specularColor > 0)
	{
		TraceReflectedCaustic(specularColor, surface.V, surface.roughness, worldPos);
	}
	if (diffuseColor > 0)
	{
		TraceDirectCaustic(diffuseColor, worldPos);
	}

}
*/

/*
[shader("closesthit")]
void ClosestHit(inout CausticRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//payload.RayHitT = RayTCurrent();
	//if (payload.SkipShading)
	//{
	//	return;
	//}

	float3 worldPos = GetIntersectionPoint();
	float2 xy = GetProjectWorldCoords(worldPos);

	//if (xy <= 1 && xy >= 0)
	//{
		//g_screenOutput[xy * g_dynamic.resolution] = float4(1.0, 0.0, 0.0, 1.0);
	//}
	// ---------------------------------------------- //
   // ------------ SURFACE CREATION ---------------- //
  // ---------------------------------------------- //

  //VertexData vertex = ExtractVertexData(attr, worldPos);
  //GBuffer gBuf = ExtractGBuffer(vertex);
  //
  ////float3 reflectedDirection = normalize(-WorldRayDirection() - 2 * dot(-WorldRayDirection(), normal) * normal);
  //SurfaceProperties Surface = BuildSurface(gBuf, WorldRayDirection());
  //
  //
  //float3 color = payload.color;


	// ---------------------------------------------- //
   // ----------------- SHADING -------------------- //
  // ---------------------------------------------- //

  //float3 rayColor = UnPackDec4(payload.color);

  //float3 rayColor = payload.color;

  // Diffuse & specular factors
  //float3 diffuse, specular;
  //CalcReflectionFactors(Surface, WorldRayDirection(), diffuse, specular);

//#if 0
//	RussianRouletteCaustic(Surface, diffuse, specular, gBuf.occlusion, worldPos);
//#else
//	FullTraceCaustic(Surface, diffuse, specular, gBuf.occlusion, worldPos);
//#endif
}
*/


float3 GetScreenCoords(float3 worldPos)
{
	float2 dims = g_dynamic.resolution;

	float4 p_xy = mul((ViewProjMatrix), float4(worldPos, 1.0));

	p_xy /= p_xy.w;
	p_xy.y = -p_xy.y;

	return float3((p_xy.xy + 1) / 2 * (dims - 1) + 0.5, p_xy.z);
}

struct LightParams
{
	float distanceFalloff;
	float radius;
	float3 color;
	float radiusSq;
};

LightParams GetLightParams(CausticRayPayload payload)
{
	LightParams params;

	LightData lightData = lightBuffer[DispatchRaysIndex().z];
	params.radiusSq = lightData.radiusSq;

	// modify 1/d^2 * R^2 to fall off at a fixed radius
	// (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
	//float distanceFalloff = lightRadiusSq / (payload.RayHitT * payload.RayHitT);
	//distanceFalloff = max(0, distanceFalloff - rsqrt(distanceFalloff));
	float invRadius = rsqrt(params.radiusSq);
	params.radius = 1 / invRadius;

	params.distanceFalloff = max(0, params.radiusSq / (payload.RayHitT * payload.RayHitT) - invRadius * payload.RayHitT);
	params.color = payload.Color;

	return params;
}

[shader("closesthit")]
void ClosestHit(inout CausticRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.RayHitT += RayTCurrent();

	LightParams lightParams = GetLightParams(payload);

	if (payload.SkipShading || lightParams.distanceFalloff <= 0)
	{
		return;
	}

	float3 worldPos = GetIntersectionPoint();

	float3 dirToViewer = normalize(ViewerPos.xyz - worldPos);
	worldPos += dirToViewer * 2.0f;

	float3 screenCoords = GetScreenCoords(worldPos);

	// ---------------------------------------------- //
   // ------------ SURFACE CREATION ---------------- //
  // ---------------------------------------------- //

	VertexData vertex = ExtractVertexData(attr, worldPos);
	GBuffer gBuf = ExtractGBuffer(vertex);

	// ---------------------------------------------- //
   // ----------------- SHADING -------------------- //
  // ---------------------------------------------- //

	float sceneDepth = g_mainDepth.Load(int3(screenCoords.xy, 0));

	SurfaceProperties Surface = BuildSurface(gBuf);

	// ---------------------------------------------- //
   // ----------------- REFLECTIONS ---------------- //
  // ---------------------------------------------- //

	float3 diffuse = 0;
	float3 specular = 0;

	const float MaxRecursion = g_dynamic.causticMaxRayRecursion;
	
	if (payload.Count < MaxRecursion)
	{
		RandomHandle rh = RandomInit(1);		

		float3 direction = -GetReflectedDirection(Surface, WorldRayDirection(), rh);
		float3 origin = worldPos - WorldRayDirection() * 2.0f;	 // Lift off the surface a bit

		SetSurfaceView(Surface, direction);

		CalcReflectionFactors(Surface, -WorldRayDirection(), diffuse, specular);

		RayDesc rayDesc = { origin,
			0.0f,
			direction,
			lightParams.radius - payload.RayHitT
		};

		CausticRayPayload payloadNew;
		payload.SkipShading = false;
		payload.RayHitT = payload.RayHitT;
		payloadNew.Color = lightParams.color * specular;
		payloadNew.Count = payload.Count + 1;
		TraceRay(g_accel, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, rayDesc, payloadNew);
	}

	float3 colors[4];
		colors[0] = float3(1, 0, 0);
		colors[1] = float3(0, 1, 0);
		colors[2] = float3(0, 0, 1);
		colors[3] = float3(1, 1, 0);

	if (
		screenCoords.z >= sceneDepth &&
		screenCoords.z > 0 &&
		screenCoords.x >= 0 && screenCoords.y >= 0 && screenCoords.x < g_dynamic.resolution.x && screenCoords.y < g_dynamic.resolution.y
		&& payload.Count > 0
		)
	{
		float3 accumValue = 0;
		// calc diffuse
		// Diffuse & specular factors
		SetSurfaceView(Surface, dirToViewer);
		//float3 diffuse, specular;
		CalcReflectionFactors(Surface, -WorldRayDirection(), diffuse, specular);

		//accumValue = ApplyLightCommonPBR(Surface, -WorldRayDirection(), lightParams.color);// *distanceFalloff;

		//accumValue = colors[payload.Count] * specular;
		accumValue = (diffuse+specular) * lightParams.color * lightParams.distanceFalloff;

		float cameraDir = worldPos - ViewerPos;

		float distFromCameraSq = dot(cameraDir, cameraDir);
		//float k = 5000;

		g_screenOutput[screenCoords.xy] += float4(accumValue, 1) / distFromCameraSq * lightParams.radiusSq * g_dynamic.causticPowerScale;;





		//float3 colors[4];
		//colors[0] = float3(1, 0, 0);
		//colors[1] = float3(0, 1, 0);
		//colors[2] = float3(0, 0, 1);
		//colors[3] = float3(1, 1, 0);
		//
		//
		//accumValue = Surface.N;;
		//g_screenOutput[screenPos] +=
			//float4(1, 0, 0, 1);
			//float4((diffuse + specular) * color, 1);
			//float4(dot(Surface.N, -viewDir),-dot(Surface.N, -viewDir),0, 1);
			//float4(gBuf.normal, 1);
			//float4(color, 1);
			//colors[payload.Count];
			//float4((specular + diffuse)*color, 1);
			//float4(colors[0].xyz, 1);


	}	

}

