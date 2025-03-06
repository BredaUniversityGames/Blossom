#version 460 core
#extension GL_GOOGLE_include_directive : require

// This shader is not currently used 

out vec4 FragColor;

in vec3 normals;
in vec2 uvs;
uniform sampler2D tex;


void main()
{
	//FragColor = vec4(normals, 1);
	FragColor = pow(texture(tex, uvs), vec4(2.2));
}
