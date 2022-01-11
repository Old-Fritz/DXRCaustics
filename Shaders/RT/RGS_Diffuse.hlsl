

#include "Common.hlsli"



[shader("raygeneration")]
void RayGen()
{
	// Just launch ray in camera view direction
	const uint2 threadId	= Res::RT::GridCoords();
	const Ray ray			= Screen::GenerateCameraRay(threadId);

	const RayDesc rayDesc	= RT::CreateRayDesc(ray);
	RT::BasePayload payload		= RT::CreateBasePayload();

	RT::TraceAnyRay(payload, rayDesc);
}

