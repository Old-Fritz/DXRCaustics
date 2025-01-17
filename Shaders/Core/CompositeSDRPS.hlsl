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

#include "ShaderUtility.hlsli"
#include "PresentRS.hlsli"

Texture2D<float3> MainBuffer : register(t0);
Texture2D<float4> OverlayBuffer : register(t1);

[RootSignature(Present_RootSig)]
float3 main( float4 position : SV_Position ) : SV_Target0
{
	float3 MainColor = ApplyDisplayProfile(MainBuffer[(int2)position.xy], DISPLAY_PLANE_FORMAT);
	float4 OverlayColor = OverlayBuffer[(int2)position.xy];
	return OverlayColor.rgb + MainColor.rgb * (1.0 - OverlayColor.a);
}
