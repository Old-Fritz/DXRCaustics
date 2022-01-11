
#ifndef UTILS_OUTPUT_H_INCLUDED
#define UTILS_OUTPUT_H_INCLUDED

#include "Common.hlsli"

// output
float4 Output::ReadOutput(uint2 screenCoords)								{	return Res::GetScreenOutput()[screenCoords];	}

void Output::WriteOutput(uint2 index, float4 color)							{ Res::GetScreenOutput()[index] = float4(color	 );	}
void Output::WriteOutput(uint2 index, float3 color, float w)				{ WriteOutput(index, float4(color,		w));		}
void Output::WriteOutput(uint2 index, float2 color, float z, float w)		{ WriteOutput(index, float4(color,	z,	w));		}
void Output::WriteOutput(uint2 index, float x, float y, float z, float w)	{ WriteOutput(index, float4(x, y,	z,	w));		}

void Output::AddOutput(uint2 index, float4 color)							{ Res::GetScreenOutput()[index] += float4(color	 );	}
void Output::AddOutput(uint2 index, float3 color, float w)					{ AddOutput(index, float4(color,		w));		}
void Output::AddOutput(uint2 index, float2 color, float z, float w)			{ AddOutput(index, float4(color,	z,	w));		}
void Output::AddOutput(uint2 index, float x, float y, float z, float w)		{ AddOutput(index, float4(x, y,	z,		w));		}

void Output::SubOutput(uint2 index, float4 color)							{ Res::GetScreenOutput()[index] -= float4(color );	}
void Output::SubOutput(uint2 index, float3 color, float w)					{ SubOutput(index, float4(color,		w));		}
void Output::SubOutput(uint2 index, float2 color, float z, float w)			{ SubOutput(index, float4(color,	z,	w));		}
void Output::SubOutput(uint2 index, float x, float y, float z, float w)		{ SubOutput(index, float4(x, y,	z,		w));		}


#endif // UTILS_OUTPUT_H_INCLUDED