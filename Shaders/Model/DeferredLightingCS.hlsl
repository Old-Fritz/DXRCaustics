

#include "Common.hlsli"


[RootSignature(Deferred_RootSig)]
[numthreads(8, 8, 1)]
void main(uint2 pixelPos : SV_DispatchThreadID)
{
	const Ray viewRay = Screen::GBufferViewRay(pixelPos);
	const Mat::Material matData = Mat::GBuf::LoadData(pixelPos);
	Lights::LightsAccumulator acc = Lights::InitAccumulator(matData, viewRay);
	Output::WriteOutput(pixelPos, acc.AccumulateScreenSpace(pixelPos), matData.m_baseColor.w);


	//Output::WriteOutput(pixelPos, Lights::InitAccumulator(Mat::GBuf::LoadData(pixelPos), Screen::GBufferViewRay(pixelPos)).AccumulateScreenSpace(pixelPos), 1.0f); //Mat::GBuf::Transparency(pixelPos));
}
