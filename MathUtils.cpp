#include "MathUtils.h"

#include "Log.h"

#include <cmath>

vec3 operator/(const vec3& a, float s)
{
    if (s == 0)
    {
        Log::Error("Divide by zero");
        return vec3zero;
    }

    return vec3(
        a.x / s,
        a.y / s,
        a.z / s);
}

float vec3Length(const vec3& v)
{
    return sqrtf(vec3Dot(v, v));
}

vec3 vec3Normalize(const vec3& a)
{
    return a / vec3Length(a);
}
