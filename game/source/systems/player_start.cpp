#include "precompiled/game_precompiled.hpp"
#include <systems/player_start.hpp>
#include <physics/rigidbody.hpp>
#include <displacement/displacer.hpp>

#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <resources/resource_manager.hpp>
#include <resources/model/model_loader.hpp>
#include <systems/simple_animation.hpp>
#include <tools/log.hpp>

void bee::PlayerStart::SubscribeToEvents()
{
    Engine.ECS().Registry.on_update<PlayerStart>().connect<PlayerStart::OnPatch>();
}

void bee::PlayerStart::UnsubscribeToEvents()
{
    Engine.ECS().Registry.on_update<PlayerStart>().connect<PlayerStart::OnPatch>();
}

void bee::PlayerStart::OnPatch(entt::registry& registry, entt::entity entity)
{
    Log::Info("Updating Player attributes");
    auto& start = registry.get<PlayerStart>(entity);

    for (auto&& [e, player] : registry.view<Player>().each())
    {
        auto poi = player.currentPOI;
        player = start.playerAttributes;
        player.currentPOI = poi;
    }

    for (auto&& [e, camera] : registry.view<PlayerCamera>().each())
    {
        auto entity = camera.player;
        camera = start.cameraAttributes;
        camera.player = entity;
    }
}

entt::entity bee::SpawnPlayerSystem(const GameConstants& playerConstants)
{
    Transform playerInitial{};
    Player playerComponent{};
    PlayerCamera playerCamera{};

    for (auto&& [e, transform, start] : Engine.ECS().Registry.view<Transform, PlayerStart>().each())
    {
        playerInitial = transform;
        playerComponent = start.playerAttributes;
        playerCamera = start.cameraAttributes;
    }

    // Create player and camera
    auto m_currentPlayer = Engine.ECS().CreateEntity();
    auto cameraEntity = Engine.ECS().CreateEntity();

    // player
    { 
        Player& player = Engine.ECS().CreateComponent<Player>(m_currentPlayer, playerComponent);
        Transform& transform = Engine.ECS().CreateComponent<Transform>(m_currentPlayer, playerInitial);
        RigidBody& rigidBody = Engine.ECS().CreateComponent<RigidBody>(m_currentPlayer);

        Engine.ECS().CreateComponent<DisplacerFocus>(m_currentPlayer);
        Engine.ECS().CreateComponent<Displacer>(m_currentPlayer);

        transform.Name = "Player";
        rigidBody.maxVelocity = 25.0f;
        rigidBody.damping = 2.0f;

        auto& animation = Engine.ECS().CreateComponent<SimpleAnimationComponent>(m_currentPlayer);
        std::vector<ResourceHandle<Model>> keyframes;

        for (auto& path : playerConstants.playerAnimations)
        {
            keyframes.push_back(
                Engine.Resources().Models().FromGLTF(FileIO::Directory::Asset, path)
            );
            for(auto& keyframe : keyframes.back().Retrieve()->materials)
            {
                keyframe.Retrieve()->IsDitherable = false;
            }
        }

        for (auto index : playerConstants.animationOrdering)
        {
            animation.keyframes.push_back(keyframes.at(index));
        }

        animation.next_frame_interval = 0.016f;
    }

    // camera
    { 
        auto& cam = Engine.ECS().CreateComponent<bee::CameraComponent>(cameraEntity);
        auto& transform = Engine.ECS().CreateComponent<Transform>(cameraEntity);

        transform.Name = "Camera";

        PlayerCamera& cameraComponent = Engine.ECS().CreateComponent<PlayerCamera>(cameraEntity, playerCamera);
        cameraComponent.player = m_currentPlayer;
    }

    return m_currentPlayer;
}
