#define HLSL
#include "RayTracingInclude.hlsli"



[shader("anyhit")]
void AnyHit(inout BackwardRayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{

	float3 worldPos = GetIntersectionPoint();

	// ---------------------------------------------- //
   // --------- SURFACE TRANSPARENCY --------------- //
  // ---------------------------------------------- //

	//float3 screenCoords = GetScreenCoords(worldPos);
	VertexData vertex = ExtractVertexData(attrib, worldPos, float3(0,0,0));
	float transparency = ExtractTransparency(vertex);

	if (transparency < 0.1f)
	{
		IgnoreHit();
	}
}