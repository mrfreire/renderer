#pragma once

#include "Geometry.h"

#include <stddef.h>
#include <stdint.h>

//
// RASTERIZER INPUT: triangles
// RASTERIZER OUTPUT: buffers
//

struct TriangleInput
{
    const VertexData* const m_vertexArray;
    const int m_indices[3];  // Clockwise
};

struct TriangleData
{
    vec3 m_normal;
    vec3 m_interpNormals[3];
    float m_minX;
    float m_maxX;
    float m_minY;
    float m_maxY;
};

struct FragmentInput
{
    int m_x;
    int m_y;
    // TODO(manuel): z
    float m_interpValues[3];
};

struct ScanData
{
    FragmentInput* m_fragmentsIn;
    int m_capacity;
    int m_fragmentsCount;
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

namespace RasterizerUtils
{
    void TriangleSetup(TriangleData* triangle, const TriangleInput& input);

    void TriangleTraversal(
        ScanData* scan,
        FragmentInput* fragmentsTmpBuffer,
        const TriangleInput& input,
        const TriangleData& triangle);
    
    void TriangleShading(RasterBuffers* buffers, const TriangleInput& input, const ScanData& scan);
}
