#ifndef RAYTRACING_HLSL_COMPAT_H_INCLUDED
#define RAYTRACING_HLSL_COMPAT_H_INCLUDED

#ifndef HLSL
#include "HlslCompat.h"
#endif

// Volatile part (can be split into its own CBV). 
struct DynamicCB
{
	float4x4 cameraToWorld;
	float3   worldCameraPosition;
	uint causticMaxRayRecursion;
	float2   resolution;
	float causticRaysPerPixel;
	float causticPowerScale;
};

struct RayTraceMeshInfo
{
	uint  m_indexOffsetBytes;
	uint  m_uvAttributeOffsetBytes;
	uint  m_normalAttributeOffsetBytes;
	uint  m_tangentAttributeOffsetBytes;
	uint  m_positionAttributeOffsetBytes;
	uint  m_attributeStrideBytes;
	uint  m_materialInstanceId;
};

struct MaterialConstantsRT
{
	float4 baseColorFactor;				// default=[1,1,1,1]	16
	float3 emissiveFactor;				// default=[0,0,0]		
	float normalTextureScale;			// default=1			32
	float2 metallicRoughnessFactor;		// default=[1,1]		
	uint flags;							
	uint __pad0;						//						48
	uint __pad1[52];
};

struct MeshConstantsRT
{
	float4x4 world;
	float3 worldIT_0;
	uint __pad0;
	float3 worldIT_1;
	uint __pad1;
	float3 worldIT_2;
	uint __pad2;
	uint __pad3[36];
};

#endif //RAYTRACING_USER_HLSL_COMPAT_H_INCLUDED
