#ifndef LIGHT_MATERIALS_H_INCLUDED
#define LIGHT_MATERIALS_H_INCLUDED

#include "Common.hlsli"


float4 Mat::Tex::BaseColor(			Vertex vertex, uint matFlags) { return SampleTex(g_BaseColorSampler,		g_BaseColorTexture,			VB::GetUV(vertex, matFlags, BASECOLOR));	}
float2 Mat::Tex::MetallicRoughness( Vertex vertex, uint matFlags) { return SampleTex(g_MetallicRoughnessSampler,g_MetallicRoughnessTexture, VB::GetUV(vertex, matFlags, METALROUGH)).bg;}
float1 Mat::Tex::Occlusion(			Vertex vertex, uint matFlags) { return SampleTex(g_OcclusionSampler,		g_OcclusionTexture,			VB::GetUV(vertex, matFlags, OCCLUSION));	}
float3 Mat::Tex::Emissive(			Vertex vertex, uint matFlags) { return SampleTex(g_EmissiveSampler,			g_EmissiveTexture,			VB::GetUV(vertex, matFlags, EMISSIVE));		}
float3 Mat::Tex::Normal(			Vertex vertex, uint matFlags) { return SampleTex(g_NormalSampler,			g_NormalTexture,			VB::GetUV(vertex, matFlags, NORMAL)) * 2-1; }
float1 Mat::Tex::Transparency(		Vertex vertex, uint matFlags) { return SampleTex(g_BaseColorSampler,		g_BaseColorTexture,			VB::GetUV(vertex, matFlags, BASECOLOR)).a;	}

float1 Mat::GBuf::Depth(				uint2 index) { return g_GBDepth[			index];		}
float4 Mat::GBuf::BaseColor(			uint2 index) { return g_GBBaseColor[		index];		}
float2 Mat::GBuf::MetallicRoughness(	uint2 index) { return g_GBMetallicRoughness[index];		}
float1 Mat::GBuf::Occlusion(			uint2 index) { return g_GBOcclusion[		index];		}
float3 Mat::GBuf::Emissive(				uint2 index) { return g_GBEmissive[			index];		}
float3 Mat::GBuf::Normal(				uint2 index) { return g_GBNormal[			index].xyz;	}
float1 Mat::GBuf::Transparency(			uint2 index) { return g_GBBaseColor[		index].a;	}

float1 Mat::Array::Depth(				uint2 index, uint arraySlice) { return g_ArrayGBDepth.Load(				int4(index, arraySlice, 0));	}
float4 Mat::Array::BaseColor(			uint2 index, uint arraySlice) { return g_ArrayGBBaseColor.Load(			int4(index, arraySlice, 0));	}
float2 Mat::Array::MetallicRoughness(	uint2 index, uint arraySlice) { return g_ArrayGBMetallicRoughness.Load(	int4(index, arraySlice, 0));	}
float1 Mat::Array::Occlusion(			uint2 index, uint arraySlice) { return g_ArrayGBOcclusion.Load(			int4(index, arraySlice, 0));	}
float3 Mat::Array::Emissive(			uint2 index, uint arraySlice) { return g_ArrayGBEmissive.Load(			int4(index, arraySlice, 0));	}
float3 Mat::Array::Normal(				uint2 index, uint arraySlice) { return g_ArrayGBNormal.Load(			int4(index, arraySlice, 0)).xyz;}
float1 Mat::Array::Transparency(		uint2 index, uint arraySlice) { return g_ArrayGBBaseColor.Load(			int4(index, arraySlice, 0)).a;	}

Mat::Material Mat::Tex::LoadData(Vertex vertex, MaterialConstants matConsts)
{
	Mat::Material matData;

	matData.m_baseColor			= matConsts.BaseColorFactor			* Mat::Tex::BaseColor(			vertex, matConsts.Flags);
	matData.m_metallicRoughness	= matConsts.MetallicRoughnessFactor * Mat::Tex::MetallicRoughness(	vertex, matConsts.Flags);
	matData.m_occlusion			=									  Mat::Tex::Occlusion(			vertex, matConsts.Flags);
	matData.m_emissive			= matConsts.EmissiveFactor			* Mat::Tex::Emissive(			vertex,	matConsts.Flags);

#ifndef NO_TANGENT_FRAME
	// Read normal map and convert to SNORM (TODO:  convert all normal maps to R8G8B8A8_SNORM?)
	matData.m_normal			= Mat::Tex::Normal(vertex, matConsts.Flags);
	matData.m_normal			= normalize(matData.m_normal * float3(matConsts.NormalTextureScale, matConsts.NormalTextureScale, 1));
	matData.m_normal			= mul(matData.m_normal, ConstructTangentFrame(vertex.m_tangent, vertex.m_normal));
#else
	matData.m_normal			= normalize(vertex.m_normal);
#endif

	return matData;
}

Mat::Material Mat::GBuf::LoadData(uint2 index)
{
	Mat::Material matData;

	matData.m_baseColor			= Mat::GBuf::BaseColor(			index);
	matData.m_metallicRoughness = Mat::GBuf::MetallicRoughness(	index);
	matData.m_occlusion			= Mat::GBuf::Occlusion(			index);
	matData.m_emissive			= Mat::GBuf::Emissive(			index);
	matData.m_normal			= Mat::GBuf::Normal(			index);

	return matData;
}

Mat::Material Mat::Array::LoadData(uint2 index, uint arraySlice)
{
	Mat::Material matData;

	matData.m_baseColor			= Mat::Array::BaseColor(		index, arraySlice);
	matData.m_metallicRoughness	= Mat::Array::MetallicRoughness(index, arraySlice);
	matData.m_occlusion			= Mat::Array::Occlusion(		index, arraySlice);
	matData.m_emissive			= Mat::Array::Emissive(			index, arraySlice);
	matData.m_normal			= Mat::Array::Normal(			index, arraySlice);

	return matData;
}


#endif // LIGHT_MATERIALS_H_INCLUDED