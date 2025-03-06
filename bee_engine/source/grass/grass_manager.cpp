#include <precompiled/engine_precompiled.hpp>
#include "grass/grass_manager.hpp"

#include "core/engine.hpp"
#include "core/ecs.hpp"
#include <grass/grass_chunk.hpp>
#include <core/transform.hpp>
#include <glm/glm.hpp>
#include <rendering/render_components.hpp>
#include <glm/gtx/norm.hpp>

#include "core/input.hpp"
#include "rendering/debug_render.hpp"
#include "tools/log.hpp"

bee::GrassManager::GrassManager()
{
}

void bee::GrassManager::Update(float dt) 
{
    if(Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::F5))
    {
        if(m_lodRange[0] == 0)
            m_lodRange[0] = 48;
        else
            m_lodRange[0] = 0;
    }


    auto cameraView = Engine.ECS().Registry.view<CameraComponent, Transform>();
    auto& playerTransform = std::get<1>(cameraView[*cameraView.begin()]);
    auto view = bee::Engine.ECS().Registry.view<GrassChunk, Transform>();

    glm::vec4 cameraPosition4 = glm::vec4{ playerTransform.GetTranslation(), 1.0f};

    if (view.begin() == view.end()) return;

    auto grassParent = std::get<1>(view[*view.begin()]).Parent();

    if (grassParent != entt::null) 
        cameraPosition4 = glm::inverse(Engine.ECS().Registry.get<Transform>(grassParent).World()) * cameraPosition4;

    glm::vec3 cameraPosition{ cameraPosition4 };


    Camera frameCamera{};

    if (cameraView.begin() != cameraView.end())
    {
        auto cameraTransform = Engine.ECS().Registry.get<Transform>(cameraView.front()).World();
        auto& cameraComponent = Engine.ECS().Registry.get<CameraComponent>(cameraView.front());

        if (cameraComponent.isOrthographic)
        {
            frameCamera = Camera::Orthographic(
                cameraTransform[3],
                cameraTransform[3] + cameraTransform * glm::vec4(World::FORWARD, 0.0f),
                cameraComponent.aspectRatio,
                cameraComponent.fieldOfView * 100.0f,
                cameraComponent.nearClip,
                cameraComponent.farClip
            );
        }
        else
        {
            frameCamera = Camera::Perspective(
                cameraTransform[3],
                cameraTransform[3] + cameraTransform * glm::vec4(World::FORWARD, 0.0f),
                cameraComponent.aspectRatio,
                cameraComponent.fieldOfView,
                cameraComponent.nearClip,
                cameraComponent.farClip
            );
        }
    }
    else
    {
        Log::Warn("RENDERER: No camera exists in the scene to render from.");
    }

    Engine.ECS().Registry.remove<CulledGrass>(view.begin(), view.end());
    for(auto entity : view)
    {
        auto [grassChunk, transform] = view[entity];
        float sqrDistance = glm::distance2(cameraPosition, transform.GetTranslation());

        Engine.DebugRenderer().AddBounds(DebugCategory::Enum::Rendering, 
                                         grassChunk.bounds.GetCenter() + transform.GetTranslation(),
                                         grassChunk.bounds.GetSize(),
                                         glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });

        bool foundLod{ false };
        for(size_t i = m_lodRange.size(); i >= 1; --i)
        {
            if (sqrDistance > static_cast<float>(m_lodRange[i - 1] * m_lodRange[i - 1]))
            {
                grassChunk.lod = static_cast<uint32_t>(i);
                foundLod = true;
                break;
            }
        }
        if(!foundLod)
            grassChunk.lod = 0;


        glm::mat4 world = transform.World();
        auto bounds = grassChunk.bounds.ApplyTransform(world);
        if (!bounds.FrustumTest(frameCamera.GetFrustum()))
            Engine.ECS().Registry.emplace<CulledGrass>(entity);
    }

}

entt::entity bee::GrassManager::CreateChunk(glm::vec3 position, ResourceHandle<Image> heightmap, float terrainHeight) 
{
    entt::entity entity = Engine.ECS().CreateEntity();
    Engine.ECS().Registry.emplace<Transform>(entity, Transform{});
    Engine.ECS().Registry.patch<Transform>(entity,
                                           [&position](auto& transform)
                                           {
                                               transform.SetTranslation(position);
                                               transform.SetTranslation(position);
                                               transform.SetTranslation(position);
                                               transform.Name = "Grass Chunk";
                                           });

    auto& grassChunk = Engine.ECS().Registry.emplace<GrassChunk>(entity, 16, 16, 0, heightmap);

    glm::vec3 extents = grassChunk.bounds.GetExtents();
    extents.z = std::max(terrainHeight * 1.5f, 1.0f);
    grassChunk.bounds.SetExtents(extents);

    return entity;
}
