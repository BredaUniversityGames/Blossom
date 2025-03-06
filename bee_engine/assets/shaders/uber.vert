#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "locations.glsl"
#include "uniforms.glsl"
#include "constants.glsl"

layout (location = POSITION_LOCATION) in vec3 a_position;
layout (location = NORMAL_LOCATION) in vec3 a_normal;
layout (location = TANGENT_LOCATION) in vec4 a_tangent;
layout (location = DISPLACEMENT_LOCATION) in vec4 a_displacement;
layout (location = TEXTURE0_LOCATION) in vec2 a_texture0;
layout (location = TEXTURE1_LOCATION) in vec2 a_texture1;

out vec3 v_position;
out vec3 v_normal;
out mat3x3 v_TBN;
out vec4 v_displacement;
out vec2 v_texture0;
out vec2 v_texture1;

uniform sampler3D u_windNoise;

vec3 windMovement(vec3 position);

void main()
{       
    mat4 world = bee_transforms[gl_InstanceID].world;
    mat4 wvp = bee_projection * bee_view * world;

    vec3 pivotPoint = a_position - a_displacement.xyz;
    vec3 windVelocity = windMovement(pivotPoint) * a_displacement.w;
    vec3 adjustedPosition = windVelocity + a_position;
    vec3 newDelta = adjustedPosition - pivotPoint;
    adjustedPosition = max(length(a_displacement.xyz) / length(newDelta), 0.0) * newDelta + pivotPoint;

    mat3x3 normalMatrix = mat3x3(world);
    vec3 normal = normalize(normalMatrix * a_normal);
    vec3 tangent = normalize(normalMatrix * a_tangent.xyz);

    tangent = normalize(tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(tangent, normal);
    mat3x3 TBN = mat3x3(tangent, bitangent, normal);

    v_position = (world * vec4(adjustedPosition, 1.0)).xyz;
    v_normal = normalize((world * vec4(a_normal, 0.0)).xyz);
    v_TBN = TBN;
    v_displacement = a_displacement;
    v_texture0 = a_texture0;
    v_texture1 = a_texture1;
    gl_Position = wvp * vec4(adjustedPosition, 1.0);
}

vec3 windMovement(vec3 position) 
{
    vec2 scrollDirection = vec2(cos(ambientWind.direction + PI / 2), sin(ambientWind.direction + PI / 2));
    vec2 scroll = scrollDirection * bee_time * ambientWind.speed;
    
    vec2 windNoise = texture(u_windNoise, vec3(scroll, 0.0) + position).xy;
    windNoise = windNoise * 2 - 1;
    float angle = ambientWind.direction + (windNoise.x * 0.25);
    
	float strengthNoise = windNoise.y;
	float strength = (strengthNoise * 0.5 + 0.5) * ambientWind.strength + 0.5;

    vec3 direction = vec3(cos(angle), sin(angle), 0.0);

    return direction * strength;
}
