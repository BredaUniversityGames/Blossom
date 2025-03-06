#include <precompiled/engine_precompiled.hpp>
#include <resources/model/model.hpp>

#include <core/ecs.hpp>
#include <core/transform.hpp>
#include <rendering/render_components.hpp>

#include <resources/mesh/mesh_common.hpp>

#include <core/engine.hpp>
#include <physics/physics_system.hpp>

#include <jolt/Physics/Body/BodyCreationSettings.h>
#include "physics/helpers.hpp"
#include "physics/layers.hpp"

void bee::Model::InstantiateScene(entt::registry& registry, entt::entity parentEntity, bool startHidden)
{
    auto& transform = registry.get_or_emplace<bee::Transform>(parentEntity);

    for (auto rootNodeIndex : rootNodes) {
        InstantiateNodeRecursive(registry, parentEntity, rootNodeIndex, startHidden);
    }
}

void bee::Model::InstantiateNodeRecursive(entt::registry& registry, entt::entity parentEntity, int node, bool hidden)
{
    const auto& currentNode = nodes[node];
    if(currentNode.lodLevel > 0)
        return;

    const auto entity = registry.create();

    // Transform
    auto& transform = registry.emplace<bee::Transform>(entity);
    transform.SetParent(parentEntity);

    transform.SetTranslation(currentNode.translation);
    transform.SetRotation(currentNode.rotation);
    transform.SetScale(currentNode.scale);

    // Primitives
    if (currentNode.meshIndex != -1)
    {
        auto& lods = meshes[currentNode.meshIndex];
        auto& colliderGroup = colliderGroups[currentNode.meshIndex];

        // Cache all the entities required for all the primitives.
        // Assumes 0th LOD exists, which it always should.
        assert(lods.size() > 0 && "LOD mesh received with no levels contained.");
        assert(lods.size() <= 3 && "LOD mesh received with too many levels.");
        std::vector<entt::entity> entities{ lods[0].primitiveMaterialPairs.size() };
        for (size_t i = 0; i < entities.size(); ++i)
            entities[i] = registry.create();

        // At this point we have to effectively perform a transpose.
        // The LOD mesh received has all the meshes stored per LOD level.
        // But we have to create a Mesh Renderer for each primitive, and the Mesh Renderer stores the LODs.
        // To illustrate:
        // [[x0, y0, z0],
        //  [x1, y1, z1],
        //  [x2, y2, z2]]
        // ->
        // [[x0, x1, x2],
        //  [y0, y1, y2],
        //  [z0, z1, z2]]
        //
        // And this between the MeshRenderer::Levels and lods::primitive_material_pairs.
        // That's why we cache the entities first, so we can reuse them in the loop below.

        for(size_t i = 0; i < lods.size(); ++i)
        {
            for (size_t j = 0; j < lods[i].primitiveMaterialPairs.size(); ++j) 
            {
                auto& prim = lods[i].primitiveMaterialPairs[j];

                const auto primitiveEntity = entities[j];

                auto& primitiveTranform = registry.get_or_emplace<bee::Transform>(primitiveEntity);
                if(!primitiveTranform.HasParent())
                    primitiveTranform.SetParent(entity);

                auto& mesh = registry.get_or_emplace<bee::MeshRenderer>(primitiveEntity);
                
                mesh.LODs[i] = prim.first;

                if (i == 0)
                {
                    auto& colliderShape = colliderGroup.colliders[j];

                    if (colliderShape.has_value())
                    {
                        auto& collider = registry.emplace<bee::ColliderComponent>(primitiveEntity);

                        collider.shape = colliderShape.value();
                    }
                }
                
                //This was needed because some sample inputs for GLTF were devoid of materials or textures
                if (prim.second != -1)
                    mesh.Material = materials[prim.second];

                if (hidden)
                    registry.emplace_or_replace<TagNoDraw>(primitiveEntity);
            }
        }
    }

    // Load children
    for (auto newNode : currentNode.children) InstantiateNodeRecursive(registry, entity, newNode, hidden);
}

void GenerateDisplacementDataRecurse(bee::Model& model, glm::mat4 parentModelMatrix, int node, glm::vec3 minBounds, glm::vec3 maxBounds, float trunkPercentage, float distanceMaxBend)
{
    const auto& currentNode = model.nodes[node];

    glm::mat4 matrix = glm::mat4(1.0f);

    matrix = glm::translate(matrix, currentNode.translation);
    matrix = matrix * glm::mat4_cast(currentNode.rotation);
    matrix = glm::scale(matrix, currentNode.scale);

    matrix = matrix * parentModelMatrix;

    if (currentNode.meshIndex != -1)
    {
        auto& primSets = model.meshes[currentNode.meshIndex];

        for(auto& primSet : primSets)
        {
            for (auto& prim : primSet.primitiveMaterialPairs)
            {
                auto& mesh = prim.first;

                bee::mesh_utils::GenerateDisplacementData(mesh, matrix, minBounds, maxBounds, trunkPercentage, distanceMaxBend);
            }
        }
    }

    for (auto newNode : currentNode.children)
    {
        GenerateDisplacementDataRecurse(model, matrix, newNode, minBounds, maxBounds, trunkPercentage, distanceMaxBend);
    }
}

void bee::Model::GenerateDisplacementData(float trunkPercentage, float distanceMaxBend)
{
    for (auto nodeIndex : rootNodes)
    {
        GenerateDisplacementDataRecurse(*this, glm::mat4(1.0f), nodeIndex, this->minBounds, this->maxBounds, trunkPercentage, distanceMaxBend);
    }
}
