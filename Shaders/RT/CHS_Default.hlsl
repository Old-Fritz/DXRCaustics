

#include "Common.hlsli"




[shader("closesthit")]
void ClosestHit(inout RT::BasePayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.RayHitT = RayTCurrent();
	if (!payload.SkipShading)
	{
		// Just store barycentrics for debug purposes
		Output::WriteOutput(Res::RT::GridCoords(), attr.barycentrics, 1);
	}
}


