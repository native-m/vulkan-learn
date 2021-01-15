#include <framework/ShapeGen.h>

namespace frm
{
    size_t ShapeGen::makeTriangle(float scale, VertexPos*& verts)
    {
        verts = new VertexPos[3];

        verts[0].pos = glm::vec3(scale, scale, 1.0f);
        verts[1].pos = glm::vec3(-scale, scale, 1.0f);
        verts[2].pos = glm::vec3(0.0f, -scale, 1.0f);

        return 3;
    }

    size_t ShapeGen::makeColorTriangle(float scale, VertexPosCol*& verts)
    {
        verts = new VertexPosCol[3];

        verts[0].pos = glm::vec3(scale, scale, 1.0f);
        verts[0].col = glm::vec3(1.0f, 0.0f, 0.0f);

        verts[1].pos = glm::vec3(-scale, scale, 1.0f);
        verts[1].col = glm::vec3(0.0f, 1.0f, 0.0f);

        verts[2].pos = glm::vec3(0.0f, -scale, 1.0f);
        verts[2].col = glm::vec3(0.0f, 0.0f, 1.0f);

        return 3;
    }

    size_t ShapeGen::makeColorPlane(float scale, uint32_t*& indices, VertexPosCol*& verts, size_t& numIndices)
    {
        indices = new uint32_t[6];
        verts = new VertexPosCol[4];
        numIndices = 6;

        // Build vertices
        verts[0].pos = glm::vec3(scale, scale, 0.0f);
        verts[0].col = glm::vec3(1.0f, 0.0f, 0.0f);

        verts[1].pos = glm::vec3(-scale, scale, 0.0f);
        verts[1].col = glm::vec3(0.0f, 1.0f, 0.0f);

        verts[2].pos = glm::vec3(-scale, -scale, 0.0f);
        verts[2].col = glm::vec3(0.0f, 0.0f, 1.0f);

        verts[3].pos = glm::vec3(scale, -scale, 0.0f);
        verts[3].col = glm::vec3(1.0f, 0.0f, 1.0f);

        // Build indices
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 2;
        indices[4] = 3;
        indices[5] = 0;

        return 4;
    }

    size_t ShapeGen::makePlane(float scale, uint32_t*& indices, VertexPosTex*& verts, size_t& numIndices)
    {
        indices = new uint32_t[6];
        verts = new VertexPosTex[4];
        numIndices = 6;

        // Build vertices
        verts[0].pos = glm::vec3(scale, scale, 0.0f);
        verts[0].uv = glm::vec2(1.0f, 1.0f);

        verts[1].pos = glm::vec3(-scale, scale, 0.0f);
        verts[1].uv = glm::vec2(0.0f, 1.0f);

        verts[2].pos = glm::vec3(-scale, -scale, 0.0f);
        verts[2].uv = glm::vec2(0.0f, 0.0f);

        verts[3].pos = glm::vec3(scale, -scale, 0.0f);
        verts[3].uv = glm::vec2(1.0f, 0.0f);

        // Build indices
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 2;
        indices[4] = 3;
        indices[5] = 0;

        return 4;
    }
}
