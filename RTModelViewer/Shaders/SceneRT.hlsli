#ifndef SCENE_RT_H_INCLUDED
#define SCENE_RT_H_INCLUDED

#include "Unpacking.hlsli"


struct TextureCoords
{
	float2			uv;
	float2			ddxUV;
	float2			ddyUV;
};

struct VertexData
{
	float3			normal;
	float3			tangent;
	float3			bitangent;
	TextureCoords	tc;
};

uint3 Load3x16BitIndices(
	uint offsetBytes)
{
	const uint dwordAlignedOffset = offsetBytes & ~3;

	const uint2 four16BitIndices = g_meshData.Load2(dwordAlignedOffset);

	uint3 indices;

	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x & 0xffff;
		indices.y = (four16BitIndices.x >> 16) & 0xffff;
		indices.z = four16BitIndices.y & 0xffff;
	}
	else
	{
		indices.x = (four16BitIndices.x >> 16) & 0xffff;
		indices.y = four16BitIndices.y & 0xffff;
		indices.z = (four16BitIndices.y >> 16) & 0xffff;
	}

	return indices;
}

// unpack attributes
float2 GetUVAttribute(uint byteOffset)
{
	return UnPackFloat16_2(g_meshData.Load(byteOffset));
}
float4 GetNormalAttribute(uint byteOffset)
{
	// TODO: change to worldIT mul
	return (UnPackDec4(g_meshData.Load(byteOffset)) * 2 - 1) / 100.0f;
}
float4 GetTangentAttribute(uint byteOffset)
{
	// TODO: change to worldIT mul
	return (UnPackDec4(g_meshData.Load(byteOffset)) * 2 - 1) / 100.0f;
}
float3 GetPositionAttribute(uint byteOffset)
{
	return asfloat(g_meshData.Load3(byteOffset)) * ModelScale;
}

float3 RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection)
{
	float t = dot(-planeNormal, rayOrigin - planeOrigin) / dot(planeNormal, rayDirection);
	return rayOrigin + rayDirection * t;
}

float3 BarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2)
{
	// REF: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	// From "Real-Time Collision Detection" by Christer Ericson

	float3 e0 = v1 - v0;
	float3 e1 = v2 - v0;
	float3 e2 = pt - v0;
	float d00 = dot(e0, e0);
	float d01 = dot(e0, e1);
	float d11 = dot(e1, e1);
	float d20 = dot(e2, e0);
	float d21 = dot(e2, e1);
	float denom = 1.0 / (d00 * d11 - d01 * d01);
	float v = (d11 * d20 - d01 * d21) * denom;
	float w = (d00 * d21 - d01 * d20) * denom;
	float u = 1.0 - v - w;
	return float3(u, v, w);
}

float3 GetIntersectionPoint()
{
	return WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}

float3 GetScreenSpaceIntersectionPoint(float2 readGBufferAt, float4x4 cameraToWorld)
{
	// Screen position for the ray
	float2 screenPos = readGBufferAt / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates
	screenPos.y = -screenPos.y;

	float sceneDepth = g_GBDepth.Load(int4(readGBufferAt, DispatchRaysIndex().z, 0));

	// Unproject into the world position using depth
	float4 unprojected = mul(cameraToWorld, float4(screenPos, sceneDepth, 1));
	return unprojected.xyz / unprojected.w;
}

float3 GetProjectWorldCoords(float3 worldPos)
{
	return mul(ViewProjMatrix, float4(worldPos, 1.0));
}

VertexData ExtractVertexData(in BuiltInTriangleIntersectionAttributes attr, float3 worldPos)
{
	VertexData vertex;

	uint materialID = MaterialID;
	uint triangleID = PrimitiveIndex();

	// per mesh data
	RayTraceMeshInfo info = g_meshInfo[materialID];
	MeshConstantsRT meshConstants = g_meshConstants[materialID];
	float3x3 worldIT = float3x3(meshConstants.worldIT_0, meshConstants.worldIT_1, meshConstants.worldIT_2);
	//worldIT = float3x3(0.01f, 0, 0,
	//	0, 0.01f, 0,
	//	0, 0, 0.01f);

	// triangle intersection data
	const uint3 ii = Load3x16BitIndices(info.m_indexOffsetBytes + triangleID * 3 * 2);
	float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	// ---------------------------------------------- //
   // ------------------ NORMAL -------------------- //
  // ---------------------------------------------- //

  // extract normal attributes
	const float4 normal0 = GetNormalAttribute(info.m_normalAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float4 normal1 = GetNormalAttribute(info.m_normalAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float4 normal2 = GetNormalAttribute(info.m_normalAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);
	const float4 normal = normal0 * bary.x + normal1 * bary.y + normal2 * bary.z;
	vertex.normal = normalize(normal.xyz);
	//vertex.normal = mul(worldIT, normal.xyz);

	const float4 tangent0 = GetTangentAttribute(info.m_tangentAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float4 tangent1 = GetTangentAttribute(info.m_tangentAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float4 tangent2 = GetTangentAttribute(info.m_tangentAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);
	const float4 tangent = tangent0 * bary.x + tangent1 * bary.y + tangent2 * bary.z;
	vertex.tangent = normalize(tangent.xyz);
	//vertex.tangent = mul(worldIT, tangent.xyz);

	// Reintroduced the bitangent because we aren't storing the handedness of the tangent frame anywhere.  Assuming the space
	// is right-handed causes normal maps to invert for some surfaces.
	//bool isRightHanded = true;
	//float scaleBitangent = (isRightHanded ? 1.0 : -1.0);
	float scaleBitangent = tangent.w;
	vertex.bitangent = normalize(cross(vertex.normal, vertex.tangent) * scaleBitangent);

	// ---------------------------------------------- //
   // ----------------- TEXTURE UV ----------------- //
  // ---------------------------------------------- //

  //---------------------------------------------------------------------------------------------
  // Extract position and uv vertex data

  // TODO: Should just store uv partial derivatives in here rather than loading position and caculating it per pixel
	const float3 p0 = GetPositionAttribute(info.m_positionAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float3 p1 = GetPositionAttribute(info.m_positionAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float3 p2 = GetPositionAttribute(info.m_positionAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);

	const float2 uv0 = GetUVAttribute(info.m_uvAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float2 uv1 = GetUVAttribute(info.m_uvAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float2 uv2 = GetUVAttribute(info.m_uvAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);
	vertex.tc.uv = bary.x * uv0 + bary.y * uv1 + bary.z * uv2;

	//---------------------------------------------------------------------------------------------
	// Compute partial derivatives of UV coordinates:
	//
	//  1) Construct a plane from the hit triangle
	//  2) Intersect two helper rays with the plane:  one to the right and one down
	//  3) Compute barycentric coordinates of the two hit points
	//  4) Reconstruct the UV coordinates at the hit points
	//  5) Take the difference in UV coordinates as the partial derivatives X and Y

	// Normal for plane
	float3 triangleNormal = normalize(cross(p2 - p0, p1 - p0));

	// Helper rays
	uint2 threadID = DispatchRaysIndex().xy;
	float3 ddxOrigin, ddxDir, ddyOrigin, ddyDir;
	GenerateCameraRay(uint2(threadID.x + 1, threadID.y), ddxOrigin, ddxDir);
	GenerateCameraRay(uint2(threadID.x, threadID.y + 1), ddyOrigin, ddyDir);

	// Intersect helper rays
	float3 xOffsetPoint = RayPlaneIntersection(worldPos, triangleNormal, ddxOrigin, ddxDir);
	float3 yOffsetPoint = RayPlaneIntersection(worldPos, triangleNormal, ddyOrigin, ddyDir);

	// Compute barycentrics 
	float3 baryX = BarycentricCoordinates(xOffsetPoint, p0, p1, p2);
	float3 baryY = BarycentricCoordinates(yOffsetPoint, p0, p1, p2);

	// Compute UVs and take the difference
	float3x2 uvMat = float3x2(uv0, uv1, uv2);
	vertex.tc.ddxUV = mul(baryX, uvMat) - vertex.tc.uv;
	vertex.tc.ddyUV = mul(baryY, uvMat) - vertex.tc.uv;

	return vertex;
}

#endif //#ifndef SCENE_RT_H_INCLUDED