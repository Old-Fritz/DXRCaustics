#define HLSL
#include "RayTracingInclude.hlsli"



[shader("anyhit")]
void AnyHit(inout BackwardRayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{

	float3 worldPos = GetIntersectionPoint();

	// ---------------------------------------------- //
   // --------- SURFACE TRANSPARENCY --------------- //
  // ---------------------------------------------- //

	VertexData vertex = ExtractVertexData(attrib, worldPos);
	float transparency = ExtractTransparency(vertex);

	if (transparency < 1.0f)
	{
		IgnoreHit();
	}
}