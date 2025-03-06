#pragma once
#include <resources/resource_handle.hpp>

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <entt/entity/fwd.hpp>

#include <jolt/Jolt.h>
#include <jolt/Physics/Collision/Shape/Shape.h>

namespace bee {


struct ModelNode
{
    int32_t meshIndex = -1;
    std::vector<int> children;
    uint32_t lodLevel;

    glm::vec3 translation{}, scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

struct PrimitiveSet
{
    std::vector<std::pair<ResourceHandle<Mesh>, int>> primitiveMaterialPairs;
};

struct ColliderGroup
{
    std::vector<std::optional<JPH::ShapeRefC>> colliders;
};

class Model
{
public:

    Model(
        std::vector<int>&& root_nodes,
        std::vector<ModelNode>&& nodes,
        std::vector<std::vector<PrimitiveSet>>&& meshes,
        std::vector<ColliderGroup>&& colliderGroups,
        std::vector<ResourceHandle<Material>>&& materials,
        glm::vec3 minBounds,
        glm::vec3 maxBounds
    ) :
        rootNodes(std::move(root_nodes)),
        nodes(std::move(nodes)),
        meshes(std::move(meshes)),
        colliderGroups(std::move(colliderGroups)),
        materials(std::move(materials)),
        minBounds(minBounds),
        maxBounds(maxBounds)
    {}

    std::vector<int> rootNodes;
    std::vector<ModelNode> nodes;
    std::vector<std::vector<PrimitiveSet>> meshes;
    std::vector<ColliderGroup> colliderGroups;
    std::vector<ResourceHandle<Material>> materials;

    glm::vec3 minBounds;
    glm::vec3 maxBounds;

    void InstantiateScene(entt::registry& registry, entt::entity parentEntity, bool startHidden = false);
    void InstantiateNodeRecursive(entt::registry& registry, entt::entity parentEntity, int node, bool hidden);

    void GenerateDisplacementData(float trunkPercentage, float distanceMaxBend);
};

}