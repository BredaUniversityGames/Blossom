#pragma once

namespace bee
{

class ScaleUpComponent
{
public:
	float speed = 1.0f;

	//Target is t = 1, can be set to negative to delay the start sequence
	float currentT = 0.0f;

	float initialScale = 0.0f;
	float targetScale = 1.0f;
};

void ScaleUpSystem(float dt);

}