#include "precompiled/game_precompiled.hpp"
#include <systems/basic_particle_system.hpp>

#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <rendering/render_components.hpp>
#include <physics/rigidbody.hpp>
#include <core/transform.hpp>
#include <tools/pcg_rand.hpp>

//TODO move somewhere else
glm::vec3 rand_vec3(pcg::RNGState& seed) {

    //Thanks to https://math.stackexchange.com/questions/44689/how-to-find-a-random-axis-or-unit-vector-in-3d

    float angle = pcg::rand0_1(seed) * 2.0f * glm::pi<float>();
    float z = (pcg::rand0_1(seed) * 2.0f - 1.0f);

    float coefficient = glm::sqrt(1.0f - z * z);

    return glm::vec3(
        coefficient * glm::cos(angle),
        coefficient * glm::sin(angle),
        z
    );
}


void bee::BasicParticleSystem(pcg::RNGState& seed, float dt)
{
    auto& registry = Engine.ECS().Registry;
    
    //Spawn all particles from emitters
    auto view = registry.view<EmitterComponent, Transform>();
    
    for (auto&& [entity, emitter, transform] : view.each()) {

        if (!emitter.disabled) emitter.currentTimer += dt;
        auto cycles = static_cast<int>(emitter.currentTimer / emitter.cycleTimer);

        if (cycles > 0) {
            emitter.currentTimer = std::fmodf(emitter.currentTimer, emitter.cycleTimer);

            for (int i = 0; i < emitter.particlesPerCycle * cycles; i++) {

                auto particle = registry.create();

                auto& particleTransform = registry.emplace<Transform>(particle);
                particleTransform.Name = "Particle";
                particleTransform.SetTranslation(transform.GetTranslation());
                particleTransform.SetScale(glm::vec3(emitter.particleScale));

                auto& rigidBody = registry.emplace<RigidBody>(particle);
                
                auto randomDir = rand_vec3(seed);
                auto randomSpeed = pcg::rand0_1(seed) * 0.5f + 0.5f;
                rigidBody.acceleration = glm::normalize(randomDir + emitter.particleDir) * emitter.particleInitialVelocity * randomSpeed;
                rigidBody.damping = 0.0f;

                auto& particleComponent = registry.emplace<ParticleComponent>(particle);

                particleComponent.particleLifetime = emitter.particleLifetime;

                auto& particleMesh = registry.emplace<MeshRenderer>(particle);
                particleMesh.LODs.front() = emitter.particleMesh;
                particleMesh.Material = emitter.particleMaterial;
            }
        }
    }

    auto view2 = registry.view<ParticleComponent>();

    for (auto&& [entity, particle] : view2.each()) {

        particle.currentLifetime += dt;
        if (particle.currentLifetime > particle.particleLifetime) {
            Engine.ECS().DeleteEntity(entity);
            continue;
        }

    }
}
