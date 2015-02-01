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

struct Plane
{
    vec3 n;
    float d;
};

//
// INTERNAL FUNCTIONS
//

static inline uint32_t CalcBufferColor(const uint8_t c[4])
{
    return (c[0] << 16) + (c[1] << 8) + c[2];
}

//
// PUBLIC FUNCTIONS
//

void RasterizerUtils::TriangleSetup(TriangleData* triangle, const TriangleInput& input)
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

void RasterizerUtils::TriangleTraversal(
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

void RasterizerUtils::TriangleShading(
    RasterBuffers* buffers,
    const TriangleInput& input,
    const ScanData& scan)
{
    for (int i = 0; i < scan.m_fragmentsCount; ++i)
    {
        const FragmentInput& fragIn = scan.m_fragmentsIn[i];

        uint8_t color[4] = {};
        for (int v = 0; v < 3; ++v)
        {
            const VertexData& vertex = input.m_vertexArray[input.m_indices[v]];

            float value = fragIn.m_interpValues[v];
            color[0] += vertex.m_color[0] * value;
            color[1] += vertex.m_color[1] * value;
            color[2] += vertex.m_color[2] * value;
            color[3] += vertex.m_color[3] * value;
        }

        buffers->m_color[fragIn.m_y * buffers->m_width + fragIn.m_x] = CalcBufferColor(color);
    }
}

void Rasterizer::RasterTriangle(RasterBuffers* buffers, const TriangleInput& input)
{
    POW2_ASSERT(buffers);

    TriangleData triangleData;
    ScanData scanData;

#if PROFILE
    double profileSetupTimeMs;
    double profileTraversalTimeMs;
    double profileShadingTimeMs;
    DebugTimer_Tic("TriangleSetup");
#endif

    RasterizerUtils::TriangleSetup(&triangleData, input);
    
#if PROFILE
    profileSetupTimeMs = DebugTimer_Toc("TriangleSetup");
    DebugTimer_Tic("TriangleTraversal");
#endif
    
    RasterizerUtils::TriangleTraversal(
        &scanData, buffers->m_fragmentsTmpBuffer, input, triangleData);

#if PROFILE
    profileTraversalTimeMs = DebugTimer_Toc("TriangleTraversal");
    DebugTimer_Tic("TriangleShading");
#endif
    
    RasterizerUtils::TriangleShading(buffers, input, scanData);
    
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
