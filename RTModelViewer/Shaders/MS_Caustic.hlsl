#define HLSL
#include "RayTracingInclude.hlsli"

[shader("miss")]
void Miss(inout CausticRayPayload payload)
{
	////if (!payload.SkipShading)
	{
		payload.Color = PackDec4(float4(GetSkybox(WorldRayDirection()), 1));
	}
	//if (!payload.SkipShading && !IsReflection)
	//{
	//	g_screenOutput[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
	//}
}

