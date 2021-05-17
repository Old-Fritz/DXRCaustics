//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):	James Stanard, Christopher Wallis
//

#define HLSL
#define RAY_TRACING

#include "RayTracingInclude.hlsli"
#include "Unpacking.hlsli"

#include "../../MiniEngine/Model/Shaders/LightGrid.hlsli"

// MATERIAL INDEX (local contant 3)
cbuffer Material : register(b3)
{
	uint MaterialID;
}

// SCENE BUFFERS (global range 1-6)
StructuredBuffer<RayTraceMeshInfo>		g_meshInfo					: register(t1);
ByteAddressBuffer						g_indices					: register(t2);
ByteAddressBuffer						g_attributes				: register(t3);
Texture2D<float>						texShadow					: register(t4);
Texture2D<float>						texSSAO						: register(t5);
StructuredBuffer<MaterialConstantsRT>	g_materialConstants			: register(t6);

// MATERIAL TEXTURES (local range 7-11)
Texture2D<float4>						g_localTexture				: register(t7);
Texture2D<float3>						g_localMetallicRoughness	: register(t8);
Texture2D<float1>						g_localOcclusion			: register(t9);
Texture2D<float3>						g_localEmissive				: register(t10);
Texture2D<float3>						g_localNormal				: register(t11);

// LIGHTING BUFFERS (global range 12-17)
Texture2D<float>						depth						: register(t12);
Texture2D<float4>						normals						: register(t13);
// from Lighting.hlsli
// StructuredBuffer<LightData>				lightBuffer					: register(t14);
// Texture2DArray<float>					lightShadowArrayTex			: register(t15);
// ByteAddressBuffer						lightGrid					: register(t16);
// ByteAddressBuffer						lightGridBitMask			: register(t17);

// SAMPLERS (global sfatic 0, 1)
SamplerState							g_s0						: register(s0);
SamplerComparisonState					shadowSampler				: register(s1);

#include "../../MiniEngine/Model/Shaders/Lighting.hlsli"
#include "../../MiniEngine/Model/Shaders/LightingPBR.hlsli"

uint3 Load3x16BitIndices(
	uint offsetBytes)
{
	const uint dwordAlignedOffset = offsetBytes & ~3;

	const uint2 four16BitIndices = g_indices.Load2(dwordAlignedOffset);

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

float GetShadow(float3 ShadowCoord)
{
	const float Dilation = 2.0;
	float d1 = Dilation * ShadowTexelSize.x * 0.125;
	float d2 = Dilation * ShadowTexelSize.x * 0.875;
	float d3 = Dilation * ShadowTexelSize.x * 0.625;
	float d4 = Dilation * ShadowTexelSize.x * 0.375;
	float result = (
		2.0 * texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy, ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(-d2, d1), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(-d1, -d2), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(d2, -d1), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(d1, d2), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(-d4, d3), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(-d3, -d4), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(d4, -d3), ShadowCoord.z) +
		texShadow.SampleCmpLevelZero(shadowSampler, ShadowCoord.xy + float2(d3, d4), ShadowCoord.z)
		) / 10.0;
	return result * result;
}

// unpack attributes
float2 GetUVAttribute(uint byteOffset)
{
	return UnPackFloat16_2(g_attributes.Load(byteOffset));
}

float4 GetNormalAttribute(uint byteOffset)
{
	return UnPackDec4(g_attributes.Load(byteOffset)) * 2 - 1;
}

float4 GetTangentAttribute(uint byteOffset)
{
	return UnPackDec4(g_attributes.Load(byteOffset));
}

float3 GetPositionAttribute(uint byteOffset)
{
	return asfloat(g_attributes.Load3(byteOffset)) * ModelScale;
}

float3 RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection)
{
	float t = dot(-planeNormal, rayOrigin - planeOrigin) / dot(planeNormal, rayDirection);
	return rayOrigin + rayDirection * t;
}

/*
	REF: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	From "Real-Time Collision Detection" by Christer Ericson
*/
float3 BarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2)
{
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

[shader("closesthit")]
void Hit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.RayHitT = RayTCurrent();
	if (payload.SkipShading)
	{
		return;
	}

	uint materialID = MaterialID;
	uint triangleID = PrimitiveIndex();

	  // ---------------------------------------------- //
	 // ------------ EXTRACT MESH DATA --------------- //
	// ---------------------------------------------- //

	RayTraceMeshInfo info = g_meshInfo[materialID];

	const uint3 ii = Load3x16BitIndices(info.m_indexOffsetBytes + triangleID * 3 * 2);
	const float2 uv0 = GetUVAttribute(info.m_uvAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float2 uv1 = GetUVAttribute(info.m_uvAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float2 uv2 = GetUVAttribute(info.m_uvAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);

	float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	float2 uv = bary.x * uv0 + bary.y * uv1 + bary.z * uv2;

	const float4 normal0 = GetNormalAttribute(info.m_normalAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float4 normal1 = GetNormalAttribute(info.m_normalAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float4 normal2 = GetNormalAttribute(info.m_normalAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);
	const float4 normalW = normal0 * bary.x + normal1 * bary.y + normal2 * bary.z;
	float3 vsNormal = normalize(normalW.xyz);
	
	const float4 tangent0 = GetTangentAttribute(info.m_tangentAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float4 tangent1 = GetTangentAttribute(info.m_tangentAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float4 tangent2 = GetTangentAttribute(info.m_tangentAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);
	const float4 tangentW = tangent0 * bary.x + tangent1 * bary.y + tangent2 * bary.z;
	float3 vsTangent = normalize(tangentW.xyz);

	// Reintroduced the bitangent because we aren't storing the handedness of the tangent frame anywhere.  Assuming the space
	// is right-handed causes normal maps to invert for some surfaces.  The Sponza mesh has all three axes of the tangent frame.
	//bool isRightHanded = true;
	//float scaleBitangent = (isRightHanded ? 1.0 : -1.0);
	float scaleBitangent = tangentW.w; 
	float3 vsBitangent = normalize(cross(vsNormal, vsTangent)) * scaleBitangent;

	//const float3 bitangent0 = asfloat(g_attributes.Load3(info.m_bitangentAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes));
	//const float3 bitangent1 = asfloat(g_attributes.Load3(info.m_bitangentAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes));
	//const float3 bitangent2 = asfloat(g_attributes.Load3(info.m_bitangentAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes));
	//float3 vsBitangent = normalize(bitangent0 * bary.x + bitangent1 * bary.y + bitangent2 * bary.z);

	// TODO: Should just store uv partial derivatives in here rather than loading position and caculating it per pixel
	const float3 p0 = GetPositionAttribute(info.m_positionAttributeOffsetBytes + ii.x * info.m_attributeStrideBytes);
	const float3 p1 = GetPositionAttribute(info.m_positionAttributeOffsetBytes + ii.y * info.m_attributeStrideBytes);
	const float3 p2 = GetPositionAttribute(info.m_positionAttributeOffsetBytes + ii.z * info.m_attributeStrideBytes);

	float3 worldPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();


	  // ---------------------------------------------- //
	 // ----------------- TEXTURE UV ----------------- //
	// ---------------------------------------------- //
	
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
	float3 xOffsetPoint = RayPlaneIntersection(worldPosition, triangleNormal, ddxOrigin, ddxDir);
	float3 yOffsetPoint = RayPlaneIntersection(worldPosition, triangleNormal, ddyOrigin, ddyDir);

	// Compute barycentrics 
	float3 baryX = BarycentricCoordinates(xOffsetPoint, p0, p1, p2);
	float3 baryY = BarycentricCoordinates(yOffsetPoint, p0, p1, p2);

	// Compute UVs and take the difference
	float3x2 uvMat = float3x2(uv0, uv1, uv2);
	float2 ddxUV = mul(baryX, uvMat) - uv;
	float2 ddyUV = mul(baryY, uvMat) - uv;

	  // ---------------------------------------------- //
	 // ------------ SURFACE CREATION ---------------- //
	// ---------------------------------------------- //

	// Retrieve material constants
	MaterialConstantsRT material = g_materialConstants[g_meshInfo[materialID].m_materialInstanceId];

	// Load and modulate textures
	float4 baseColor = material.baseColorFactor * g_localTexture.SampleGrad(g_s0, uv, ddxUV, ddyUV);
	float2 metallicRoughness = material.metallicRoughnessFactor * g_localMetallicRoughness.SampleGrad(g_s0, uv, ddxUV, ddyUV).bg;
	float occlusion = g_localOcclusion.SampleGrad(g_s0, uv, ddxUV, ddyUV);
	float3 emissive = material.emissiveFactor * g_localEmissive.SampleGrad(g_s0, uv, ddxUV, ddyUV);

	// calculate normal
	float3 normal = g_localNormal.SampleGrad(g_s0, uv, ddxUV, ddyUV) * 2.0 - 1.0;
	normal = normalize(normal * float3(material.normalTextureScale, material.normalTextureScale, 1));
	float3x3 tbn = float3x3(vsTangent, vsBitangent, vsNormal);
	normal = mul(normal, tbn);

	// build surface
	SurfaceProperties Surface;
	Surface.N = normal;
	Surface.V = normalize(ViewerPos - worldPosition);
	Surface.NdotV = saturate(dot(Surface.N, Surface.V));
	Surface.c_diff = baseColor.rgb * (1 - kDielectricSpecular) * (1 - metallicRoughness.x) * occlusion;
	Surface.c_spec = lerp(kDielectricSpecular, baseColor.rgb, metallicRoughness.x) * occlusion;
	Surface.roughness = metallicRoughness.y;
	Surface.alpha = metallicRoughness.y * metallicRoughness.y;
	Surface.alphaSqr = Surface.alpha * Surface.alpha;

	  // ---------------------------------------------- //
	 // ----------------- SHADING -------------------- //
	// ---------------------------------------------- //

	// Begin accumulating light starting with emissive
	float3 colorAccum = emissive;

	uint2 pixelPos = uint2(DispatchRaysIndex().xy);
	float ssao = texSSAO[pixelPos];

	Surface.c_diff *= ssao;
	Surface.c_spec *= ssao;

#if 1
	// Old-school ambient light
	colorAccum += Surface.c_diff * AmbientIntensity;
#else
	// Add IBL
	colorAccum += Diffuse_IBL(Surface);
	colorAccum += Specular_IBL(Surface);
#endif

	float3 shadowCoord = mul(SunShadowMatrix, float4(worldPosition, 1.0f)).xyz;
	colorAccum += ApplyDirectionalLightPBR(Surface, SunDirection, SunIntensity, shadowCoord, texShadow);
	ShadeLightsPBR(colorAccum, pixelPos, Surface, worldPosition);


	  // ---------------------------------------------- //
	 // ----------------- REFLECTIONS ---------------- //
	// ---------------------------------------------- //


	// TODO: Should be passed in via material info
	//if (IsReflection)
	//{
	//	float reflectivity = normals[DispatchRaysIndex().xy].w;
	//	colorAccum = g_screenOutput[DispatchRaysIndex().xy].rgb + reflectivity * colorAccum;
	//}

	  // ---------------------------------------------- //
	 // ----------------- OUTPUT --------------------- //
	// ---------------------------------------------- //

	g_screenOutput[DispatchRaysIndex().xy] = float4(colorAccum, 1.0);
}