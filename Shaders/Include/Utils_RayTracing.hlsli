#ifndef UTILS_RT_H_INCLUDED
#define UTILS_RT_H_INCLUDED

#include "Common.hlsli"


/// Extract data
float3 RT::GeometryIntersectionPoint()
{
	return WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}
	
Vertex RT::ExtractVertexData(float2 barycentrics, float3 worldPos, uint2 threadID)
{
	//Vertex vertex = (Vertex) 0;

	const uint materialID = Res::RT::MaterialId();
	const uint triangleID = PrimitiveIndex();

	VB::MeshDataExtractor dataExtractor;
	dataExtractor.m_vertex = (Vertex) 0;
	dataExtractor.m_info = g_meshInfo[materialID];
	// per mesh data
	//MeshLayoutInfo info = g_meshInfo[materialID];

	//MeshConstantsRT meshConstants = g_meshConstants[materialID];
	//float3x3 worldIT = float3x3(meshConstants.worldIT_0, meshConstants.worldIT_1, meshConstants.worldIT_2);
	//worldIT = float3x3(0.01f, 0, 0,
	//	0, 0.01f, 0,
	//	0, 0, 0.01f);
	
	// triangle intersection data
	const uint3 indices = VB::Load3x16BitIndices(Res::RT::MeshData(), dataExtractor.m_info.m_indexOffsetBytes + triangleID * 3 * 2);
	const float3 bary		= float3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);


	dataExtractor.ExtractNormal(Res::RT::MeshData(), indices, bary);
#ifdef USE_SAMPLE_GRAD
	dataExtractor.ExtractDerivativesUV(Res::RT::MeshData(), indices, bary, worldPos, threadID, g_rtCb.resolution, g_rtCb.cameraToWorld, g_rtCb.worldCameraPosition);
#else
	dataExtractor.ExtractBaseUV(Res::RT::MeshData(), indices, bary);
#endif

	return dataExtractor.m_vertex;

	//return ExtractMeshData(Res::RT::MeshData(), triangleID, attr.barycentrics, worldPos, screenCoords, info);
}

Ray RT::GeometryViewRay(float3 primaryOrigin)
{
	Ray ray;
	ray.m_origin	= RT::GeometryIntersectionPoint();				// World Pos
	ray.m_direction = GetDirection(ray.m_origin, primaryOrigin);
		
	return ray;
}

// any hit
float RT::GetTransparency(float3 worldPos, float2 barycentrics)
{
	const Vertex vertex = RT::ExtractVertexData(barycentrics, worldPos, Res::RT::GridCoords());
	return Mat::Tex::Transparency(vertex, Res::RT::MaterialFlags());
}
void RT::TestTransparency(float transparency)
{
	if (transparency < 0.15f)
	{
		IgnoreHit();
	}
}





/// Trace params


// payloads
RT::BasePayload RT::CreateBasePayload()
{
	BasePayload payload;
	payload.SkipShading = false;
	payload.RayHitT = FLT_MAX;

	return payload;
}
RT::BackwardPayload RT::CreateBacwardRayPayload()
{
	RT::BackwardPayload payload;
	payload.Color = 0;

	return payload;

}
RT::CausticPayload RT::CreateCausticRayPayload()
{
	RT::CausticPayload payload;
	payload.RayHitT = 0;
	payload.Color = 0;
	payload.Count = 1;

	return payload;
}


// ray desc
RayDesc RT::CreateRayDesc(float3 origin, float3 direction, float offset, float maxDist)
{
	const RayDesc rayDesc = { origin, offset, direction, maxDist };
	return rayDesc;
}
RayDesc RT::CreateRayDesc(Ray ray, float offset, float maxDist)
{
	return CreateRayDesc(ray.m_origin, ray.m_direction, offset, maxDist);
}


void RT::TraceShadowRay(inout BasePayload payload, RayDesc rayDesc)
{
	TraceAnyRay(payload, rayDesc, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH);
}

Ray RT::TraceReflcetedRay(inout RT::BackwardPayload rayPayload, float3 normal, float alphaSq, Ray primaryRay, float2 eps, float TMin, float TMax)
{
	Ray ray;
	// reflect primary ray in random direction (by 'eps') using importance sampling
	ray.m_direction			= Reflect::GetReflectedDirection(primaryRay.m_direction, normal, alphaSq, eps);
	// lift ray origin a bit to avoid z-fighting
	ray.m_origin			= Reflect::LiftRayOrigin(primaryRay.m_origin, primaryRay.m_direction);

	// create desc and trace ray
	const RayDesc rayDesc = CreateRayDesc(ray.m_origin, ray.m_direction, TMin, TMax);

	TraceAnyRay(rayPayload, rayDesc);

	return ray;
}

// random
uint2					RT::Rand::RecSeqIndex(uint3 index)						{ return index.xy + Res::RT::GridRes() * uint2(index.z % 2, index.z / 2); }
float2					RT::Rand::RecSeqBlueNoiseSeed( uint3 index)				{ return Random::RecSeqBlueNoiseSeed(RecSeqIndex(index), Res::RT::BlueNoiseTex());	}
float2					RT::Rand::RecSeqHammersleySeed(uint3 index)				{ return Random::RecSeqHammersleySeed(index, Res::RT::GridDims());		}
float2					RT::Rand::RecSeqSeed(uint3 index )						{ return Res::RT::UseFeature2() ? RecSeqHammersleySeed(index) : RecSeqBlueNoiseSeed(index);; }
 
Random::RandomHandle	RT::Rand::InitRandom(uint elCount)						{ return Random::InitRandom(elCount, Res::RT::RecSeqBasis(), Res::RT::RecSeqAlpha(), RecSeqSeed()); }

#endif