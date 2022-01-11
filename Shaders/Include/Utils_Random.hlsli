#ifndef UTILS_RANDOM_H_INCLUDED
#define UTILS_RANDOM_H_INCLUDED

#include "Common.hlsli"

// Helper function for Hammersley sequences
float1 Random::RadicalInverse(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10;
}

// Hammersly sequence
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 Random::Hammersley2D(uint elIndex, uint elCount)
{
	return float2(float(elIndex) / float(elCount), RadicalInverse(elIndex));
}

// Rec Seq Seed
float2 Random::RecSeqBlueNoiseSeed(uint2 index, Texture2D<float4> blueNoiseRGBA)
{
	const uint noiseSize = 1024;
	return blueNoiseRGBA.Load(int3(index.xy % noiseSize, 0)).rg;
}
float2 Random::RecSeqHammersleySeed(uint1 index, uint1 resolution) { return Hammersley2D(index, resolution); }
float2 Random::RecSeqHammersleySeed(uint2 index, uint2 resolution)
{
	const uint elIndex = index.y * resolution.x + index.x;
	const uint elCount = resolution.x * resolution.y;
	return Hammersley2D(elIndex, elCount);
}
float2 Random::RecSeqHammersleySeed(uint3 index, uint3 resolution)
{
	const uint sliceSize = resolution.x * resolution.y;
	const uint elIndex = index.z * sliceSize + index.y * resolution.x + index.x;
	const uint elCount = sliceSize * resolution.z;
	return Hammersley2D(elIndex, elCount);
}

// Rec Seq static
float1 Random::RecSeqIndex(uint elIndex, uint elCount, uint recSeqBasis)				{ return recSeqBasis * elCount + elIndex; }
float2 Random::RecSeqElement(float2 recSeqSeed, float1 recSeqIndex, float2 recSeqAlpha)	{ return frac(recSeqSeed + recSeqAlpha * recSeqIndex); }
float2 Random::RecSeqElement(float2 recSeqSeed, float2 recSeqIndex, float2 recSeqAlpha)	{ return frac(recSeqSeed + recSeqAlpha * recSeqIndex); }

// Rec Seq struct
float2 Random::RecSeq::IndexElement(uint elIndex, uint elCount)							{ return Element(Index(	elIndex,	elCount					));	}
float1 Random::RecSeq::Index(uint elIndex, uint elCount)								{ return RecSeqIndex(	elIndex,	elCount,		m_basis);	}
float2 Random::RecSeq::Element(float1 recSeqIndex)										{ return RecSeqElement(	m_seed,		recSeqIndex,	m_alpha);	}
float2 Random::RecSeq::Element(float2 recSeqIndex)										{ return RecSeqElement(	m_seed,		recSeqIndex,	m_alpha);	}

// Random handle
float2 Random::RandomHandle::NextRandom()
{
	float2 el = m_recSeq.IndexElement(m_currentElement, m_seqLength);
	m_currentElement = (m_currentElement + 1) % m_seqLength;

	return el;
}
float2 Random::RandomHandle::SingleRandom()												{ return m_recSeq.IndexElement(0, 1);				}
float2 Random::RandomHandle::SingleHammersley()											{ return m_recSeq.Element(Hammersley2D(0, 1).y);	}


Random::RecSeq Random::InitRecSeq(uint basis, float2 alpha, float2 seed)													{ const  RecSeq recSeq = { seed, alpha, basis };				return recSeq;	}
Random::RecSeq Random::InitRecSeq(uint basis, float2 alpha, uint2 index, Texture2D<float4> blueNoiseRGBA) 					{ return InitRecSeq(basis, alpha, RecSeqBlueNoiseSeed( index, blueNoiseRGBA));	}
Random::RecSeq Random::InitRecSeq(uint basis, float2 alpha, uint2 index, uint2 resolution)									{ return InitRecSeq(basis, alpha, RecSeqHammersleySeed(index, resolution));		}

Random::RandomHandle Random::InitRandom(uint count, RecSeq recSeq)															{ const  RandomHandle handle = { recSeq, count, 0};				return handle;	}
Random::RandomHandle Random::InitRandom(uint count, uint basis, float2 alpha, float2 seed)									{ return InitRandom(	count, InitRecSeq(basis, alpha, seed));					};
Random::RandomHandle Random::InitRandom(uint count, uint basis, float2 alpha, uint2 index, Texture2D<float4> blueNoiseRGBA)	{ return InitRandom(	count, InitRecSeq(basis, alpha, index, blueNoiseRGBA)); };
Random::RandomHandle Random::InitRandom(uint count, uint basis, float2 alpha, uint2 index, uint2 resolution)				{ return InitRandom(	count, InitRecSeq(basis, alpha, index, resolution));	};
#endif