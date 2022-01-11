

#include "Common.hlsli"



void AccumulateReflection(inout Lights::LightsAccumulator lightsAcc)
{
	const uint sampleCount			= Res::RT::ReflectionSamplesCount();
	const Ray primaryRay			= { lightsAcc.m_worldPos, -lightsAcc.m_hit.m_view.m_viewDir };

	Random::RandomHandle rh			= RT::Rand::InitRandom(sampleCount);
	float3 sampleAccum				= 0;

	for (uint i = 0; i < sampleCount; i++)
	{
		RT::BackwardPayload rayPayload = RT::CreateBacwardRayPayload();;

		const Ray reflRay = RT::TraceReflcetedRay(rayPayload, lightsAcc.m_hit.m_surf.m_normal, 
		                                          lightsAcc.m_hit.m_surf.m_alphaSqr, primaryRay, rh.NextRandom());

		// calculate specular factor for reflected light to calculate impact 
		lightsAcc.m_hit.SetLight(reflRay.m_direction);
		//ray.m_specular = lightsAcc.m_hit.CalcSpecularFactor();

		sampleAccum += rayPayload.Color * lightsAcc.m_hit.CalcSpecularFactor();
	}

	lightsAcc.m_colSum += sampleAccum / sampleCount;
}


[shader("raygeneration")]
void RayGen()
{
	// Crate view vector in GBuffer intersection point and load material data for this point
	const uint2 threadId						= Res::RT::GridCoords();

	const Ray viewRay							= Screen::GBufferViewRay(threadId);
	const Mat::Material matData					= Mat::GBuf::LoadData(threadId);

	// Init light accumulator with view vector pointed to camera and calculate all lights (including screen dependent effect like AO)
	Lights::LightsAccumulator lightsAcc			= Lights::InitAccumulator(matData, viewRay);
	lightsAcc.AccumulateScreenSpace(threadId);

	// Trace reflected rays and sum result
	AccumulateReflection(lightsAcc);

	// Write out final sum
	Output::WriteOutput(threadId, lightsAcc.m_colSum);
}
