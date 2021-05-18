#ifndef RAYTRACING_INPUT_H_INCLUDED
#define RAYTRACING_INPUT_H_INCLUDED

#define RAY_TRACING

#include "RayTracingHlslCompat.h"

struct RayPayload
{
	bool SkipShading;
	float RayHitT;
};

#ifndef SINGLE
static const float FLT_MAX = asfloat(0x7F7FFFFF);
#endif

// SAMPLERS (global sfatic 0, 1)
SamplerState							g_s0						: register(s0);
SamplerComparisonState					shadowSampler				: register(s1);

// TLAS (global view 0)
RaytracingAccelerationStructure			g_accel						: register(t0);
// SCENE BUFFERS (global range 1-6)
StructuredBuffer<RayTraceMeshInfo>		g_meshInfo					: register(t1);
ByteAddressBuffer						g_meshData					: register(t2);
Texture2D<float>						texShadow					: register(t3);
Texture2D<float>						texSSAO						: register(t4);
StructuredBuffer<MaterialConstantsRT>	g_materialConstants			: register(t5);
StructuredBuffer<MeshConstantsRT>		g_meshConstants				: register(t6);

// GBUFFER TEXTURES (srv local range 7-11)
Texture2D<float4>						g_localBaseColor			: register(t7);
Texture2D<float3>						g_localMetallicRoughness	: register(t8);
Texture2D<float1>						g_localOcclusion			: register(t9);
Texture2D<float3>						g_localEmissive				: register(t10);
Texture2D<float3>						g_localNormal				: register(t11);

// LIGHTING BUFFERS (global range 12-17)
Texture2D<float>						depth						: register(t12);
Texture2D<float4>						normals						: register(t13);
// StructuredBuffer<LightData>			lightBuffer					: register(t14);
// Texture2DArray<float>				lightShadowArrayTex			: register(t15);
// ByteAddressBuffer					lightGrid					: register(t16);
// ByteAddressBuffer					lightGridBitMask			: register(t17);


// OUTPUTS ( global range 2-10)
RWTexture2D<float4> g_screenOutput : register(u2);

// GLOBAL CONSTANT BUFFERS (0, 1)
cbuffer HitShaderConstants : register(b0)
{
	column_major float4x4	SunShadowMatrix;
	float4					ViewerPos;
	float4					SunDirection;
	float4					SunIntensity;
	float4					AmbientIntensity;
	float4					ShadowTexelSize;
	float4					InvTileDim;
	uint4					TileCount;
	uint4					FirstLightIndex;
	float					ModelScale;
	uint					IsReflection;
	uint					UseShadowRays;
}

// GLOBAL DYNAMIC
cbuffer b1 : register(b1)
{
	DynamicCB g_dynamic;
};

// MATERIAL INDEX (local contant 3)
cbuffer Material : register(b3)
{
	uint MaterialID;
}

inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
	float2 xy = index + 0.5; // center in the middle of the pixel
	float2 screenPos = xy / g_dynamic.resolution * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates
	screenPos.y = -screenPos.y;

	// Unproject into a ray
	float4 unprojected = mul(g_dynamic.cameraToWorld, float4(screenPos, 0, 1));
	float3 world = unprojected.xyz / unprojected.w;
	origin = g_dynamic.worldCameraPosition;
	direction = normalize(world - origin);
}

#include "SceneRT.hlsli"
#include "MaterialsRT.hlsli"
#include "LightingRT.hlsli"
#include "ShadingRT.hlsli"

#endif // RAYTRACING_INPUT_H_INCLUDED