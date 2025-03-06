#include <precompiled/engine_precompiled.hpp>
#include "core/engine.hpp"

#include <chrono>
#include <cstdarg>
#include <iostream>


#include <jolt/Jolt.h>
#include <jolt/Physics/PhysicsSystem.h>
#include <jolt/RegisterTypes.h>


#include "core/audio.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/fileio.hpp"
#include "core/input.hpp"
#include "tools/serialization.hpp"
#include "core/transform.hpp"
#include "physics/layers.hpp"
#include "physics/physics_system.hpp"
#include "rendering/debug_render.hpp"
#include "rendering/render.hpp"
#include "terrain/terrain_collider.hpp"
#include "tools/log.hpp"
#include "tools/pcg_rand.hpp"
#include "tools/thread_pool.hpp"
#include "wind/wind.hpp"
#include <displacement/displacement_manager.hpp>
#include <grass/grass_manager.hpp>
#include <rendering/shader_db.hpp>
#include <core/time.hpp>

#include <resources/resource_manager.hpp>
using namespace bee;

// Make the engine a global variable on free store memory.
bee::EngineClass bee::Engine;

JPH::PhysicsSystem &EngineClass::PhysicsSystem() 
{
  return *m_physicsSystem->m_joltPhysicsSystem;
}

void EngineClass::Initialize(Mode mode) 
{
    Log::Initialize();

    m_time = std::make_unique<bee::Time>();
    m_fileIO = std::make_unique<bee::FileIO>();
    m_ECS = std::make_unique<bee::EntityComponentSystem>();
    m_resources = std::make_unique<bee::ResourceManager>();
    m_device = std::make_unique<bee::Device>(mode);
    m_input = std::make_unique<bee::Input>();
    m_audio = std::make_unique<bee::Audio>();
    m_debugRenderer = std::make_unique<bee::DebugRenderer>();
    m_shaderDB = std::make_unique<bee::ShaderDB>();
    m_renderer = std::make_unique<bee::Renderer>();

    pcg::SeedGlobal(time(nullptr), 2897346558132);

    m_grassManager = std::make_unique<bee::GrassManager>();
    m_windMap = std::make_unique<bee::WindMap>();
    m_displacementManager = std::make_unique<bee::DisplacementManager>();

    m_physicsSystem = std::make_unique<bee::PhysicsSystem>();

    Transform::SubscribeToEvents();

    Log::Info("Engine Initialization Success");
}

void EngineClass::Shutdown()
{
    Transform::UnsubscribeToEvents();

    m_displacementManager.reset();
    m_grassManager.reset();
    m_windMap.reset();
    m_renderer.reset();
    m_debugRenderer.reset();
    m_audio.reset();
    m_input.reset();
    m_device.reset();
    m_resources.reset();
    m_ECS.reset();
    m_fileIO.reset();
    m_physicsSystem.reset();
}

void EngineClass::Run(std::function<void(float)> mainLoop) 
{
    while (!m_device->ShouldClose())
    {
        m_time->Tick();
        float dt = m_time->GetDeltaTime().count() / 1000.0f;

        m_input->Update();
        m_audio->Update();
        m_device->BeginFrame();

        mainLoop(dt);

        m_grassManager->Update(dt);
        m_displacementManager->Update(dt);

        m_ECS->RemovedDeleted();

        m_device->EndFrame();
        m_device->Update();
    }
}

void bee::EngineClass::RenderSystems()
{
    m_renderer->Render();

    if(m_debugRenderer->GetCategoryFlags() & DebugCategory::Physics)
        m_physicsSystem->DrawBodies();
    m_debugRenderer->Render();
}

ThreadPool &bee::EngineClass::ThreadPool()
{
    if (!m_pool)
        m_pool = std::make_unique<bee::ThreadPool>(4);
    return *m_pool;
}
