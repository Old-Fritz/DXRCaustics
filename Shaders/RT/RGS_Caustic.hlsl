

#include "Common.hlsli"



[shader("raygeneration")]
void RayGen()
{
	const uint2 threadId = Res::RT::GridCoords();

	// Experimental feature - deny processing for half of pixels in checkerboard order. Pixels swap every frame.
	if (Res::RT::FailCheckerBoard(threadId)) return;

	// Generate number of rays
	Random::RandomHandle rh = RT::Caustic::InitCausticRandomHandle();
	const uint numRays		= RT::Caustic::GetNumCausticRays(rh.NextRandom().y);

	// select one og method to genererate first rays
	if(Res::RT::UseFeature1())
	{
		// Trace rays from light position
		RT::Caustic::GenerateForwardCausticRays(rh, threadId, Res::RT::LightIndex(), numRays);
		return;
	}

	// Trace rays from positions in caustic maps
	RT::Caustic::GenerateCausticMapRays(rh, threadId, Res::RT::LightIndex(), numRays);
}