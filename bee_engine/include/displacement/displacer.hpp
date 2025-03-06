#pragma once
#include <glm/vec3.hpp>
#include <visit_struct/visit_struct.hpp>

namespace bee
{

struct Displacer
{
    float radius{2.0f};
};

struct DisplacerFocus
{
    float influenceSize{32.0f};
    glm::vec3 previousPosition{ 0.0f };
};

}

VISITABLE_STRUCT(bee::Displacer, radius);
VISITABLE_STRUCT(bee::DisplacerFocus, influenceSize, previousPosition);
