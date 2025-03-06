#pragma once

namespace Ease
{

float SmoothStep(float t);
float InSine(float t);
float OutSine(float t);
float InPow(float t, float pow);
float OutPow(float t, float pow);
float InOutPow(float t, float pow);
float EaseOutBack(float t);

}
