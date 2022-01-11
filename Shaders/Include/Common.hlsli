//#pragma once

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

///
/// HEADERS
///

// outdated warning about for-loop variable scope
#pragma warning (disable: 3078)
// single-iteration loop
#pragma warning (disable: 3557)


// Defines and constants
#include "Common_Defines.hlsli"
// Constant Buffers
#include "CommonConstantBuffers.h"

// stage specific includes 
#if		defined(STAGE_RT)
	#include "Common_RT.hlsli"
#elif	defined(STAGE_PS)
	#include "Common_PS.hlsli"
#elif	defined(STAGE_CS) 
	#include "Common_CS.hlsli"
#endif

#include "Common_Helpers.hlsli"
#include "Common_Lighting.hlsli"
#include "Common_Resources.hlsli"
#include "Common_Utils.hlsli"
#if	defined(STAGE_RT)
#include "Common_Raytracing.hlsli"
#endif


///
/// IMPLEMENTATIONS
///

/// Resources ///
#include "Res_Global.hlsli"
#if !defined(STAGES_VS) // ps/cs/rt supported
// Materials
#include "Res_MaterialTextures.hlsli"
#include "Res_GBuffer.hlsli"
#include "Res_ArrayGBuffer.hlsli"
// lighting
#include "Res_LightGrid.hlsli"
#include "Res_Lighting.hlsli"
// shader stages
#if		defined(STAGE_RT)
#include "Res_RayTracing.hlsli"
#elif	defined(STAGE_CS)
#include "Res_Compute.hlsli"
#elif	defined(STAGE_PS)
#include "Res_Pixel.hlsli"
#endif


/// utils ///
#include "Utils_Waves.hlsli"
// math 
#include "Utils_Random.hlsli"
#include "Utils_Reflection.hlsli"
#include "Utils_Screen.hlsli"
// data
#include "Utils_Output.hlsli"
#include "Utils_Unpacking.hlsli"
#include "Utils_Vertices.hlsli"
//rt
#ifdef STAGE_RT
#include "Utils_RayTracing.hlsli"
#include "Utils_Caustic.hlsli"
#endif

/// Lighting ///
#include "Light_Materials.hlsli"
#include "Light_Accumulate.hlsli"
#include "Light_Shadows.hlsli"
#include "Light_PBR.hlsli"
#endif


#endif // COMMON_H_INCLUDED
