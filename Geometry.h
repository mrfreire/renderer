#pragma once

#include "MathUtils.h"

#include "External/pow2assert.h"

#include <stdint.h>

struct VertexData
{
    vec4 m_pos;  // window coordinates
    uint8_t m_color[4];
};

POW2_STATIC_ASSERT(sizeof(VertexData) == 20);  // size of VertexData is performance-sensitive
