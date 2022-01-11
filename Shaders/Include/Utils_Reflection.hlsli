#ifndef UTILS_REFLECTION_H_INCLUDED
#define UTILS_REFLECTION_H_INCLUDED

#include "Common.hlsli"

// Reflection
bool Reflect::ValidataNormal(inout float3 normal)
{
		// Check if normal is real and non-zero
	const float lenSq = dot(normal, normal);
	if (!isfinite(lenSq) || lenSq < 1e-6)
	{
		return false;
	}

	normal = normal * rsqrt(lenSq);

	return true;
}
Ray Reflect::ReflectRay(Ray primaryRay, float3 normal)
{
	Ray ray;
	ray.m_direction = Reflect::GetReflectedDirection(primaryRay.m_direction, normal);
	ray.m_origin = Reflect::LiftRayOrigin(primaryRay.m_origin, primaryRay.m_direction);

	return ray;
}
float3 Reflect::GetReflectedDirection(float3 primaryRayDirection, float3 normal)
{
	return reflect(primaryRayDirection, normal);
}
float3 Reflect::GetReflectedDirection(float3 primaryRayDirection, float3 normal, float alphaSqr, float2 eps)
{
	const float3 H = ImportanceSamplingGGX(eps, alphaSqr, normal);

	return GetReflectedDirection(primaryRayDirection, H);
}

float3 Reflect::LiftRayOrigin(float3 origin, float3 primaryRayDirection)
{
	return origin - primaryRayDirection * 0.1f; // Lift off the surface a 
}


// Reference: Real Shading in Unreal Engine 4
// by Brian Karis
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float3 Reflect::SampleToWorld(in float phi, in float cosTheta, in float sinTheta, in float3 N)
{
	float3 H;

	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;

	const float3 upVec = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	const float3 tangentX = normalize(cross(upVec, N));
	const float3 tangentY = cross(N, tangentX);

	return tangentX * H.x + tangentY * H.y + N * H.z;
}

// Importance sample a GGX specular function
float3 Reflect::ImportanceSamplingGGX(in float2 eps, in float alphaSqr, in float3 N)
{
	static const float XM_2PI = 6.283185307179586476925286766559f;

	const float phiAngle = XM_2PI * eps.x;
	const float cosTheta = sqrt((1 - eps.y) / (1 + (alphaSqr - 1) * eps.y));
	const float sinTheta = sqrt(1 - cosTheta * cosTheta);

	return SampleToWorld(phiAngle, cosTheta, sinTheta, N);
}

// ----------------------------------------------------------------------------
float3 Reflect::SampleSphericalDome(float2 eps, float baseRadius)
{
	//if (useCosineWeightedSampleDistribution)
	//{
	//	uv.x = sqrt(uv.x);
	//}

	//const float3 domeSampleDirection = SampleSphericalDome(additiveRecurrenceSequenceElement, ambientOcclusionSphericalDomeRadius);
	//const float3 rayDirection = mul(invertedBasis, domeSampleDirection);

	static const float XM_2PI = 6.283185307179586476925286766559f;

	const float phiAngle = XM_2PI * eps.x;
	const float sinTheta = eps.y * baseRadius;
	const float cosTheta = sqrt(1.0f - sinTheta * sinTheta);


	return float3(sinTheta * cos(phiAngle), sinTheta * sin(phiAngle), cosTheta);
}

// ----------------------------------------------------------------------------
float3x3 Reflect::OrthonormalBasis(float3 normal)
{
	const int mark = (normal.z > 0.0f) * 2 - 1;

	const float a = -1.0f / (mark + normal.z);
	const float b = normal.x * normal.y * a;

	const float3 tangent = float3(1.0f + mark * normal.x * normal.x * a, mark * b, -mark * normal.x);
	const float3 bitangent = float3(b, mark + normal.y * normal.y * a, -normal.y);

	return float3x3(tangent, bitangent, normal);
}
#endif // UTILS_REFLECTION_H_INCLUDED