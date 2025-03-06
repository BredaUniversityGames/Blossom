#version 460 core

#include "../locations.glsl"
#include "../uniforms.glsl"

layout (vertices = 3) out;

uniform float u_tessDist;
uniform float u_tessFactorMax;
uniform vec4 u_eyePos;

in vec2 v_uvs[];
out vec2 uvsCoords[];

float ComputeEdgeTessFactor(vec4 cameraPos, vec4 edgeMidPos, float tessDist, float tessFactorMax)
{
	float dist = distance(cameraPos, edgeMidPos);

	float tessScale = 1 - dist / tessDist;

	float tessFactor = tessScale * (tessFactorMax-1) + 1;
	tessFactor = clamp(tessFactor, 1, tessFactorMax);

	return tessFactor;
}

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	uvsCoords[gl_InvocationID] = v_uvs[gl_InvocationID];


	vec4 pos0 = gl_in[0].gl_Position; 
	vec4 pos1 = gl_in[1].gl_Position; 
	vec4 pos2 = gl_in[2].gl_Position; 

	vec4 edgeMidPos0 = (pos1 + pos2) / 2;   
	vec4 edgeMidPos1 = (pos2 + pos0) / 2;   
	vec4 edgeMidPos2 = (pos0 + pos1) / 2;

	vec4 center = (pos0 + pos1 + pos2) / 3;

	gl_TessLevelOuter[0] = ComputeEdgeTessFactor(u_eyePos, edgeMidPos0, u_tessDist, u_tessFactorMax);
	gl_TessLevelOuter[1] = ComputeEdgeTessFactor(u_eyePos, edgeMidPos1, u_tessDist, u_tessFactorMax);
	gl_TessLevelOuter[2] = ComputeEdgeTessFactor(u_eyePos, edgeMidPos2, u_tessDist, u_tessFactorMax);

	gl_TessLevelInner[0] = ComputeEdgeTessFactor(u_eyePos, center, u_tessDist, u_tessFactorMax);

}
