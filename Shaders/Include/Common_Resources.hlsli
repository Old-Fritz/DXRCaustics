//#pragma once

#ifndef RES_COMMON_H_INCLUDED
#define RES_COMMON_H_INCLUDED

#include "Common.hlsli"

#define REG_SLOT(r, n)	r##n
#define REG(r, n)		register ( REG_SLOT(r, n) )

#define S_REG(n)		REG(s, n)
#define T_REG(n)		REG(t, n)
#define B_REG(n)		REG(b, n)
#define U_REG(n)		REG(u, n)



// Methods to receive data from shader resources
namespace Res
{
	// lightGrid
	uint				LightIndex();
	Lights::LightData	GetLightData(uint lightIndex = LightIndex());
	float4x4			ArrayCameraToWorld(uint lightIndex);
	uint4				PreLoadLightGrid(uint tileIndex);
	uint				GetGroupBits(uint groupIndex, uint tileIndex, uint lightBitMaskGroups[4]);


	// GBuffer
	Texture2D<float>	MainDepth();


	// Global
	float4x4			MainCameraToWorld();
	float4x4			MainViewProjMatrix();

	uint2				ScreenResolution();
	uint2				MapsArrayResolution();

	float3				ViewerPos();
	uint				FrameIndexMod2();

	RWTexture2D<float4> GetScreenOutput();


	namespace RT
	{
		Texture2D<float4>	BlueNoiseTex();
		uint				RecSeqBasis();
		float2				RecSeqAlpha();

		// reflections
		uint				ReflectionSamplesCount();

		// caustics
		float				CausticRaysPerPixel();

		bool				UseExperimentalCheckerboard();
		bool				FailCheckerBoard(uint2 coords);
		bool				UseFeature1();
		bool				UseFeature2();
		bool				UseFeature3();
		bool				UseFeature4();


		// 2D grid + lights
		uint2				GridCoords(); 
		uint				LightIndex();
		uint2				GridRes();
		uint				LightsCount();


		// full dispatch params
		uint3				GridDims();
		uint3				GridInd();

		// materials
		uint				MaterialId();
		uint				MaterialInstId(		uint materialId	= MaterialId());
		MaterialConstants	MaterialConstants(	uint matInstId	= MaterialInstId());
		uint				MaterialFlags(		uint matInstId	= MaterialInstId());

		ByteAddressBuffer	MeshData();
		RaytracingAccelerationStructure Tlas();

	}
};

#endif // RES_COMMON_H_INCLUDED
