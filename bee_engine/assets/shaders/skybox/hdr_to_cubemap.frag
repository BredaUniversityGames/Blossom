#version 460 core

#include "../constants.glsl"

out vec4 frag_color;

in vec2 texCoord;

uniform sampler2D u_hdr;
uniform int u_face;

vec3 uvToXYZ(int face, vec2 uv);
vec2 dirToUV(vec3 dir);
vec3 panoramaToCubeMap(int face, vec2 texCoord);

void main()
{    
    frag_color = vec4(panoramaToCubeMap(u_face, texCoord), 1.0);
}

vec3 uvToXYZ(int face, vec2 uv)
{
	if(face == 0)
		return vec3(+1.0, +uv.x, +uv.y);

	else if(face == 1)
		return vec3(-1.0, -uv.x, +uv.y);

	else if(face == 2)
		return vec3(+uv.x, -uv.y, -1.0);

	else if(face == 3)
		return vec3(+uv.x, +uv.y, +1.0);

	else if(face == 4)
		return vec3(+uv.x, -1.0, +uv.y);

	else if(face == 5)
		return vec3(-uv.x, +1.0, +uv.y);
}

vec2 dirToUV(vec3 dir)
{
	return vec2(
		0.5f + 0.5f * atan(dir.z, dir.x) / PI,
		1.f - acos(dir.y) / PI);
}

vec3 panoramaToCubeMap(int face, vec2 texCoord)
{
	vec2 texCoordNew = texCoord*2.0-1.0; //< mapping vom 0,1 to -1,1 coords
	vec3 scan = uvToXYZ(face, texCoordNew); 
	vec3 direction = normalize(scan);
	vec2 src = dirToUV(direction);

	return  texture(u_hdr, src).rgb; //< get the color from the panorama
}
