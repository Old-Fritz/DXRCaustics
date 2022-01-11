

#include "Common.hlsli"



[shader("miss")]
void Miss(inout RT::BackwardPayload payload)
{
	// Sample sky texture on background
	payload.Color = Lights::GetSkybox(WorldRayDirection());
}

