#include "Rasterizer.h"

#include "DebugTimer.h"
#include "Log.h"

#include "External/pow2assert.h"

#include <math.h>

//
// CONFIGURATION
//
// Keeping multiple parallel implementations to evaluate relative performance.
//

#define PROFILE 1

enum class ScanConversionMode
{
    FirstApproach
};

static const ScanConversionMode g_scanConversionMode = ScanConversionMode::FirstApproach;

//
// DATA STRUCTURES
//

struct TriangleData
{
    vec3 m_normal;
    vec3 m_interpNormals[3];
    float m_minX;
    float m_maxX;
    float m_minY;
    float m_maxY;
};

struct ScanData
{
    FragmentInput* m_fragmentsIn;
    int m_capacity;
    int m_fragmentsCount;
};

struct Plane
{
    vec3 n;
    float d;
};

//
// HELPER FUNCTIONS
//

static inline uint32_t ColorToBufferColor(const vec4 c)
{
    return 
        ((uint8_t)(c.x * 255) << 16) +
        ((uint8_t)(c.y * 255) << 8) +
        (uint8_t)(c.z * 255) +
        ((uint8_t)(c.w * 255) << 24);
}

static inline vec4 BufferColorToColor(const uint32_t c)
{
    vec4 ret;
    ret.x = (c >> 16 & 0xff) / 255.0f;
    ret.y = (c >> 8 & 0xff) / 255.0f;
    ret.z = (c & 0xff) / 255.0f;
    ret.w = (c >> 24 & 0xff) / 255.0f;
    return ret;
}

//
// PIPELINE FUNCTIONS
//

static void TriangleSetup(TriangleData* triangle, const TriangleInput& input)
{
    // TODO(manuel): check CW / CCW

    vec3 vertices[3];
    for (int i = 0; i < 3; ++i)
    {
        vertices[i] = vec4xyz(input.m_vertexArray[input.m_indices[i]].m_pos);
    }
    
    const vec3 vAB = vec3Normalize(vertices[1] - vertices[0]);
    const vec3 vBC = vec3Normalize(vertices[2] - vertices[1]);
    const vec3 vCA = vec3Normalize(vertices[0] - vertices[2]);
    
    triangle->m_normal = vec3Normalize(vec3Cross(-vCA, vAB));
    
    const vec3 edgeNormals[3] = {
        vec3Normalize(vec3Cross(vBC, triangle->m_normal)),    // BC edge
        vec3Normalize(vec3Cross(vCA, triangle->m_normal)),    // CA edge
        vec3Normalize(vec3Cross(vAB, triangle->m_normal)) };  // AB edge
    const Plane edgePlanes[3] = {
        { edgeNormals[0], vec3Dot(edgeNormals[0], vertices[1]) },
        { edgeNormals[1], vec3Dot(edgeNormals[1], vertices[2]) },
        { edgeNormals[2], vec3Dot(edgeNormals[2], vertices[0]) } };
    const float distToEdges[3] = {
        vec3Dot(vertices[0], edgePlanes[0].n) - edgePlanes[0].d,
        vec3Dot(vertices[1], edgePlanes[1].n) - edgePlanes[1].d,
        vec3Dot(vertices[2], edgePlanes[2].n) - edgePlanes[2].d };
    
    for (int i = 0; i < 3; ++i)
    {
        triangle->m_interpNormals[i] = -edgeNormals[i] / distToEdges[i];
    }
    
    triangle->m_minX = (int)fminf(
        fminf(vertices[0].x, vertices[1].x),
        vertices[2].x);
    triangle->m_maxX = (int)fmaxf(
        fmaxf(vertices[0].x, vertices[1].x),
        vertices[2].x);
    triangle->m_minY = (int)fminf(
        fminf(vertices[0].y, vertices[1].y),
        vertices[2].y);
    triangle->m_maxY = (int)fmaxf(
        fmaxf(vertices[0].y, vertices[1].y),
        vertices[2].y);
}

static void TriangleTraversal(
    ScanData* scan,
    FragmentInput* fragmentsTmpBuffer,
    const TriangleInput& input,
    const TriangleData& triangle)
{
    // Calculate distance from pixel (x, y) to each vertex by using the distance to the
    // opposite plane.

    *scan = {};
    scan->m_fragmentsIn = fragmentsTmpBuffer;
           
    for (int y = triangle.m_minY; y <= triangle.m_maxY; ++y)
    {
        for (int x = triangle.m_minX; x <= triangle.m_maxX; ++x)
        {
            float interp[3];
            bool fragment = true;
            float accum = 0;

            for (int v = 0; v < 3; ++v)
            {
                const VertexData& vertex = input.m_vertexArray[input.m_indices[v]];
                vec3 p = vec3(x - vertex.m_pos.x, y - vertex.m_pos.y, 0);
                interp[v] = 1 - vec3Dot(triangle.m_interpNormals[v], p);
                fragment &= (interp[v] >= 0 && interp[v] <= 1);
                accum += interp[v];
            }

            float finalInterp[3];
            for (int v = 0; v < 3; ++v)
            {
                finalInterp[v] = interp[v] / accum;
            }

            if (fragment)
            {
                // Generate fragment
                scan->m_fragmentsIn[scan->m_fragmentsCount++] = {
                    x,
                    y,
                    { interp[0], interp[1], interp[2] } };
            }
        }
    }
}

static void TriangleShading(
    RasterBuffers* buffers,
    const TriangleInput& input,
    const ScanData& scan)
{
    for (int i = 0; i < scan.m_fragmentsCount; ++i)
    {
        const FragmentInput& fragIn = scan.m_fragmentsIn[i];

        // Calculate colour
        vec4 baseColor = vec4(0, 0, 0, 0);
        vec2 textureCoord = vec2(0, 0);
        for (int v = 0; v < 3; ++v)
        {
            const VertexData& vertex = input.m_vertexArray[input.m_indices[v]];

            float value = fragIn.m_interpValues[v];

            baseColor = baseColor + (vertex.m_color * value);
            
            textureCoord.x += vertex.m_textureCoord.x * value;
            textureCoord.y += vertex.m_textureCoord.y * value;
        }
        
        textureCoord.x = fmaxf(textureCoord.x, 0);
        textureCoord.x = fminf(textureCoord.x, 1);
        textureCoord.y = fmaxf(textureCoord.y, 0);
        textureCoord.y = fminf(textureCoord.y, 1);
        
        // Texturing (clamp)
        int texturePoint[2];
        texturePoint[0] = (int)(textureCoord.x * input.m_texture.m_width);
        texturePoint[1] = (int)(textureCoord.y * input.m_texture.m_height);
        vec4 textureColor = BufferColorToColor(
            input.m_texture.m_data[texturePoint[1] * input.m_texture.m_width + texturePoint[0]]);
        
        // Produce fragment
        vec4 outColor = baseColor * textureColor;
        buffers->m_color[fragIn.m_y * buffers->m_width + fragIn.m_x] = ColorToBufferColor(outColor);
    }
}

//
// EXTERNAL FUNCTIONS
//

void Rasterizer::RasterTriangle(RasterBuffers* buffers, const TriangleInput& input)
{
    POW2_ASSERT(buffers);

    POW2_ASSERT(ColorToBufferColor(BufferColorToColor(0xafbfcfdf)) == 0xafbfcfdf);

    TriangleData triangleData;
    ScanData scanData;

#if PROFILE
    double profileSetupTimeMs;
    double profileTraversalTimeMs;
    double profileShadingTimeMs;
    DebugTimer_Tic("TriangleSetup");
#endif

    TriangleSetup(&triangleData, input);
    
#if PROFILE
    profileSetupTimeMs = DebugTimer_Toc("TriangleSetup");
    DebugTimer_Tic("TriangleTraversal");
#endif
    
    TriangleTraversal(&scanData, buffers->m_fragmentsTmpBuffer, input, triangleData);

#if PROFILE
    profileTraversalTimeMs = DebugTimer_Toc("TriangleTraversal");
    DebugTimer_Tic("TriangleShading");
#endif
    
    TriangleShading(buffers, input, scanData);
    
#if PROFILE
    profileShadingTimeMs = DebugTimer_Toc("TriangleShading");
    double totalTimeMs = profileSetupTimeMs + profileTraversalTimeMs + profileShadingTimeMs;
    int testedPixelsCount = 
        (triangleData.m_maxX - triangleData.m_minX) * (triangleData.m_maxY - triangleData.m_minY);
    int generatedFragmentsCount = scanData.m_fragmentsCount;
    float ratioTestedPixelsToFragments = generatedFragmentsCount / (float)testedPixelsCount;
    double timePerTestedPixelNs = 1000000 * profileTraversalTimeMs / (double)testedPixelsCount;
    double timePerGeneratedFragmentNs =
        1000000 * profileTraversalTimeMs / (double)generatedFragmentsCount;
    Log::Debug("RASTER PERFORMANCE DATA:");
    Log::Debug("\tTotal time: %01fms", totalTimeMs);
    Log::Debug("\tSetup time: %01fms", profileSetupTimeMs);
    Log::Debug("\tTraversal time: %01fms", profileTraversalTimeMs);
    Log::Debug("\tShading time: %01fms", profileShadingTimeMs);
    Log::Debug("\tTested pixels count: %d", testedPixelsCount);
    Log::Debug("\tGenerated fragments count: %d", generatedFragmentsCount);
    Log::Debug("\tRatio tested pixels to fragments: %.02f", ratioTestedPixelsToFragments);
    Log::Debug("\tTime per tested pixel: %.0fns", timePerTestedPixelNs);
    Log::Debug("\tTime per generated fragment: %.0fns", timePerGeneratedFragmentNs);
#endif
}
