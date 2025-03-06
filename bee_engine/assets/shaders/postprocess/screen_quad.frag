#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(location = 1) uniform sampler2D tex;

void main()
{
	vec3 texCol = texture(tex, TexCoords).rgb;
	FragColor = vec4(texCol, 1.0);
}