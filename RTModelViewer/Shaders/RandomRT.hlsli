#ifndef RANDOM_RT_H_INCLUDED
#define RANDOM_RT_H_INCLUDED

struct RandomHandle
{
	float2 seed;
	uint seqLength;
	uint currentElement;
};



// Helper function for Hammersley sequences
float RadicalInverse(in uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10;
}

// Hammersly sequence
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 Hammersley2D(in uint i, in uint N)
{
	return float2(float(i) / float(N), RadicalInverse(i));
}

float2 GetRecSeqSeed()
{
	const uint3 dispatchRaysIndex = DispatchRaysIndex();
	const uint noiseSize = 1024;

	const float2 additiveRecurrenceSequenceSeed = BlueNoiseRGBA.Load(int3(dispatchRaysIndex.xy % noiseSize, 0)).rg;

	//const float3 domeSampleDirection = SampleSphericalDome(additiveRecurrenceSequenceElement, ambientOcclusionSphericalDomeRadius);
	//const float3 rayDirection = mul(invertedBasis, domeSampleDirection);

	return additiveRecurrenceSequenceSeed;
}

float GetRecSeqIndex(in uint i, in uint N)
{
	return AdditiveRecurrenceSequenceIndexBasis * N + i;
}

float2 GetRecSeqElement(float2 recSeqSeed, float recSeqIndex)
{
	return frac(recSeqSeed + AdditiveRecurrenceSequenceAlpha * recSeqIndex);
}

float2 GetRecSeqElement(float2 recSeqSeed, float2 recSeqIndex)
{
	return frac(recSeqSeed + AdditiveRecurrenceSequenceAlpha * recSeqIndex);
}

float2 GetSingleRandomHammersley()
{
	return GetRecSeqElement(GetRecSeqSeed(), Hammersley2D(0, 1).y);
}

float2 GetSingleRandom()
{
	return GetRecSeqElement(GetRecSeqSeed(), GetRecSeqIndex(0, 1));
}

RandomHandle RandomInit(uint count)
{
	RandomHandle rh;
	rh.seed = GetRecSeqSeed();
	rh.seqLength = count;
	rh.currentElement = 0;

	return rh;
}

float2 GetNextRandom(inout RandomHandle rh)
{
	float2 el = GetRecSeqElement(rh.seed, GetRecSeqIndex(rh.currentElement, rh.seqLength));
	rh.currentElement = (rh.currentElement + 1) % rh.seqLength;

	return el;
}



// Reference: Real Shading in Unreal Engine 4
// by Brian Karis
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float3 SampleToWorld(in float phi, in float cosTheta, in float sinTheta, in float3 N)
{
	float3 H;

	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;

	float3 upVec = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangentX = normalize(cross(upVec, N));
	float3 tangentY = cross(N, tangentX);

	return tangentX * H.x + tangentY * H.y + N * H.z;
}

// Importance sample a GGX specular function
float3 ImportanceSamplingGGX(in float2 uv, in float a, in float3 N)
{
	static const float XM_2PI = 6.283185307179586476925286766559f;

	float phiAngle = XM_2PI * uv.x;
	float cosTheta = sqrt((1 - uv.y) / (1 + (a * a - 1) * uv.y));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

	return SampleToWorld(phiAngle, cosTheta, sinTheta, N);
}

// ----------------------------------------------------------------------------
float3 SampleSphericalDome(float2 uv, float baseRadius = 1.0f)
{
	//if (useCosineWeightedSampleDistribution)
	//{
	//	uv.x = sqrt(uv.x);
	//}

	static const float XM_2PI = 6.283185307179586476925286766559f;

	const float sinTheta = uv.x * baseRadius;
	const float cosTheta = sqrt(1.0f - uv.x * uv.x * baseRadius * baseRadius);

	const float phiAngle = uv.y * XM_2PI;

	return float3(sinTheta * cos(phiAngle), sinTheta * sin(phiAngle), cosTheta);
}

// ----------------------------------------------------------------------------
float3x3 OrthonormalBasis(float3 normal)
{
	const int mark = (normal.z > 0.0f) * 2 - 1;

	const float a = -1.0f / (mark + normal.z);
	const float b = normal.x * normal.y * a;

	const float3 tangent = float3(1.0f + mark * normal.x * normal.x * a, mark * b, -mark * normal.x);
	const float3 bitangent = float3(b, mark + normal.y * normal.y * a, -normal.y);

	return float3x3(tangent, bitangent, normal);
}

#endif