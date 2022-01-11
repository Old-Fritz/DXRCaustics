

#include "Common.hlsli"



[shader("closesthit")]
void ClosestHit(inout RT::BasePayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	// update distance
	payload.RayHitT = RayTCurrent();
	if (payload.SkipShading)
	{
		return;
	}

	// Crate view vector in hit point and extract vertex and material data
	// For reflection rays view direction should ve pointed to ray origin, nor camera
	const float3 originVec						= g_rtCb.IsReflection ? WorldRayOrigin() : Res::ViewerPos();
	const Ray viewRay							= RT::GeometryViewRay(originVec);

	// Pass threadId in vertex data extraction for SampleGrad support if needed
	const uint2 threadId						= Res::RT::GridCoords();
	const Vertex vertex							= RT::ExtractVertexData(attr.barycentrics, viewRay.m_origin, threadId);
	const Mat::Material matData					= Mat::Tex::LoadData(vertex, Res::RT::MaterialConstants());

	// Init light accumulator with view vector pointed to camera 
	Lights::LightsAccumulator lightsAcc			= Lights::InitAccumulator(matData, viewRay);


	// calculate all lights, but without screen space effects for reflections
	if (g_rtCb.IsReflection)
	{
		lightsAcc.AccumulateAll();

		// All materials reflect the same (used for tests)
		float reflectivity = 0.5;
		lightsAcc.m_colSum = Output::ReadOutput(threadId).xyz + reflectivity * lightsAcc.m_colSum;
	}
	else
	{
		lightsAcc.AccumulateScreenSpace(threadId);
	}

	// Write out final sum directly to output target
	Output::WriteOutput(threadId, lightsAcc.m_colSum);
}
