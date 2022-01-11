

#include "Common.hlsli"



[shader("raygeneration")]
void RayGen()
{
	// find position by GBuffer
	const uint2 threadId		= Res::RT::GridCoords();
	const float3 worldPos		= Screen::GBufferIntersectionPoint(threadId);

	// Trace ray from pointed position to source of light
	const RayDesc shadowRayDesc = RT::CreateRayDesc(worldPos, g_globalCB.SunDirection);
	RT::BasePayload rayPayload	= RT::CreateBasePayload();

	RT::TraceAnyRay(rayPayload, shadowRayDesc);


	// if any hits have changed RayHitT value then draw shadow 
	if (rayPayload.RayHitT < FLT_MAX)
	{
		Output::WriteOutput(threadId, 0);
	}
	else
	{
		Output::WriteOutput(threadId, 1, 1, 1);
	}
}
