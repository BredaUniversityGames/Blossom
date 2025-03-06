#pragma once

#include <jolt/Jolt.h>
#include <jolt/Core/Color.h>
#include <glm/glm.hpp>

glm::vec3 JoltToGlm(JPH::Vec3 in);
glm::vec4 JoltToGlm(JPH::Vec4 in);
glm::vec4 JoltToGlm(JPH::Color in);

JPH::Vec3 GlmToJolt(glm::vec3 in);
JPH::Vec4 GlmToJolt(glm::vec4 in);
JPH::Quat GlmToJolt(glm::quat in);