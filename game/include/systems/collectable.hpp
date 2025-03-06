#pragma once

namespace bee
{

struct Collectable
{
	bool isActive;
};

void CollectItems();

void OnPlayerCollect(entt::entity player, entt::entity collectable);

}