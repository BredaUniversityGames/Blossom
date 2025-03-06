#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "../locations.glsl"
#include "grass_locations.glsl"
#include "grass_structures.glsl"
#include "../uniforms.glsl"
#include "../matrices.glsl"
#include "../easings.glsl"

layout (location = GRASS_LOCATION) in vec3 a_position;
layout (location = TEXTURE0_LOCATION) in vec2 a_texture0;
layout (location = TEXTURE1_LOCATION) in vec2 a_texture1;

out vec3 v_position;
out vec3 v_color;
out vec3 v_normal;
out vec2 v_texture0; // Texture UV's
out vec2 v_texture1; // Chunk UV's; normalized coordinate in the chunk.
out vec2 v_terrainUv;
out float spawnChance;
out float v_lodLevel;

uniform mat4 u_world;
uniform mat4 u_terrainTransform; // Transforms grass space into terrain UV space.
uniform mat4 u_displacementTransform; // Transforms from world space into displacement uv space.

uniform float u_heightModifier;

uniform sampler2D u_noise;
uniform sampler3D u_windNoise;
uniform sampler2D u_heightMap;
uniform sampler2D u_lengthMap;
uniform sampler2D u_displacementMap;

uniform uint u_lodLevel;

vec3 windMovement(vec2 terrainUv);
vec3 computeNormal(vec2 uv);
vec3 bezier(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t);
vec3 bezierGrad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t);

float random(vec2 st) {
    // Dot product of the input with a large prime number vector
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{       
    // Set UVs; texture and chunk.
    v_texture0 = a_texture0;
    v_texture1 = instanceData[gl_InstanceID].chunkUv;
    vec4 grassBladeWorld = u_world * vec4(instanceData[gl_InstanceID].position.xyz, 1.0);
    v_terrainUv = (u_terrainTransform * grassBladeWorld).xy;

    // Add randomized yaw.
    vec4 noise = texture2D(u_noise, v_texture1 * 16.0);
    mat3 rotMatZ = rotationZMatrix(random(v_texture1 * 0.5) * PI * 2);
    
    vec3 wind = windMovement(v_texture1);

	// Determine where to sample the displacement map.
	vec2 displacementSampleUV = (u_displacementTransform * grassBladeWorld).xy;
	
	vec4 displacementSample = texture(u_displacementMap, displacementSampleUV);
	vec3 displacement = (displacementSample.xyz * 2 - 1); // From 0..1 to -1..1.

	displacement = displacement.yxz * 1.0; // Swizzle.
	displacement *= (noise.xyz * 2 - 1) + 1.0; // Apply some noise to make displacement less uniform.

    vec2 windForce = vec2(a_texture0.y * wind.y, a_texture0.y * wind.x);
    vec2 displacementForce = vec2(a_texture0.y * displacement.y, a_texture0.y * -displacement.x);

    vec2 pushForce = mix(windForce, displacementForce, easeOutSine(length(displacement)));
    
    // Rotation matrices, using vertical texture uv to increase strength over length.
    mat3 rotMatY = rotationYMatrix(pushForce.x);
    mat3 rotMatX = rotationXMatrix(displacementForce.y);
    mat3 rotation = rotMatZ * rotMatX * rotMatY;

    vec3 p0 = vec3(0.0);
    vec3 p1 = vec3(0.0, 0.333, 0.0);
    vec3 p2 = vec3(0.0, 0.666, 0.0);
    vec3 p3 = vec3(0.0, cos(material.bladeBending * windForce.x), sin(material.bladeBending * windForce.x));
    vec3 curve = bezier(p0, p1, p2, p3, v_texture0.y);

    vec3 curveGrad = bezierGrad(p0, p1, p2, p3, v_texture0.y);
    mat2 curveRot90 = mat2(0.0, 1.0, -1.0, 0.0);


    float height = mix(material.minHeight, material.maxHeight, random(v_texture1));
    float lengthSample = texture2D(u_lengthMap, v_terrainUv).x;
    spawnChance = lengthSample;
    height *= lengthSample;
    if(noise.x > lengthSample)
        height *= 0;

    vec3 scale = vec3(1.0, 1.0, height);
    scale.x = u_lodLevel > 0 ? ((u_lodLevel + 1) * (u_lodLevel + 1)) * 0.5 : 1.0;
    if(height < material.cutoffLength)
        scale = vec3(0.0);

    vec4 position = vec4(a_position, 1.0);
    position.zy = curve.yz;
    position = vec4(position.xyz * scale * rotation + instanceData[gl_InstanceID].position.xyz, 1.0);    
    
    v_position = vec3((u_world * position).xyz);

    // Apply height map sample.
    float heightMapSample = texture2D(u_heightMap, v_terrainUv).x * u_heightModifier;
    v_position.z += heightMapSample;

    // Blend normal over distance to reduce noise.
    vec3 normal = rotation * vec3(0.0, curveRot90 * curveGrad.zy);
    float distanceBlend = smoothstep(0.0, material.distanceNormalBlend, distance(bee_eyePos, v_position));
    normal = mix(normal, vec3(0.0, 0.0, 1.0), distanceBlend);
    normal = normalize(normal);

    v_normal = normalize((u_world * vec4(normal, 0.0)).xyz);

    mat4 vp = bee_projection * bee_view;
    gl_Position = vp * vec4(v_position, 1.0);
    
    v_color = vec3(1.0);
    v_lodLevel = material.lodsAdjustments[u_lodLevel];
    
}


vec3 bezier(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) 
{
    return (1.0 - t) * (1.0 - t) * (1.0 - t) * p0 +
            3.0 * (1.0 - t) * (1.0 - t) * t * p1 +
            3.0 * (1.0 - t) * t * t * p2 +
            t * t * t * p3;
}
vec3 bezierGrad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) 
{
    return 3.0 * (1.0 - t) * (1.0 - t) * (p1 - p0) +
           6.0 * (1.0 - t) * t * (p2 - p1) +
           3.0 * t * t * (p3 - p2);
}

vec3 windMovement(vec2 chunkUv) 
{
    vec2 scrollDirection = vec2(cos(ambientWind.direction + PI / 2), sin(ambientWind.direction + PI / 2));
    vec2 scroll = scrollDirection * bee_time * ambientWind.speed;
    
    vec2 windNoise = texture(u_windNoise, vec3(chunkUv + scroll, 0.0)).xy;
    windNoise = windNoise * 2 - 1;
    float angle = ambientWind.direction + (windNoise.x * 0.25);
    
	float strengthNoise = windNoise.y;
	float strength = (strengthNoise * 0.5 + 0.5) * ambientWind.strength + 0.5;

    vec3 direction = vec3(cos(angle), sin(angle), 0.0);

    return direction * strength;
}

vec3 computeNormal(vec2 uv)
{
    float depth = textureSize(u_heightMap, 0).x / 100;
    float width = textureSize(u_heightMap, 0).y / 100;
    float scale = 1;

    float samplepoint = 0.03f; 

	vec2 b = uv + vec2(0.0f, -samplepoint / depth);
    vec2 c = uv + vec2(samplepoint / width, -samplepoint / depth);
    vec2 d = uv + vec2(samplepoint / width, 0.0f);
    vec2 e = uv + vec2(samplepoint / width, samplepoint / depth);
    vec2 f = uv + vec2(0.0f, samplepoint / depth);
    vec2 g = uv + vec2(-samplepoint / width, samplepoint / depth);
    vec2 h = uv + vec2(-samplepoint / width, 0.0f);
    vec2 i = uv + vec2(-samplepoint / width, -samplepoint / depth);
 
    float zb = texture(u_heightMap, b).r * scale;
    float zc = texture(u_heightMap, c).r * scale;
    float zd = texture(u_heightMap, d).r * scale;
    float ze = texture(u_heightMap, e).r * scale;
    float zf = texture(u_heightMap, f).r * scale;
    float zg = texture(u_heightMap, g).r * scale;
    float zh = texture(u_heightMap, h).r * scale;
    float zi = texture(u_heightMap, i).r * scale;
 
    float x = zg + 2 * zh + zi - zc - 2 * zd - ze;
    float y = 2 * zb + zc + zi - ze - 2 * zf - zg;
    float z = 8.0f;
 
    return normalize(vec3(x, y, z));
}
