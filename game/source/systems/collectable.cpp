#include "precompiled/game_precompiled.hpp"
#include "systems/collectable.hpp"

#include "core/engine.hpp"
#include "core/ecs.hpp"
#include "core/transform.hpp"
#include "systems/player.hpp"
#include <systems/basic_particle_system.hpp>
#include <core/audio.hpp>

#include <jolt/Jolt.h>
#include <jolt/Physics/Collision/Shape/SphereShape.h>
#include <jolt/Physics/Collision/CollideShape.h>
#include "systems/collisions.hpp"
#include "physics/physics_system.hpp"
#include "physics/debug_renderer.hpp"
#include "physics/helpers.hpp"
#include <string>

void bee::CollectItems()
{
	auto collectables = Engine.ECS().Registry.view<Collectable>();
	auto players = Engine.ECS().Registry.view<Transform, Player>();

	for (auto&& [playerEntity, transform, player] : players.each())
	{
        if (player.currentScore >= player.scoreCap) continue; //No collecting needed if player is full

		JPH::SphereShape sphere = JPH::SphereShape(player.playerCollectionRadius);
        JPH::Mat44 Jtransform = JPH::Mat44::sIdentity();
        Jtransform.SetTranslation(GlmToJolt(transform.GetTranslation()));

        JPH::CollideShapeSettings collideSettings;

        CollectableCollisionCollector collector;

        Engine.PhysicsSystem()
            .GetNarrowPhaseQuery()
            .CollideShape(&sphere, JPH::Vec3Arg(1.0f, 1.0f, 1.0f), Jtransform, collideSettings, JPH::RVec3Arg(0.0f, 0.0f, 0.0f), collector);

#if defined(JPH_DEBUG_RENDERER) && defined(BEE_EDITOR)

        JPH::Color color = JPH::Color(0.0f, 0.0f, 255.0f, 255.0f);

        JPH::DebugRenderer::sInstance->DrawSphere(Jtransform.GetTranslation(), sphere.GetRadius(), color);
#endif

        JPH::BodyInterface& bodyInterface = Engine.PhysicsSystem().GetBodyInterface();

        for (int i = 0; i < collector.collidedBodies.size(); i += 1)
        {
            entt::entity entity = static_cast<entt::entity>(bodyInterface.GetUserData(collector.collidedBodies[i]));

            auto* collect = Engine.ECS().Registry.try_get<Collectable>(entity);

            if (collect != nullptr)
            {
                Collectable& collectable = collectables.get<Collectable>(entity);

                if (collectable.isActive)
                {
                    OnPlayerCollect(playerEntity, entity);
                }
            }
        }
	}
}

void bee::OnPlayerCollect(entt::entity player, entt::entity collectable)
{
    auto& registry = Engine.ECS().Registry;

    static int flowerAudio = 0;

    //Give player points

    if (auto* playerComponent = registry.try_get<Player>(player)) {
        playerComponent->currentScore += 1;
    }

    //Burst of particles
    if (auto* particleEmitter = registry.try_get<EmitterComponent>(collectable)) {
        particleEmitter->currentTimer += 50 * particleEmitter->cycleTimer;
        particleEmitter->particleInitialVelocity *= 3.0f;
        particleEmitter->disabled = true;
    }

    //Disable collectable
    if (auto* collectableComponent = registry.try_get<Collectable>(collectable)) {
        collectableComponent->isActive = false;
    }

    std::string sfxName = "audio/sfxFlowerHit" + std::to_string(flowerAudio) + ".wav";
    Engine.Audio().PlaySound(sfxName.c_str());

    flowerAudio = (flowerAudio + 1) % 5;
}
