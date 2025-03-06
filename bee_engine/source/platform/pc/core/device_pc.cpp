#include <precompiled/engine_precompiled.hpp>
#include "platform/pc/core/device_pc.hpp"

#include <tinygltf/stb_image.h>

#include <code_utils/bee_utils.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "core/device.hpp"
#include "platform/opengl/open_gl.hpp"
#include "tools/log.hpp"

using namespace bee;

static void ErrorCallback(int error, const char* description) { fputs(description, stderr); }

void LogOpenGLVersionInfo()
{
    const auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const auto shaderVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    Log::Info("OpenGL Vendor {}", vendor);
    Log::Info("OpenGL Renderer {}", renderer);
    Log::Info("OpenGL Version {}", version);
    Log::Info("OpenGL Shader Version {}", shaderVersion);
}

Device::Device(Mode mode)
{
    if (!glfwInit())
    {
        Log::Critical("GLFW init failed");
        BEE_ASSERT(false);
        exit(EXIT_FAILURE);
    }

    Log::Info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(BEE_DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    if(mode == Mode::FULLSCREEN)
    {
        m_fullscreen = true;
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }
    else
    {
        m_fullscreen = false;
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }

    m_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(m_monitor);

    auto maxScreenWidth = videoMode->width;
    auto maxScreenHeight = videoMode->height;

    m_width = 1920;
    m_height = 1080;

    if (m_fullscreen)
    {
        m_width = maxScreenWidth;
        m_height = maxScreenHeight;
        m_window = glfwCreateWindow(m_width, m_height, "Blossom", nullptr, nullptr);
    }
    else
    {
        m_window = glfwCreateWindow(m_width, m_height, "Blossom", nullptr, nullptr);
    }

    if (!m_window)
    {
        Log::Critical("GLFW window could not be created");
        glfwTerminate();
        BEE_ASSERT(false);
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_window);

    m_vsync = true;
    if (!m_vsync) glfwSwapInterval(0);

    if (m_fullscreen) glfwSetWindowPos(m_window, 0, 0);

    int major = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MAJOR);
    int minor = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MINOR);
    int revision = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_REVISION);
    Log::Info("GLFW OpenGL context version {}.{}.{}", major, minor, revision);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        Log::Critical("GLAD failed to initialize OpenGL context");
        BEE_ASSERT(false);
        exit(EXIT_FAILURE);
    }

    //Load icon 

    {
        int width{}; int height{}; int comp{};
        const char* iconPath = "assets/icons/blossom128x128.png";
        stbi_uc* imageData = stbi_load(iconPath, &width, &height, &comp, 4);

        if (imageData) 
        {
            GLFWimage image{};
            image.height = height; image.width = width; image.pixels = imageData;
            glfwSetWindowIcon(m_window, 1, &image);
            stbi_image_free(imageData);
        }
        else
        {
            Log::Warn("Failed to set Window Icon: {}", iconPath);
        }
    }

    LogOpenGLVersionInfo();
    InitDebugMessages();
}

float bee::Device::GetMonitorUIScale() const
{ 
    float xscale, yscale;
    glfwGetMonitorContentScale(m_monitor, &xscale, &yscale);
    return xscale;
}

bee::Device::~Device() { glfwTerminate(); }

void bee::Device::Update()
{
    glfwPollEvents();
    glfwSwapBuffers(m_window);
}

bool bee::Device::ShouldClose() { return glfwWindowShouldClose(m_window); }

void bee::Device::CloseWindow() { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }

GLFWwindow* bee::Device::GetWindow() { return m_window; }
