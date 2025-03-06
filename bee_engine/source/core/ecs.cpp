#include <precompiled/engine_precompiled.hpp>
#include "core/ecs.hpp"
#include <core/transform.hpp>

using namespace bee;
using namespace std;
using namespace entt;

constexpr float kMaxDeltaTime = 1.0f / 30.0f;

EntityComponentSystem::EntityComponentSystem() = default;

bee::EntityComponentSystem::~EntityComponentSystem() = default;

void EntityComponentSystem::DeleteEntity(entt::entity e)
{
    BEE_ASSERT(Registry.valid(e));
    Registry.emplace_or_replace<Delete>(e);
}

void EntityComponentSystem::RemovedDeleted()
{
    bool isDeleteQueueEmpty;
    do
    {
        // Deleting entities can cause other entities to be deleted,
        // so we need to do this in a loop
        const auto del = Registry.view<Delete>();
        isDeleteQueueEmpty = del.empty();

        if (!isDeleteQueueEmpty) Registry.destroy(del.begin(), del.end());

    } while (!isDeleteQueueEmpty);
}

void bee::EntityComponentSystem::Clear()
{
    // We are destroying all the entities anyway, 
    // there is no point in properly adjusting the hierarhcies

    Transform::UnsubscribeToEvents();
    Registry.clear();
    Transform::SubscribeToEvents();
}
