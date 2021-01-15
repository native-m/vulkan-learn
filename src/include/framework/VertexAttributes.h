#pragma once

#include <framework/Common.h>

namespace frm
{
    struct VertexPos
    {
        glm::vec3 pos;
    };

    struct VertexPosCol
    {
        glm::vec3 pos;
        glm::vec3 col;
    };

    struct VertexPosNorm
    {

    };

    struct VertexPosTex
    {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    struct VertexPosTexNorm
    {

    };
}
