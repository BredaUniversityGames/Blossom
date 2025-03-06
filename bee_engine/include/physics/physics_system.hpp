#pragma once

#include <cstdarg>
#include <memory>

#include <jolt/Jolt.h>
#include <jolt/Physics/PhysicsSettings.h>
#include <jolt/Physics/PhysicsSystem.h>

#include "core/engine.hpp"
#include "rendering/debug_render.hpp"

#include "layers.hpp"
#include "tools/log.hpp"

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface 
{
public:

    BPLayerInterfaceImpl();

    virtual unsigned int GetNumBroadPhaseLayers() const override;

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer m_ObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

namespace bee
{

struct ColliderComponent
{
    JPH::ShapeRefC shape;
    JPH::BodyID body{};
};

void InitColliderTransforms(entt::registry& registry, entt::entity entity);
void UpdateColliderTransforms(entt::registry& registry, entt::entity entity);

class PhysicsSystem
{
public:

    PhysicsSystem();
    ~PhysicsSystem();

    void DrawBodies();
    void OnDestroyCollider(entt::registry& registry, entt::entity entity);

    std::unique_ptr<JPH::PhysicsSystem> m_joltPhysicsSystem;

private:
    BPLayerInterfaceImpl m_broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl m_objectVsBroadphaseLayerFilter;
    ObjectLayerPairFilterImpl m_objectVsObjectLayerFilter;

#if defined(JPH_DEBUG_RENDERER)
    JPH::BodyManager::DrawSettings m_drawSettings{};
#endif

};

}