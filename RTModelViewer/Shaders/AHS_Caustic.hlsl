#define HLSL
#include "RayTracingInclude.hlsli"



[shader("anyhit")]
void AnyHit(inout CausticRayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
	float3 worldPos = GetIntersectionPoint();

	  // ---------------------------------------------- //
	 // --------- SURFACE TRANSPARENCY --------------- //
	// ---------------------------------------------- //

	VertexData vertex = ExtractVertexData(attrib, worldPos);
	float transparency = ExtractTransparency(vertex);

	if (transparency < 0.001f)
	{
		IgnoreHit();
	}
}