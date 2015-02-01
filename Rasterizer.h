#pragma once

#include "Geometry.h"

#include <stddef.h>
#include <stdint.h>

// Input: triangles
// Output: buffers

struct RasterBuffers
{
	uint32_t* m_color;
	//uint32_t* m_depth;
	size_t m_width;
	size_t m_height;
	size_t m_colorTotalBytes;
	size_t m_colorPixelBytes;
};

// TODO: Better by const reference or value?
void RasterizeTriangle(const VertexData* vertices,
					   const size_t verticesLen,
					   const int indices[3],
					   RasterBuffers* buffers);
