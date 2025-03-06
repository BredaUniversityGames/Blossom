#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <visit_struct/visit_struct.hpp>
#include <resources/resource_handle.hpp>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "math/geometry.hpp"
#if defined(BEE_PLATFORM_PC)
#include "resources/mesh/mesh_gl.hpp"
#endif

namespace bee
{

struct TagNoDraw {};

struct Light
{
    enum class Type
    {
        Point,
        Directional,
        Spot
    };

    Light() = default;
    Light(const glm::vec3& color, float intensity, float range, Type type)
        : Color(color), Intensity(intensity), Range(range), Type(type) {}

    glm::vec3 Color = {};
    float Intensity = 0;
    float Range = 0;
    float ShadowExtent = 30.0f;
    bool CastShadows = true;
    Type Type = Type::Point;   
};

struct CameraComponent
{
    float nearClip = 0.1f, farClip = 1000.0f;
    float aspectRatio = 16.0f / 9.0f, fieldOfView = 3.14f * 0.25f;
    bool isOrthographic = false;
};

struct MeshRenderer
{
    ResourceHandle<Material> Material;

    ResourceHandle<Mesh> GetMesh() const
    {
        auto mesh = LODs.at(ActiveLevel);
        return mesh.Valid() ? mesh : LODs.at(0);
    }
    std::vector<ResourceHandle<Mesh>> LODs{ 3 };
    uint32_t ActiveLevel{ 0 };

    MeshRenderer() = default;
};

}  // namespace bee

VISITABLE_STRUCT(bee::MeshRenderer, LODs, ActiveLevel, Material);
VISITABLE_STRUCT(bee::Light, Color, Intensity, Range, ShadowExtent, CastShadows);
VISITABLE_STRUCT(bee::CameraComponent, nearClip, farClip, aspectRatio, fieldOfView, isOrthographic);
