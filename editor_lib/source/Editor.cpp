#include <precompiled/editor_precompiled.hpp>
#include <Editor.hpp>

#include <level/level.hpp>
#include <imgui_config/imgui_config.hpp>
#include <noise_editor/noise_editor.hpp>
#include <main_viewport/main_viewport.hpp>
#include <scene_viewer/scene_viewer.hpp>
#include <asset_browser/asset_browser.hpp>
#include <graphics_menu/graphics_menu.hpp>
#include <level_editor/prop_builder.hpp>
#include <level_editor/level_editor.hpp>

#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <rendering/debug_render.hpp>
#include <file_dialog/windows_file_dialog.hpp>
#include <tools/log.hpp>

#include <tools/serialization.hpp>

bee::Editor::Editor()
{
	ImGuiImpl::InitializeImGui();
	ImGuiImpl::SetImGuiFont(""); //currently the path argument does not work
	ImGuiImpl::SetImGuiStyle();
	
	m_menus.emplace(
		"Noise Editor", std::make_pair(true, std::make_unique<NoiseEditor>())
	);

	m_menus.emplace(
		"Main Viewport", std::make_pair(true, std::make_unique<MainViewport>())
	);

	m_menus.emplace(
		"Asset Browser", std::make_pair(true, std::make_unique<AssetBrowser>())
	);

    m_menus.emplace(
        "Level Editor", std::make_pair(true, std::make_unique<LevelEditor>())
    );

    m_menus.emplace(
        "ECS Viewer", std::make_pair(true, std::make_unique<SceneViewer>())
    );

    m_menus.emplace(
        "Graphic Settings", std::make_pair(true, std::make_unique<GraphicsMenu>())
    );
}

bee::Editor::~Editor()
{
	ImGuiImpl::ShutdownImGui();
}

void bee::Editor::UpdateSystems(BlossomGame& game, float dt)
{
	ImGuiImpl::StartFrame();
	
	//Draw toolbar
    
    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);


    { //Styling
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    }
    static bool always_open = true;
    ImGui::Begin("DockSpace", &always_open, window_flags);
    ImGui::PopStyleVar(3);

    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("DockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.0f, 7.0f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 7.0f });
        ImGui::BeginMainMenuBar();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New...", nullptr)) {
                if (game.OpenLevel()) {
                    game.GetLevel()->GenerateAll();
                }
            }

            if (ImGui::MenuItem("Open...", nullptr)) {
                auto selected_path = WindowsFileDialog().WithFilter(L"Level", L"*.json").Input();
                if (selected_path) {

                    auto relative_path = std::filesystem::relative(selected_path.value(), "assets/");
                    if (game.OpenLevel(relative_path.string())) LoadMenuData(game);
                }
            }

            if (ImGui::MenuItem("Save...", nullptr)) {
                if (auto level = game.GetLevel()) {

                    //Check Level Path triggers the file dialog if the level has no filepath bound
                    if (auto level_path = CheckLevelPath(level)) {

                        //Apply all changes in editor (builds the levels)
                        ApplyChanges(game);

                        auto stream = Engine.FileIO().OpenWriteStream(FileIO::Directory::Asset, level_path.value());

                        if (stream.is_open()) {

                            level->SetPath(level_path.value());
                            auto archive = JSONSaver(stream);
                            level->SaveToArchive(archive);
                            SaveMenuData(game);

                        }
                        else {
                            Log::Warn("Could not open file {}", level_path.value());
                        }
                    }
                }
            }

            if (ImGui::MenuItem("Save as...", nullptr)) {
                if (auto level = game.GetLevel()) {

                    auto selected_path = WindowsFileDialog().WithFilter(L"Level", L"*.json").Output(L"default_level.json");
                    if (selected_path) {

                        //Apply all changes in editor (builds the levels)
                        ApplyChanges(game);

                        auto relative_path = std::filesystem::relative(selected_path.value(), "assets/");
                        auto stream = Engine.FileIO().OpenWriteStream(FileIO::Directory::Asset, relative_path.string());

                        if (stream.is_open()) {

                            level->SetPath(relative_path.string());
                            auto archive = JSONSaver(stream);
                            level->SaveToArchive(archive);
                            SaveMenuData(game);

                        }
                        else {
                            Log::Warn("Could not open file {}", relative_path.string());
                        }
                    }
                }
            }

            if (ImGui::MenuItem("Reset Level", nullptr))
            {
                auto levelPath = game.GetLevel()->GetPath();
                game.OpenLevel(levelPath);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            for (auto&& [name, active_menu] : m_menus) {
                ImGui::MenuItem(name.c_str(), nullptr, &active_menu.first);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Build")) 
        {
            if (ImGui::Button("Build Level")) {
                ApplyChanges(game);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug"))
        {
            auto currentFlags = Engine.DebugRenderer().GetCategoryFlags();
        
            auto displayDebugCategory = [](const std::string& name, uint32_t& currentFlags, uint32_t flagId) {
                bool active = currentFlags & (1 << flagId);
                if (ImGui::MenuItem(name.c_str(), nullptr, &active)) {
                    currentFlags = currentFlags ^ (1 << flagId);
                }
            };
        
            displayDebugCategory("General", currentFlags, 0);
            displayDebugCategory("Gameplay", currentFlags, 1);
            displayDebugCategory("Physics", currentFlags, 2);
            displayDebugCategory("Sound", currentFlags, 3);
            displayDebugCategory("Rendering", currentFlags, 4);
            displayDebugCategory("AINavigation", currentFlags, 5);
            displayDebugCategory("AIDecision", currentFlags, 6);
            displayDebugCategory("Editor", currentFlags, 7);
            displayDebugCategory("AccelStructs", currentFlags, 8);

            bool all = currentFlags == 0xffffffff;

            if (ImGui::MenuItem("All", nullptr, &all))
            {
                if (!all)
                {
                    currentFlags = 0;
                }
                else
                {
                    currentFlags = 0xffffffff;
                }
            }
        
            Engine.DebugRenderer().SetCategoryFlags(currentFlags);
        
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Stats"))
        {
            ImGui::Text("Deltatime: %.3f ms", dt * 1000.0f); //dt in seconds
            ImGui::EndMenu();
        }

        //TODO: show deltatime here too
        ImGui::EndMainMenuBar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }

	//Draw all active menus

	for (auto&& [name, active_menu] : m_menus) {
		if (active_menu.first == false) continue;
        active_menu.second->Show(game);
	}
	
    //ImGui::End(); //End Dockspace
	ImGuiImpl::EndFrame();
}

void bee::Editor::ApplyChanges(BlossomGame& game)
{
    for (auto&& [name, menu] : m_menus) {
        menu.second->ApplyChanges(game);
    }
}

void bee::Editor::LoadMenuData(BlossomGame& game)
{
    auto level = game.GetLevel();
    auto levelPath = level->GetPath();

    //Case 1: a blank level was loaded
    if (levelPath.empty()) {
        for (auto&& [name, menu] : m_menus) menu.second->LoadFromFile(game, nullptr);
        return;
    }

    auto editFile = levelPath;
    editFile.insert(levelPath.find_last_of("."), ".editor");

    //Case 2: a level was loaded from disk and an editor file was found
    if (Engine.FileIO().Exists(FileIO::Directory::Asset, editFile)) {

        auto stream = Engine.FileIO().OpenReadStream(FileIO::Directory::Asset, editFile);
        cereal::JSONInputArchive in(stream);

        for (auto&& [name, menu] : m_menus) menu.second->LoadFromFile(game, &in);
    }
    //Case 3: a level was loaded from disk but the editor file was not found
    else {
        for (auto&& [name, menu] : m_menus) menu.second->LoadFromFile(game, nullptr);
    }
}

void bee::Editor::SaveMenuData(BlossomGame& game)
{
    auto level = game.GetLevel();
    auto levelPath = level->GetPath();

    auto editFile = levelPath.insert(levelPath.find_last_of("."), ".editor");

    auto stream = Engine.FileIO().OpenWriteStream(bee::FileIO::Directory::Asset, editFile);
    cereal::JSONOutputArchive out(stream);

    for (auto&& [name, menu] : m_menus) {
        menu.second->SaveToFile(game, out);
    }
}

std::optional<std::string> bee::Editor::CheckLevelPath(std::shared_ptr<Level> level)
{

    if (auto path = level->GetPath(); path.empty()) {
        auto selected_path = WindowsFileDialog().WithFilter(L"Level", L"*.json").Output(L"default_level.json");

        if (selected_path)
            return std::filesystem::relative(selected_path.value(), "assets/").string();
        else
            return std::nullopt;
    }
    else {
        return path;
    }

    return std::nullopt;
}
