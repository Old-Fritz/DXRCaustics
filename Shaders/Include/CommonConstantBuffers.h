#ifndef COMMON_CONSTANT_BUFFERS_H_INCLUDED
#define COMMON_CONSTANT_BUFFERS_H_INCLUDED

#ifndef HLSL
#include "HlslCompat.h"
#else
#define CB_ALIGN


#ifdef USE_SAMPLE_GRAD
struct TexCoords
{
	float2			xy;
	float2			ddxUV;
	float2			ddyUV;
};
#else
typedef float2 TexCoords;
#endif

struct Vertex
{
	float3 m_normal;
#ifndef NO_TANGENT_FRAME
	float4 m_tangent;
#endif
	TexCoords m_uv0;
#ifndef NO_SECOND_UV
	TexCoords m_uv1;
#endif
};

struct Ray
{
	float3 m_origin;
	float3 m_direction;
};

struct MaterialConstants
{
	float4	BaseColorFactor;			// default=[1,1,1,1]	16
	float3	EmissiveFactor;				// default=[0,0,0]		
	float	NormalTextureScale;			// default=1			32
	float2	MetallicRoughnessFactor;	// default=[1,1]		
	uint	Flags;
};
struct MaterialConstantsRT
{
	MaterialConstants	Constants;
	uint __pad0;						//						48
	uint __pad1[52];
};

#endif

CB_ALIGN
struct GlobalConstants
{
	column_major float4x4 ViewProjMatrix;			// ViewProjMatrix
	column_major float4x4 SunShadowMatrix;
	//float4x4 ViewProjMatrix;			// ViewProjMatrix
	//float4x4 SunShadowMatrix;

	float3 ViewerPos;								// CameraPos
	uint pad0;

	float3 SunDirection;
	uint pad1;

	float3 SunIntensity;
	uint pad2;

	float3 AmbientIntensity;
	uint pad3;

	float4 ShadowTexelSize;
	float4 InvTileDim;
	uint4 TileCount;
	uint4 FirstLightIndex;

	uint FrameIndexMod2;
	float IBLRange;
	float IBLBias;
};



// Volatile part (can be split into its own CBV). 
CB_ALIGN
struct DynamicCB
{
	float4x4 cameraToWorld;

	float3   worldCameraPosition;
	uint causticMaxRayRecursion;

	float2   resolution;
	float causticRaysPerPixel;
	float causticPowerScale;

	float ModelScale;
	uint AdditiveRecurrenceSequenceIndexBasis;
	float2 AdditiveRecurrenceSequenceAlpha;

	uint IsReflection;
	uint UseShadowRays;
	uint ReflSampleCount;
	uint UseExperimentalCheckerboard;

	uint Feature1;
	uint Feature2;
	uint Feature3;
	uint Feature4;
};

struct MeshLayoutInfo
{
	uint  m_indexOffsetBytes;
	uint  m_uvAttributeOffsetBytes;
	uint  m_normalAttributeOffsetBytes;
	uint  m_tangentAttributeOffsetBytes;
	uint  m_positionAttributeOffsetBytes;
	uint  m_attributeStrideBytes;
	uint  m_materialInstanceId;
};

CB_ALIGN
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



#endif // COMMON_CONSTANT_BUFFERS_H_INCLUDED