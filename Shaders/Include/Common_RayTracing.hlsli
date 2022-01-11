//#pragma once

#ifndef INCLUDE_RAYTRACING_H_INCLUDED
#define INCLUDE_RAYTRACING_H_INCLUDED

#include "Common.hlsli"

namespace RT
{
	/// payloads ///
	struct BasePayload
	{
		bool SkipShading;
		float RayHitT;
	};

	struct BackwardPayload
	{
		float3 Color;
	};

	struct CausticPayload
	{
		float3 Color;
		uint Count;
		float RayHitT;
	};



	/// extract data
	float3		GeometryIntersectionPoint();
	Ray			GeometryViewRay(float3 primaryOrigin = Res::ViewerPos());

	Vertex		ExtractVertexData(float2 barycentrics, float3 worldPos, uint2 threadID);

	// any hit
	float		GetTransparency(float3 worldPos, float2 barycentrics);
	void		TestTransparency(float transparency);


	/// Trace params

	// payloads
	BasePayload		CreateBasePayload();
	BackwardPayload	CreateBacwardRayPayload();
	CausticPayload	CreateCausticRayPayload();

	// ray desc
	RayDesc CreateRayDesc(float3 origin, float3 direction, float offset = 0.0f, float maxDist = FLT_MAX);
	RayDesc CreateRayDesc(Ray ray, float offset = 0.0f, float maxDist = FLT_MAX);

	// traces
	template<typename PayloadType>
	void TraceAnyRay(inout PayloadType rayPayload, RayDesc rayDesc, uint flags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES)
	{
		TraceRay(Res::RT::Tlas(),	// Tlas
		flags,						// Ray Flags
		~0,							// Acceptable instance groups
		0,							// Offset in shader table
		1,							// Multiplier in shader table
		0,							// MissShader Index
		rayDesc,					// ray
		rayPayload);				// payload
	}

	void TraceShadowRay(inout BasePayload payload, RayDesc rayDesc);

	Ray TraceReflcetedRay(inout BackwardPayload rayPayload, float3 normal, float alphaSq, Ray primaryRay, float2 eps, float TMin = 0, float TMax = FLT_MAX);


	///  Random 
	namespace Rand
	{
		uint2 RecSeqIndex(uint3 index);

		float2 RecSeqBlueNoiseSeed( uint3 index	= Res::RT::GridInd());
		float2 RecSeqHammersleySeed(uint3 index	= Res::RT::GridInd());
		float2 RecSeqSeed(			uint3 index = Res::RT::GridInd());

		Random::RandomHandle InitRandom(uint count);
	}

	/// Caustics
	namespace Caustic
	{
		//caustic
		Random::RandomHandle InitCausticRandomHandle();
		uint GetNumCausticRays(float eps);
		void TraceCausticRay(float3 origin, float3 direction, float3 color, uint hitCount, float rayT, float TMin, float TMax);
		void GenerateCausticRays(inout Random::RandomHandle rh, PBR::LightHit lightHit, uint numRays, Ray primaryRay, float3 color, uint hitCount, float rayT, float TMin, float TMax);

		void GenerateCausticMapRays(	inout Random::RandomHandle rh, uint2 threadId, uint lightIndex, uint numRays);
		void GenerateForwardCausticRays(inout Random::RandomHandle rh, uint2 threadId, uint lightIndex, uint numRays);
	};
}



#endif // INCLUDE_RAYTRACING_H_INCLUDED