#include <precompiled/engine_precompiled.hpp>
#include "physics/physics_system.hpp"

#include "core/engine.hpp"
#include "core/ecs.hpp"
#include "core/transform.hpp"

#include <jolt/Jolt.h>
#include <jolt/RegisterTypes.h>
#include <jolt/Core/Color.h>
#include <jolt/Physics/Body/BodyCreationSettings.h>

#include "physics/helpers.hpp"
#include "physics/debug_renderer.hpp"

BPLayerInterfaceImpl::BPLayerInterfaceImpl()
{
	m_ObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	m_ObjectToBroadPhase[Layers::COLLECTABLE] = BroadPhaseLayers::COLLECTABLE;
}

unsigned int BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
{
	return BroadPhaseLayers::NUM_LAYERS;
}

JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
{
	JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
	return m_ObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
{
	switch ((JPH::BroadPhaseLayer::Type)inLayer)
	{
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::COLLECTABLE: return "COLLECTABLE";
	default:													JPH_ASSERT(false); return "INVALID";
	}
}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
{
	switch (inObject1)
	{
	case Layers::NON_MOVING:
	case Layers::COLLECTABLE:
		return inObject2 == Layers::MOVING; // Non moving only collides with moving
	case Layers::MOVING:
		return true; // Moving collides with everything
	default:
		//JPH_ASSERT(false);
		return false;
	}
}

bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
{
	switch (inLayer1)
	{
	case Layers::NON_MOVING:
	case Layers::COLLECTABLE:
		return inLayer2 == BroadPhaseLayers::MOVING;
	case Layers::MOVING:
		return true;
	default:
		//JPH_ASSERT(false);
		return false;
	}
}

static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, unsigned int inLine)
{
	// Print to the TTY
	if (inMessage == nullptr)
	{
		bee::Log::Info("{}:{}: ({}) ", inFile, inLine, inExpression);
	}
	else
	{
		bee::Log::Info("{}:{}: ({}) {}", inFile, inLine, inExpression, inMessage);
	}

	// Breakpoint
	return true;
};

static void TraceImpl(const char* inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	bee::Log::Info("{}", &buffer[0]);
}

void bee::PhysicsSystem::OnDestroyCollider(entt::registry& registry, entt::entity entity)
{
	auto* collider = registry.try_get<ColliderComponent>(entity);
	if (collider != nullptr && !collider->body.IsInvalid())
	{
		auto& bodyInterface = m_joltPhysicsSystem->GetBodyInterface();

		bodyInterface.RemoveBody(collider->body);
		bodyInterface.DestroyBody(collider->body);
	}
}

bee::PhysicsSystem::PhysicsSystem()
{
	//jolt setup
	JPH::RegisterDefaultAllocator();

	JPH::Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

		JPH::Factory::sInstance = new JPH::Factory();

	JPH::RegisterTypes();

	// add TempAllocator and JobSystem here if needed

	// may need to update later
	const unsigned int cMaxBodies = 1024 * 64;

	// default setting
	const unsigned int cNumBodyMutexes = 0;

	const unsigned int cMaxBodyPairs = 1024;

	const unsigned int cMaxContactConstraints = 1024;

	m_joltPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
	m_joltPhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_broadPhaseLayerInterface, m_objectVsBroadphaseLayerFilter, m_objectVsObjectLayerFilter);

	bee::Engine.ECS().Registry.on_destroy<ColliderComponent>().connect<&PhysicsSystem::OnDestroyCollider>(*this);

#if defined(JPH_DEBUG_RENDERER) && defined(BEE_PLATFORM_PC)
	m_drawSettings.mDrawShape = true;

	// Jolt automatically assignes this to a global variable 
	new PhysicsRendererImpl();
#endif
}

bee::PhysicsSystem::~PhysicsSystem()
{
	// For some reason adding this line causes a runtime error on exit
	//bee::Engine.ECS().Registry.on_destroy<ColliderComponent>().disconnect<&PhysicsSystem::OnDestroyCollider>(*this);

	m_joltPhysicsSystem.reset();
}

void bee::PhysicsSystem::DrawBodies()
{
#if defined(JPH_DEBUG_RENDERER) && defined(BEE_PLATFORM_PC)
	m_joltPhysicsSystem->DrawBodies(m_drawSettings, JPH::DebugRenderer::sInstance);
#endif
}

void bee::InitColliderTransforms(entt::registry& registry, entt::entity entity)
{
	Transform* transform = registry.try_get<Transform>(entity);

	if (transform != nullptr)
	{
		ColliderComponent* collider = registry.try_get<ColliderComponent>(entity);

		if (collider != nullptr && collider->shape.GetPtr() != nullptr)
		{
			glm::vec3 scale, translation;
			glm::quat rotation;

			Decompose(transform->World(), translation, scale, rotation);

			JPH::PhysicsSystem& physicsSystem = Engine.PhysicsSystem();
			JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
			JPH::BodyCreationSettings bodySettings(collider->shape.GetPtr()->ScaleShape(GlmToJolt(scale)).Get(), GlmToJolt(translation), GlmToJolt(rotation), JPH::EMotionType::Static, Layers::NON_MOVING);

			JPH::Body* body = bodyInterface.CreateBody(bodySettings);

            if (body != nullptr)
            {
				if (!collider->body.IsInvalid())
				{
					bodyInterface.RemoveBody(collider->body);
					bodyInterface.DestroyBody(collider->body);
				}

                bodyInterface.AddBody(body->GetID(), JPH::EActivation::DontActivate);
                body->SetUserData(static_cast<JPH::uint64>(entity));
                collider->body = body->GetID();
            }
            else
            {
                Log::Warn("physics body creation failed");
                Log::Info("num bodies: {:d}, max bodies: {:d}", physicsSystem.GetNumBodies(), physicsSystem.GetMaxBodies());
            }
		}

		for (auto iter = transform->begin(); iter != transform->end(); ++iter)
		{
			InitColliderTransforms(registry, *iter);
		}
	}
}

void bee::UpdateColliderTransforms(entt::registry& registry, entt::entity entity)
{
	Transform* transform = registry.try_get<Transform>(entity);

	if (transform != nullptr)
	{
		ColliderComponent* collider = registry.try_get<ColliderComponent>(entity);

		if (collider != nullptr && !collider->body.IsInvalid())
		{
			glm::vec3 scale, translation;
			glm::quat rotation;

			Decompose(transform->World(), translation, scale, rotation);

			JPH::PhysicsSystem& physicsSystem = Engine.PhysicsSystem();
			JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

			bodyInterface.SetShape(collider->body, collider->shape.GetPtr()->ScaleShape(GlmToJolt(scale)).Get(), false, JPH::EActivation::DontActivate);
			bodyInterface.SetPositionAndRotation(collider->body, GlmToJolt(translation), GlmToJolt(rotation), JPH::EActivation::DontActivate);
		}

		for (auto iter = transform->begin(); iter != transform->end(); ++iter)
		{
			UpdateColliderTransforms(registry, *iter);
		}
	}
}