#include <precompiled/engine_precompiled.hpp>
#include "math/easing.hpp"

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>

float Ease::SmoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

float Ease::InSine(float t)
{
    return 1.0f - glm::cos((t * glm::pi<float>()) * 0.5f);
}

float Ease::OutSine(float t)
{
    return glm::sin((t * glm::pi<float>()) * 0.5f);
}

float Ease::InPow(float t, float pow)
{
    return std::powf(t, pow);
}

float Ease::OutPow(float t, float pow)
{
    return 1.0f - std::powf(1.0f - t, pow);
}

float Ease::InOutPow(float t, float pow)
{
    return t < 0.5f ? std::powf(2, pow - 1) * std::powf(t, pow) : 1.0f - std::powf(-2 * t - 2, pow) * 0.5f;
}

float Ease::EaseOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;

    return 1.0f + c3 * std::powf(t - 1.0f, 3.0f) + c1 * std::powf(t - 1.0f, 2.0f);
}