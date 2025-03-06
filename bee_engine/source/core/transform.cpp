#include <precompiled/engine_precompiled.hpp>
#include "core/transform.hpp"

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>

using namespace bee;
using namespace glm;

void Transform::SetParent(entt::entity parent)
{
    if(parent == entt::null)
    {
        m_parent = entt::null;
        return;
    }

    BEE_ASSERT(Engine.ECS().Registry.valid(parent));
    auto& parentTransform = Engine.ECS().Registry.get<Transform>(parent);
    // We can always get the entity from the transform.
    const entt::entity entity = to_entity(Engine.ECS().Registry, *this);
    parentTransform.AddChild(entity);
    m_parent = parent;
}

void bee::Transform::DetachChildren(entt::registry& registry)
{
    for (auto child : *this) {
        auto& transform = registry.get<Transform>(child);
        transform.m_parent = entt::null;
    }

    m_first = entt::null;
}

void Transform::MarkDirty() const
{
    m_dirty = true;

    if (m_first != entt::null)
    {
        auto itr = m_first;
        while (true)
        {
            auto& t = Engine.ECS().Registry.get<Transform>(itr);

            t.MarkDirty();

            if (t.m_next == entt::null)
            {
                break;
            }

            itr = t.m_next;
        }
    }
}

void Transform::AddChild(entt::entity child)
{
    BEE_ASSERT(Engine.ECS().Registry.valid(child));
    if (m_first == entt::null)
    {
        m_first = child;
    }
    else
    {
        auto itr = m_first;
        while (true)
        {
            auto& t = Engine.ECS().Registry.get<Transform>(itr);
            if (t.m_next == entt::null)
            {
                t.m_next = child;
                break;
            }
            itr = t.m_next;
        }
    }
}

void Transform::OnTransformCreate(entt::registry& registry, entt::entity entity)
{
}

void Transform::OnTransformDestroy(entt::registry& registry, entt::entity entity)
{
    // Delete all children of the entity.
    if(registry.valid(entity))
	{	
        auto& transform = registry.get<Transform>(entity);

        if (registry.valid(transform.m_parent)) {

            //Adjust parent linked list
            Transform* transform_adjust = nullptr;
            for (entt::entity target = registry.get<Transform>(transform.m_parent).m_first; target != entity;) {
                transform_adjust = registry.try_get<Transform>(target);
                target = transform_adjust->m_next;
            }

            if (transform_adjust) transform_adjust->m_next = transform.m_next;

        }

        //Delete children
        for (auto child : transform)
            if (registry.valid(child))
                Engine.ECS().DeleteEntity(child);
	}
}

void Transform::OnTransformUpdate(entt::registry& registry, entt::entity entity)
{
    auto& transform = registry.get<Transform>(entity);
    transform.MarkDirty();
}

void bee::Decompose(const mat4& transform, vec3& translation, vec3& scale, quat& rotation)
{
    auto m44 = transform;
    translation.x = m44[3][0];
    translation.y = m44[3][1];
    translation.z = m44[3][2];

    scale.x = length(vec3(m44[0][0], m44[0][1], m44[0][2]));
    scale.y = length(vec3(m44[1][0], m44[1][1], m44[1][2]));
    scale.z = length(vec3(m44[2][0], m44[2][1], m44[2][2]));

    mat4 myrot(m44[0][0] / scale.x, m44[0][1] / scale.x, m44[0][2] / scale.x, 0, m44[1][0] / scale.y, m44[1][1] / scale.y,
               m44[1][2] / scale.y, 0, m44[2][0] / scale.z, m44[2][1] / scale.z, m44[2][2] / scale.z, 0, 0, 0, 0, 1);
    rotation = quat_cast(myrot);
}

// Iterator implementation
Transform::Iterator::Iterator(entt::entity ent) : m_current(ent) {}

// Iterator implementation
Transform::Iterator& Transform::Iterator::operator++()
{
    BEE_ASSERT(Engine.ECS().Registry.valid(m_current));
    auto& t = Engine.ECS().Registry.get<Transform>(m_current);
    m_current = t.m_next;
    return *this;
}

// Iterator implementation
bool Transform::Iterator::operator!=(const Iterator& iterator) { return m_current != iterator.m_current; }

// Iterator implementation
entt::entity Transform::Iterator::operator*() { return m_current; }

// Transform implementation
glm::mat4 Transform::CalcWorld() const
{
    const auto translation = glm::translate(glm::mat4(1.0f), Translation);
    const auto rotation = glm::mat4_cast(Rotation);
    const auto scale = glm::scale(glm::mat4(1.0f), Scale);
    if (m_parent == entt::null) return translation * rotation * scale;
    BEE_ASSERT(Engine.ECS().Registry.valid(m_parent));
    const auto& parent = Engine.ECS().Registry.get<Transform>(m_parent);
    return parent.World() * translation * rotation * scale;
}

void bee::Transform::SubscribeToEvents()
{
    // We subscribe to the creation and destruction of the Transform component.
    Engine.ECS().Registry.on_construct<Transform>().connect<&Transform::OnTransformCreate>();
    Engine.ECS().Registry.on_destroy<Transform>().connect<&Transform::OnTransformDestroy>();
    Engine.ECS().Registry.on_update<Transform>().connect<&Transform::OnTransformUpdate>();
}

void bee::Transform::UnsubscribeToEvents()
{
    // Un-subscribe to the events of the ECS.
    Engine.ECS().Registry.on_construct<Transform>().disconnect<&Transform::OnTransformCreate>();
    Engine.ECS().Registry.on_destroy<Transform>().disconnect<&Transform::OnTransformDestroy>();
    Engine.ECS().Registry.on_update<Transform>().disconnect<&Transform::OnTransformUpdate>();
}
