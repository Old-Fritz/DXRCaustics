#define HLSL
#include "RayTracingInclude.hlsli"



[shader("anyhit")]
void AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
	float3 worldPos = GetIntersectionPoint();

	// ---------------------------------------------- //
   // --------- SURFACE TRANSPARENCY --------------- //
  // ---------------------------------------------- //

	//float3 screenCoords = DispatchRaysIndex().xyz;
	float3 screenCoords = GetScreenCoords(worldPos);
	VertexData vertex = ExtractVertexData(attrib, worldPos, screenCoords);
	float transparency = ExtractTransparency(vertex);

	if (transparency < 0.1f)
	{
		IgnoreHit();
	}
}