

#include "Common.hlsli"



[shader("anyhit")]
void AnyHit(inout RT::CausticPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
	// Extract only UV from mesh and Alpha from material textures for fast alpha kill
	const float transparency = RT::GetTransparency(RT::GeometryIntersectionPoint(), attrib.barycentrics);
	RT::TestTransparency(transparency);
}



