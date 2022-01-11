#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include "Common.hlsli"


// Unpack base types
namespace Unpacking
{

#define PT_USHORT4N		uint2		// unshort4
#define PT_UBYTE4		uint		// ubyte4
#define PT_UBYTE4N		uint		// unbyte4
#define PT_DEC4			uint		// xint		(10:10:10:2)
#define PT_COLOR		uint		// ubyte4
#define PT_FLOAT16_2	uint		// unshort2


	// unpacking
	float4	UnPackUShort4N(		PT_USHORT4N		val);
	uint4	UnPackUByte4(		PT_UBYTE4		val);
	float4	UnPackUByte4N(		PT_UBYTE4N		val);
	float2	UnPackFloat16_2(	PT_FLOAT16_2	val);
	float4	UnPackDec4(			PT_DEC4			val);
	float4	UnPackColor(		PT_COLOR		val);
	PT_DEC4	PackDec4(			float4			val);
}


// vertices export
namespace VB
{
	TexCoords GetUV(Vertex vertex, uint matFlags, uint offset);

	// load data from buffer
	uint3	Load3x16BitIndices(		ByteAddressBuffer meshData,	uint byteOffset);

	float2	LoadUVAttribute(		ByteAddressBuffer meshData,	uint byteOffset);
	float4	LoadNormalAttribute(	ByteAddressBuffer meshData,	uint byteOffset);
	float4	LoadTangentAttribute(	ByteAddressBuffer meshData,	uint byteOffset);
	float3	LoadPositionAttribute(	ByteAddressBuffer meshData,	uint byteOffset);

	// helper functions
	float3 RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection);
	float3 BarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2);

	struct MeshDataExtractor
	{
		Vertex			m_vertex;
		MeshLayoutInfo	m_info;

		void ExtractNormal(ByteAddressBuffer meshData, uint3 indices, float3 bary);
		void ExtractBaseUV(ByteAddressBuffer meshData, uint3 indices, float3 bary);

		void ExtractDerivativesUV(ByteAddressBuffer meshData, uint3 ii, float3 bary, float3 worldPos, uint2 threadID, uint2 resolution, float4x4 cameraToWorld, float3 cameraPos);
	};
}


// Transforms between world and screen coordinates
namespace Screen
{
	// Screen coords  - > World pos
	float3 GetScreenWorldPos(float2 coords, float2 resolution, float4x4 cameraToWorld, float depth = 0.0f);
	float3 GBufferIntersectionPoint(float2 screenCoords, float4x4 cameraToWorld = Res::MainCameraToWorld());
	float3 ArrayIntersectionPoint(float2 gridCoords, uint lightIndex, float4x4 cameraToWorld);
	float3 ArrayIntersectionPoint(float2 gridCoords, uint lightIndex = Res::LightIndex());

	// World pos -> Screen coords
	float3	GetCoords(			float3 worldPos,		uint2 resolution = Res::ScreenResolution(), float4x4 ViewProjMatrix = Res::MainViewProjMatrix());
	bool	CoordsVisible(		float3 screenCoords,	uint2 resolution = Res::ScreenResolution(), Texture2D<float> depthTex = Res::MainDepth());
	bool	CoordsInFrustum(	float3 screenCoords,	uint2 resolution = Res::ScreenResolution());
	bool	CoordsInDepth(		float3 screenCoords,	Texture2D<float> depthTex = Res::MainDepth());

	// Ray Helpers
	Ray GenerateCameraRay(float2 coords, float2 res = Res::ScreenResolution(), float4x4 camToWorld = Res::MainCameraToWorld(), float3 origin = Res::ViewerPos());
	Ray GenerateArrayLightRay(float2 gridCoords, Lights::LightData lightData);

	Ray GBufferViewRay(float2 screenCoords, float3 primaryOrigin = Res::ViewerPos(), float4x4 cameraToWorld = Res::MainCameraToWorld());
	Ray GBufferPrimaryRay(float2 screenCoords);
}


// Helper functions for calculate reflected directions with importance sampling
namespace Reflect
{
	bool ValidataNormal(inout float3 normal);

	// reflect ray
	Ray		ReflectRay(Ray primaryRay, float3 normal);

	float3	GetReflectedDirection(float3 primaryRayDirection, float3 normal);
	float3	GetReflectedDirection(float3 primaryRayDirection, float3 normal, float alphaSqr, float2 eps);

	float3	LiftRayOrigin(float3 origin, float3 primaryRayDirection);


	// importance sampling
	float3	SampleToWorld(in float phi, in float cosTheta, in float sinTheta, in float3 N);
	float3	ImportanceSamplingGGX(in float2 eps, in float alphaSqr, in float3 N);
	float3	SampleSphericalDome(float2 eps, float baseRadius = 1.0f);
	float3x3 OrthonormalBasis(float3 normal);
}



// Random numbers
namespace Random
{
	// Hammersly sequence
	float1 RadicalInverse(uint bits);
	float2 Hammersley2D(uint elIndex, uint elCount);

	// Additive recurrence sequence
	float2 RecSeqBlueNoiseSeed( uint2 index, Texture2D<float4> blueNoiseRGBA);
	float2 RecSeqHammersleySeed(uint1 index, uint1 res);
	float2 RecSeqHammersleySeed(uint2 index, uint2 res);
	float2 RecSeqHammersleySeed(uint3 index, uint3 res);

	float1 RecSeqIndex(uint elIndex, uint elCount, uint recSeqBasis);

	float2 RecSeqElement(float2 recSeqSeed, float1 recSeqIndex,	float2 recSeqAlpha);
	float2 RecSeqElement(float2 recSeqSeed, float2 recSeqIndex,	float2 recSeqAlpha);

	struct RecSeq
	{
		float2	m_seed;
		float2	m_alpha;
		uint	m_basis;

		float2 IndexElement(uint elIndex, uint elCount);

		float1 Index(uint elIndex, uint elCount);

		float2 Element(float1 recSeqIndex);
		float2 Element(float2 recSeqIndex);
	};

	struct RandomHandle
	{
		RecSeq	m_recSeq;
		uint	m_seqLength;
		uint	m_currentElement;

		float2	NextRandom();

		float2	SingleRandom();
		float2	SingleHammersley();
	};

	RecSeq InitRecSeq(uint basis, float2 alpha, float2 seed);
	RecSeq InitRecSeq(uint basis, float2 alpha, uint2 index, Texture2D<float4> blueNoiseRGBA);
	RecSeq InitRecSeq(uint basis, float2 alpha, uint2 index, uint2 resolution);

	RandomHandle InitRandom(uint count, RecSeq recSeq);
	RandomHandle InitRandom(uint count, uint basis, float2 alpha, float2 seed);
	RandomHandle InitRandom(uint count, uint basis, float2 alpha, uint2 index, Texture2D<float4> blueNoiseRGBA);
	RandomHandle InitRandom(uint count, uint basis, float2 alpha, uint2 index, uint2 resolution);
}


// Wave operations helpers
namespace WaveOp
{
	uint WaveOr(uint mask);
	uint64_t Ballot64(bool b);

	// Helper function for iterating over a sparse list of bits.
	uint PullNextBit(inout uint bits);
}


// Manipulate output target 
namespace Output
{
	float4 ReadOutput(uint2 index);

	void WriteOutput(uint2 index, float4 color);
	void WriteOutput(uint2 index, float3 color, float w = 1.0f);
	void WriteOutput(uint2 index, float2 color, float z = 0.0f, float w = 1.0f);
	void WriteOutput(uint2 index, float x, float y = 0.0f, float z = 0.0f, float w = 1.0f);	

	void AddOutput(uint2 index, float4 color);
	void AddOutput(uint2 index, float3 color, float w = 1.0f);
	void AddOutput(uint2 index, float2 color, float z = 0.0f, float w = 1.0f);
	void AddOutput(uint2 index, float x, float y = 0.0f, float z = 0.0f, float w = 1.0f);

	void SubOutput(uint2 index, float4 color);
	void SubOutput(uint2 index, float3 color, float w = 1.0f);
	void SubOutput(uint2 index, float2 color, float z = 0.0f, float w = 1.0f);
	void SubOutput(uint2 index, float x, float y = 0.0f, float z = 0.0f, float w = 1.0f);

}


#endif