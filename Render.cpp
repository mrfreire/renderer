#include "DebugTimer.h"
#include "MathUtils.h"
#include "Rasterizer.h"
#include "SizeOfArray.h"

#include "External/pow2assert.h"

#include <stdint.h>
#include <string.h>

enum class RenderMode
{
    NORMAL,
    WIREFRAME
};

struct RenderState
{
    RenderMode m_mode;
};

void Render(RasterBuffers* buffers)
{
    DebugTimer_Tic(__FUNCTION__);

    //
    // Clear buffer
    //

    DebugTimer_Tic("ClearBuffer");

    memset(buffers->m_color, 0x7f, buffers->m_colorTotalBytes);

    DebugTimer_TocAndPrint("ClearBuffer");

    //
    // Setup geometry (in window coordinates already)
    //

    const float verticesViewport[][3] = {
        { 0.0f, 0.8f, -1.0f },
        { 0.8f, -0.8f, 0.0f },
        { -0.8f, -0.8f, 0.0f } };

    const float wD2 = buffers->m_width / 2;
    const float hD2 = buffers->m_height / 2;

    vec4 vertices[SizeOfArray(verticesViewport)];
    for (int i = 0; i < SizeOfArray(verticesViewport); ++i)
    {
        vertices[i].x = wD2 + verticesViewport[i][0] * wD2;
        vertices[i].y = hD2 + verticesViewport[i][1] * hD2;
        vertices[i].z = verticesViewport[i][2];
        vertices[i].w = 1;
    }

    static int frame = 0;
    ++frame;

    const VertexData vertexData[] = {
        { vertices[0], { 0, 255, 0, 255 } },
        { vertices[1], { 0, 0, 255, 255 } },
        { vertices[2], { 255, 0, 0, 255 } } };

    const int triangles[] = {
        0, 1, 2
    };

    POW2_ASSERT(SizeOfArray(triangles) % 3 == 0);

    //
    // Rasterize each triangle
    //

    for (int i = 0; i < SizeOfArray(triangles); i += 3)
    {
        RasterizeTriangle(vertexData, sizeof(vertexData), triangles, buffers);
    }

    DebugTimer_TocAndPrint(__FUNCTION__);
}
