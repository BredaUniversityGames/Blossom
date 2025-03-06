#pragma once
#include "core/engine.hpp"

struct GLFWwindow;
struct GLFWmonitor;

namespace bee
{

class Device
{
public:
    Device(Mode mode);
    ~Device();

    bool ShouldClose();
    void CloseWindow();
    GLFWwindow* GetWindow();
    GLFWmonitor* GetMonitor();
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    void BeginFrame() {}
    void EndFrame() {}
    float GetMonitorUIScale() const;

private:
    friend class EngineClass;
    void Update();

    GLFWwindow* m_window = nullptr;
    GLFWmonitor* m_monitor = nullptr;
    bool m_vsync = true;
    bool m_fullscreen = false;
    int m_width = -1;
    int m_height = -1;
};

}  // namespace bee
