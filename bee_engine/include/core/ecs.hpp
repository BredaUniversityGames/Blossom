#pragma once

#include <code_utils/bee_utils.hpp>
#include <entt/entity/registry.hpp>

namespace bee
{

class EntityComponentSystem
{
public:
    EntityComponentSystem();
    ~EntityComponentSystem();

    NON_COPYABLE(EntityComponentSystem);
    NON_MOVABLE(EntityComponentSystem);

    entt::registry Registry;
    entt::entity CreateEntity() { return Registry.create(); }
    void DeleteEntity(entt::entity e);
    void RemovedDeleted();

    void Clear();

    template <typename T, typename... Args>
    decltype(auto) CreateComponent(entt::entity entity, Args&&... args);

private:

    struct Delete {}; // Tag component for entities to be deleted
};

template <typename T, typename... Args>
decltype(auto) EntityComponentSystem::CreateComponent(entt::entity entity, Args&&... args)
{
    return Registry.emplace<T>(entity, args...);  // TODO: std::move this
}

}  // namespace bee
