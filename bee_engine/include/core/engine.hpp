#pragma once
#include <memory>
#include <string>
#include <functional>

namespace JPH
{
class PhysicsSystem;
}

namespace bee
{
class ShaderDB;
class Time;
class EntityComponentSystem;
class FileIO;
class ResourceManager;
class Device;
class Input;
class Audio;
class DebugRenderer;
class Renderer;
class Serializer;
class Profiler;
class ThreadPool;
class GrassManager;
class WindMap;
class Game;
class Level;
class DisplacementManager;
class PhysicsSystem;

enum class Mode
{
    WINDOW, FULLSCREEN
};

class EngineClass
{
public:
    void Initialize(Mode mode);
    void Shutdown();

    //void MainLoop(float deltatime)
    void Run(std::function<void(float)> mainLoop);

    //Renders all subsystems of the engine, should be called inside main loop lambda
    //all objects, terrain, grass, debug elements
    void RenderSystems();

    FileIO& FileIO() { return *m_fileIO; }
    ResourceManager& Resources() { return *m_resources; }
    Device& Device() { return *m_device; }
    Input& Input() { return *m_input; }
    Audio& Audio() { return *m_audio; }
    DebugRenderer& DebugRenderer() { return *m_debugRenderer; }
    Renderer& Renderer() { return *m_renderer; }
    EntityComponentSystem& ECS() { return *m_ECS; }
    ThreadPool& ThreadPool(); // Thread pool does lazy initialization
    GrassManager& GetGrassManager() { return *m_grassManager; }
    WindMap& GetWindMap() { return *m_windMap; }
    DisplacementManager& DisplacementManager() { return *m_displacementManager; }
    JPH::PhysicsSystem& PhysicsSystem();
    Time& GetTime() { return *m_time; }

    ShaderDB& ShaderDB() { return *m_shaderDB; }

private:

    std::unique_ptr<bee::Time> m_time;
    std::unique_ptr<bee::FileIO> m_fileIO;
    std::unique_ptr<bee::ResourceManager> m_resources;
    std::unique_ptr<bee::Device> m_device;
    std::unique_ptr<bee::DebugRenderer> m_debugRenderer;
    std::unique_ptr<bee::Renderer> m_renderer;
    std::unique_ptr<bee::Input> m_input;
    std::unique_ptr<bee::Audio> m_audio;
    std::unique_ptr<bee::ThreadPool> m_pool;
    std::unique_ptr<bee::EntityComponentSystem> m_ECS;
    std::unique_ptr<bee::GrassManager> m_grassManager;
    std::unique_ptr<bee::WindMap> m_windMap;
    std::unique_ptr<bee::DisplacementManager> m_displacementManager;
    std::unique_ptr<bee::PhysicsSystem> m_physicsSystem;
    std::unique_ptr<bee::ShaderDB> m_shaderDB;
};

extern EngineClass Engine;

}  // namespace bee