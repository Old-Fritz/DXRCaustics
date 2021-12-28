//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):	James Stanard, Christopher Wallis
//

#define HLSL
#include "RayTracingInclude.hlsli"

[shader("closesthit")]
void ClosestHit(inout BackwardRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//payload.RayHitT = RayTCurrent();
	//if (payload.SkipShading)
	//{
	//	return;
	//}

	float3 worldPos = GetIntersectionPoint();

	  // ---------------------------------------------- //
	 // ------------ SURFACE CREATION ---------------- //
	// ---------------------------------------------- //

	float3 screenCoords = GetScreenCoords(worldPos);
	VertexData vertex = ExtractVertexData(attr, worldPos, screenCoords);
	GBuffer gBuf = ExtractGBuffer(vertex);

	SurfaceProperties Surface = BuildSurface(gBuf, normalize(ViewerPos.xyz - worldPos));

	  // ---------------------------------------------- //
	 // ----------------- SHADING -------------------- //
	// ---------------------------------------------- //

	// Begin accumulating light starting with emissive
	float3 colorAccum = gBuf.emissive;

	AccumulateLights(colorAccum, Surface, worldPos, DispatchRaysIndex().xy);

	  // ---------------------------------------------- //
	 // ----------------- REFLECTIONS ---------------- //
	// ---------------------------------------------- //

	//// TODO: Should be passed in via material info
	//if (IsReflection)
	//{
	//	float reflectivity = 0.5;
	//	colorAccum = g_screenOutput[DispatchRaysIndex().xy].rgb + reflectivity * colorAccum;
	//}

	  // ---------------------------------------------- //
	 // ----------------- OUTPUT --------------------- //
	// ---------------------------------------------- //

	payload.Color = float4(colorAccum, 1.0);
	//payload.Color = PackDec4(float4(colorAccum * 0.1, 1.0));
	//g_screenOutput[DispatchRaysIndex().xy] = float4(colorAccum, 1.0);
}