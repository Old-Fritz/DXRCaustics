#ifndef UTILS_WAVES_H_INCLUDED
#define UTILS_WAVES_H_INCLUDED

#include "Common.hlsli"

// options for F+ variants and optimizations
#if 1 // SM6.0
#define _WAVE_OP
#endif

// options for F+ variants and optimizations
#ifdef _WAVE_OP // SM 6.0 (new shader compiler)

uint WaveOp::WaveOr(uint mask)
{
	return WaveActiveBitOr(mask);
}

uint64_t WaveOp::Ballot64(bool b)
{
	uint4 ballots = WaveActiveBallot(b);
	return (uint64_t)ballots.y << 32ll | (uint64_t)ballots.x;
}

#endif // _WAVE_OP

// Helper function for iterating over a sparse list of bits.  Gets the offset of the next
// set bit, clears it, and returns the offset.
uint WaveOp::PullNextBit(inout uint bits)
{
	const uint bitIndex = firstbitlow(bits);
	bits ^= 1u << bitIndex;
	return bitIndex;
}

#endif  //UTILS_WAVES_H_INCLUDED