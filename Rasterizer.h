#pragma once

#include "Geometry.h"
#include "MathUtils.h"

#include <stddef.h>
#include <stdint.h>

//
// RASTERIZER INPUT: triangles
// RASTERIZER OUTPUT: buffers
//

struct TextureData
{
    int m_width;
    int m_height;
    uint32_t* m_data;
};

struct TriangleInput
{
    const VertexData* m_vertexArray;
    TextureData m_texture;
    int m_indices[3];  // Clockwise
};

struct FragmentInput
{
    int m_x;
    int m_y;
    // TODO(manuel): z
    float m_interpValues[3];
};

struct RasterBuffers
{
    uint32_t* m_color;
    FragmentInput* m_fragmentsTmpBuffer;
    //uint32_t* m_depth;
    size_t m_width;
    size_t m_height;
    size_t m_colorBufferBytes;
    size_t m_fragmentsTmpBufferBytes;
    size_t m_bytesPerPixel;
};

namespace Rasterizer
{
    void RasterTriangle(RasterBuffers* buffers, const TriangleInput& input);
}
