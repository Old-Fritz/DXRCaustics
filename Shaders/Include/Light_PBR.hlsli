#ifndef LIGHT_PBR_H_INCLUDED
#define LIGHT_PBR_H_INCLUDED

#include "Common.hlsli"

#define USE_CORRECT


  ////////////////////////////////////////////////////////////
 /////////////////////    FRESNEL    ////////////////////////
////////////////////////////////////////////////////////////

// Shlick's approximation of Fresnel
float3 PBR::Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
	return lerp(F0, F90, Pow5(1.0 - cosine));
}

float1 PBR::Fresnel_Shlick(float1 F0, float1 F90, float cosine)
{
	return lerp(F0, F90, Pow5(1.0 - cosine));
}


  ////////////////////////////////////////////////////////////
 /////////////////////  DIFFUSE TERM ////////////////////////
////////////////////////////////////////////////////////////


// Burley's diffuse BRDF

float3 PBR::Diffuse_Burley(float roughness, float NdotV, Light light)
{
	const float fd90			= 0.5 + 2.0 * roughness * light.m_LdotH * light.m_LdotH;
	const float lightScatter	= Fresnel_Shlick(1, fd90, light.m_NdotL).x;
	const float viewScatter		= Fresnel_Shlick(1, fd90, NdotV).x;
	return		lightScatter * viewScatter;
}

float3 PBR::Diffuse_Burley_Correct(float roughness, float NdotV, Light light)
{
	const float fd90			= 0.5 + 2.0 * roughness * light.m_LdotH * light.m_LdotH;
	const float lightScatter	= Fresnel_Shlick(1, fd90, light.m_NdotL).x;
	const float viewScatter		= Fresnel_Shlick(1, fd90, NdotV).x;
	return		lightScatter * viewScatter / PI;
}


  ////////////////////////////////////////////////////////////
 ///////////////////// SPECULAR TERM ////////////////////////
////////////////////////////////////////////////////////////

//						a^2
//	D =	--------------------------------------
//		pi * ((n  h)^2 * (a^2 - 1) + 1)^2

// GGX specular D (normal distribution)
float PBR::Specular_D_GGX(float alphaSqr, float NdotH)
{
	const float lower = lerp(1, alphaSqr, NdotH * NdotH); // l = (n  h)^2 * (a^2 - 1) + 1
	return alphaSqr / max(1e-6, PI * lower * lower);							// D = a^2 / (pi * l^2)
}

//								1													 
//	V ~	-------------------------------------------------   , k = 0.5a			G = 4 * (n  V) * (n  l) * V
//		 ((n  v) * (1 - k) + k) * ((n  l) * (1 - k) + k)							

// Schlick-Smith specular geometric visibility function
float PBR::G_Schlick_Smith(float alpha, float NdotV, Light light)
{
	return 1.0 / max(1e-6, lerp(NdotV, 1, alpha * 0.5) * lerp(light.m_NdotL, 1, alpha * 0.5));
}
float PBR::G_Schlick_Smith_Correct(float alpha, float NdotV, Light light)
{
	return 1.0 / max(1e-6, (2 * lerp(2 * NdotV * light.m_NdotL, NdotV + light.m_NdotL, alpha)));
}

// A microfacet based BRDF.
// alpha:	This is roughness squared as in the Disney PBR model by Burley et al.
// c_spec:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals.  This is the specular albedo.
// NdotV, NdotL, LdotH, NdotH:  vector dot products
//  N - surface normal
//  V - normalized view vector
//  L - normalized direction to light
//  H - normalized half vector (L+V)/2 -- halfway between L and V
float3 PBR::Specular_BRDF(float3 specular, float alphaSqr, float alpha, float NdotV, Light light)
{
	// Normal Distribution term
	const float ND = Specular_D_GGX(alphaSqr, light.m_NdotH);

	// Geometric Visibility term
#ifdef USE_CORRECT
	const float GV = G_Schlick_Smith_Correct(alpha, NdotV, light) / 4;
#else
	const float GV = G_Schlick_Smith(alpha, NdotV, light);
#endif
	// Fresnel term
	const float3 F = Fresnel_Shlick(specular, 1.0, light.m_LdotH);

	return ND * GV * F;
}


  ////////////////////////////////////////////////////////////
 /////////////////////      BRDF     ////////////////////////
////////////////////////////////////////////////////////////

float3 PBR::LightHit::CalcDiffuseFactor()
{
#ifdef USE_CORRECT
	return m_surf.m_c_diff * Diffuse_Burley_Correct(m_surf.m_roughness, m_view.m_NdotV, m_light) * m_light.m_NdotL;
#else
	return m_material.m_c_diff * Diffuse_Burley(m_surf.m_roughness, m_view.NdotV, m_light) * Light.NdotL;
#endif
}
float3 PBR::LightHit::CalcSpecularFactor()
{
	return Specular_BRDF(m_surf.m_c_spec, m_surf.m_alphaSqr, m_surf.m_alpha, m_view.m_NdotV, m_light) * m_light.m_NdotL;
}

  //////////////////////////////////////////////////
 ///////////////////// GI /////////////////////////
//////////////////////////////////////////////////

// Diffuse irradiance
float3 PBR::LightHit::Diffuse_IBL(TextureCube<float3> irradTex, SamplerState texSampler)
{
	// Assumption:  L = N

	//return Surface.c_diff * irradTex.Sample(texSampler, Surface.N);

	// This is nicer but more expensive, and specular can often drown out the diffuse anyway
	const float LdotH = saturate(dot(m_surf.m_normal, normalize(m_surf.m_normal + m_view.m_viewDir)));
	const float fd90	= 0.5 + 2.0 * m_surf.m_roughness * LdotH * LdotH;
	const float3 DiffuseBurley = m_surf.m_c_diff * Fresnel_Shlick(1, fd90, m_view.m_NdotV);
	return DiffuseBurley * irradTex.SampleLevel(texSampler, m_surf.m_normal, 0);
}

// Approximate specular IBL by sampling lower mips according to roughness.  Then modulate by Fresnel. 
float3 PBR::LightHit::Specular_IBL(TextureCube<float3> radTex, SamplerState texSampler, float iblRange, float iblBias)
{
	const float lod = m_surf.m_roughness * iblRange + iblBias;
	const float3 specular = Fresnel_Shlick(m_surf.m_c_spec, 1, m_view.m_NdotV);
	return specular * radTex.SampleLevel(texSampler, reflect(-m_view.m_viewDir, m_surf.m_normal), lod);
}


void PBR::LightHit::ApplyAO(float ao)
{
	m_surf.m_c_diff *= ao;
	m_surf.m_c_spec *= ao;
}



/// Light / View updates ///
void PBR::LightHit::SetSurface(Mat::Material matData)
{
	m_surf.Update(matData);
}
void PBR::LightHit::SetLight(float3 lightDir)
{
	m_light.Update(m_surf.m_normal, lightDir, m_view.m_viewDir);
}
void PBR::LightHit::SetView(float3 viewDir)
{
	m_view.Update(m_surf.m_normal, viewDir);
}
void PBR::LightHit::SetView(float3 viewerPos, float3 worldPos)
{
	SetView(normalize(viewerPos - worldPos));
}


void PBR::View::Update(float3 normal, float3 viewDir)
{
	m_viewDir	= viewDir;
	m_NdotV		= saturate(dot(normal, viewDir));
}
void PBR::Light::Update(float3 normal, float3 lightDir, float3 viewDir)
{
	// Half vector
	const float3 H = normalize(lightDir + viewDir);

	// Pre-compute dot products
	m_NdotL = saturate(dot(normal,	lightDir));
	m_LdotH = saturate(dot(lightDir,H));
	m_NdotH = saturate(dot(normal,	H));
}


/// Init surface ///
void PBR::Surface::Update(Mat::Material matData)
{
	m_normal		= matData.m_normal;
	m_c_diff		= matData.m_baseColor.rgb * (1 - kDielectricSpecular) * (1 - matData.m_metallicRoughness.x)	* matData.m_occlusion; // 0 for ideal metals
	m_c_spec		= lerp(kDielectricSpecular, matData.m_baseColor.rgb, matData.m_metallicRoughness.x)			* matData.m_occlusion; // baseColor for ideal metals
	m_roughness		= matData.m_metallicRoughness.y;
	m_alpha			= matData.m_metallicRoughness.y * matData.m_metallicRoughness.y;
	m_alphaSqr		= m_alpha * m_alpha;
}

PBR::LightHit PBR::BuildSurface(Mat::Material matData)
{
	// kDielectricSpecular - 0.16 * reflectance * reflactance, from 0.04 to 0.16, default 0.04

	LightHit hit;
	hit.SetSurface(matData);

	return hit;
}




#endif // LIGHT_PBR_H_INCLUDED