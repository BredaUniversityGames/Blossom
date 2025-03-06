#include "precompiled/game_precompiled.hpp"
#include <systems/simple_animation.hpp>

#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <rendering/render.hpp>
#include <core/transform.hpp>
#include <resources/model/model.hpp>

namespace bee::detail {

void SubmitNodesRecursive(ResourceHandle<Model> model, const glm::mat4 transform, int node_index) {

    auto& node = model.Retrieve()->nodes.at(node_index);
    auto& mesh = model.Retrieve()->meshes.at(node.meshIndex);

    auto this_transform = transform *
        glm::translate(glm::mat4(1.0f), node.translation) *
        glm::scale(glm::mat4(1.0f), node.scale) *
        glm::mat4_cast(node.rotation);

    for (auto&& [prim, mat_id] : mesh[0].primitiveMaterialPairs) {
        Engine.Renderer().QueueMesh(this_transform, prim, model.Retrieve()->materials.at(mat_id), nullptr);
    }

    for (auto node_id : node.children) {
        SubmitNodesRecursive(model, this_transform, node_id);
    }
}

}

void bee::DrawModel(ResourceHandle<Model> model, const glm::mat4 transform)
{
    if (auto model_ptr = model.Retrieve()) {
        for (auto child_id : model_ptr->rootNodes)
            detail::SubmitNodesRecursive(model, transform, child_id);
    }
}

void bee::SimpleAnimationSystem(float dt)
{
    auto view = Engine.ECS().Registry.view<Transform, SimpleAnimationComponent>();

    for (auto&& [entity, transform, animation] : view.each()) {

        animation.animation_timer += dt;
        if (animation.animation_timer > animation.next_frame_interval) {

            float remainder = std::fmodf(animation.animation_timer, animation.next_frame_interval);
            float increment = std::floorf(animation.animation_timer / animation.next_frame_interval);

            animation.animation_timer = remainder;
            animation.current_frame = (animation.current_frame + static_cast<size_t>(increment)) % animation.keyframes.size();
        }

        DrawModel(animation.keyframes[animation.current_frame], transform.World());
    }
}
