
#ifndef UTILS_SCREEN_H_INCLUDED
#define UTILS_SCREEN_H_INCLUDED

#include "Common.hlsli"


// Screen coords  - > World pos
float3 Screen::GetScreenWorldPos(float2 coords, float2 resolution, float4x4 cameraToWorld, float depth)
{
	const float2 xy = coords + 0.5; // center in the middle of the pixel
	float2 screenPos = xy / resolution * 2.0 - 1.0;

	// Unproject (Invert Y for DirectX-style coordinates)
	const float4 unprojected = mul(cameraToWorld, float4(screenPos.x, -screenPos.y, depth, 1));
	
	return unprojected.xyz / unprojected.w;
}

float3 Screen::GBufferIntersectionPoint(float2 screenCoords, float4x4 cameraToWorld)
{
	const float sceneDepth = Mat::GBuf::Depth(screenCoords);
	return Screen::GetScreenWorldPos(screenCoords, Res::ScreenResolution(), cameraToWorld, sceneDepth);
}
float3 Screen::ArrayIntersectionPoint(float2 gridCoords, uint lightIndex, float4x4 cameraToWorld)
{
	const float sceneDepth = Mat::Array::Depth(gridCoords, lightIndex);
	return Screen::GetScreenWorldPos(gridCoords, Res::MapsArrayResolution(), cameraToWorld, sceneDepth);
}
float3 Screen::ArrayIntersectionPoint(float2 gridCoords, uint lightIndex)
{
	return ArrayIntersectionPoint(gridCoords, lightIndex, Res::ArrayCameraToWorld(lightIndex));
}

// World pos -> Screen coords
bool Screen::CoordsInFrustum(float3 screenCoords, uint2 resolution)
{
	return screenCoords.z > 0
		&& screenCoords.x >= 0
		&& screenCoords.y >= 0
		&& screenCoords.x < resolution.x
		&& screenCoords.y < resolution.y;
}
bool Screen::CoordsVisible(float3 screenCoords, uint2 resolution, Texture2D<float> depthTex)
{
	if (CoordsInFrustum(screenCoords, resolution))
	{
		return CoordsInDepth(screenCoords, depthTex);
	}

	return false;
}
bool Screen::CoordsInDepth(float3 screenCoords, Texture2D<float> depthTex)
{
	//return depthTex(shadowSampler, screenCoords.xy, screenCoords.z);
	//const float depth = depthTex.SampleLevel(defaultSampler, screenCoords.xy, 0);
	const float depth = depthTex.Load(uint3(screenCoords.xy, 0));

	//return screenCoords.z >= depth;
	return abs(screenCoords.z - depth) < 0.00001f;
}

float3 Screen::GetCoords(float3 worldPos, uint2 resolution, float4x4 ViewProjMatrix)
{
	float4 p_xy = mul((ViewProjMatrix), float4(worldPos, 1.0));

	p_xy /= p_xy.w;
	p_xy.y = -p_xy.y;

	return float3((p_xy.xy + 1) / 2 * (resolution - 1) + 0.5, p_xy.z);
}

// Ray Helpers
Ray Screen::GenerateCameraRay(float2 coords, float2 res, float4x4 camToWorld, float3 origin)
{
	const float3 world = GetScreenWorldPos(coords, res, camToWorld, 0);
	const Ray ray = { origin, GetDirection(origin, world) };

	return ray;
}

Ray Screen::GenerateArrayLightRay(float2 gridCoords, const Lights::LightData lightData)
{
	return GenerateCameraRay(gridCoords, Res::MapsArrayResolution(), lightData.m_cameraToWorld, lightData.m_pos);
}

Ray Screen::GBufferViewRay(float2 screenCoords, float3 primaryOrigin, float4x4 cameraToWorld)
{
	Ray ray;
	ray.m_origin	= GBufferIntersectionPoint(screenCoords, cameraToWorld); // World Pos
	ray.m_direction	= GetDirection(ray.m_origin, primaryOrigin);

	return ray;
}
Ray Screen::GBufferPrimaryRay(float2 screenCoords)
{
	Ray ray;
	ray.m_origin	= GBufferIntersectionPoint(screenCoords);
	ray.m_direction = GetDirection(Res::ViewerPos(), ray.m_origin);
	return ray;
}



//inline void GenerateLightCameraRay(uint2 index, out float3 origin, out float3 direction)
//{
//	float3 world = GetScreenWorldPos(index, DispatchRaysDimensions().xy, g_LightBuffer[DispatchRaysIndex().z].cameraToWorld, 0);
//	origin = g_LightBuffer[DispatchRaysIndex().z].pos;
//	direction = normalize(world - origin);
//}

//Ray Screen::GenerateCameraRay(uint2 index, uint2 resolution, float4x4 cameraToWorld, float3 cameraPos)
//{
//	return GenerateRay(coords, res, )
//	const float3 world = GetScreenWorldPos(index, resolution, cameraToWorld, 0);
//	origin = cameraPos;
//	direction = normalize(world - origin);
//}

#endif // UTILS_SCREEN_H_INCLUDED