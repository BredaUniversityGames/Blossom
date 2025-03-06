#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "locations.glsl"

in vec2 v_texture0;

layout(location = BASE_COLOR_SAMPLER_LOCATION) uniform sampler2D s_base_color;

uniform vec3 color;

out vec4 frag_color;

void main()
{
	vec4 color = pow(texture(s_base_color, v_texture0), vec4(2.2f)) * vec4(color, 1.0f);

	frag_color = color;
}