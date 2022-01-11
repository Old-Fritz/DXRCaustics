

#include "Common.hlsli"



[shader("miss")]
void Miss(inout RT::BasePayload payload)
{
	if (!payload.SkipShading && !g_rtCb.IsReflection)
	{
		// just fill with 0
		uint2 threadId = Res::RT::GridCoords();
		Output::WriteOutput(threadId, 0);
	}
}

