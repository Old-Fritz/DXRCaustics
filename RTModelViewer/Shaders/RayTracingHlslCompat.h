#ifndef RAYTRACING_HLSL_COMPAT_H_INCLUDED
#define RAYTRACING_HLSL_COMPAT_H_INCLUDED

#ifndef HLSL
#include "HlslCompat.h"
#endif


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
	float4 baseColorFactor; // default=[1,1,1,1]
	float3 emissiveFactor; // default=[0,0,0]
	float normalTextureScale; // default=1
	float metallicFactor; // default=1
	float roughnessFactor; // default=1
	uint flags;
};

#endif //RAYTRACING_USER_HLSL_COMPAT_H_INCLUDED
