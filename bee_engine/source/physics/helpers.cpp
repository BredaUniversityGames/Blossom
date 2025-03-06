#include <precompiled/engine_precompiled.hpp>
#include "physics/helpers.hpp"

glm::vec3 JoltToGlm(JPH::Vec3 in)
{
    return glm::vec3(in.GetX(), in.GetY(), in.GetZ());
}

glm::vec4 JoltToGlm(JPH::Vec4 in)
{
    return glm::vec4(in.GetX(), in.GetY(), in.GetZ(), in.GetW());
}

glm::vec4 JoltToGlm(JPH::Color in)
{
    return glm::vec4(in.r, in.g, in.b, in.a);
}

JPH::Vec3 GlmToJolt(glm::vec3 in)
{
    return JPH::Vec3(in.x, in.y, in.z);
}

JPH::Vec4 GlmToJolt(glm::vec4 in)
{
    return JPH::Vec4(in.x, in.y, in.z, in.w);
}

JPH::Quat GlmToJolt(glm::quat in)
{
    return JPH::Quat(in.x, in.y, in.z, in.w);
}