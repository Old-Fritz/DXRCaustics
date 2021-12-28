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

struct BackwardRayPayload
{
	//bool SkipShading;
	float3 Color;
	//float  RayHitT;
};

struct CausticRayPayload
{
	float3 Color;
	uint Count;
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
Texture2D<float>						g_mainDepth					: register(t5);

// GBUFFER TEXTURES (srv local range 5 - 9)
Texture2D<float4>						g_localBaseColor			: register(t6);
Texture2D<float3>						g_localMetallicRoughness	: register(t7);
Texture2D<float1>						g_localOcclusion			: register(t8);
Texture2D<float3>						g_localEmissive				: register(t9);
Texture2D<float3>						g_localNormal				: register(t10);

// LIGHTING BUFFERS (global range 10 - 15)
TextureCube<float3>						radianceIBLTexture			: register(t11);
TextureCube<float3>						irradianceIBLTexture		: register(t12);
Texture2D<float>						lightSSAO					: register(t13);
Texture2D<float>						lightSunShadow				: register(t14);
StructuredBuffer<LightData>				lightBuffer					: register(t15);
Texture2DArray<float>					lightShadowArrayTex			: register(t16);
ByteAddressBuffer						lightGrid					: register(t17);
ByteAddressBuffer						lightGridBitMask			: register(t18);
Texture2D<float4>						BlueNoiseRGBA				: register(t19);


// GBuffer (16 - 21)
Texture2DArray<float>					g_GBDepth					: register(t20);
Texture2DArray<float4>					g_GBBaseColor				: register(t21);
Texture2DArray<float3>					g_GBMetallicRoughness		: register(t22);
Texture2DArray<float1>					g_GBOcclusion				: register(t23);
Texture2DArray<float3>					g_GBEmissive				: register(t24);
Texture2DArray<float4>					g_GBNormal					: register(t25);


// OUTPUTS ( global range 2-10)
RWTexture2D<float4> g_screenOutput : register(u2);

// GLOBAL CONSTANT BUFFERS (0, 1)
cbuffer HitShaderConstants : register(b0)
{
	column_major float4x4	ViewProjMatrix;
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
	uint					AdditiveRecurrenceSequenceIndexBasis;

	float2					AdditiveRecurrenceSequenceAlpha;
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

inline void GenerateLightCameraRay(uint2 index, out float3 origin, out float3 direction)
{
	float2 xy = index + 0.5; // center in the middle of the pixel
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates
	screenPos.y = -screenPos.y;

	// Unproject into a ray
	float4 unprojected = mul(lightBuffer[DispatchRaysIndex().z].cameraToWorld, float4(screenPos, 0, 1));
	float3 world = unprojected.xyz / unprojected.w;
	origin = lightBuffer[DispatchRaysIndex().z].pos;
	direction = normalize(world - origin);
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

inline bool ScreenCoordsInFrustum(float3 screenCoords)
{
	return screenCoords.x >= 0 && screenCoords.y >= 0 && screenCoords.x < g_dynamic.resolution.x&& screenCoords.y < g_dynamic.resolution.y;
}

inline bool ScreenCoordsVisible(float3 screenCoords)
{
	float sceneDepth = g_mainDepth.Load(int3(screenCoords.xy, 0));

	return screenCoords.z >= sceneDepth && screenCoords.z > 0 && ScreenCoordsInFrustum(screenCoords);
}

float3 GetScreenCoords(float3 worldPos)
{
	float2 dims = g_dynamic.resolution;

	float4 p_xy = mul((ViewProjMatrix), float4(worldPos, 1.0));

	p_xy /= p_xy.w;
	p_xy.y = -p_xy.y;

	return float3((p_xy.xy + 1) / 2 * (dims - 1) + 0.5, p_xy.z);
}

#include "RandomRT.hlsli"
#include "SceneRT.hlsli"
#include "MaterialsRT.hlsli"
#include "LightingRT.hlsli"
#include "ReflectRT.hlsli"


#endif // RAYTRACING_INPUT_H_INCLUDED