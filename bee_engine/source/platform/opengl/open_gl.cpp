#include <precompiled/engine_precompiled.hpp>

#ifdef BEE_PLATFORM_PC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // PLATFORM_PC

#include <platform/opengl/open_gl.hpp>
#include <string>
#include <code_utils/bee_utils.hpp>
#include <tools/log.hpp>
#include <GLFW/glfw3.h>

using namespace bee;

static void APIENTRY DebugCallbackFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                       const GLchar* message, const GLvoid* userParam)
{
    // Skip some less useful info
    if (id == 131218)  // http://stackoverflow.com/questions/12004396/opengl-debug-context-performance-warning
        return;

    // UNUSED(length);
    // UNUSED(userParam);
    std::string sourceString;
    std::string typeString;
    std::string severityString;

    // The AMD variant of this extension provides a less detailed classification of the error,
    // which is why some arguments might be "Unknown".
    switch (source)
    {
        case GL_DEBUG_CATEGORY_API_ERROR_AMD:
        case GL_DEBUG_SOURCE_API:
        {
            sourceString = "API";
            break;
        }
        case GL_DEBUG_CATEGORY_APPLICATION_AMD:
        case GL_DEBUG_SOURCE_APPLICATION:
        {
            sourceString = "Application";
            break;
        }
        case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        {
            sourceString = "Window System";
            break;
        }
        case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        {
            sourceString = "Shader Compiler";
            break;
        }
        case GL_DEBUG_SOURCE_THIRD_PARTY:
        {
            sourceString = "Third Party";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_SOURCE_OTHER:
        {
            sourceString = "Other";
            break;
        }
        default:
        {
            sourceString = "Unknown";
            break;
        }
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
        {
            typeString = "Error";
            break;
        }
        case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        {
            typeString = "Deprecated Behavior";
            break;
        }
        case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        {
            typeString = "Undefined Behavior";
            break;
        }
        case GL_DEBUG_TYPE_PORTABILITY_ARB:
        {
            typeString = "Portability";
            break;
        }
        case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
        case GL_DEBUG_TYPE_PERFORMANCE:
        {
            typeString = "Performance";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_TYPE_OTHER:
        {
            typeString = "Other";
            break;
        }
        default:
        {
            typeString = "Unknown";
            break;
        }
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
        {
            severityString = "High";
            break;
        }
        case GL_DEBUG_SEVERITY_MEDIUM:
        {
            severityString = "Medium";
            break;
        }
        case GL_DEBUG_SEVERITY_LOW:
        {
            severityString = "Low";
            break;
        }
        default:
        {
            severityString = "Unknown";
            return;
        }
    }

    Log::Warn("GL Debug Callback:\n source: {}:{} \n type: {}:{} \n id: {} \n severity: {}:{} \n  message: {}", source,
                 sourceString.c_str(), type, typeString.c_str(), id, severity, severityString.c_str(), message);
    BEE_ASSERT(type != GL_DEBUG_TYPE_ERROR);
}

// Renders a 1x1 XY quad in NDC
void bee::RenderQuad()
{
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO = 0;

    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture coordinates
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// RenderCube() renders a 1x1 3D cube in NDC.
void bee::RenderCube()
{
    static GLuint cubeVAO = 0;
    static GLuint cubeVBO = 0;
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // top-right
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,   // bottom-right
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // top-right
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,   // top-left
            // front face
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // bottom-right
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // top-left
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
            // left face
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-right
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top-left
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // bottom-right
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-right
            // right face
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-left
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top-right
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,    // top-left
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // top-right
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,   // top-left
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,    // bottom-left
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,    // bottom-left
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom-right
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // top-right
            // top face
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top-left
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,    // bottom-right
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top-right
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,    // bottom-right
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top-left
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f    // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void bee::LabelGL(GLenum type, GLuint name, const std::string& label)
{
    std::string typeString;
    switch (type)
    {
        case GL_BUFFER:
             typeString = "GL_BUFFER";
            break;
        case GL_SHADER:
            typeString = "GL_SHADER";
            break;
        case GL_PROGRAM:
            typeString = "GL_PROGRAM";
            break;
        case GL_VERTEX_ARRAY:
            typeString = "GL_VERTEX_ARRAY";
            break;
        case GL_QUERY:
            typeString = "GL_QUERY";
            break;
        case GL_PROGRAM_PIPELINE:
            typeString = "GL_PROGRAM_PIPELINE";
            break;
        case GL_TRANSFORM_FEEDBACK:
            typeString = "GL_TRANSFORM_FEEDBACK";
            break;
        case GL_SAMPLER:
            typeString = "GL_SAMPLER";
            break;
        case GL_TEXTURE:
            typeString = "GL_TEXTURE";
            break;
        case GL_RENDERBUFFER:
            typeString = "GL_RENDERBUFFER";
            break;
        case GL_FRAMEBUFFER:
            typeString = "GL_FRAMEBUFFER";
            break;
        default:
            typeString = "UNKNOWN";
            break;
    }

    const std::string temp = 
        "[" + typeString + ":" + std::to_string(name) + "] "
        + label.substr(label.find_last_of("/") + 1);

    glObjectLabel(type, name, static_cast<GLsizei>(temp.length()), temp.c_str());
}

void bee::PushDebugGL(std::string_view message)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, message.data());
}

void bee::PopDebugGL()
{
    glPopDebugGroup();
}


#if defined(BEE_PLATFORM_PC) && defined(BEE_DEBUG)
static void APIENTRY DebugCallbackFuncAMD(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message,
                                          void* userParam)
{
    DebugCallbackFunc(GL_DEBUG_CATEGORY_API_ERROR_AMD, category, id, severity, length, message, userParam);
}

void bee::InitDebugMessages()
{
    // Query the OpenGL function to register your callback function.
    PFNGLDEBUGMESSAGECALLBACKPROC _glDebugMessageCallback =
        (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
    PFNGLDEBUGMESSAGECALLBACKARBPROC _glDebugMessageCallbackARB =
        (PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
    PFNGLDEBUGMESSAGECALLBACKAMDPROC _glDebugMessageCallbackAMD =
        (PFNGLDEBUGMESSAGECALLBACKAMDPROC)wglGetProcAddress("glDebugMessageCallbackAMD");

    glDebugMessageCallback(DebugCallbackFunc, nullptr);

    // Register your callback function.
    if (_glDebugMessageCallback != nullptr)
    {
        _glDebugMessageCallback(DebugCallbackFunc, nullptr);
    }
    else if (_glDebugMessageCallbackARB != nullptr)
    {
        _glDebugMessageCallbackARB(DebugCallbackFunc, nullptr);
    }

    // Additional AMD support
    if (_glDebugMessageCallbackAMD != nullptr)
    {
        _glDebugMessageCallbackAMD(DebugCallbackFuncAMD, nullptr);
    }

    // Enable synchronous callback. This ensures that your callback function is called
    // right after an error has occurred. This capability is not defined in the AMD
    // version.
    if ((_glDebugMessageCallback != nullptr) || (_glDebugMessageCallbackARB != nullptr))
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

#elif defined(BEE_PLATFORM_SWITCH) && defined(DEBUG)

void InitDebugMessages() { glDebugMessageCallback(DebugCallbackFunc, NULL); }

#endif