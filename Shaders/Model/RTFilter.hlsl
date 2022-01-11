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
// Author(s):	Alex Nankervis
//

// outdated warning about for-loop variable scope
#pragma warning (disable: 3078)



// output
RWTexture2D<float4>						g_screenOutput				: register(u0);


// GBuffer (16 - 21)
Texture2D<uint3>						g_Caustics					: register(t0);


#define _RootSig \
	"RootFlags(0), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1))," \
	"DescriptorTable(UAV(u0, numDescriptors = 1))," 


[RootSignature(_RootSig)]
[numthreads(8, 8, 1)]
void main(uint2 DTid :SV_DispatchThreadID)
//uint2 Gid : SV_GroupID,
//uint2 GTid : SV_GroupThreadID,
//uint GI : SV_GroupIndex)
{
	uint2 pixelPos = DTid;

	uint3 caustic = g_Caustics[pixelPos];

	g_screenOutput[pixelPos] += float4(caustic/10000.0f, 0);
}