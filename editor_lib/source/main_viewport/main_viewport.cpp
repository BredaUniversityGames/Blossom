#include <precompiled/editor_precompiled.hpp>
#include <main_viewport/main_viewport.hpp>

#include <core/engine.hpp>
#include <rendering/render.hpp>
#include <core/input.hpp>

#include <imguizmo/ImGuizmo.h>
#include <core/device.hpp>
#include <imgui/imgui.h>

bee::MainViewport::MainViewport()
{
    Engine.Renderer().SetScreenBlit(false);
	unsigned int* gl_buffer = reinterpret_cast<uint32_t*>(Engine.Renderer().GetOutputFramebuffer());
	renderer_colour_buffer = *gl_buffer;
}

void bee::MainViewport::Show(BlossomGame& game)
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

    bool open = true;
    ImGui::Begin(u8"\U0000f11b Game", &open, ImGuiWindowFlags_NoScrollbar);

    ImGuizmo::SetDrawlist(); //Sets Transform Guizmo to appear in this window

    float width = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();
    auto screenAspectRatio = (float)Engine.Device().GetHeight() / (float)Engine.Device().GetWidth();

    const auto lm = static_cast<size_t>(renderer_colour_buffer);
    const ImTextureID id = reinterpret_cast<void*>(lm);

    if (height / width < screenAspectRatio)
        width = height * 1.0f / screenAspectRatio;
    else
        height = width * screenAspectRatio;

    //Adjust input
    auto pos = ImGui::GetWindowPos();
    Engine.Input().SetGameAreaPosition(pos.x, pos.y);
    Engine.Input().SetGameAreaSize(width, height);

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::Image(id, ImVec2(width, height), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
 
    auto btnColor = colors[ImGuiCol_Button];
    btnColor.w *= 0.4f;
    const float UIScale = 1.0f;  // Game.Device().GetMonitorUIScale();
    const auto s = ImGui::GetIO().FontGlobalScale * UIScale;
    const ImVec2 btnSize(24.0f * s, 24.0f * s);
    ImGui::SetCursorPos(ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, btnColor);

    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}
