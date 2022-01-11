#ifndef HLSL_COMPAT_H_INCLUDED
#define HLSL_COMPAT_H_INCLUDED

#define OUTPARAM(type, name)    type& name
#define INOUTPARAM(type, name)    type& name
#define column_major
typedef unsigned int UINT;
#define CB_ALIGN __declspec(align(256)) 

struct float2
{
    float   x, y;
};

struct float3
{
    float   x, y, z;

	float3() : float3(0.0f, 0.0f, 0.0f) {}
	float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	float3(const Math::Vector3& vec)
	{
		x = vec.GetX();
		y = vec.GetY();
		z = vec.GetZ();
	}
};

struct float4
{
    float   x, y, z, w;


	float4() : float4(0.0f, 0.0f, 0.0f, 0.0f) {}
	float4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	float4(const Math::Vector4& vec)
	{
		x = vec.GetX();
		y = vec.GetY();
		z = vec.GetZ();
		w = vec.GetW();
	}
};

__declspec(align(16))
struct uint4
{
    UINT    x, y, z, w;

	uint4() : uint4(0, 0, 0, 0) {}
	uint4(UINT _x, UINT _y, UINT _z, UINT _w) : x(_x), y(_y), z(_z), w(_w) {}
	uint4(const Math::UintVector4& vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		w = vec.w;
	}
};

#if 1
typedef Math::Matrix4 float4x4;
#else
__declspec(align(16))
struct float4x4
{
    float   mat[16];

	float4x4(const Math::Matrix4& matrix)
	{
		mat = matrix;
	}
};
#endif

typedef UINT uint;

typedef UINT uint2[2];

inline
float3 operator + (const float3& a, const float3& b)
{
    return float3{ a.x + b.x, a.y + b.y, a.z + b.z };
}


inline
float3 operator - (const float3& a, const float3& b)
{
    return float3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

inline
float3 operator * (const float3& a, const float3& b)
{
    return float3{ a.x * b.x, a.y * b.y, a.z * b.z };
}


inline
float3 abs(const float3& a)
{
    return float3{ std::abs(a.x), std::abs(a.y), std::abs(a.z) };
}

inline
float min(float a, float b)
{
    return std::min(a, b);
}

inline
float3 min(const float3& a, const float3& b)
{
    return float3{ std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
}


inline
float max(float a, float b)
{
    return std::max(a, b);
}

inline
float3 max(const float3& a, const float3& b)
{
    return float3{ std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
}

inline
float sign(float v)
{
    if (v < 0)
        return -1;
    return 1;
}

inline
float   dot(float3 a, float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline
float3 cross(float3 a, float3 b)
{
    return float3
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

#endif // HLSL_COMPAT_H_INCLUDED
