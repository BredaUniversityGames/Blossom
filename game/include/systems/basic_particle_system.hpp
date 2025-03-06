#pragma once
#include <glm/glm.hpp>
#include <resources/resource_handle.hpp>
#include <tools/pcg_rand.hpp>

namespace bee {

struct EmitterComponent
{
    uint32_t particlesPerCycle = 0;
    float particleLifetime = 1.0f;

    glm::vec3 particleDir{};
    glm::vec3 particleGravity{};
    glm::vec3 particleSpawnOffset{};

    float particleScale = 1.0f;
    float particleInitialVelocity{};
    float currentTimer = 0.0f;
    float cycleTimer = 1.0f;

    bool disabled = false;

    ResourceHandle<Mesh> particleMesh{};
    ResourceHandle<Material> particleMaterial{};
};

struct ParticleComponent {

    float currentLifetime{};
    float particleLifetime{};

    glm::vec3 currentAccel;
    glm::vec3 currentVelocity;

};

void BasicParticleSystem(pcg::RNGState& seed, float dt);

}