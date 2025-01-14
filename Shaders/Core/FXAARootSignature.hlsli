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

#define FXAA_RootSig \
	"RootFlags(0), " \
	"RootConstants(b0, num32BitConstants=7), " \
	"DescriptorTable(UAV(u0, numDescriptors = 5))," \
	"DescriptorTable(SRV(t0, numDescriptors = 6))," \
	"StaticSampler(s0," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"filter = FILTER_MIN_MAG_MIP_LINEAR)"

cbuffer CB0 : register(b0)
{
	float2 RcpTextureSize;
	float ContrastThreshold;	// default = 0.2, lower is more expensive
	float SubpixelRemoval;		// default = 0.75, lower blurs less
	uint LastQueueIndex;
	uint2 StartPixel;
}
