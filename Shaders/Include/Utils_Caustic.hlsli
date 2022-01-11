#ifndef RT_CAUSTICS_H_INCLUDED
#define RT_CAUSTICS_H_INCLUDED


#include "Common.hlsli"

Random::RandomHandle RT::Caustic::InitCausticRandomHandle()
{
	return Rand::InitRandom(Res::RT::CausticRaysPerPixel() + 3);
}
uint RT::Caustic::GetNumCausticRays(float eps)
{
	const float rpp = Res::RT::CausticRaysPerPixel();
	return trunc(rpp) + (eps < frac(rpp) ? 1 : 0);
}
void RT::Caustic::TraceCausticRay(float3 origin, float3 direction, float3 color, uint hitCount, float rayT, float TMin, float TMax)
{
	const RayDesc rayDesc = RT::CreateRayDesc(origin, direction, TMin, TMax);

	RT::CausticPayload rayPayLoad;
	rayPayLoad.RayHitT = rayT;
	rayPayLoad.Color = color;
	rayPayLoad.Count = hitCount;

	RT::TraceAnyRay(rayPayLoad, rayDesc);
}


void RT::Caustic::GenerateCausticRays(inout Random::RandomHandle rh, PBR::LightHit lightHit, uint numRays, Ray primaryRay, float3 color, uint hitCount, float rayT, float TMin, float TMax)
{
	// origin doesn't changes
	const float3 origin = Reflect::LiftRayOrigin(primaryRay.m_origin, primaryRay.m_direction);

	// generate rays
	for (int i = 0; i < numRays; ++i)
	{
		// generate new direction according surface properties and random 'eps'
		const float3 direction = Reflect::GetReflectedDirection(primaryRay.m_direction, lightHit.m_surf.m_normal, lightHit.m_surf.m_alphaSqr, rh.NextRandom());

		// calculate amount of light that will be reflected to generated direction from new view
		lightHit.SetView(direction);
		// update light params for new view
		lightHit.SetLight(-primaryRay.m_direction);
		// calc specular factor to scale payload color
		const float3 specular = lightHit.CalcSpecularFactor();
		// launch new ray 
		TraceCausticRay(origin, direction, color * specular, hitCount, rayT, TMin, TMax);
	}
}

void RT::Caustic::GenerateCausticMapRays(inout Random::RandomHandle rh, uint2 threadId, uint lightIndex, uint numRays)
{
	Lights::LightData lightData = Res::GetLightData(lightIndex);

	// find first intersection using caustic map with depth for this light source
	Ray primaryRay;
	primaryRay.m_origin = Screen::ArrayIntersectionPoint(threadId, lightIndex, lightData.m_cameraToWorld);

	// compute light direction to intersection point with distance info
	const Lights::LightDirection lightDir = lightData.DirDataNorm(primaryRay.m_origin);
	primaryRay.m_direction = -lightDir.m_dir;

	// calc fall off factors
	float coneFalloff = lightData.ConeFallOff(lightDir.m_dir);
	if (coneFalloff > 0)
	{
		coneFalloff /= Res::RT::CausticRaysPerPixel();
		const float lightDist = rcp(lightDir.m_invLightDist);
		const float maxDist = lightData.m_radius - lightDist;

		// don't trace empty rays
		if (maxDist > 0)
		{
			// Extract material data for intersection point and prepare all data for light hit
			const Mat::Material matData = Mat::Array::LoadData(threadId, lightIndex);
			const PBR::LightHit lightHit = PBR::BuildSurface(matData);

			GenerateCausticRays(rh, lightHit, numRays, primaryRay, lightData.m_color * coneFalloff, 1, lightDist, 0.0f, maxDist);
		}

	}

}

void RT::Caustic::GenerateForwardCausticRays(inout Random::RandomHandle rh, uint2 threadId, uint lightIndex, uint numRays)
{
	Lights::LightData lightData = Res::GetLightData(lightIndex);

	// generate rays
	for (int i = 0; i < numRays; ++i)
	{
		// Generate forward rays with small random offsets inside texel
		const Ray ray = Screen::GenerateArrayLightRay(threadId + rh.NextRandom(), lightData);

		float coneFalloff = lightData.ConeFallOff(-ray.m_direction);
;
		if (coneFalloff > 0)
		{
			coneFalloff /= Res::RT::CausticRaysPerPixel();
			// we begin with 0 counts, maximum distance as radius, and 0 ray length, and full color (according cone falloff)
			TraceCausticRay(ray.m_origin, ray.m_direction, lightData.m_color * coneFalloff, 0, 0.0f, 0.0f, lightData.m_radius);
		}
	}
}



#endif // RT_CAUSTICS_H_INCLUDED