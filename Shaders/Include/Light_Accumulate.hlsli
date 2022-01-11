#ifndef LIGHT_ACCUMULATE_H_INCLUDED
#define LIGHT_ACCUMULATE_H_INCLUDED

#include "Common.hlsli"

float3 Lights::GetSkybox(float3 dir)
{
	return g_RadianceIBLTexture.SampleLevel(defaultSampler, dir, g_globalCB.IBLBias);
}

/// Point Light ///
Lights::LightDirection Lights::PointLight::DirData(float3 worldPos)
{
	LightDirection dirData;
	dirData.m_dir = m_pos - worldPos;

	// normalize
	const float lightDistSq = dot(dirData.m_dir, dirData.m_dir);
	dirData.m_invLightDist = rsqrt(lightDistSq);

	return dirData;
}

Lights::LightDirection Lights::PointLight::DirDataNorm(float3 worldPos)
{
	LightDirection dirData = DirData(worldPos);
	dirData.Norm();
	return dirData;
}
float Lights::PointLight::DistFalloff(float invLightDist)
{
	// modify 1/d^2 * R^2 to fall off at a fixed radius
	// (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
	const float distanceFalloff = m_radiusSq * (invLightDist * invLightDist);

	return max(0, distanceFalloff - rsqrt(distanceFalloff));
}
float Lights::PointLight::Falloff(float3 worldPos)
{
	return DistFalloff(DirData(worldPos).m_invLightDist);
}



/// Cone Light ///
float Lights::ConeLight::ConeFallOff(float3 lightDir)
{
	const float coneFalloff = dot(-lightDir, m_coneDir);

	return saturate((coneFalloff - m_coneAngles.y) * m_coneAngles.x);
}
float Lights::ConeLight::FallOff(LightDirection dirData)
{
	return DistFalloff(dirData.m_invLightDist) * ConeFallOff(dirData.m_dir);
}



/// LightsAccumulator ///
Lights::LightsAccumulator Lights::InitAccumulator(Mat::Material matData, Ray viewRay)
{
	Lights::LightsAccumulator lightsAcc;

	lightsAcc.m_colSum		= matData.m_emissive;
	lightsAcc.m_worldPos	= viewRay.m_origin;
	lightsAcc.m_hit.SetSurface(matData);
	lightsAcc.m_hit.SetView(viewRay.m_direction);

	return lightsAcc;
}

float3 Lights::LightsAccumulator::AccumulateScreenSpace(uint2 pixelPos)
{
	AddOccludedAmbient(pixelPos);
	AddSunLight();
	AddScreenSpaceLights(pixelPos);

	return m_colSum;
}
float3 Lights::LightsAccumulator::AccumulateAll()
{
	AddAmbient();
	AddSunLight();
	AddAllLights();

	return m_colSum;
}


/// Apply shadows and occlusions ///
float Lights::LightsAccumulator::OccludeDirectional(float4x4 shadowMatrix, Texture2D<float> texShadow)
{
	const float3 shadowCoord = Shadows::CalcShadowCoord(shadowMatrix, m_worldPos);

	//return Shadows::GetDefaultSampledShadow(shadowCoord.xy, shadowCoord.z, texShadow);
	return Shadows::GetShadow(shadowCoord.xy, shadowCoord.z, texShadow);
}
float Lights::LightsAccumulator::OccludeCone(uint lightIndex, float4x4 shadowMatrix, Texture2DArray<float> texShadow)
{
	const float3 shadowCoord = Shadows::CalcShadowCoord(shadowMatrix, m_worldPos);

	//return Shadows::GetDefaultSampledShadow(float3(shadowCoord.xy, lightIndex), shadowCoord.z, texShadow);
	return Shadows::GetShadow(float3(shadowCoord.xy, lightIndex), shadowCoord.z, texShadow);
}
float Lights::LightsAccumulator::OccludeAmbient(uint2 pixelPos)
{
	return g_TexSSAO[pixelPos];
}



///	Apply lights ///
float3 Lights::LightsAccumulator::ApplyLightCommon(float3 color, float3 dir)
{
	m_hit.SetLight(dir);
	const float3 diffuse	= m_hit.CalcDiffuseFactor();
	const float3 specular	= m_hit.CalcSpecularFactor();

	return color * (diffuse + specular);
}
float3 Lights::LightsAccumulator::ApplyDirectionalLight(float3 color, float3 dir)
{
	return ApplyLightCommon(color, dir);
}
float3 Lights::LightsAccumulator::ApplyPointLight(PointLight light)
{
	const LightDirection dirData = light.DirDataNorm(m_worldPos);

	return light.Falloff(dirData.m_invLightDist) * ApplyLightCommon(light.m_color, dirData.m_dir);
}
float3 Lights::LightsAccumulator::ApplyConeLight(ConeLight light)
{
	const LightDirection dirData = light.DirDataNorm(m_worldPos);

	return light.FallOff(dirData) * ApplyLightCommon(light.m_color, dirData.m_dir);
}
float3 Lights::LightsAccumulator::ApplyAmbientLight(float3 color)
{
	const float3 diffuse	= m_hit.Diffuse_IBL(g_IrradianceIBLTexture, defaultSampler);
	const float3 specular	= m_hit.Specular_IBL(g_RadianceIBLTexture, defaultSampler, g_globalCB.IBLRange, g_globalCB.IBLBias);

	return color * (diffuse + specular);
	//return m_surf.c_diff * g_globalCB.AmbientIntensity.xyz;
}



/// Shade global lights ///
void Lights::LightsAccumulator::AddSunLight()
{
	const float sunShadow = OccludeDirectional(g_globalCB.SunShadowMatrix, g_TexSunShadow);
	m_colSum += sunShadow * ApplyDirectionalLight(g_globalCB.SunIntensity, g_globalCB.SunDirection);
}
void Lights::LightsAccumulator::AddOccludedAmbient(uint2 pixelPos)
{
	const float ambientOcclusion = OccludeAmbient(pixelPos);
	m_colSum += ambientOcclusion * ApplyAmbientLight(g_globalCB.AmbientIntensity);
}
void Lights::LightsAccumulator::AddAmbient()
{
	m_colSum += ApplyAmbientLight(g_globalCB.AmbientIntensity);
}


/// Shade lights from light buffer ///
void Lights::LightsAccumulator::AddPointLight(			uint lightIndex)
{
	const LightData lightData = Res::GetLightData(lightIndex);

	m_colSum += ApplyPointLight((PointLight)lightData);
}
void Lights::LightsAccumulator::AddConeLight(			uint lightIndex)
{
	const LightData lightData = Res::GetLightData(lightIndex);

	m_colSum += ApplyConeLight((ConeLight)lightData);
}
void Lights::LightsAccumulator::AddConeShadowedLight(	uint lightIndex)
{
	const LightData lightData = Res::GetLightData(lightIndex);

	const float shadow = OccludeCone(lightIndex, lightData.m_shadowTextureMatrix, g_LightShadowArrayTex);
	m_colSum += shadow * ApplyConeLight((ConeLight)lightData);
}

void Lights::LightsAccumulator::AddScreenSpaceLights(uint2 pixelPos)
{
	const uint2 tilePos = GetTilePos(pixelPos, g_globalCB.InvTileDim.xy);
	const uint tileIndex = GetTileIndex(tilePos, g_globalCB.TileCount.x);
	const uint tileOffset = GetTileOffset(tileIndex);
	
	AddGridLightsBitMask(tileIndex);
	//AddGridLightsBitMaskSorted(tileIndex);
	//AddGridLightsScalarLoop(tileOffset);
	//AddGridLightsScalarBranch(tileOffset);
	//AddGridLightsScalar(tileOffset);
	//AddAllLights();
}

/// Select lights to shade ///
void Lights::LightsAccumulator::AddAllLights()
{
	uint lightIndex;
	for (lightIndex = 0; lightIndex < g_globalCB.FirstLightIndex.x; lightIndex++) { AddPointLight(lightIndex);		} 	// sphere
	for (lightIndex = 0; lightIndex < g_globalCB.FirstLightIndex.y; lightIndex++) { AddConeLight(lightIndex);		} 	// cone
	for (lightIndex = 0; lightIndex < g_globalCB.FirstLightIndex.z; lightIndex++) { AddConeShadowedLight(lightIndex); }	// cone w/ shadow map
}
void Lights::LightsAccumulator::AddGridLightsScalar(uint tileOffset)
{
	const uint tileLightCount				= g_LightGrid.Load(tileOffset + 0);
	const uint tileLightCountSphere			= (tileLightCount >> 0)			& 0xff;
	const uint tileLightCountCone			= (tileLightCount >> 8)			& 0xff;
	const uint tileLightCountConeShadowed	= min((tileLightCount >> 16)	& 0xff, g_globalCB.FirstLightIndex.z);

	uint tileLightLoadOffset = tileOffset + 4;
	uint n;
	for (n = 0; n < tileLightCountSphere;		n++, tileLightLoadOffset += 4)		{ AddPointLight(g_LightGrid.Load(tileLightLoadOffset));			} 	// sphere
	for (n = 0; n < tileLightCountCone;			n++, tileLightLoadOffset += 4)		{ AddConeLight(g_LightGrid.Load(tileLightLoadOffset));			} 	// cone
	for (n = 0; n < tileLightCountConeShadowed; n++, tileLightLoadOffset += 4)		{ AddConeShadowedLight(g_LightGrid.Load(tileLightLoadOffset));	}	// cone w/ shadow map
}


/// Select lights to shade wave instricts ///
void Lights::LightsAccumulator::AddGridLightsScalarBranch(	uint tileOffset)
{
	if (WaveOp::Ballot64(tileOffset == WaveReadLaneFirst(tileOffset)) == ~0ull) 		// uniform branch
	{
		tileOffset = WaveReadLaneFirst(tileOffset);
		AddGridLightsScalar(tileOffset);
	}
	else
	{
		AddGridLightsScalar(tileOffset);
	}
}
void Lights::LightsAccumulator::AddGridLightsScalarLoop(	uint tileOffset)
{
	uint64_t threadMask = WaveOp::Ballot64(tileOffset != ~0); // attempt to get starting exec mask
	const uint64_t laneBit = 1ull << WaveGetLaneIndex();
	while ((threadMask & laneBit) != 0) // is this thread waiting to be processed?
	{ // exec is now the set of remaining threads
		const uint uniformTileOffset = WaveReadLaneFirst(tileOffset);				// grab the tile offset for the first active thread
		const uint64_t uniformMask = WaveOp::Ballot64(tileOffset == uniformTileOffset); 	// mask of which threads have the same tile offset as the first active thread
		if (any((uniformMask & laneBit) != 0)) // is this thread one of the current set of uniform threads?
		{
			AddGridLightsScalar(tileOffset);
		}
		threadMask &= ~uniformMask;		 // strip the current set of uniform threads from the exec mask for the next loop iteration
	}
}
void Lights::LightsAccumulator::AddGridLightsBitMaskSorted(	uint tileIndex)
{
#if defined(LIGHT_GRID_PRELOADING)		// Light Grid Preloading setup
	const uint4 lightBitMask = Res::PreLoadLightGrid(tileIndex);
	uint lightBitMaskGroups[4] = { lightBitMask.x, lightBitMask.y, lightBitMask.z, lightBitMask.w };
#else
	uint lightBitMaskGroups[4] = { 0, 0, 0, 0 };
#endif
	// Get light type groups - these can be predefined as compile time constants to enable unrolling and better scheduling of vector reads
	const uint pointLightGroupTail		= POINT_LIGHT_GROUPS_TAIL;
	const uint spotLightGroupTail		= SPOT_LIGHT_GROUPS_TAIL;
	const uint spotShadowLightGroupTail = SHADOWED_SPOT_LIGHT_GROUPS_TAIL;

	uint groupBitsMasks[4] = { 0, 0, 0, 0 };
	for (int i = 0; i < 4; i++)
	{
		groupBitsMasks[i] = WaveOp::WaveOr(Res::GetGroupBits(i, tileIndex, lightBitMaskGroups));			// combine across threads
	}
	uint groupIndex;
	for (groupIndex = 0; groupIndex < pointLightGroupTail; groupIndex++)
	{
		uint groupBits = groupBitsMasks[groupIndex];
		while (groupBits != 0) { AddPointLight(32 * groupIndex + WaveOp::PullNextBit(groupBits)); }
	}
	for (groupIndex = pointLightGroupTail; groupIndex < spotLightGroupTail; groupIndex++)
	{
		uint groupBits = groupBitsMasks[groupIndex];
		while (groupBits != 0) { AddConeLight(32 * groupIndex + WaveOp::PullNextBit(groupBits)); }
	}
	for (groupIndex = spotLightGroupTail; groupIndex < spotShadowLightGroupTail; groupIndex++)
	{
		uint groupBits = groupBitsMasks[groupIndex];
		while (groupBits != 0) { AddConeShadowedLight(32 * groupIndex + WaveOp::PullNextBit(groupBits)); }
	}
}
void Lights::LightsAccumulator::AddGridLightsBitMask(		uint tileIndex)
{
#if defined(LIGHT_GRID_PRELOADING)		// Light Grid Preloading setup
	const uint4 lightBitMask = Res::PreLoadLightGrid(tileIndex);
	uint lightBitMaskGroups[4] = { lightBitMask.x, lightBitMask.y, lightBitMask.z, lightBitMask.w };
#else
	uint lightBitMaskGroups[4] = { 0, 0, 0, 0 };
#endif
	uint64_t threadMask = WaveOp::Ballot64(tileIndex != ~0); // attempt to get starting exec mask
	for (uint groupIndex = 0; groupIndex < 4; groupIndex++)
	{
		// combine across threads
		uint groupBits = WaveOp::WaveOr(Res::GetGroupBits(groupIndex, tileIndex, lightBitMaskGroups));
		while (groupBits != 0)
		{
			const uint lightIndex = 32 * groupIndex + WaveOp::PullNextBit(groupBits);
			if (lightIndex < g_globalCB.FirstLightIndex.x)			{ AddPointLight(		lightIndex); } 		// sphere
			else if (lightIndex < g_globalCB.FirstLightIndex.y)		{ AddConeLight(			lightIndex); } 		// cone
			else if (lightIndex < g_globalCB.FirstLightIndex.z)		{ AddConeShadowedLight(	lightIndex); }	// cone w/ shadow map
		}
	}
}


#endif // LIGHT_ACCUMULATE_H_INCLUDED