

#include "Common.hlsli"



[shader("raygeneration")]
void RayGen()
{
	// read normal in GBuffer to generate reflected rays
	const uint2 threadId	= Res::RT::GridCoords();
	const float3 normal		= Mat::GBuf::Normal(threadId);
#ifdef VALIDATE_NORMAL
	if (!RT::ValidataNormal(normal)) return;
#endif

	// Find direction of primary rays ising Depth value in GBuffer and reflect it
	const Ray primaryRay	= Screen::GBufferPrimaryRay(threadId);
	const Ray ray			= Reflect::ReflectRay(primaryRay, normal);

	// Prepare structs for tracing
	const RayDesc rayDesc	= RT::CreateRayDesc(ray);
	RT::BasePayload payload	= RT::CreateBasePayload();

	// trace on infinity distance
	RT::TraceAnyRay(payload, rayDesc);
}

