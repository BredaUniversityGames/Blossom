#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "../locations.glsl"
#include "../uniforms.glsl"

layout (triangles, fractional_odd_spacing, ccw) in;
in vec2 uvsCoords[];

uniform sampler2D s_heightmap;

uniform mat4 u_model;
uniform float u_heightModifier;
uniform float u_normalScale;
uniform vec2 u_terrainSize;

out vec3 v_position;
out vec3 v_normal;
out mat3x3 v_TBN;
out vec4 v_displacement;
out vec2 v_texture0;
out vec2 v_texture1;

vec3 computeNormal(vec2 uv)
{
    float depth = textureSize(s_heightmap, 0).x / u_terrainSize.x;
    float width = textureSize(s_heightmap, 0).y / u_terrainSize.y;
    float scale = u_heightModifier * u_normalScale;

    float samplepoint = 0.03f; 

	vec2 b = uv + vec2(0.0f, -samplepoint / depth);
    vec2 c = uv + vec2(samplepoint / width, -samplepoint / depth);
    vec2 d = uv + vec2(samplepoint / width, 0.0f);
    vec2 e = uv + vec2(samplepoint / width, samplepoint / depth);
    vec2 f = uv + vec2(0.0f, samplepoint / depth);
    vec2 g = uv + vec2(-samplepoint / width, samplepoint / depth);
    vec2 h = uv + vec2(-samplepoint / width, 0.0f);
    vec2 i = uv + vec2(-samplepoint / width, -samplepoint / depth);
 
    float zb = texture(s_heightmap, b).r * scale;
    float zc = texture(s_heightmap, c).r * scale;
    float zd = texture(s_heightmap, d).r * scale;
    float ze = texture(s_heightmap, e).r * scale;
    float zf = texture(s_heightmap, f).r * scale;
    float zg = texture(s_heightmap, g).r * scale;
    float zh = texture(s_heightmap, h).r * scale;
    float zi = texture(s_heightmap, i).r * scale;
 
    float x = zg + 2 * zh + zi - zc - 2 * zd - ze;
    float y = 2 * zb + zc + zi - ze - 2 * zf - zg;
    float z = 8.0f;
 
    return normalize(vec3(x, y, z));
}

void main()
{
	float u = gl_TessCoord.x; 
	float v = gl_TessCoord.y;
	float w = gl_TessCoord.z;

	vec2 texCoord = u * uvsCoords[0] + v * uvsCoords[1] + w * uvsCoords[2];
	float height = texture(s_heightmap, texCoord).r * u_heightModifier; 


	vec4 pos0 = gl_in[0].gl_Position;
	vec4 pos1 = gl_in[1].gl_Position;
	vec4 pos2 = gl_in[2].gl_Position;

	vec4 interpolatedPos = u * pos0 + v * pos1 + w * pos2;
    interpolatedPos.z = height;

    vec3 interpolatedNormal = computeNormal(texCoord);

    // Tangents 
    vec4 edge0 = pos1 - pos0;
    vec4 edge1 = pos2 - pos0;
    vec2 deltaUV0 = uvsCoords[1] - uvsCoords[0];
    vec2 deltaUV1 = uvsCoords[2] - uvsCoords[0];

    float invDet = 1.0f / (deltaUV0.x  * deltaUV1.y - deltaUV1.x  * deltaUV0.y);

    vec3 tangentApprox = vec3(invDet * (deltaUV1.y * edge0 - deltaUV0.y * edge1));

	mat4 world = u_model;
    mat4 wvp = bee_viewProjection * world;

    mat3x3 normalMatrix = mat3x3(world);
    vec3 normal = normalize(normalMatrix * interpolatedNormal);
    vec3 tangent = normalize(normalMatrix * tangentApprox);

    tangent = normalize(tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(tangent, normal);
    mat3x3 TBN = mat3x3(tangent, bitangent, normal);

    v_position = (world * interpolatedPos).xyz;
	v_normal = normalize((world * vec4(computeNormal(texCoord), 0.0)).xyz);
    v_TBN = TBN;
    v_displacement = vec4(0.0f);
    v_texture0 = texCoord;
    v_texture1 = vec2(0);

	gl_Position = wvp * interpolatedPos;
}
