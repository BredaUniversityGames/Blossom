#include <precompiled/engine_precompiled.hpp>
#include "rendering/debug_render.hpp"
#include <glm/gtc/constants.hpp>

void bee::DebugRenderer::AddLine(DebugCategory::Enum category, const glm::vec2& from, const glm::vec2& to, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    AddLine(category, glm::vec3(from.x, from.y, 0.0f), glm::vec3(to.x, to.y, 0.0f), color);
}

void bee::DebugRenderer::AddCircle(DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 32.0f;
    float t = 0.0f;

    glm::vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
    for (; t < glm::two_pi<float>(); t += dt)
    {
        glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
}

void bee::DebugRenderer::AddCircle(DebugCategory::Enum category, const glm::vec3& center, const glm::quat& rotation, float radius, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 32.0f;
    float t = 0.0f;

    glm::mat3 rot = glm::mat3_cast(rotation);

    glm::vec3 v0(radius * cos(t), radius * sin(t), 0.0f);
    for (; t < glm::two_pi<float>(); t += dt)
    {
        glm::vec3 v1(radius * cos(t + dt), radius * sin(t + dt), 0.0f);
        AddLine(category, (rot * v0) + center, (rot * v1) + center, color);
        v0 = v1;
    }
}

void bee::DebugRenderer::AddSphere(DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 64.0f;
    float t = 0.0f;

    glm::vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
        glm::vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
}

void bee::DebugRenderer::AddSquare(DebugCategory::Enum category, const glm::vec3& center, float size, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    const float s = size * 0.5f;
    auto A = center + glm::vec3(-s, -s, 0.0f);
    auto B = center + glm::vec3(-s, s, 0.0f);
    auto C = center + glm::vec3(s, s, 0.0f);
    auto D = center + glm::vec3(s, -s, 0.0f);

    AddLine(category, A, B, color);
    AddLine(category, B, C, color);
    AddLine(category, C, D, color);
    AddLine(category, D, A, color);
}

void bee::DebugRenderer::AddBounds(DebugCategory::Enum category, const glm::vec3& center, const glm::vec3& size,
    const glm::vec4& color)
{
    glm::vec3 halfSize = { size.x / 2.0f, size.y / 2.0f, size.z / 2.0f };

    // Define the 8 corners of the AABB
    std::array<glm::vec3, 8> corners = {
        glm::vec3{center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        glm::vec3{center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        glm::vec3{center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z},
        glm::vec3{center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z},
        glm::vec3{center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z},
        glm::vec3{center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z},
        glm::vec3{center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z},
        glm::vec3{center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
    };

    // Draw the 12 edges of the AABB
    // Bottom face
    AddLine(category, corners[0], corners[1], color);
    AddLine(category, corners[1], corners[2], color);
    AddLine(category, corners[2], corners[3], color);
    AddLine(category, corners[3], corners[0], color);

    // Top face
    AddLine(category, corners[4], corners[5], color);
    AddLine(category, corners[5], corners[6], color);
    AddLine(category, corners[6], corners[7], color);
    AddLine(category, corners[7], corners[4], color);

    // Vertical edges
    AddLine(category, corners[0], corners[4], color);
    AddLine(category, corners[1], corners[5], color);
    AddLine(category, corners[2], corners[6], color);
    AddLine(category, corners[3], corners[7], color);
}
