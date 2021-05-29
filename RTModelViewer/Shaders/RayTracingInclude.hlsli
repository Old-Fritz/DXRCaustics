#ifndef RAYTRACING_INPUT_H_INCLUDED
#define RAYTRACING_INPUT_H_INCLUDED

#define RAY_TRACING

#include "RayTracingHlslCompat.h"
#include "LightGrid.hlsli"

struct RayPayload
{
	bool SkipShading;
	float RayHitT;
};

#ifndef SINGLE
static const float FLT_MAX = asfloat(0x7F7FFFFF);
#endif

// SAMPLERS (global sfatic 0, 1, 2)
SamplerState							defaultSampler				: register(s0);
SamplerComparisonState					shadowSampler				: register(s1);

// TLAS (global view 0)
RaytracingAccelerationStructure			g_accel						: register(t0);
// SCENE BUFFERS (global range 1-4)
StructuredBuffer<RayTraceMeshInfo>		g_meshInfo					: register(t1);
ByteAddressBuffer						g_meshData					: register(t2);
StructuredBuffer<MaterialConstantsRT>	g_materialConstants			: register(t3);
StructuredBuffer<MeshConstantsRT>		g_meshConstants				: register(t4);

// GBUFFER TEXTURES (srv local range 5 - 9)
Texture2D<float4>						g_localBaseColor			: register(t5);
Texture2D<float3>						g_localMetallicRoughness	: register(t6);
Texture2D<float1>						g_localOcclusion			: register(t7);
Texture2D<float3>						g_localEmissive				: register(t8);
Texture2D<float3>						g_localNormal				: register(t9);

// LIGHTING BUFFERS (global range 10 - 15)
TextureCube<float3>						radianceIBLTexture			: register(t10);
TextureCube<float3>						irradianceIBLTexture		: register(t11);
Texture2D<float>						lightSSAO					: register(t12);
Texture2D<float>						lightSunShadow				: register(t13);
StructuredBuffer<LightData>				lightBuffer					: register(t14);
Texture2DArray<float>					lightShadowArrayTex			: register(t15);
ByteAddressBuffer						lightGrid					: register(t16);
ByteAddressBuffer						lightGridBitMask			: register(t17);

// GBuffer (16 - 21)
Texture2D<float>						g_GBDepth					: register(t18);
Texture2D<float4>						g_GBBaseColor				: register(t19);
Texture2D<float3>						g_GBMetallicRoughness		: register(t20);
Texture2D<float1>						g_GBOcclusion				: register(t21);
Texture2D<float3>						g_GBEmissive				: register(t22);
Texture2D<float4>						g_GBNormal					: register(t23);


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
	float					IBLRange;
	float					IBLBias;
	uint					IsReflection;
	uint					UseShadowRays;

}

// GLOBAL DYNAMIC
cbuffer b1 : register(b1)
{
	DynamicCB g_dynamic;
};

// MATERIAL INDEX (local constant 2)
cbuffer Material : register(b2)
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

#endif // RAYTRACING_INPUT_H_INCLUDED