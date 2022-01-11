//#pragma once

#ifndef COMMON_HELPERS_H_INCLUDED
#define COMMON_HELPERS_H_INCLUDED

#include "Common.hlsli"

#ifndef ENABLE_TRIANGLE_ID
#define ENABLE_TRIANGLE_ID 0
#endif

#if ENABLE_TRIANGLE_ID

uint HashTriangleID(uint vertexID)
{
	// TBD SM6.1 stuff
	uint Index0 = EvaluateAttributeAtVertex(vertexID, 0);
	uint Index1 = EvaluateAttributeAtVertex(vertexID, 1);
	uint Index2 = EvaluateAttributeAtVertex(vertexID, 2);

	// When triangles are clipped (to the near plane?) their interpolants can sometimes
	// be reordered.  To stabilize the ID generation, we need to sort the indices before
	// forming the hash.
	uint I0 = __XB_Min3_U32(Index0, Index1, Index2);
	uint I1 = __XB_Med3_U32(Index0, Index1, Index2);
	uint I2 = __XB_Max3_U32(Index0, Index1, Index2);
	return (I2 & 0xFF) << 16 | (I1 & 0xFF) << 8 | (I0 & 0xFF0000FF);
}

#endif // ENABLE_TRIANGLE_ID

float3 GetDirection(float3 from, float3 to)
{
	return normalize(to - from);
}

Ray GetRay(float3 from, float3 to)
{
	const Ray ray = { from, GetDirection(from, to) };
	return ray;
}

float Pow5(float x)
{
	float xSq = x * x;
	return xSq * xSq * x;
}

float3x3 ConstructTangentFrame(float3 tangent, float3 bitangent, float3 normal)
{
	return float3x3(tangent, bitangent, normal);
}

float3x3 ConstructTangentFrame(float4 tangentRaw, float3 normalRaw)
{
	const float3 normal = normalize(normalRaw);
	const float3 tangent = normalize(tangentRaw.xyz);
	const float3 bitangent = normalize(cross(normal, tangent)) * tangentRaw.w;
	
	return ConstructTangentFrame(tangent, bitangent, normal);
}

///  Common  ///
uint2 GetTilePos(float2 pos, float2 invTileDim)
{
	return pos * invTileDim;
}
uint GetTileIndex(uint2 tilePos, uint tileCountX)
{
	return tilePos.y * tileCountX + tilePos.x;
}
uint GetTileOffset(uint tileIndex)
{
	return tileIndex * TILE_SIZE;
}

#endif