#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "../locations.glsl"
#include "../uniforms.glsl"

layout (location = POSITION_LOCATION) in vec3 a_position;
layout (location = TEXTURE0_LOCATION) in vec2 a_texture0;

out vec2 v_uvs;

void main()
{       
    v_uvs = a_texture0;
    gl_Position = vec4(a_position, 1.0);
}
