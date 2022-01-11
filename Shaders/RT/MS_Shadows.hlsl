

#include "Common.hlsli"



[shader("miss")]
void Miss(inout RT::BasePayload payload)
{
	if (!payload.SkipShading)
	{
		// just fill with 0
		const uint2 threadId = Res::RT::GridCoords();
		Output::WriteOutput(threadId, 0);
	}
}