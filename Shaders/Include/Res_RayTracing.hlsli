#ifndef INCLUDE_RT_ROOT_SIGNATURE_H_INCLUDED
#define INCLUDE_RT_ROOT_SIGNATURE_H_INCLUDED

#include "Common.hlsli"


// TLAS (global view 0)
RaytracingAccelerationStructure			g_accel								: register(t0);
// SCENE BUFFERS (global range 1-4)
StructuredBuffer<MeshLayoutInfo>		g_meshInfo							: register(t1);
ByteAddressBuffer						g_meshData							: register(t2);
StructuredBuffer<MaterialConstantsRT>	g_materialConstants					: register(t3);
StructuredBuffer<MeshConstantsRT>		g_meshConstants						: register(t4);
Texture2D<float>						g_mainDepth							: register(t5);

Texture2D<float4>						g_BlueNoiseRGBA						: register(t19);

// OUTPUTS ( global range 2-10)
RWTexture2D<float4>						g_screenOutput						: register(u2);

// GLOBAL CONSTANT BUFFERS (0, 1)

// GLOBAL DYNAMIC
cbuffer RayTracingCB : register(b0)
{
	DynamicCB g_rtCb;
};

// GlobalCB : register(b1)


// MATERIAL INDEX (local constant 2)
cbuffer MaterialCb : register(b2)
{
	uint MaterialID;
}

/// 
/// Resource access implementation
/// 

uint Res::LightIndex()
{
	return RT::LightIndex();
}

bool Res::RT::FailCheckerBoard(uint2 coords)
{
	return (((coords.x % 2) == (coords.y % 2)) == (bool) FrameIndexMod2()) && UseExperimentalCheckerboard();
}

// global rt only
float4x4			Res::MainCameraToWorld()					{ return g_rtCb.cameraToWorld; }
uint2				Res::ScreenResolution()						{ return g_rtCb.resolution; }
uint2				Res::MapsArrayResolution()					{ return Res::RT::GridRes(); }

// random
Texture2D<float4>	Res::RT::BlueNoiseTex()						{ return g_BlueNoiseRGBA;								}
uint				Res::RT::RecSeqBasis()						{ return g_rtCb.AdditiveRecurrenceSequenceIndexBasis;	}
float2				Res::RT::RecSeqAlpha()						{ return g_rtCb.AdditiveRecurrenceSequenceAlpha;		}


// reflections
uint				Res::RT::ReflectionSamplesCount()			{ return g_rtCb.ReflSampleCount;						}

// caustics
float				Res::RT::CausticRaysPerPixel()				{ return g_rtCb.causticRaysPerPixel;					}

bool				Res::RT::UseExperimentalCheckerboard()		{ return g_rtCb.UseExperimentalCheckerboard;			}
bool				Res::RT::UseFeature1()						{ return g_rtCb.Feature1;								}
bool				Res::RT::UseFeature2()						{ return g_rtCb.Feature2;								}
bool				Res::RT::UseFeature3()						{ return g_rtCb.Feature3;								}
bool				Res::RT::UseFeature4()						{ return g_rtCb.Feature4;								}



// 2D grid + lights
uint2				Res::RT::GridCoords()						{ return DispatchRaysIndex().xy;						}
uint				Res::RT::LightIndex()						{ return DispatchRaysIndex().z;							}
uint2				Res::RT::GridRes()							{ return DispatchRaysDimensions().xy;					}
uint				Res::RT::LightsCount()						{ return DispatchRaysDimensions().z;					}


// full dispatch params
uint3				Res::RT::GridDims()							{ return DispatchRaysDimensions();						}
uint3				Res::RT::GridInd()							{ return DispatchRaysIndex();							}


// materials
uint				Res::RT::MaterialId()						{ return MaterialID;									}
uint				Res::RT::MaterialInstId(uint materialId)	{ return g_meshInfo[materialId].m_materialInstanceId;	}
MaterialConstants	Res::RT::MaterialConstants(uint matInstId)	{ return g_materialConstants[matInstId].Constants;		}
uint				Res::RT::MaterialFlags(uint matInstId)		{ return MaterialConstants(matInstId).Flags;			}

RWTexture2D<float4> Res::GetScreenOutput()						{ return g_screenOutput; }


ByteAddressBuffer	Res::RT::MeshData()							{ return g_meshData; }
RaytracingAccelerationStructure Res::RT::Tlas()					{ return g_accel; }

#endif // INCLUDE_RT_ROOT_SIGNATURE_H_INCLUDED