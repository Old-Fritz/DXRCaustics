

#include "Common.hlsli"



[shader("closesthit")]
void ClosestHit(inout RT::CausticPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.RayHitT += RayTCurrent();

	Lights::LightData lightData = g_LightBuffer[Res::RT::LightIndex()];

	const float distFalloff = lightData.DistFalloff(rcp(payload.RayHitT));

	if (distFalloff > 0)
	{
		// Create view vector pointing to main camera
		const Ray viewRay						= RT::GeometryViewRay();

		// Transform world coordinate to main screen coordinate
		const float3 screenCoords				= Screen::GetCoords(viewRay.m_origin);

		// Extract material params from meshes and textures and prepare light hit structure
		const Vertex vertex						= RT::ExtractVertexData(attr.barycentrics, viewRay.m_origin, screenCoords.xy);
		const Mat::Material matData				= Mat::Tex::LoadData(vertex, Res::RT::MaterialConstants());
		PBR::LightHit lightHit					= PBR::BuildSurface(matData);

		const float3 primaryRayDir				= WorldRayDirection();



		if (payload.Count > 0 && Screen::CoordsVisible(screenCoords))
		{
			lightHit.SetView(viewRay.m_direction);
			lightHit.SetLight(-primaryRayDir);
			const float3 specular	= lightHit.CalcSpecularFactor();
			const float3 diffuse	= lightHit.CalcDiffuseFactor();

			float3 accumValue = (diffuse + specular) * payload.Color * distFalloff;

			float3 cameraDir = viewRay.m_origin - g_globalCB.ViewerPos.xyz;
			float distFromCameraSq = dot(cameraDir, cameraDir);


			//accumValue /= distFromCameraSq;
			//accumValue *= payload.RayHitT * payload.RayHitT;


			g_screenOutput[screenCoords.xy] += float4(accumValue, 1) / distFromCameraSq * payload.RayHitT * payload.RayHitT * g_rtCb.causticPowerScale;
		}

						// Try to launch next ray
		if (payload.Count < g_rtCb.causticMaxRayRecursion)
		{
			Random::RandomHandle rh = RT::Rand::InitRandom(1);

			const Ray primaryRay = { viewRay.m_origin, primaryRayDir };
			RT::Caustic::GenerateCausticRays(rh, lightHit, 1, primaryRay, payload.Color, payload.Count + 1, payload.RayHitT, 0.0f, lightData.m_radius - payload.RayHitT);
		}
	}
}

