#pragma once
#include <resources/model/model.hpp>
#include <visit_struct/visit_struct.hpp>
#include <game/constants.hpp>

namespace bee {

struct BeautifyTag {};

struct POIComponent {
	int currentScore = 0;
	int scoreGoal = 25;
	float activationRange = 30.0f;
	bool started = false;
	bool completed = false;
	ResourceHandle<Model> particleModel{};
};

void POISystem(float dt, int hiveAudioChannelID);
void StartNewPOI(entt::entity poiEntity);
void DepositPOI(entt::entity player, entt::entity poiEntity);
void FinishPOI(entt::entity poiEntity);


}

VISITABLE_STRUCT(bee::POIComponent, scoreGoal, activationRange, particleModel);