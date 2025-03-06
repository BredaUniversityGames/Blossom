#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(location = 0) uniform sampler2D scene;
layout(location = 1) uniform sampler2D bloomBlur;

uniform float u_weight;

void main()
{
    vec3 result = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    result = mix(result, bloomColor, u_weight);

    FragColor = vec4(result, 1.0);
}