#ifndef SCENE_RT_H_INCLUDED
#define SCENE_RT_H_INCLUDED

#include "Common.hlsli"

// Vertex ops
TexCoords VB::GetUV(Vertex vertex, uint matFlags, uint offset)
{
#ifndef NO_SECOND_UV
#ifdef USE_SAMPLE_GRAD
	const bool useSecondUV = (matFlags >> offset) & 1;
	const TexCoords res =
	{
			lerp(vertex.m_uv0.xy,	vertex.m_uv1.xy,	useSecondUV)
		,	lerp(vertex.m_uv0.ddxUV,vertex.m_uv1.ddxUV,	useSecondUV)
		,	lerp(vertex.m_uv0.ddyUV,vertex.m_uv1.ddyUV,	useSecondUV)
	};
	return res;
#else
	return lerp(vertex.m_uv0, vertex.m_uv1, (matFlags >> offset) & 1);
#endif
#else
	return vertex.m_uv0;
#endif
}

// data load 
uint3	VB::Load3x16BitIndices(	ByteAddressBuffer meshData, uint offsetBytes)
{
	const uint dwordAlignedOffset = offsetBytes & ~3;

	const uint2 four16BitIndices = meshData.Load2(dwordAlignedOffset);

	uint3 indices;

	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x			& 0xffff;
		indices.y = (four16BitIndices.x >> 16)	& 0xffff;
		indices.z = four16BitIndices.y			& 0xffff;
	}
	else
	{
		indices.x = (four16BitIndices.x >> 16)	& 0xffff;
		indices.y = four16BitIndices.y			& 0xffff;
		indices.z = (four16BitIndices.y >> 16)	& 0xffff;
	}

	return indices;
}


float2	VB::LoadUVAttribute(		ByteAddressBuffer meshData, uint byteOffset)
{
	return Unpacking::UnPackFloat16_2(meshData.Load(byteOffset));
}
float4	VB::LoadNormalAttribute(	ByteAddressBuffer meshData, uint byteOffset)
{
// TODO: change to worldIT mul
	return (Unpacking::UnPackDec4(meshData.Load(byteOffset)) * 2 - 1) / 100.0f;
}
float4	VB::LoadTangentAttribute(	ByteAddressBuffer meshData, uint byteOffset)
{
// TODO: change to worldIT mul
	return (Unpacking::UnPackDec4(meshData.Load(byteOffset)) * 2 - 1) / 100.0f;
}
float3	VB::LoadPositionAttribute(	ByteAddressBuffer meshData, uint byteOffset)
{
	return asfloat(meshData.Load3(byteOffset)) * 100.0f;
}

// helpers
float3 VB::RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection)
{
	const float t = dot(-planeNormal, rayOrigin - planeOrigin) / dot(planeNormal, rayDirection);
	return rayOrigin + rayDirection * t;
}
float3 VB::BarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2)
{
	// REF: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	// From "Real-Time Collision Detection" by Christer Ericson

	const float3 e0 = v1 - v0;
	const float3 e1 = v2 - v0;
	const float3 e2 = pt - v0;
	const float d00 = dot(e0, e0);
	const float d01 = dot(e0, e1);
	const float d11 = dot(e1, e1);
	const float d20 = dot(e2, e0);
	const float d21 = dot(e2, e1);
	const float denom = 1.0 / (d00 * d11 - d01 * d01);
	const float v = (d11 * d20 - d01 * d21) * denom;
	const float w = (d00 * d21 - d01 * d20) * denom;
	const float u = 1.0 - v - w;
	return float3(u, v, w);
}


// mesh expotter
void VB::MeshDataExtractor::ExtractNormal(ByteAddressBuffer meshData, uint3 indices, float3 bary)
{
	const uint3 address = indices * m_info.m_attributeStrideBytes;
	for (uint i = 0; i < 3; i++)
	{
		m_vertex.m_normal	+= LoadNormalAttribute(	meshData, address[i] + m_info.m_normalAttributeOffsetBytes).xyz	* bary[i];
#ifndef NO_TANGENT_FRAME
		m_vertex.m_tangent	+= LoadTangentAttribute(meshData, address[i] + m_info.m_tangentAttributeOffsetBytes)	* bary[i];
#endif
	}

#ifndef NO_TANGENT_FRAME
	m_vertex.m_tangent.w *= 100;
#endif
}
void VB::MeshDataExtractor::ExtractBaseUV(ByteAddressBuffer meshData, uint3 indices, float3 bary)
{
	const uint3 address = indices * m_info.m_attributeStrideBytes;
	for (uint i = 0; i < 3; i++)
	{
		m_vertex.m_uv0.xy		+= LoadUVAttribute(meshData, address[i] + m_info.m_uvAttributeOffsetBytes) * bary[i];
	}
}
void VB::MeshDataExtractor::ExtractDerivativesUV(ByteAddressBuffer meshData, uint3 indices, float3 bary, float3 worldPos, uint2 threadID, uint2 resolution, float4x4 cameraToWorld, float3 cameraPos)
{
	const uint3 address = indices * m_info.m_attributeStrideBytes;

	// TODO: Should just store uv partial derivatives in here rather than loading position and caculating it per pixel
	const float3 p0		= LoadPositionAttribute(	meshData, address.x + m_info.m_positionAttributeOffsetBytes	);
	const float3 p1		= LoadPositionAttribute(	meshData, address.y + m_info.m_positionAttributeOffsetBytes	);
	const float3 p2		= LoadPositionAttribute(	meshData, address.z + m_info.m_positionAttributeOffsetBytes	);

	const float2 uv0	= LoadUVAttribute(			meshData, address.x + m_info.m_uvAttributeOffsetBytes		);
	const float2 uv1	= LoadUVAttribute(			meshData, address.y + m_info.m_uvAttributeOffsetBytes		);
	const float2 uv2	= LoadUVAttribute(			meshData, address.z + m_info.m_uvAttributeOffsetBytes		);
	m_vertex.m_uv0.xy = bary.x * uv0 + bary.y * uv1 + bary.z * uv2;

	//---------------------------------------------------------------------------------------------
	// Compute partial derivatives of UV coordinates:
	//
	//  1) Construct a plane from the hit triangle
	//  2) Intersect two helper rays with the plane:  one to the right and one down
	//  3) Compute barycentric coordinates of the two hit points
	//  4) Reconstruct the UV coordinates at the hit points
	//  5) Take the difference in UV coordinates as the partial derivatives X and Y

	// Normal for plane
	const float3 triangleNormal = normalize(cross(p2 - p0, p1 - p0));

	// Helper rays
	//float3 ddxOrigin, ddxDir, ddyOrigin, ddyDir;
	const Ray ddxRay = Screen::GenerateCameraRay(uint2(threadID.x + 1, threadID.y), resolution, cameraToWorld, cameraPos);
	const Ray ddyRay = Screen::GenerateCameraRay(uint2(threadID.x, threadID.y + 1), resolution, cameraToWorld, cameraPos);

	// Intersect helper rays
	const float3 xOffsetPoint = RayPlaneIntersection(worldPos, triangleNormal, ddxRay.m_origin, ddxRay.m_direction);
	const float3 yOffsetPoint = RayPlaneIntersection(worldPos, triangleNormal, ddyRay.m_origin, ddyRay.m_direction);

	// Compute barycentrics 
	const float3 baryX			= BarycentricCoordinates(xOffsetPoint, p0, p1, p2);
	const float3 baryY			= BarycentricCoordinates(yOffsetPoint, p0, p1, p2);

	// Compute UVs and take the difference
	const float3x2 uvMat = float3x2(uv0, uv1, uv2);
#ifdef USE_SAMPLE_GRAD
	m_vertex.m_uv0.ddxUV = mul(baryX, uvMat) - m_vertex.m_uv0.xy;
	m_vertex.m_uv0.ddyUV = mul(baryY, uvMat) - m_vertex.m_uv0.xy;
#endif
}
#endif //#ifndef SCENE_RT_H_INCLUDED




