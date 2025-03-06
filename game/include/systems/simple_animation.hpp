#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <resources/resource_handle.hpp>

namespace bee {

void DrawModel(ResourceHandle<Model> model, const glm::mat4 transform);
void SimpleAnimationSystem(float dt);

struct SimpleAnimationComponent
{
    std::vector<bee::ResourceHandle<bee::Model>> keyframes;
    size_t current_frame = 0;

    float animation_timer{};
    float next_frame_interval = 1.0f;
};

} // namespace bee
