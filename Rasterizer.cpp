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

struct RasterPerfData
{
#ifdef PROFILE
	double m_totalTimeMs;
	double m_setupTimeMs;
	double m_scanTimeMs;
	int m_testedPixelsCount;
	int m_generatedFragmentsCount;
#endif
};

//
// INTERNAL FUNCTIONS
//

static inline uint32_t CalcBufferColor(const uint8_t c[4])
{
	return (c[0] << 16) + (c[1] << 8) + c[2];
}

static void PrintRasterPerfData(const RasterPerfData& data)
{
#ifdef PROFILE
	float ratioTestedPixelsToFragments =
		data.m_generatedFragmentsCount / (float)data.m_testedPixelsCount;
	double timePerTestedPixelNs =
		1000000 * data.m_scanTimeMs / (double)data.m_testedPixelsCount;
	double timePerGeneratedFragmentNs =
		1000000 * data.m_scanTimeMs / (double)data.m_generatedFragmentsCount;

	Log::Debug("RASTER PERFORMANCE DATA:");
	Log::Debug("\tTotal time: %01fms", data.m_totalTimeMs);
	Log::Debug("\tSetup time: %01fms", data.m_setupTimeMs);
	Log::Debug("\tScan time: %01fms", data.m_scanTimeMs);
	Log::Debug("\tTested pixels count: %d", data.m_testedPixelsCount);
	Log::Debug("\tGenerated fragments count: %d", data.m_generatedFragmentsCount);
	Log::Debug("\tRatio tested pixels to fragments: %.02f", ratioTestedPixelsToFragments);
	Log::Debug("\tTime per tested pixel: %.0fns", timePerTestedPixelNs);
	Log::Debug("\tTime per generated fragment: %.0fns", timePerGeneratedFragmentNs);
#endif
}

static void RasterizeTriangleFirstApproach(const VertexData* vertices,
									const size_t verticesLen,
									const int indices[3],
									RasterBuffers* buffers,
									RasterPerfData* perfData)
{
#ifdef PROFILE
	DebugTimer_Tic(__FUNCTION__);
#endif

	//
	// TRIANGLE SETUP
	//

	// TODO(manuel): check CW / CCW
	// TODO(manuel): precalculate

#ifdef PROFILE
	DebugTimer_Tic("TriangleSetup");
#endif

	const vec3 positions[3] = {
		vec4xyz(vertices[indices[0]].m_pos),
		vec4xyz(vertices[indices[1]].m_pos),
		vec4xyz(vertices[indices[2]].m_pos) };
	const vec3 vAB = vec3Normalize(positions[1] - positions[0]);
	const vec3 vBC = vec3Normalize(positions[2] - positions[1]);
	const vec3 vCA = vec3Normalize(positions[0] - positions[2]);
	const vec3 triangleNormal = vec3Normalize(vec3Cross(-vCA, vAB));
	const vec3 edgeNormals[3] = {
		vec3Normalize(vec3Cross(vBC, triangleNormal)),  // BC edge
		vec3Normalize(vec3Cross(vCA, triangleNormal)),  // CA edge
		vec3Normalize(vec3Cross(vAB, triangleNormal)) };  // AB edge
	const Plane edgePlanes[3] = {
		{ edgeNormals[0], vec3Dot(edgeNormals[0], positions[1]) },
		{ edgeNormals[1], vec3Dot(edgeNormals[1], positions[2]) },
		{ edgeNormals[2], vec3Dot(edgeNormals[2], positions[0]) } };
	const float distToEdges[3] = {
		vec3Dot(positions[0], edgePlanes[0].n) - edgePlanes[0].d,
		vec3Dot(positions[1], edgePlanes[1].n) - edgePlanes[1].d,
		vec3Dot(positions[2], edgePlanes[2].n) - edgePlanes[2].d };
	const vec3 interpolationNormals[3] = {
		-edgeNormals[0] / distToEdges[0],
		-edgeNormals[1] / distToEdges[1],
		-edgeNormals[2] / distToEdges[2] };

	const int minX = (int)fminf(fminf(positions[0].x, positions[1].x), positions[2].x);
	const int maxX = (int)fmaxf(fmaxf(positions[0].x, positions[1].x), positions[2].x);
	const int minY = (int)fminf(fminf(positions[0].y, positions[1].y), positions[2].y);
	const int maxY = (int)fmaxf(fmaxf(positions[0].y, positions[1].y), positions[2].y);

#ifdef PROFILE
	perfData->m_setupTimeMs = DebugTimer_Toc("TriangleSetup");
	perfData->m_testedPixelsCount = (maxX - minX) * (maxY - minY);
	perfData->m_generatedFragmentsCount = 0;
#endif

	//
	// SCAN CONVERSION
	//

#ifdef PROFILE
	DebugTimer_Tic("Scan");
#endif

	for (int y = minY; y <= maxY; ++y)
	{
		for (int x = minX; x <= maxX; ++x)
		{
			// Calculate distance from pixel (x, y) to each vertex by using the distance to the
			// opposite plane.

			float interp[3];
			bool fragment = true;
			float accum = 0;

			for (int v = 0; v < 3; ++v)
			{
				vec3 p = vec3(x - positions[v].x, y - positions[v].y, 0);
				interp[v] = 1 - vec3Dot(interpolationNormals[v], p);
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
#ifdef PROFILE
				++perfData->m_generatedFragmentsCount;
#endif
				uint8_t color[4] = {};
				for (int v = 0; v < 3; ++v)
				{
					color[0] += vertices[v].m_color[0] * finalInterp[v];
					color[1] += vertices[v].m_color[1] * finalInterp[v];
					color[2] += vertices[v].m_color[2] * finalInterp[v];
					color[3] += vertices[v].m_color[3] * finalInterp[v];
				}
				buffers->m_color[y * buffers->m_width + x] = CalcBufferColor(color);
			}
		}
	}

#ifdef PROFILE
	perfData->m_scanTimeMs = DebugTimer_Toc("Scan");
	perfData->m_totalTimeMs = DebugTimer_Toc(__FUNCTION__);
#endif
}

//
// PUBLIC FUNCTIONS
//

void RasterizeTriangle(const VertexData* vertices,
					   const size_t verticesLen,
					   const int indices[3],
					   RasterBuffers* buffers)
{
	POW2_ASSERT(buffers);
	
	RasterPerfData perfData;

	if (g_scanConversionMode == ScanConversionMode::FirstApproach)
	{
		RasterizeTriangleFirstApproach(vertices, verticesLen, indices, buffers, &perfData);
	}
	else
	{
		POW2_ASSERT(false);
	}

#if PROFILE
	PrintRasterPerfData(perfData);
#endif
}
