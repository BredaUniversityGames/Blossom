#include <precompiled/game_precompiled.hpp>
#include "game/blossom.hpp"

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/transform.hpp"
#include "physics/physics_system.hpp"
#include "rendering/debug_render.hpp"
#include "systems/player.hpp"
#include "systems/player_camera.hpp"
#include "tools/log.hpp"
#include "systems/collectable.hpp"
#include <systems/simple_animation.hpp>
#include "core/ecs.hpp"
#include "ui/ui.hpp"
#include <systems/simple_animation.hpp>


#include <resources/image/image_common.hpp>
#include <resources/model/model.hpp>
#include <resources/resource_handle.hpp>
#include <resources/resource_manager.hpp>


#include <rendering/render.hpp>
#include <rendering/render_components.hpp>


#include "grass/grass_manager.hpp"
#include "terrain/terrain_chunk.hpp"


#include <level/level.hpp>
#include <math/geometry.hpp>

#include "displacement/displacer.hpp"
#include "platform/pc/core/device_pc.hpp"
#include "terrain/terrain_collider.hpp"
#include <physics/rigidbody.hpp>
#include <systems/collisions.hpp>
#include <systems/point_of_interest.hpp>
#include <systems/basic_particle_system.hpp>
#include <systems/model_root_component.hpp>
#include <systems/player_start.hpp>
#include <systems/scale_in_system.hpp>
#include <systems/orbiting_bee_system.hpp>
#include <core/time.hpp>
#include <core/audio.hpp>

#if defined(BEE_EDITOR)
#include <../../editor_lib/include/Editor.hpp>
#endif

bee::BlossomGame::BlossomGame()
{
    Engine.DebugRenderer().SetCategoryFlags({}/*DebugCategory::Enum::Rendering*/);
    ModelRootComponent::SubscribeToEvents();
    PlayerStart::SubscribeToEvents();
    OrbitalSpawnerComponent::SubscribeToEvents();

#if defined(BEE_EDITOR)
    m_editor = std::make_unique<Editor>();
#endif

    // Load UI resources
    {
        std::vector<float> positions = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f,
                                        -0.5f, 0.5f,  0.0f, 0.5f, 0.5f,  0.0f};

        std::vector<uint32_t> indices = {0, 1, 2, 3, 2, 1};

        std::vector<float> uvs = {
            0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        };

        m_UIQuad = Engine.Resources().Meshes().FromRawData(
            "UI_Quad", std::move(indices), std::move(positions), {}, // No normals
            std::move(uvs), {}, {}, // No tangets, no displacement
            {});

        m_playButton = Engine.Resources().Images().FromFile(
            FileIO::Directory::Asset, constants.playButton, ImageFormat::RGBA8);
        m_playButtonHovered = Engine.Resources().Images().FromFile(
            FileIO::Directory::Asset, constants.playButtonHovered, ImageFormat::RGBA8);

        m_resetButton = Engine.Resources().Images().FromFile(
            FileIO::Directory::Asset, constants.resetButton, ImageFormat::RGBA8);
        m_resetButtonHovered = Engine.Resources().Images().FromFile(
            FileIO::Directory::Asset, constants.resetButtonHovered, ImageFormat::RGBA8);

        m_exitButton = Engine.Resources().Images().FromFile(
            FileIO::Directory::Asset, constants.exitButton, ImageFormat::RGBA8);
        m_exitButtonHovered = Engine.Resources().Images().FromFile(
            FileIO::Directory::Asset, constants.exitButtonHovered, ImageFormat::RGBA8);
    }

    // Open default startup level
    if (OpenLevel("levels/default_level.json")) {
        Log::Info("Successfully loaded file from Disk");
    }

    // Music
    Engine.Audio().LoadSound("audio/bgmCrocus.wav", true);
    //Engine.Audio().LoadSound("audio/sfxWind.wav", true);
    //Engine.Audio().LoadSound("audio/ambient wind.mp3", true);
    //Engine.Audio().LoadSound("audio/ambient wind.wav", true);
    Engine.Audio().LoadSound("audio/ambient wind amplified.wav", true);
    Engine.Audio().LoadSound("audio/sfxHiveSounds.wav", true);
    //SFX
    Engine.Audio().LoadSound("audio/sfxDeliverPollen.wav", false);
    Engine.Audio().LoadSound("audio/sfxFlowerHit0.wav", false);
    Engine.Audio().LoadSound("audio/sfxFlowerHit1.wav", false);
    Engine.Audio().LoadSound("audio/sfxFlowerHit2.wav", false);
    Engine.Audio().LoadSound("audio/sfxFlowerHit3.wav", false);
    Engine.Audio().LoadSound("audio/sfxFlowerHit4.wav", false);
    Engine.Audio().LoadSound("audio/sfxPOIComplete.wav", false);

    // Play background music
    Engine.Audio().PlaySound("audio/bgmCrocus.wav");
    //Engine.Audio().PlaySound("audio/sfxWind.wav");
    //Engine.Audio().PlaySound("audio/ambient wind.mp3");
    //Engine.Audio().PlaySound("audio/ambient wind.wav");
    Engine.Audio().PlaySound("audio/ambient wind amplified.wav");
    m_hiveAudioChannelID = Engine.Audio().PlaySound("audio/sfxHiveSounds.wav");
    Engine.Audio().SetChannelVolume(m_hiveAudioChannelID, 0);
}

bee::BlossomGame::~BlossomGame() {}

void bee::BlossomGame::Update(float dt)
{
    switch (m_state)
    {
    case State::MAIN_MENU:
    {
        ui::Update(dt);
    }
    break;
    case State::PLAYING: 
    {
        auto rigidBodies = Engine.ECS().Registry.view<Transform, RigidBody>();

        UpdatePlayerControlInversion();

        //Runs on Fixed Timestep
        {
            auto view = Engine.ECS().Registry.view<Transform, RigidBody>();
            auto fixedTimeStep = Engine.GetTime().GetFixedTimeStep().count() / 1000.0f;

            for (uint32_t i = 0; i < Engine.GetTime().GetFixedStepsNeeded(); i++)
            {
                if (!m_freeCamEnabled) {
                    PlayerMovementSystem(m_currentLevel, fixedTimeStep);
                }

                for (auto&& [e, transform, body] : view.each())
                    bee::UpdateRigidBody(body, transform, fixedTimeStep);
                TerrainCollisionHandlingSystem(m_currentLevel, fixedTimeStep);
                OrbitalParticleMovementSystem(fixedTimeStep);
            }
        }

        bool switchFreeCam =
            Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::E) ||
            Engine.Input().GetGamepadButtonOnce(0, Input::GamepadButton::StickPressRight);

        if (switchFreeCam) {
            m_freeCamEnabled = !m_freeCamEnabled;
        }

        if (m_freeCamEnabled) {
            FreeCameraSystem(dt);
        }
        else {
            PlayerCameraSystem(dt);
            UpdateHUD(dt);
            POISystem(dt, m_hiveAudioChannelID);
        }

        OrbitalSpawnerSystem();
        CollectItems();
        BasicParticleSystem(pcg::g_rng, dt);
        ScaleUpSystem(dt);
        CheckWinCondition();

        if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Escape) ||
            Engine.Input().GetGamepadButton(0, Input::GamepadButton::MenuRight))
        {
            m_state = State::MAIN_MENU;
            Engine.Input().EnableCursor();
            ShowMainMenu();
        }
    } 
    break;
    }

    auto meshRendererView = Engine.ECS().Registry.view<Transform, MeshRenderer>(entt::exclude<TerrainChunk, TagNoDraw>);
    auto cameraView = Engine.ECS().Registry.view<CameraComponent, Transform>();
    auto& cameraTransform = std::get<1>(cameraView[*cameraView.begin()]);

    auto& lodDistances = m_currentLevel->GetLODs();

    for (auto [entity, transform, model] : meshRendererView.each())
    {
        glm::mat4 worldTransform = transform.World();
        float distanceFromCamera = glm::distance(glm::vec3(worldTransform[3][0], worldTransform[3][1], worldTransform[3][2]), cameraTransform.GetTranslation());

        model.ActiveLevel = 0;

        for(size_t i = 0; i < lodDistances.distances.size(); i++)
            if (distanceFromCamera > lodDistances.distances[i])
                model.ActiveLevel = i + 1;

        Engine.Renderer().QueueMesh(worldTransform, model.GetMesh(), model.Material, &model);
    }

    if (m_currentLevel) {
        auto& lighting = m_currentLevel->GetLighting();
        auto& terrain = m_currentLevel->GetTerrain();

        Light light;
        light.Type = Light::Type::Directional;
        light.Color = lighting.sunColour;
        light.Intensity = lighting.sunIntensity;

        // multiplied by sqrt 2 to fill the entire map
        light.ShadowExtent = terrain.mapSizeX * std::sqrtf(2.0f);

        Engine.Renderer().SetAmbientFactor(lighting.ambientFactor);
        Engine.Renderer().QueueLight(glm::mat4_cast(lighting.sunDirection), light);
    }

    // Player animation and render
    SimpleAnimationSystem(dt);
    DebugDrawSpawnerOrbitals();

    Engine.RenderSystems(); // First render frame to viewport

#if defined(BEE_EDITOR)
    m_editor->UpdateSystems(*this, dt); // Then render imgui UI
#endif

}

bool bee::BlossomGame::OpenLevel(const std::string &file) {

    victory = false;
    auto stream = Engine.FileIO().OpenReadStream(FileIO::Directory::Asset, file);

    if (!stream.is_open()) {
        Log::Warn("Failed to open file: {}", file);
        return false;
    }

    Engine.ECS().Clear();
    JSONLoader loader{ stream };
    
    //Loaded level from the input stream
    auto new_level = std::make_shared<Level>(loader);

    //Gameplay Setup
    {
        CreateHUDResources();
        m_currentPlayer = SpawnPlayerSystem(constants);
        PlayerMovementSystem(new_level, 0.0f);
        PlayerCameraSystem(0.0f); //Makes sure the camera start close to the player
        if (m_state == State::MAIN_MENU) ShowMainMenu();
    }

    m_currentLevel = new_level;
    m_currentLevel->GenerateAll();
    m_currentLevel->SetPath(file);

    auto POIView = Engine.ECS().Registry.view<POIComponent>();

    for (auto [entity, poi] : POIView.each())
    {
        InitColliderTransforms(Engine.ECS().Registry, entity);
    }
    //Engine.PhysicsSystem().

#if defined(BEE_EDITOR)
    m_editor->LoadMenuData(*this);
#endif

    return true;
}

void bee::BlossomGame::ShowMainMenu() {

    auto ui_ent = bee::Engine.ECS().CreateEntity();
    UIMenu &menu = Engine.ECS().CreateComponent<bee::UIMenu>(ui_ent);

    menu.mesh = m_UIQuad;

    // Play button
    {
        Button& button = menu.buttons.emplace_back();

        button.position.x = 0.0f;
        button.position.y = 0.0f;
        button.scale = 1.1f;

        button.plainTex = m_playButton;
        button.hoveredTex = m_playButtonHovered;
        button.texture = m_playButton;
        button.inverseAspect = 1.0f / image_utils::GetAspectRatio(button.texture);

        button.callback = [this](entt::entity ui_ent) {
            Log::Info("Game Started");

            SetState(BlossomGame::State::PLAYING);
            Engine.Input().DisableCursor();
            Engine.ECS().DeleteEntity(ui_ent);
        };
    }

    // Reset button
    {
        Button& button = menu.buttons.emplace_back();

        button.position.x = 0.0f;
        button.position.y = -0.4f;
        button.scale = 0.7f;

        button.plainTex = m_resetButton;
        button.hoveredTex = m_resetButtonHovered;
        button.texture = button.plainTex;
        button.inverseAspect = 1.0f / image_utils::GetAspectRatio(button.texture);

        button.callback = [this](entt::entity ui_ent) {

            auto levelPath = GetLevel()->GetPath();
            OpenLevel(levelPath);
        };
    }

    // Quit button
    {
        Button& button = menu.buttons.emplace_back();

        button.position.x = 0.0f;
        button.position.y = -0.7f;
        button.scale = 0.7f;

        button.plainTex = m_exitButton;
        button.hoveredTex = m_exitButtonHovered;
        button.texture = button.plainTex;
        button.inverseAspect = 1.0f / image_utils::GetAspectRatio(button.texture);

        button.callback = [](entt::entity ui_ent) {
            Log::Info("Game Closed");

#if defined(BEE_PLATFORM_PC)
            if (!IN_EDITOR)
                Engine.Device().CloseWindow();
#endif
        };
    }
}

void bee::BlossomGame::CheckWinCondition()
{
    if (!victory)
    {
        auto POIview = Engine.ECS().Registry.view<POIComponent>();

        bool checkComplete = true;

        for (auto&& [e, poi] : POIview.each())
        {
            if (!poi.completed) checkComplete = false;
        }

        if (checkComplete)
        {
            victory = true;
            auto victoryText = Engine.ECS().Registry.create();
            auto& ui = Engine.ECS().Registry.emplace<UIElement>(victoryText);

            ui.draw_on_main_menu = false;
            ui.origin = UIElement::Origin::CENTRE;
            ui.position = { 0.0f, 0.6f };
            ui.scale = { constants.victoryTextSize, constants.victoryTextSize };
            ui.visible = true;
            ui.layer = 0;
            ui.texture = Engine.Resources().Images().FromFile(FileIO::Directory::Asset,
                constants.victoryTextPath, ImageFormat::RGBA8);
        }
    }
}

void bee::BlossomGame::CreateHUDResources()
{
    //Load all textures
    auto LoadTexture = [&](const std::string& path)
        {
            return Engine.Resources().Images().FromFile(bee::FileIO::Directory::Asset, path, bee::ImageFormat::RGBA8);
        };

    auto hiveHoneyBackImage = LoadTexture(constants.hiveHoneyBack);
    auto hiveHoneyDynamicImage = LoadTexture(constants.hiveHoneyDynamic);
    auto hiveHoneyBarImage = LoadTexture(constants.hiveHoneyBar);

    auto playerHoneyBackImage = LoadTexture(constants.playerHoneyBack);
    auto playerHoneyDynamicImage = LoadTexture(constants.playerHoneyDynamic);
    auto playerHoneyBarImage = LoadTexture(constants.playerHoneyBar);

    auto& registry = Engine.ECS().Registry;

    hiveBar = DynamicBar(registry,
        hiveHoneyBackImage, hiveHoneyDynamicImage, hiveHoneyBarImage,
        UIElement::Origin::WEST, { -0.33f, 0.85f }, glm::vec2(0.3f), true, 0.08f
    );

    playerBar = DynamicBar(registry,
        playerHoneyBackImage, playerHoneyDynamicImage, playerHoneyBarImage,
        UIElement::Origin::SOUTH_WEST, { -0.93f, -0.9f }, glm::vec2(0.3f), false, 0.03f
    );

}

void bee::BlossomGame::UpdateHUD(float dt)
{

    auto& registry = Engine.ECS().Registry;
    Player* playerComponent = registry.try_get<Player>(m_currentPlayer);
    if (playerComponent == nullptr) return;

    POIComponent* currentPOI = registry.try_get<POIComponent>(playerComponent->currentPOI);

    //Always Show Player Pollen Bar

    float pollenPercentage = static_cast<float>(playerComponent->currentScore) / static_cast<float>(playerComponent->scoreCap);

    playerBar.SetVisible(registry, true);
    playerBar.SetFill(registry, pollenPercentage);

    if (currentPOI != nullptr) {

        hiveBar.SetVisible(registry, true);
        float percentageComplete = static_cast<float>(currentPOI->currentScore) / static_cast<float>(currentPOI->scoreGoal);
        hiveBar.SetFill(registry, percentageComplete);

    }
    else {
        hiveBar.SetFill(registry, 0.0f);
        hiveBar.SetVisible(registry, false);
    }
}
