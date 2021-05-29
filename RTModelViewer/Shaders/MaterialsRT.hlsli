#ifndef MATERIALS_RT_H_INCLUDED
#define MATERIALS_RT_H_INCLUDED

#include "LightingPBR.hlsli"

struct GBuffer
{
	float4			baseColor;
	float2			metallicRoughness;
	float			occlusion;
	float3			emissive;
	float3			normal;
};

// sample textures
float4 TexSample(Texture2D<float4> tex, TextureCoords coords)
{
	return tex.SampleGrad(defaultSampler, coords.uv, coords.ddxUV, coords.ddyUV);
}
float3 TexSample(Texture2D<float3> tex, TextureCoords coords)
{
	return tex.SampleGrad(defaultSampler, coords.uv, coords.ddxUV, coords.ddyUV);
}
float2 TexSample(Texture2D<float2> tex, TextureCoords coords)
{
	return tex.SampleGrad(defaultSampler, coords.uv, coords.ddxUV, coords.ddyUV);
}
float1 TexSample(Texture2D<float1> tex, TextureCoords coords)
{
	return tex.SampleGrad(defaultSampler, coords.uv, coords.ddxUV, coords.ddyUV);
}

GBuffer ExtractGBuffer(VertexData vertex)
{
	GBuffer gBuf;

	// Retrieve material constants
	MaterialConstantsRT mat = g_materialConstants[g_meshInfo[MaterialID].m_materialInstanceId];

	// Load and modulate textures
	gBuf.baseColor = TexSample(g_localBaseColor, vertex.tc) * mat.baseColorFactor;
	gBuf.metallicRoughness = TexSample(g_localMetallicRoughness, vertex.tc).bg * mat.metallicRoughnessFactor;
	gBuf.occlusion = TexSample(g_localOcclusion, vertex.tc);
	gBuf.emissive = TexSample(g_localEmissive, vertex.tc) * mat.emissiveFactor;
	gBuf.normal = TexSample(g_localNormal, vertex.tc) * 2.0 - 1.0;

	// calculate normal
	gBuf.normal = normalize(gBuf.normal * float3(mat.normalTextureScale, mat.normalTextureScale, 1));
	float3x3 tbn = float3x3(vertex.tangent, vertex.bitangent, vertex.normal);
	gBuf.normal = mul(gBuf.normal, tbn);

	return gBuf;
}


SurfaceProperties BuildSurface(GBuffer gBuf, float3 viewDirection)
{
	SurfaceProperties Surface;

	Surface.N = gBuf.normal;
	Surface.V = viewDirection;
	Surface.NdotV = saturate(dot(Surface.N, Surface.V));
	Surface.c_diff = gBuf.baseColor.rgb * (1 - kDielectricSpecular) * (1 - gBuf.metallicRoughness.x) * gBuf.occlusion;
	Surface.c_spec = lerp(kDielectricSpecular, gBuf.baseColor.rgb, gBuf.metallicRoughness.x) * gBuf.occlusion;
	Surface.roughness = gBuf.metallicRoughness.y;
	Surface.alpha = gBuf.metallicRoughness.y * gBuf.metallicRoughness.y;
	Surface.alphaSqr = Surface.alpha * Surface.alpha;

	return Surface;
}

#endif // MATERIALS_RT_H_INCLUDED