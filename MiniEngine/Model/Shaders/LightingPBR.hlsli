#ifndef LIGHTING_PBR_H_INCLUDED
#define LIGHTING_PBR_H_INCLUDED

#define PBR_LIGHTING

#include "Shadows.hlsli"
#include "Lighting.hlsli"

// Numeric constants
static const float PI = 3.14159265;
static const float3 kDielectricSpecular = float3(0.04, 0.04, 0.04);

// Flag helpers
static const uint BASECOLOR = 0;
static const uint METALLICROUGHNESS = 1;
static const uint OCCLUSION = 2;
static const uint EMISSIVE = 3;
static const uint NORMAL = 4;

struct SurfaceProperties
{
	float3 N;
	float3 V;
	float3 c_diff;
	float3 c_spec;
	float roughness;
	float alpha; // roughness squared
	float alphaSqr; // alpha squared
	float NdotV;
};

struct LightProperties
{
	float3 L;
	float NdotL;
	float LdotH;
	float NdotH;
};

//
// Shader Math
//

float Pow5(float x)
{
	float xSq = x * x;
	return xSq * xSq * x;
}

// Shlick's approximation of Fresnel
float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
	return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Shlick(float F0, float F90, float cosine)
{
	return lerp(F0, F90, Pow5(1.0 - cosine));
}

// Burley's diffuse BRDF
float3 Diffuse_Burley(SurfaceProperties Surface, LightProperties Light)
{
	float fd90 = 0.5 + 2.0 * Surface.roughness * Light.LdotH * Light.LdotH;
	return Surface.c_diff * Fresnel_Shlick(1, fd90, Light.NdotL).x * Fresnel_Shlick(1, fd90, Surface.NdotV).x;
}

// GGX specular D (normal distribution)
float Specular_D_GGX(SurfaceProperties Surface, LightProperties Light)
{
	float lower = lerp(1, Surface.alphaSqr, Light.NdotH * Light.NdotH);
	return Surface.alphaSqr / max(1e-6, PI * lower * lower);
}

// Schlick-Smith specular geometric visibility function
float G_Schlick_Smith(SurfaceProperties Surface, LightProperties Light)
{
	return 1.0 / max(1e-6, lerp(Surface.NdotV, 1, Surface.alpha * 0.5) * lerp(Light.NdotL, 1, Surface.alpha * 0.5));
}

// Schlick-Smith specular visibility with Hable's LdotH approximation
float G_Shlick_Smith_Hable(SurfaceProperties Surface, LightProperties Light)
{
	return 1.0 / lerp(Light.LdotH * Light.LdotH, 1, Surface.alphaSqr * 0.25);
}


// A microfacet based BRDF.
// alpha:	This is roughness squared as in the Disney PBR model by Burley et al.
// c_spec:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals.  This is the specular albedo.
// NdotV, NdotL, LdotH, NdotH:  vector dot products
//  N - surface normal
//  V - normalized view vector
//  L - normalized direction to light
//  H - normalized half vector (L+V)/2 -- halfway between L and V
float3 Specular_BRDF(SurfaceProperties Surface, LightProperties Light)
{
	// Normal Distribution term
	float ND = Specular_D_GGX(Surface, Light);

	// Geometric Visibility term
	//float GV = G_Schlick_Smith(Surface, Light);
	float GV = G_Shlick_Smith_Hable(Surface, Light);

	// Fresnel term
	float3 F = Fresnel_Shlick(Surface.c_spec, 1.0, Light.LdotH);

	return ND * GV * F;
}

// Diffuse irradiance
float3 Diffuse_IBL(SurfaceProperties Surface)
{
	// Assumption:  L = N

	//return Surface.c_diff * irradianceIBLTexture.Sample(defaultSampler, Surface.N);

	// This is nicer but more expensive, and specular can often drown out the diffuse anyway
	float LdotH = saturate(dot(Surface.N, normalize(Surface.N + Surface.V)));
	float fd90 = 0.5 + 2.0 * Surface.roughness * LdotH * LdotH;
	float3 DiffuseBurley = Surface.c_diff * Fresnel_Shlick(1, fd90, Surface.NdotV);
	return DiffuseBurley * irradianceIBLTexture.SampleLevel(defaultSampler, Surface.N, 0);
}

// Approximate specular IBL by sampling lower mips according to roughness.  Then modulate by Fresnel. 
float3 Specular_IBL(SurfaceProperties Surface)
{
	float lod = Surface.roughness * IBLRange + IBLBias;
	float3 specular = Fresnel_Shlick(Surface.c_spec, 1, Surface.NdotV);
	return specular * radianceIBLTexture.SampleLevel(defaultSampler, reflect(-Surface.V, Surface.N), lod);
}


float GetShadowConeLightPBR(uint lightIndex, float3 shadowCoord)
{
	float result = lightShadowArrayTex.SampleCmpLevelZero(shadowSampler, float3(shadowCoord.xy, lightIndex), shadowCoord.z);

	return result * result;
}

void CalcReflectionFactors(SurfaceProperties Surface, float3 L, out float3 diffuse, out float3 specular)
{
	LightProperties Light;
	Light.L = L;

	// Half vector
	float3 H = normalize(L + Surface.V);

	// Pre-compute dot products
	Light.NdotL = saturate(dot(Surface.N, L));
	Light.LdotH = saturate(dot(L, H));
	Light.NdotH = saturate(dot(Surface.N, H));

	// Diffuse & specular factors
	diffuse = Diffuse_Burley(Surface, Light) * Light.NdotL;
	specular = Specular_BRDF(Surface, Light) * Light.NdotL;
}

float3 ApplyLightCommonPBR(SurfaceProperties Surface, float3 L, float3 c_light)
{
	// Diffuse & specular factors
	float3 diffuse, specular;
	CalcReflectionFactors(Surface, L, diffuse, specular);

	// Directional light
	return c_light * (diffuse + specular);
}

float3 ApplyDirectionalLightPBR(
	SurfaceProperties Surface,
	float3 L, 
	float3 c_light,
	float3	shadowCoord,	// Shadow coordinate (Shadow map UV & light-relative Z)
	Texture2D<float> ShadowMap
)
{
	float shadow = GetSampledShadow(shadowCoord, ShadowTexelSize.x, ShadowMap);

	return shadow * ApplyLightCommonPBR(Surface, L, c_light);
}

float3 ApplyPointLightPBR(SurfaceProperties Surface, float3 c_light,
	float3	worldPos,		// World-space fragment position
	float3	lightPos,		// World-space light position
	float	lightRadiusSq
)
{
	float3 lightDir = lightPos - worldPos;
	float lightDistSq = dot(lightDir, lightDir);
	float invLightDist = rsqrt(lightDistSq);
	lightDir *= invLightDist;

	// modify 1/d^2 * R^2 to fall off at a fixed radius
	// (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
	float distanceFalloff = lightRadiusSq * (invLightDist * invLightDist);
	distanceFalloff = max(0, distanceFalloff - rsqrt(distanceFalloff));

	return distanceFalloff * ApplyLightCommonPBR(Surface, lightDir, c_light);
}

float3 ApplyConeLightPBR(SurfaceProperties Surface, float3 c_light,
	float3	worldPos,		// World-space fragment position
	float3	lightPos,		// World-space light position
	float	lightRadiusSq,
	float3	coneDir,
	float2	coneAngles
)
{
	float3 lightDir = lightPos - worldPos;
	float lightDistSq = dot(lightDir, lightDir);
	float invLightDist = rsqrt(lightDistSq);
	lightDir *= invLightDist;

	// modify 1/d^2 * R^2 to fall off at a fixed radius
	// (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
	float distanceFalloff = lightRadiusSq * (invLightDist * invLightDist);
	distanceFalloff = max(0, distanceFalloff - rsqrt(distanceFalloff));

	float coneFalloff = dot(-lightDir, coneDir);
	coneFalloff = saturate((coneFalloff - coneAngles.y) * coneAngles.x);

	return (coneFalloff * distanceFalloff) * ApplyLightCommonPBR(Surface, lightDir, c_light);
}

float3 ApplyConeShadowedLightPBR(SurfaceProperties Surface, float3 c_light,
	float3	worldPos,		// World-space fragment position
	float3	lightPos,		// World-space light position
	float	lightRadiusSq,
	float3	coneDir,
	float2	coneAngles,
	float4x4 shadowTextureMatrix,
	uint	lightIndex
)
{
#ifdef RAY_TRACING
	shadowTextureMatrix = transpose(shadowTextureMatrix);
#endif
	float4 shadowCoord = mul(shadowTextureMatrix, float4(worldPos, 1.0));
	shadowCoord.xyz *= rcp(shadowCoord.w);
	float shadow = GetShadowConeLightPBR(lightIndex, shadowCoord.xyz);

	return shadow * ApplyConeLightPBR(Surface, c_light,
		worldPos,
		lightPos,
		lightRadiusSq,
		coneDir,
		coneAngles
	);
}

#ifdef RAY_TRACING
float3 ApplyDirectionalLightRT(
	SurfaceProperties Surface,
	float3 L,
	float3 c_light,
	float3  shadowOrigin    // pos
)
{
	float shadow = GetShadowRT(L, shadowOrigin, FLT_MAX);

	return shadow * ApplyLightCommonPBR(Surface, L, c_light);
}


float3 ApplyConeShadowedLightRT(SurfaceProperties Surface, float3 c_light,
	float3	worldPos,		// World-space fragment position
	float3	lightPos,		// World-space light position
	float	lightRadiusSq,
	float3	coneDir,
	float2	coneAngles
)
{
	float3 lightDir = lightPos - worldPos;
	float lightDistSq = dot(lightDir, lightDir);
	float invLightDist = rsqrt(lightDistSq);
	float lightDist = 1.0f / invLightDist;
	lightDir *= invLightDist;

	// modify 1/d^2 * R^2 to fall off at a fixed radius
	// (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
	float distanceFalloff = lightRadiusSq * (invLightDist * invLightDist);
	distanceFalloff = max(0, distanceFalloff - rsqrt(distanceFalloff));

	float coneFalloff = dot(-lightDir, coneDir);
	coneFalloff = saturate((coneFalloff - coneAngles.y) * coneAngles.x);

	float shadow = 1.0;

	if (coneFalloff && distanceFalloff)
	{
		shadow = GetShadowRT(lightDir, worldPos, lightDist);
	}

	return shadow * (coneFalloff * distanceFalloff) *
		ApplyLightCommonPBR(Surface, lightDir, c_light);
}
#endif

#define POINT_LIGHT_ARGS_PBR \
	Surface, lightData.color, \
	worldPos, \
	lightData.pos, \
	lightData.radiusSq \

#define CONE_LIGHT_ARGS_PBR \
	POINT_LIGHT_ARGS_PBR, \
	lightData.coneDir, \
	lightData.coneAngles

#define SHADOWED_LIGHT_ARGS_PBR \
	CONE_LIGHT_ARGS_PBR, \
	lightData.shadowTextureMatrix, \
	lightIndex

void ShadeLightsPBR(inout float3 colorSum, uint2 pixelPos,
	SurfaceProperties Surface,
	float3 worldPos
)
{
	uint2 tilePos = GetTilePos(pixelPos, InvTileDim.xy);
	uint tileIndex = GetTileIndex(tilePos, TileCount.x);
	uint tileOffset = GetTileOffset(tileIndex);

	// Light Grid Preloading setup
	uint lightBitMaskGroups[4] = { 0, 0, 0, 0 };
#if defined(LIGHT_GRID_PRELOADING)
	uint4 lightBitMask = lightGridBitMask.Load4(tileIndex * 16);

	lightBitMaskGroups[0] = lightBitMask.x;
	lightBitMaskGroups[1] = lightBitMask.y;
	lightBitMaskGroups[2] = lightBitMask.z;
	lightBitMaskGroups[3] = lightBitMask.w;
#endif

	uint tileLightCount = lightGrid.Load(tileOffset + 0);
	uint tileLightCountSphere = (tileLightCount >> 0) & 0xff;
	uint tileLightCountCone = (tileLightCount >> 8) & 0xff;
	uint tileLightCountConeShadowed = (tileLightCount >> 16) & 0xff;

	uint tileLightLoadOffset = tileOffset + 4;

	// sphere
	uint n;
	for (n = 0; n < tileLightCountSphere; n++, tileLightLoadOffset += 4)
	{
		uint lightIndex = lightGrid.Load(tileLightLoadOffset);
		LightData lightData = lightBuffer[lightIndex];
		colorSum += ApplyPointLightPBR(POINT_LIGHT_ARGS_PBR);
	}

	// cone
	for (n = 0; n < tileLightCountCone; n++, tileLightLoadOffset += 4)
	{
		uint lightIndex = lightGrid.Load(tileLightLoadOffset);
		LightData lightData = lightBuffer[lightIndex];
		colorSum += ApplyConeLightPBR(CONE_LIGHT_ARGS_PBR);
	}

	// cone w/ shadow map
	for (n = 0; n < tileLightCountConeShadowed; n++, tileLightLoadOffset += 4)
	{
		uint lightIndex = lightGrid.Load(tileLightLoadOffset);
		LightData lightData = lightBuffer[lightIndex];
#ifdef RAY_TRACING
		if (UseShadowRays)
		{
			colorSum += ApplyConeShadowedLightRT(CONE_LIGHT_ARGS_PBR);
		}
		else
#endif
		{
			colorSum += ApplyConeShadowedLightPBR(SHADOWED_LIGHT_ARGS_PBR);
		}
	}
}

void ShadeLightsPBR(inout float3 colorSum,
	SurfaceProperties Surface,
	float3 worldPos
)
{
	for (uint lightIndex = 0; lightIndex < MAX_LIGHTS; lightIndex++)
	{
		LightData lightData = lightBuffer[lightIndex];

		if (lightIndex < FirstLightIndex.x) // sphere
		{
			colorSum += ApplyPointLightPBR(POINT_LIGHT_ARGS_PBR);
		}
		else if (lightIndex < FirstLightIndex.y) // cone
		{
			colorSum += ApplyConeLightPBR(CONE_LIGHT_ARGS_PBR);
		}
		else if (lightIndex < FirstLightIndex.z)// cone w/ shadow map
		{
#ifdef RAY_TRACING
			if (UseShadowRays)
			{
				colorSum += ApplyConeShadowedLightRT(CONE_LIGHT_ARGS_PBR);
			}
			else
#endif
			{
				colorSum += ApplyConeShadowedLightPBR(SHADOWED_LIGHT_ARGS_PBR);
			}
		}
	}
}
#endif // LIGHTING_PBR_H_INCLUDED