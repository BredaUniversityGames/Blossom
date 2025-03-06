#pragma once
#include <systems/player.hpp>
#include <systems/player_camera.hpp>
#include <game/constants.hpp>

namespace bee
{

//Marker component for setting the spawn location of the player
class PlayerStart
{
public:
    Player playerAttributes;
    PlayerCamera cameraAttributes;

    static void SubscribeToEvents();
    static void UnsubscribeToEvents();

private:
    static void OnPatch(entt::registry& registry, entt::entity entity);
};

entt::entity SpawnPlayerSystem(const GameConstants& playerAnimationData);

}


VISITABLE_STRUCT(bee::PlayerStart, playerAttributes, cameraAttributes);