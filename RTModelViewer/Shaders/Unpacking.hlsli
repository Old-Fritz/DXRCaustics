#ifndef UNPACKING_H_INCLUDED
#define UNPACKING_H_INCLUDED

// Packed types							PIX Type
#define PT_USHORT4N		uint2		// unshort4
#define PT_UBYTE4		uint		// ubyte4
#define PT_UBYTE4N		uint		// unbyte4
#define PT_DEC4			uint		// xint		(10:10:10:2)
#define PT_COLOR		uint		// ubyte4
#define PT_FLOAT16_2	uint		// unshort2

// unpacking
float4 UnPackUShort4N(PT_USHORT4N val)
{
	float4 res;
	res.x = val.x & 0xFFFF;
	res.y = (val.x >> 16) & 0xFFFF;
	res.z = val.y & 0xFFFF;
	res.w = (val.y >> 16) & 0xFFFF;
	return res / 0xFFFF;
}
uint4 UnPackUByte4(PT_UBYTE4 val)
{
	uint4 res;
	res.x = val & 0xFF;
	res.y = (val >> 8) & 0xFF;
	res.z = (val >> 16) & 0xFF;
	res.w = (val >> 24) & 0xFF;
	return res;
}
float4 UnPackUByte4N(PT_UBYTE4N val)
{
	uint4 res = UnPackUByte4(val);
	return res / 255.0;
}
float2 UnPackFloat16_2(PT_FLOAT16_2 val)
{
	float2 res;
	res.x = f16tof32(val);
	res.y = f16tof32(val >> 16);
	return res;
}
float4 UnPackDec4(PT_DEC4 val)
{
	float4 res;
	res.x = val & 0x3FF;
	res.y = (val >> 10) & 0x3FF;
	res.z = (val >> 20) & 0x3FF;
	res.w = (val >> 30) & 0x3;
	return res / float4(1023.0, 1023.0, 1023.0, 3.0);
}
float4 UnPackColor(PT_COLOR val)
{
	return UnPackUByte4N(val);
}

PT_DEC4 PackDec4(float4 val)
{
	float4 limit = float4(1023, 1023, 1023, 3);
	uint4 vi = (uint4)clamp(saturate(val) * limit, 0, limit);
	return (vi.x) | (vi.y << 10) | (vi.z << 20) | (vi.w << 30);

}

#endif //UNPACKING_H_INCLUDED
