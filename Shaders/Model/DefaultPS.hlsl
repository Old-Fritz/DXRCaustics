

#define USE_GBUFFER
#include "Common.hlsli"
#include "Res_Pixel.hlsli"

cbuffer MaterialCB : register(b0)
{
	MaterialConstants g_materialCB;
}

struct VSOutput
{
	float4 position			: SV_POSITION;
	float3 normal			: NORMAL;
#ifndef NO_TANGENT_FRAME
	float4 tangent			: TANGENT;
#endif
	float2 uv0				: TEXCOORD0;
#ifndef NO_SECOND_UV
	float2 uv1				: TEXCOORD1;
#endif
	float3 worldPos			: TEXCOORD2;
	float3 sunShadowCoord	: TEXCOORD3;
};

Vertex GetVertexData(VSOutput vsOutput)
{
	Vertex vertexData;
	vertexData.m_normal = vsOutput.normal;
#ifndef NO_TANGENT_FRAME
	vertexData.m_tangent= vsOutput.tangent;
#endif
	vertexData.m_uv0	= vsOutput.uv0;
#ifndef NO_SECOND_UV
	vertexData.m_uv1	= vsOutput.uv1;
#endif

	return vertexData;
}

#ifdef USE_GBUFFER
[RootSignature(Renderer_RootSig)]
MRT main(VSOutput vsOutput)
{
	return (MRT) (Mat::Tex::LoadData(GetVertexData(vsOutput), g_materialCB));
}
#else

[RootSignature(Renderer_RootSig)]
float4 main(VSOutput vsOutput) : SV_Target0
{
	return float4(Lights::InitAccumulator(Mat::Tex::LoadData(GetVertexData(vsOutput), g_materialCB), GetRay(vsOutput.worldPos, Res::ViewerPos())).AccumulateScreenSpace(uint2(vsOutput.position.xy)), 1.0f);

}
#endif