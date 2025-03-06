#pragma once
#include <string>

namespace bee {
namespace ImGuiImpl {


void InitializeImGui();
void SetImGuiFont(const std::string& path);
void SetImGuiStyle();

void StartFrame();
void EndFrame();

void ShutdownImGui();

}
}