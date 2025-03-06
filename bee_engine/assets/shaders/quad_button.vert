#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "locations.glsl"

layout (location = POSITION_LOCATION) in vec3 a_position;
layout (location = TEXTURE0_LOCATION) in vec2 a_texture0;

uniform vec2 button_position;
uniform vec2 button_scale;
uniform mat4 camera_projection;

out vec2 v_texture0;

void main()
{
    vec3 v_position;
    v_position = vec3(button_scale, 1.0) * a_position;
    v_position = v_position + vec3(button_position, 0.0);
    v_position = (camera_projection * vec4(v_position, 1.0)).xyz;
    v_texture0 = a_texture0;
    gl_Position = vec4(v_position, 1.0);
}
