#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace bee
{

/// <summary>
/// A set of categories to turn on/off debug rendering.
/// </summary>
struct DebugCategory
{
    enum Enum
    {
        General         = 1 << 0,
        Gameplay        = 1 << 1,
        Physics         = 1 << 2,
        Sound           = 1 << 3,
        Rendering       = 1 << 4,
        AINavigation    = 1 << 5,
        AIDecision      = 1 << 6,
        Editor          = 1 << 7,
        AccelStructs    = 1 << 8,
        All             = 0xFFFFFFFF
    };
};

/// <summary>
/// Renders debug lines and shapes. It can be called from any place in the code.
/// </summary>
class DebugRenderer
{
public:
    DebugRenderer();
    ~DebugRenderer();

    /// <summary>
    /// Initialize the debug renderer.
    /// </summary>
    void Render();

    /// <summary>
    /// Add a line to be rendered.
    /// </summary>
    void AddLine(
        DebugCategory::Enum category,
        const glm::vec3& from,
        const glm::vec3& to,
        const glm::vec4& color);    // Implemented per platform.

    /// <summary>
    /// Add a line to be rendered. It will assume z = 0.
    /// </summary>
    void AddLine(
        DebugCategory::Enum category,
        const glm::vec2& from,
        const glm::vec2& to,
        const glm::vec4& color);    // Implemented for all platforms.

    /// <summary>
    /// Add a circle to be rendered. It will assume z = center.z
    /// </summary>
    void AddCircle(
        DebugCategory::Enum category,
        const glm::vec3& center,
        float radius,
        const glm::vec4& color);    // Implemented for all platforms.

    /// <summary>
   /// Add a circle to be rendered, using the rotation to determine the axis. It will assume z = center.z
   /// </summary>
    void AddCircle(
        DebugCategory::Enum category,
        const glm::vec3& center,
        const glm::quat& rotation,
        float radius,
        const glm::vec4& color);

    /// <summary>
    /// Add a sphere to be rendered.
    /// </summary>
    void AddSphere(
        DebugCategory::Enum category,
        const glm::vec3& center,
        float radius,
        const glm::vec4& color);    // Implemented for all platforms.

    /// <summary>
    /// Add a square to be rendered. It will assume z = center.z
    /// </summary>
    void AddSquare(
        DebugCategory::Enum category,
        const glm::vec3& center,
        float size,
        const glm::vec4& color);	// Implemented for all platforms.

    void AddBounds(
        DebugCategory::Enum category,
        const glm::vec3& center,
        const glm::vec3& size,
        const glm::vec4& color
    );

    /// <summary>
    /// Turn on/off the debug renderer per category.
    /// </summary>
    void SetCategoryFlags(unsigned int flags) { m_categoryFlags = flags; }

    /// <summary>
    /// Get the current category flags.
    /// </summary>
    unsigned int GetCategoryFlags() const { return m_categoryFlags; }

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    unsigned int m_categoryFlags{};
};


namespace Colors
{

inline glm::vec4 Black = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
inline glm::vec4 White = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
inline glm::vec4 Grey = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
inline glm::vec4 Red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
inline glm::vec4 Green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
inline glm::vec4 Blue = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
inline glm::vec4 Orange = glm::vec4(1.0f, 0.66f, 0.0f, 1.0f);
inline glm::vec4 Cyan = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
inline glm::vec4 Magenta = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
inline glm::vec4 Yellow = glm::vec4(1.0f, 1.0, 0.0f, 1.0f);
inline glm::vec4 Purple = glm::vec4(0.55f, 0.0, 0.65f, 1.0f);

}  // namespace colors

}  // namespace bee
