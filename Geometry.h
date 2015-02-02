#pragma once

#include "MathUtils.h"

#include "External/pow2assert.h"

#include <stdint.h>

struct VertexData
{
    vec4 m_pos;  // window coordinates
    vec4 m_color;
    vec2 m_textureCoord;
};

POW2_STATIC_ASSERT(sizeof(VertexData) == 40);  // size of VertexData is performance-sensitive
