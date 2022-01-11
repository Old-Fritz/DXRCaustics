#ifndef LIGHT_COMMON_H_INCLUDED
#define LIGHT_COMMON_H_INCLUDED

#include "Common.hlsli"

///  Material  ///
namespace Mat
{
	// Flag helpers
	static const uint BASECOLOR = 0;
	static const uint METALROUGH = 1;
	static const uint OCCLUSION = 2;
	static const uint EMISSIVE = 3;
	static const uint NORMAL = 4;

	// all material data needed to describe surface properties
	struct Material
	{
		float4	m_baseColor;
		float2	m_metallicRoughness;
		float	m_occlusion;
		float3	m_emissive;
		float3	m_normal;
	};

	// per mesh material textures
	namespace Tex
	{
		Material LoadData(			Vertex vertex, MaterialConstants matConsts);

		float4 BaseColor(			Vertex vertex, uint matFlags);
		float2 MetallicRoughness(	Vertex vertex, uint matFlags);
		float1 Occlusion(			Vertex vertex, uint matFlags);
		float3 Emissive(			Vertex vertex, uint matFlags);
		float3 Normal(				Vertex vertex, uint matFlags);
		float1 Transparency(		Vertex vertex, uint matFlags);
	}

	// prerendered texture data for each pixel of screen
	namespace GBuf
	{
		Material LoadData(			uint2 index);

		float1 Depth(				uint2 index);
		float4 BaseColor(			uint2 index);
		float2 MetallicRoughness(	uint2 index);
		float1 Occlusion(			uint2 index);
		float3 Emissive(			uint2 index);
		float3 Normal(				uint2 index);
		float1 Transparency(		uint2 index);
	}

	// array of GBuffers per each light source
	namespace Array
	{
		Material LoadData(			uint2 index, uint arraySlice);

		float1 Depth(				uint2 index, uint arraySlice);
		float4 BaseColor(			uint2 index, uint arraySlice);
		float2 MetallicRoughness(	uint2 index, uint arraySlice);
		float1 Occlusion(			uint2 index, uint arraySlice);
		float3 Emissive(			uint2 index, uint arraySlice);
		float3 Normal(				uint2 index, uint arraySlice);
		float1 Transparency(		uint2 index, uint arraySlice);
	}
}


///  Surface  ///
namespace PBR
{
	// static for surface
	struct Surface
	{
		float3	m_normal;
		float3	m_c_diff;
		float3	m_c_spec;
		float	m_roughness;
		float	m_alpha;		// roughness squared
		float	m_alphaSqr;	// alpha squared		

		void Update(Mat::Material matData);
	};

	// depends on light sources and viewers
	struct View
	{
		float3	m_viewDir;
		float	m_NdotV;

		void Update(float3 normal, float3 viewDir);
	};
	struct Light
	{
		float	m_NdotL;
		float	m_LdotH;
		float	m_NdotH;

		void Update(float3 normal, float3 lightDir, float3 viewDir);
	};

	// describe light interactions in point of surface
	struct LightHit
	{
		Surface	m_surf;
		Light	m_light;
		View	m_view;

		// update components of hit
		void SetSurface(Mat::Material matData);

		void SetView(float3 viewDir);
		void SetView(float3 viewerPos, float3 worldPos);

		void SetLight(float3 lightDir);
	
		// BSDF
		float3 CalcDiffuseFactor();
		float3 CalcSpecularFactor();

		// GI
		float3 Diffuse_IBL(	TextureCube<float3> irradTex, SamplerState texSampler);
		float3 Specular_IBL(TextureCube<float3> radTex, SamplerState texSampler, float iblRange, float iblBias);

		void ApplyAO(float ao);
		
	};


	// helper functions to calculate BRDF

	// fresnel
	float1 Fresnel_Shlick(float1 F0, float1 F90, float cosine);
	float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine);


	// diffuse factor
	float3 Diffuse_Burley(					float roughness,			float NdotV,	Light light);
	float3 Diffuse_Burley_Correct(			float roughness,			float NdotV,	Light light);

	// specular factor
	float Specular_D_GGX(					float alphaSqr,				float NdotH);
	float G_Schlick_Smith(					float alpha,				float NdotV,	Light light);
	float G_Schlick_Smith_Correct(			float alpha,				float NdotV,	Light light);
	
	float3 Specular_BRDF(float3 specular,	float alphaSqr, float alpha,float NdotV,	Light light);


	// Init surface of light hit by material info 
	LightHit BuildSurface(Mat::Material matData);

}


///  Lights  ///
namespace Lights
{
	// Light utils
	struct LightDirection
	{
		float3	m_dir;
		float	m_invLightDist;

		void Norm() { m_dir = m_dir * m_invLightDist; }
	};

	// light types
	struct PointLight
	{
		float3	m_color;
		float3	m_pos;
		float	m_radiusSq;
		float	m_radius;

		LightDirection	DirData(	float3 worldPos);
		LightDirection	DirDataNorm(float3 worldPos);

		float			Falloff(	float3 worldPos);
		float			DistFalloff(float invLightDist);
	};
	struct ConeLight : PointLight
	{
		float3 m_coneDir;
		float2 m_coneAngles; // x = 1.0f / (cos(coneInner) - cos(coneOuter)), y = cos(coneOuter)

		float ConeFallOff(float3 lightDir);
		float FallOff(LightDirection lightDir);
	};

	// data compatible with light buffer
	struct LightData : ConeLight
	{
		uint m_type;

		column_major float4x4 m_shadowTextureMatrix;
		float4x4 m_cameraToWorld;
	};


	// manage all scene lights and apply them for current light hit
	struct LightsAccumulator
	{
		PBR::LightHit	m_hit;
		float3			m_colSum;
		float3			m_worldPos;

		// calculate all lights in point and return result
		float3	AccumulateScreenSpace(uint2 pixelPos);
		float3	AccumulateAll();

		// calculate shadows and occlusions
		float	OccludeDirectional(float4x4 shadowMatrix, Texture2D<float> texShadow);
		float	OccludeCone(uint lightIndex, float4x4 shadowMatrix, Texture2DArray<float> texShadow);
		float	OccludeAmbient(uint2 pixelPos);

		// calculate single light (don't change m_colorSum)
		float3	ApplyAmbientLight(		float3 color);
		float3	ApplyLightCommon(		float3 color, float3 direction);
		float3	ApplyDirectionalLight(	float3 color, float3 direction);
		float3	ApplyPointLight(PointLight light);
		float3	ApplyConeLight(	ConeLight light);


		// Add global lights
		void	AddSunLight();
		void	AddAmbient();
		void	AddOccludedAmbient(uint2 pixelPos);


		// Add lights from light buffer
		void	AddAllLights();
		void	AddScreenSpaceLights(uint2 pixelPos);

		void	AddPointLight(			uint lightIndex);
		void	AddConeLight(			uint lightIndex);
		void	AddConeShadowedLight(	uint lightIndex);


		
		// ways to select lights from grid
		void	AddGridLightsScalar(		uint tileOffset);
		void	AddGridLightsScalarBranch(	uint tileOffset);
		void	AddGridLightsScalarLoop(	uint tileOffset);
		void	AddGridLightsBitMaskSorted(	uint tileIndex);
		void	AddGridLightsBitMask(		uint tileIndex);
	};

	// Init accumulator with all needed data
	LightsAccumulator InitAccumulator(Mat::Material matData, Ray viewRay);

	// sampe skybox color to in provided direction
	float3 GetSkybox(float3 dir);
}


///  Shadows  ///
namespace Shadows
{
	/// Shadows ///
	float SampleShadow(		float2 shadowCoord, float z, Texture2D<float> tex);
	float GetShadow(		float2 shadowCoord, float z, Texture2D<float> tex);
	float GetSampledShadow(	float2 shadowCoord, float texelSize, float z, Texture2D<float> tex);

	float SampleShadow(		float3 shadowCoord, float z, Texture2DArray<float> tex);
	float GetShadow(		float3 shadowCoord, float z, Texture2DArray<float> tex);
	float GetSampledShadow(	float3 shadowCoord, float texelSize, float z, Texture2DArray<float> tex);
	
	float GetDefaultArrayTexelSize();
	float GetDefaultTexelSize();
	
	float GetDefaultSampledShadow(float2 shadowCoord, float z, Texture2D<float> texShadow);
	float GetDefaultSampledShadow(float3 shadowCoord, float z, Texture2DArray<float> texShadow);
	
	float3 CalcShadowCoord(float4x4 shadowMat, float3 worldPos);
}

#endif // LIGHT_COMMON_H_INCLUDED