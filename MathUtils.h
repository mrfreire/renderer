#pragma once

//
// vec3
//

struct vec3
{
	float x;
	float y;
	float z;

	vec3()
	{}

	vec3(float x_, float y_, float z_)
		: x(x_)
		, y(y_)
		, z(z_)
	{}
};

vec3 operator+(const vec3& a, const vec3& b);
vec3 operator+(const vec3& a, float s);
vec3 operator-(const vec3& a, const vec3& b);
vec3 operator-(const vec3& a);
vec3 operator*(const vec3& a, const vec3& b);
vec3 operator*(const vec3& a, float s);
vec3 operator/(const vec3& a, float s);
float vec3Dot(const vec3& a, const vec3& b);
vec3 vec3Cross(const vec3& a, const vec3& b);
float vec3Length(const vec3& a);
vec3 vec3Normalize(const vec3& a);

#define vec3zero vec3(0, 0, 0)
#define vec3one vec3(1, 1, 1)

//
// vec4
//

struct vec4
{
	float x;
	float y;
	float z;
	float w;

	vec4()
	{}

	vec4(float x_, float y_, float z_, float w_)
		: x(x_)
		, y(y_)
		, z(z_)
		, w(w_)
	{}
};

vec3 vec4xyz(const vec4& a);

//
// Inline functions
//

inline vec3 operator+(const vec3& a, const vec3& b)
{
	return vec3(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z);
}

inline vec3 operator+(const vec3& a, float s)
{
	return vec3(
		a.x + s,
		a.y + s,
		a.z + s);
}

inline vec3 operator-(const vec3& a, const vec3& b)
{
	return vec3(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z);
}

inline vec3 operator-(const vec3& a)
{
	return vec3(
		-a.x,
		-a.y,
		-a.z);
}

inline vec3 operator*(const vec3& a, const vec3& b)
{
	return vec3(
		a.x * b.x,
		a.y * b.y,
		a.z * b.z);
}

inline vec3 operator*(const vec3& a, float s)
{
	return vec3(
		a.x * s,
		a.y * s,
		a.z * s);
}

inline float vec3Dot(const vec3& a, const vec3& b)
{
	return
		a.x * b.x +
		a.y * b.y +
		a.z * b.z;
}

inline vec3 vec3Cross(const vec3& a, const vec3& b)
{
	return vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

inline vec3 vec4xyz(const vec4& a)
{
	return vec3(a.x, a.y, a.z);
}
