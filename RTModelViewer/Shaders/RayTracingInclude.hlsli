#ifndef RAYTRACING_INPUT_H_INCLUDED
#define RAYTRACING_INPUT_H_INCLUDED

#include "RayTracingHlslCompat.h"

struct RayPayload
{
	bool SkipShading;
	float RayHitT;
};

#ifndef SINGLE
static const float FLT_MAX = asfloat(0x7F7FFFFF);
#endif

// TLAS (global view 0)
RaytracingAccelerationStructure g_accel : register(t0);

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

cbuffer b1 : register(b1)
{
	DynamicCB g_dynamic;
};

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

#endif // RAYTRACING_INPUT_H_INCLUDED