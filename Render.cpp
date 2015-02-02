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

// TODO(manuel): Temporary hack
static const int g_textureSize = 16;
static uint32_t g_texture[g_textureSize][g_textureSize];
static bool g_initialised = false;

void InitTexture()
{
    // Init test texture
    const uint32_t black = 0x00000000;
    const uint32_t white = 0xffffffff;
    for (int i = 0; i < g_textureSize; ++i)
    {
        bool evenRow = (i % 2 == 0);
        for (int j = 0; j < g_textureSize; ++j)
        {
            bool evenCol = (j % 2 == 0);
            if (evenRow)
            {
                g_texture[i][j] = evenCol ? white : black;
            }
            else
            {
                g_texture[i][j] = evenCol ? black : white;
            }
        }
    }
}

void Render(RasterBuffers* buffers)
{   
    if (!g_initialised)  // TODO(manuel): Temporary hack
    {
        g_initialised = true;
        InitTexture();
    }

    DebugTimer_Tic(__FUNCTION__);

    //
    // Clear buffer
    //

    DebugTimer_Tic("ClearBuffers");

    memset(buffers->m_color, 0x7f, buffers->m_colorBufferBytes);
    memset(buffers->m_fragmentsTmpBuffer, 0, buffers->m_fragmentsTmpBufferBytes);

    DebugTimer_TocAndPrint("ClearBuffers");

    //
    // Setup geometry (in window coordinates already)
    //

    /*const float verticesViewport[][3] = {
        { 0.0f, 0.8f, -1.0f },
        { 0.8f, -0.8f, 0.0f },
        { -0.8f, -0.8f, 0.0f } };*/
    const float verticesViewport[][6] = {
        { -0.6f, 0.8f, 0.0f },
        { 0.6f, -0.8f, 0.0f },
        { -0.6f, -0.8f, 0.0f },
        { 0.6f, 0.8f, 0.0f } };
    
    const float wD2 = (float)buffers->m_width / 2;
    const float hD2 = (float)buffers->m_height / 2;

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
        { vertices[0], { 0, 1, 0, 1 }, { 0.0f, 0.0f } },
        { vertices[1], { 0, 0, 1, 1 }, { 1.0f, 1.0f } },
        { vertices[2], { 1, 0, 0, 1 }, { 0.0f, 1.0f } },
        { vertices[3], { 1, 0, 0, 1 }, { 1.0f, 0.0f } } };

    const int triangles[] = {
        0, 1, 2,
        0, 3, 1,
    };

    POW2_ASSERT(SizeOfArray(triangles) % 3 == 0);

    //
    // Rasterize each triangle
    //

    for (int i = 0; i < SizeOfArray(triangles); i += 3)
    {
        TriangleInput input = {
            vertexData, 
            { g_textureSize, g_textureSize, (uint32_t*)g_texture },
            { triangles[i], triangles[i + 1], triangles[i + 2] } };

        Rasterizer::RasterTriangle(buffers, input);
    }

    DebugTimer_TocAndPrint(__FUNCTION__);
}
