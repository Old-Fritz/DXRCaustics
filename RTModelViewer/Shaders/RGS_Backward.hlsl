//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#define HLSL
#include "RayTracingInclude.hlsli"

[shader("raygeneration")]
void RayGen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	float2 readGBufferAt = DTid.xy + 0.5;

	  // ---------------------------------------------- //
	 // ------------ SURFACE CREATION ---------------- //
	// ---------------------------------------------- //

	float3 worldPos = GetScreenSpaceIntersectionPoint(readGBufferAt);
	float3 primaryRayDirection = normalize(ViewerPos.xyz - worldPos); // normalize(ViewerPos.xyz - worldPos)

	GBuffer gBuf = ExtractScreenSpaceGBuffer(readGBufferAt);
	SurfaceProperties Surface = BuildSurface(gBuf, primaryRayDirection);

	  // ---------------------------------------------- //
	 // ----------------- SHADING -------------------- //
	// ---------------------------------------------- //

	// Begin accumulating light starting with emissive
	float3 colorAccum = gBuf.emissive;

	AccumulateLights(colorAccum, Surface, worldPos, DTid);

	  // ---------------------------------------------- //
	 // ----------------- REFLECTIONS ---------------- //
	// ---------------------------------------------- //

	AccumulateReflection(colorAccum,Surface, primaryRayDirection, worldPos);

	
	  // ---------------------------------------------- //
	 // ----------------- OUTPUT --------------------- //
	// ---------------------------------------------- //

	g_screenOutput[DispatchRaysIndex().xy] = float4(colorAccum, 1.0);
}

