//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "CommonRS.hlsli"
#include "PixelPacking_Velocity.hlsli"

// We can use the original depth buffer or a linearized one.  In this case, we use linear Z because
// we have discarded the 32-bit depth buffer but still retain a 16-bit linear buffer (previously
// used by SSAO.)  Note that hyperbolic Z is reversed by default (TBD) for increased precision, so
// its Z=0 maps to the far plane.  With linear Z, Z=0 maps to the eye position.  Both extend to Z=1.

//#define USE_LINEAR_Z

Texture2D<float3> ColorBuffer : register(t0);
Texture2D<float> DepthBuffer : register(t1);
RWTexture2D<float4> PrepBuffer : register(u0);
RWTexture2D<packed_velocity_t> VelocityBuffer : register(u1);

cbuffer CB1 : register(b1)
{
	matrix CurToPrevXForm;
}

float4 GetSampleData( uint2 st )
{
	float2 CurPixel = st + 0.5;
	float Depth = DepthBuffer[st];
#ifdef USE_LINEAR_Z
	float4 HPos = float4( CurPixel * Depth, 1.0, Depth );
#else
	float4 HPos = float4( CurPixel, Depth, 1.0 );
#endif
	float4 PrevHPos = mul( CurToPrevXForm, HPos );

	PrevHPos.xyz /= PrevHPos.w;

#ifdef USE_LINEAR_Z
	PrevHPos.z = PrevHPos.w;
#endif

	float3 Velocity = PrevHPos.xyz - float3(CurPixel, Depth);

	VelocityBuffer[st] = PackVelocity(Velocity);

	// Clamp speed at 4 pixels and normalize it.
	return float4(ColorBuffer[st], 1.0) * saturate(length(Velocity.xy) / 4);
}

[RootSignature(Common_RootSig)]
[numthreads( 8, 8, 1 )]
void main( uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
	uint2 corner = DTid.xy << 1;
	float4 sample0 = GetSampleData( corner + uint2(0, 0) );
	float4 sample1 = GetSampleData( corner + uint2(1, 0) );
	float4 sample2 = GetSampleData( corner + uint2(0, 1) );
	float4 sample3 = GetSampleData( corner + uint2(1, 1) );

	float combinedMotionWeight = sample0.a + sample1.a + sample2.a + sample3.a;
	PrepBuffer[DTid.xy] = floor(0.25 * combinedMotionWeight * 3.0) / 3.0 * float4(
		(sample0.rgb + sample1.rgb + sample2.rgb + sample3.rgb) / combinedMotionWeight, 1.0 );
}
