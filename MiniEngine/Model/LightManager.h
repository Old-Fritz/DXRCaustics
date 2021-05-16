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
// Author(s):  Alex Nankervis
//			 James Stanard
//

#pragma once

#include <cstdint>
#include "Camera.h"

class StructuredBuffer;
class ByteAddressBuffer;
class ColorBuffer;
class ShadowBuffer;
class GraphicsContext;
class IntVar;
namespace Math
{
	class Vector3;
	class Matrix4;
	class Camera;
}

namespace Lighting
{
	extern IntVar LightGridDim;

	// must keep in sync with HLSL
	struct LightData
	{
		float pos[3];
		float radiusSq;
		float color[3];

		uint32_t type;
		float coneDir[3];
		float coneAngles[2];

		float shadowTextureMatrix[16];
	};
	enum { kMinLightGridDim = 8 };
	enum { MaxLights = 128 };

	extern LightData m_LightData[MaxLights];
	extern StructuredBuffer m_LightBuffer;
	extern ByteAddressBuffer m_LightGrid;

	extern ByteAddressBuffer m_LightGridBitMask;
	extern std::uint32_t m_FirstConeLight;
	extern std::uint32_t m_FirstConeShadowedLight;

	extern ShadowBuffer m_LightShadowArray;
	extern ShadowBuffer m_LightShadowTempBuffer;
	extern Math::Camera m_LightShadowCamera[MaxLights];

	void InitializeResources(void);
	void CreateRandomLights(const Math::Vector3 minBound, const Math::Vector3 maxBound);
	void FillLightGrid(GraphicsContext& gfxContext, const Math::Camera& camera);
	void Shutdown(void);
}
