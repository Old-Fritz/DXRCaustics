#define HLSL
#include "RayTracingInclude.hlsli"

[shader("miss")]
void Miss(inout BackwardRayPayload payload)
{
	//if (!payload.SkipShading)
	{
		payload.Color = PackDec4(float4(GetSkybox(WorldRayDirection()), 1));
	}
}

