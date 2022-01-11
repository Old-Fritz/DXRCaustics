//#pragma once

#ifndef COMMON_DEFINES_H_INCLUDED
#define COMMON_DEFINES_H_INCLUDED

// hack to hide in IDE
typedef uint64_t _uint64_t;
typedef _uint64_t uint64_t;



// defines
#define HLSL
#define HLSL_2021
//

// stage specific defines 
#if __SHADER_TARGET_STAGE == __SHADER_STAGE_LIBRARY
	//#define USE_SAMPLE_GRAD
	#define STAGE_RT 
#elif __SHADER_TARGET_STAGE == __SHADER_STAGE_PIXEL
	#define STAGE_PS
#elif	__SHADER_TARGET_STAGE == __SHADER_STAGE_COMPUTE
	#define STAGE_CS
#else
	#define STAGES_VS
#endif




// constants
static const float FLT_MAX = asfloat(0x7F7FFFFF);
static const float PI = 3.14159265;
static const float3 kDielectricSpecular = float3(0.04, 0.04, 0.04);

// keep in sync with C code
#define MAX_LIGHTS 128
#define TILE_SIZE (4 + MAX_LIGHTS * 4)







// root signatures
#define Renderer_RootSig \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
	"CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \
	"DescriptorTable(SRV(t0, numDescriptors = 10), visibility = SHADER_VISIBILITY_PIXEL)," \
	"DescriptorTable(Sampler(s0, numDescriptors = 10), visibility = SHADER_VISIBILITY_PIXEL)," \
	"DescriptorTable(SRV(t10, numDescriptors = 10), visibility = SHADER_VISIBILITY_PIXEL)," \
	"CBV(b1), " \
	"SRV(t20, visibility = SHADER_VISIBILITY_VERTEX), " \
	"StaticSampler(s10, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," \
	"StaticSampler(s11, visibility = SHADER_VISIBILITY_PIXEL," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"comparisonFunc = COMPARISON_GREATER_EQUAL," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)," \
	"StaticSampler(s12, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)"


#define Deferred_RootSig \
	"RootFlags(0), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"DescriptorTable(SRV(t0, numDescriptors = 6))," \
	"DescriptorTable(SRV(t10, numDescriptors = 8))," \
	"DescriptorTable(UAV(u0, numDescriptors = 1))," \
	"StaticSampler(s10, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," \
	"StaticSampler(s11, visibility = SHADER_VISIBILITY_ALL," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"comparisonFunc = COMPARISON_GREATER_EQUAL," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)," \
	"StaticSampler(s12, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)"




#endif // COMMON_DEFINES_H_INCLUDED