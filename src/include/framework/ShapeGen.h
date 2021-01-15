#pragma once

#include <framework/VertexAttributes.h>

namespace frm
{
    struct ShapeGen
    {
        static size_t makeTriangle(float scale, VertexPos*& verts);
        static size_t makeColorTriangle(float scale, VertexPosCol*& verts);
        static size_t makeColorPlane(float scale, uint32_t*& indices, VertexPosCol*& verts, size_t& numIndices);
        static size_t makePlane(float scale, uint32_t*& indices, VertexPosTex*& verts, size_t& numIndices);
    };
}
