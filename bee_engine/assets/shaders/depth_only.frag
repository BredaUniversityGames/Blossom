#version 460 core

#include "locations.glsl"

layout(location = BASE_COLOR_SAMPLER_LOCATION) uniform sampler2D s_base_color;

in vec2 v_texture0;

void main() 
{
	vec4 albedo = texture(s_base_color, v_texture0);
	if(albedo.a < 0.2)
		discard;
} 