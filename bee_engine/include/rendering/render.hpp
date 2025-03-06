#pragma once

#include <glm/glm.hpp>
#include <resources/resource_handle.hpp>
#include <visit_struct/visit_struct.hpp>
#include <rendering/render_components.hpp>
#include "resources/image/image.hpp"

namespace bee
{
    class IBLRenderer;
    class ModelRenderer;
class UIRenderer;
class GrassRenderer;
class TerrainRenderer;
class PostProcessManager;
class Skybox;

struct DebugData
{
    bool BaseColor = false;
    bool Normals = false;
    bool NormalMap = false;
    bool Metallic = false;
    bool Roughness = false;
    bool Emissive = false;
    bool Occlusion = false;
    bool DisplacementPivot = false;
    bool WindMask = false;
};

class Renderer
{
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    std::unique_ptr<ModelRenderer> m_modelRenderer; 
    std::unique_ptr<GrassRenderer> m_grassRenderer;
    std::unique_ptr<TerrainRenderer> m_terrainRenderer;
    std::unique_ptr<PostProcessManager> m_postProcessor;
    std::unique_ptr<Skybox> m_skybox;
    std::unique_ptr<UIRenderer> m_ui;
    std::unique_ptr<IBLRenderer> m_ibl;

    DebugData m_debugFlags{};

    //Internal type for renderer use
    struct ObjectInfo {
        glm::mat4 transform;
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        MeshRenderer* meshRenderer;
    };

    //Internal type for renderer use
    struct LightInfo {
        glm::mat4 transform;
        Light light;
    };

    std::vector<ObjectInfo> m_objectsToDraw{};
    std::vector<LightInfo> m_lightsToDraw{};

    float m_ditherDistance{ 2.0f };

public:
    friend ModelRenderer;
    Renderer();
    ~Renderer();

    GrassRenderer& GetGrassRenderer() { return *m_grassRenderer; }
    PostProcessManager& GetPostProcessManager() { return *m_postProcessor; }
    ModelRenderer& GetModelRenderer() { return *m_modelRenderer; }

    //Queues a mesh to be rendered at the end of this frame
    void QueueMesh(
        const glm::mat4& transform,
        ResourceHandle<Mesh> mesh, 
        ResourceHandle<Material> material,
        MeshRenderer* meshRenderer
    );
    
    void QueueLight(
        const glm::mat4& transform,
        const Light& light
    );

    void Render();

    //TODO: move to post processing?
    void SetFog(glm::vec4 fogColor, float fogNear, float fogFar);

    void SetSkybox(ResourceHandle<Image> skyboxImage);

    //Sets which percentage of direct lighting affects the final fragment
    void SetAmbientFactor(float ambientFactor);

    // Gets platform dependent handle to the output framebuffer (in OpenGL, this is a ptr to a unsigned int)
    void* GetOutputFramebuffer();

    // Set blitting the final framebuffer into the screen (disable when drawing the game on a window)
    void SetScreenBlit(bool value);
    void SetDitherDistance(float distance) { m_ditherDistance = distance; }
    float& GetDitherDistance() { return m_ditherDistance; }

    DebugData& GetDebugFlags() { return m_debugFlags; }

    static const int m_maxDirLights = 4;
};

}  // namespace bee


VISITABLE_STRUCT(bee::DebugData, BaseColor, Normals, NormalMap, Metallic, Roughness, Emissive, Occlusion);
