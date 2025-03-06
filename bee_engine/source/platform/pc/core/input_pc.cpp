#include <precompiled/engine_precompiled.hpp>
#include <GLFW/glfw3.h>

#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"

#include <tools/log.hpp>

using namespace bee;

class Input::Impl {};

enum KeyAction
{
    Release = 0,
    Press = 1,
    None = 2
};

bool keyboard_mouse_active;
bool controller_active;

constexpr int nr_keys = 350;
bool keys_down[nr_keys];
bool prev_keys_down[nr_keys];
KeyAction keys_action[nr_keys];

constexpr int nr_mousebuttons = 8;
bool mousebuttons_down[nr_mousebuttons];
bool prev_mousebuttons_down[nr_mousebuttons];
KeyAction mousebuttons_action[nr_mousebuttons];

constexpr int max_nr_gamepads = 4;
bool gamepad_connected[max_nr_gamepads];
GLFWgamepadstate gamepad_state[max_nr_gamepads];
GLFWgamepadstate prev_gamepad_state[max_nr_gamepads];
float deadzone_epsilon = 0.08f;

glm::vec2 prevMousePos;
glm::vec2 currentMousePos;

glm::vec2 mousepos;
float mousewheel = 0;

float viewport_x{}, viewport_y{};
float viewport_width{}, viewport_height{};

void bee::Input::SetGameAreaPosition(float x, float y)
{
    viewport_x = x; viewport_y = y;
}

void bee::Input::SetGameAreaSize(float width, float height)
{
    viewport_width = width; viewport_height = height;
}

glm::vec2 bee::Input::GetGameAreaPosition() const
{
    return { viewport_x, viewport_y };
}

glm::vec2 bee::Input::GetGameAreaSize() const
{
    return { viewport_width, viewport_height };
}


bool GamepadDataEqual(GLFWgamepadstate first, GLFWgamepadstate other)
{
    (int)Input::GamepadButton::DPadLeft + 1;

    for (int i = 0; i < (int)Input::GamepadButton::DPadLeft + 1; i += 1)
    {
        if (first.buttons[i] != other.buttons[i])
            return false;
    }

    for (int i = 0; i < (int)Input::GamepadAxis::TriggerRight + 1; i += 1)
    {
        if (first.axes[i] != other.axes[i])
            return false;
    }

    return true;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mousepos.x = (float)xpos;
    mousepos.y = (float)ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { mousewheel += (float)yoffset; }

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) keys_action[key] = static_cast<KeyAction>(action);
}

void mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) mousebuttons_action[button] = static_cast<KeyAction>(action);
}

Input::Input()
{
    GLFWwindow* window = static_cast<GLFWwindow*>(Engine.Device().GetWindow());

    // glfwSetJoystickCallback(joystick_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(Engine.Device().GetWindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    Update();

    SetGameAreaPosition(0.0f, 0.0f);
    SetGameAreaSize(static_cast<float>(Engine.Device().GetWidth()), static_cast<float>(Engine.Device().GetHeight()));
    Log::Info("{}, {}", Engine.Device().GetWidth(), Engine.Device().GetHeight());

    prevMousePos = mousepos;
    currentMousePos = mousepos;

}

Input::~Input()
{
    GLFWwindow* window = static_cast<GLFWwindow*>(Engine.Device().GetWindow());

    // glfwSetJoystickCallback(NULL);
    glfwSetCursorPosCallback(window, NULL);
}

void Input::Update()
{
    bool keyboard_mouse_updated = false;
    bool controller_updated = false;

    // update keyboard key states
    for (int i = 0; i < nr_keys; ++i)
    {
        prev_keys_down[i] = keys_down[i];

        if (keys_action[i] == KeyAction::Press)
            keys_down[i] = true;
        else if (keys_action[i] == KeyAction::Release)
            keys_down[i] = false;

        keyboard_mouse_updated = prev_keys_down[i] != keys_down[i] || keyboard_mouse_updated;

        keys_action[i] = KeyAction::None;
    }

    // update mouse button states
    for (int i = 0; i < nr_mousebuttons; ++i)
    {
        prev_mousebuttons_down[i] = mousebuttons_down[i];

        if (mousebuttons_action[i] == KeyAction::Press)
            mousebuttons_down[i] = true;
        else if (mousebuttons_action[i] == KeyAction::Release)
            mousebuttons_down[i] = false;

        keyboard_mouse_updated = prev_mousebuttons_down[i] != mousebuttons_down[i] || keyboard_mouse_updated;

        mousebuttons_action[i] = KeyAction::None;
    }

    // update gamepad states
    for (int i = 0; i < max_nr_gamepads; ++i)
    {
        prev_gamepad_state[i] = gamepad_state[i];

        if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i))
        {
            gamepad_connected[i] = static_cast<bool>(glfwGetGamepadState(i, &gamepad_state[i]));
            for (int axisID = 0; axisID < (int)GamepadAxis::TriggerLeft; axisID += 1)
            {
               if (abs(gamepad_state[i].axes[axisID]) <= deadzone_epsilon)
                   gamepad_state[i].axes[axisID] = 0.0f;
            }
        }

        controller_updated = !GamepadDataEqual(prev_gamepad_state[i], gamepad_state[i]) || controller_updated;
    }

    prevMousePos = currentMousePos;
    currentMousePos = mousepos;

    keyboard_mouse_updated = prevMousePos != currentMousePos || keyboard_mouse_updated;

    keyboard_mouse_active = (keyboard_mouse_updated || keyboard_mouse_active) && !controller_updated;
    controller_active = (controller_updated || controller_active) && !keyboard_mouse_updated;

    assert(!(keyboard_mouse_active == true && controller_active == true));
}

bool Input::IsGamepadAvailable(int gamepadID) const { return gamepad_connected[gamepadID]; }

bool Input::IsGamepadActive() const { return controller_active; }

bool Input::IsKeyboardMouseActive() const { return keyboard_mouse_active; }

float Input::GetGamepadAxis(int gamepadID, GamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return gamepad_state[gamepadID].axes[a];
}

float Input::GetGamepadAxisPrevious(int gamepadID, GamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return prev_gamepad_state[gamepadID].axes[a];
}

bool Input::GetGamepadButton(int gamepadID, GamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);
    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::GetGamepadButtonOnce(int gamepadID, GamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);

    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return !static_cast<bool>(prev_gamepad_state[gamepadID].buttons[b]) &&
           static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::IsMouseAvailable() const { return true; }

bool Input::GetMouseButton(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b];
}

bool Input::GetMouseButtonOnce(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b] && !prev_mousebuttons_down[b];
}

glm::vec2 Input::GetMousePosition() const { return mousepos; }

glm::vec2 Input::GetProjectedMousePosition() const {

    auto ratio = viewport_width / viewport_height;
    auto size = glm::vec2(viewport_width, viewport_height);
    auto position = GetMousePosition() - glm::vec2(viewport_x, viewport_y);

    auto projected = ((position / size) * 2.0f) - 1.0f;
    projected.x *= ratio;
    projected.y *= -1.0f;

    return projected;
}

glm::vec2 Input::GetMouseMovement() const { return currentMousePos - prevMousePos; }

void Input::EnableCursor() const 
{ 
    glfwSetInputMode(Engine.Device().GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Input::DisableCursor() const 
{ 
    glfwSetInputMode(Engine.Device().GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

float Input::GetMouseWheel() const { return mousewheel; }

bool Input::IsKeyboardAvailable() const { return true; }

bool Input::GetKeyboardKey(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k];
}

bool Input::GetKeyboardKeyOnce(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k] && !prev_keys_down[k];
}