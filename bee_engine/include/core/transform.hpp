#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entity/entity.hpp>
#include <visit_struct/visit_struct.hpp>

namespace bee
{
/// <summary>
/// Transform component. Contains the position, rotation and scale of the entity.
/// Implemented on top of the entity-component-system (entt).
/// </summary>
struct Transform
{
    glm::vec3 GetTranslation() const { return Translation; }
    glm::vec3 GetScale() const { return Scale; }
    glm::quat GetRotation() const { return Rotation; }

    void SetTranslation(const glm::vec3& value) { Translation = value; MarkDirty(); }
    void SetScale(const glm::vec3& value) { Scale = value; MarkDirty(); }
    void SetRotation(const glm::quat& value) { Rotation = value; MarkDirty(); }

    /// <summary>
    /// The name of the entity. Used for debugging and editor purposes.
    /// If small, it can benefit from the small string optimization.
    /// </summary>
    std::string Name = {};

    /// <summary>
    /// Creates a Transform with default translation (0,0,0), scale (1,1,1), and rotation (identity quaternion).
    /// </summary>
    Transform() {}

    /// <summary>
    /// Creates a Transform with the given translation, scale, and rotation.
    /// </summary>
    Transform(const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation)
        : Translation(translation), Scale(scale), Rotation(rotation)
    {
    }

    /// <summary>
    /// Sets the parent of the entity. Will automatically add the entity to
    /// the parent's children.
    /// </summary>
    /// <param name="parent">The parent entity.</param>
    void SetParent(entt::entity parent);

    // Warning: this does not delete the children, it merely removes them from the hierarchy
    void DetachChildren(entt::registry& registry);

    void MarkDirty() const;

    /// <summary>
    /// Traverses the hierarchy to get the matrix that transforms from local space to world space.
    /// </summary>
    [[nodiscard]] glm::mat4 CalcWorld() const;

    /// <summary>
    /// The matrix that transforms from local space to world space.
    /// Uses a cached value if the transform has not changed.
    /// </summary>
    [[nodiscard]] const glm::mat4& World() const
    {
        if (m_dirty)
        {
            m_world = CalcWorld();
            m_dirty = false;
        }
        return m_world;
    }

    /// <summary>True if the entity has a children.</summary>
    [[nodiscard]] bool HasChildren() const { return m_first != entt::null; }

    /// <summary>True if the entity has a parent.</summary>
    [[nodiscard]] bool HasParent() const { return m_parent != entt::null; }

    /// <summary>True if the entity has a parent.</summary>
    [[nodiscard]] entt::entity Parent() const { return m_parent; }

    /// <summary>Subscribe to the events from the ECS. Call this from the engine init.</summary>
    static void SubscribeToEvents();

    /// <summary>Un-subscribe to the events from the ECS. Call this from the engine shutdown.</summary>
    static void UnsubscribeToEvents();


    /// <summary>
    /// Translation in local space.
    /// </summary>
    glm::vec3 Translation = glm::vec3(0.0f, 0.0f, 0.0f);

    /// <summary>
    /// Scale in local space.
    /// </summary>
    glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

    /// <summary>
    /// Rotation in local space.
    /// </summary>
    glm::quat Rotation = glm::identity<glm::quat>();

private:
    // The hierarchy is implemented as a linked list.
    entt::entity m_parent{entt::null};
    entt::entity m_first{entt::null};
    entt::entity m_next{entt::null};

    mutable glm::mat4 m_world{ glm::identity<glm::mat4>() };

    mutable bool m_dirty{true};

    // Add a child to the entity. Called by SetParent.
    void AddChild(entt::entity child);

    static void OnTransformCreate(entt::registry& registry, entt::entity entity);
    static void OnTransformDestroy(entt::registry& registry, entt::entity entity);
    static void OnTransformUpdate(entt::registry& registry, entt::entity entity);

public:
    /// <summary>
    /// Iterator for the children of the entity.
    /// </summary>
    class Iterator
    {
    public:
        Iterator() = default;
        Iterator(entt::entity ent);
        Iterator& operator++();
        bool operator!=(const Iterator& iterator);
        entt::entity operator*();

    private:
        entt::entity m_current{entt::null};
    };

    /// <summary>
    /// The begin iterator for the children of the entity.
    /// </summary>
    Iterator begin() { return Iterator(m_first); }

    /// <summary>
    /// The end iterator for the children of the entity.
    /// </summary>
    Iterator end() { return Iterator(); }
};

/// Decomposes a transform matrix into its translation, scale and rotation components
void Decompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& scale, glm::quat& rotation);
 
}  // namespace bee

VISITABLE_STRUCT(bee::Transform, Name, Translation, Scale, Rotation);
