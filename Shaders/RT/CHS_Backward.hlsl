

#include "Common.hlsli"



[shader("closesthit")]
void ClosestHit(inout RT::BackwardPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	// Crate view vector in hit point and extract vertex and material data
	// As it reflection ray then view vector should be pointed to ray origin, not camera
	const Ray viewRay						= RT::GeometryViewRay( WorldRayOrigin() ); 

	// Pass threadId in vertex data extraction for SampleGrad support if needed
	const uint2 threadId					= Res::RT::GridCoords();
	const Vertex vertex						= RT::ExtractVertexData(attr.barycentrics, viewRay.m_origin, threadId);
	const Mat::Material matData				= Mat::Tex::LoadData(vertex, Res::RT::MaterialConstants());

	// Init light accumulator with view vector pointed to camera 
	Lights::LightsAccumulator lightsAcc		= Lights::InitAccumulator(matData, viewRay);


	// We trace reflected rays out of screen space so accumulate proper lights
	lightsAcc.AccumulateAll();


	// Update payload to send color back to RGS
	payload.Color = lightsAcc.m_colSum;
}
