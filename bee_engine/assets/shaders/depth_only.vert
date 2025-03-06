#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "locations.glsl"
#include "uniforms.glsl"

layout (location = POSITION_LOCATION) in vec3 a_position;
layout (location = TEXTURE0_LOCATION) in vec2 a_texture0;

out vec2 v_texture0;

void main()
{
    mat4 wvp = bee_viewProjection * bee_transforms[gl_InstanceID].world;
    gl_Position = wvp * vec4(a_position, 1.0);
    v_texture0 = a_texture0;
}