#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "../easings.glsl"
#include "../locations.glsl"
#include "../uniforms.glsl"
#include "grass_locations.glsl"
#include "grass_structures.glsl"

#define saturate(value) clamp(value, 0.0, 1.0)

in vec3 v_position;
in vec3 v_color;
in vec3 v_normal;
in vec2 v_texture0;
in vec2 v_texture1;
in vec2 v_terrainUv;
in float spawnChance;
in float v_lodLevel;

layout(location = 0) out vec4 fragColor;
layout(location = 2) out vec4 wsPositionOut;
layout(location = 3) out vec4 normalOut;


layout(location = COLOR_MAP_TEXTURE_UNIT) uniform sampler2D u_colorMap;
layout(location = SPECULAR_SAMPER_LOCATION)    uniform samplerCube s_ibl_specular;
layout(location = SHADOWMAP_LOCATION) uniform sampler2DArray[4] s_shadowmaps;

uniform float u_dither_distance = 2.0;

uniform vec2 u_tiling = vec2(1.0);
uniform vec2 u_offset = vec2(0.0);

const float c_zbias_min = 0.0000001f;
const float c_zbias_max = 0.0001f;
const float c_gamma = 2.2;
const float c_inv_gamma = 1.0 / c_gamma;
const float u_EnvIntensity = 1.0;
const float c_point_light_tweak = 1.0 / 300.0;
const float c_dir_light_tweak = 1.0 / 200.0;

const mat4 bayerMatrix = mat4(
    0.0, 12.0, 3.0, 15.0,
    8.0, 4.0, 11.0, 7.0,
    2.0, 14.0, 1.0, 13.0,
    10.0, 6.0, 9.0, 5.0
);


float CalculateShadow(vec3 position, vec3 normal, uint lightIndex);
void ApplyFog(inout vec3 pixel_color, in vec3 fog_color, float fog_near, float fog_far, float distance);
vec4 get_specular_sample_ibl(vec3 reflection, float lod);

void main() 
{
	vec4 diffuseColor = vec4(0.0, 0.0, 0.0, 1.0);
	vec2 texCoord = v_terrainUv * u_tiling + u_offset;
	
	vec3 normal = normalize(v_normal);
    
	vec3 V = bee_eyePos - v_position;
    float distance = length(V);

	// Dithering
    float normalizedDistance = clamp(distance / u_dither_distance, 0.0, 1.0);

    // Calculate screen coordinates in 4x4 grid
    ivec2 screenPos = ivec2(gl_FragCoord.xy) % 4;
    
    // Determine the threshold from the Bayer matrix
    float threshold = bayerMatrix[screenPos.x][screenPos.y] / 16.0;

    // Compare the normalized distance with the threshold
    if (normalizedDistance < threshold) {
        discard; // Discard the fragment
    }

	for (int i = 0; i < bee_directionalLightsCount; i++)
	{
		vec3 dif = vec3(0.0);
        vec3 direction = bee_directional_lights[i].direction;
        vec4 color_intensity = vec4(bee_directional_lights[i].color,
                                    bee_directional_lights[i].intensity / 1000.0);

		float intensity = abs(dot(normal, normalize(direction)));
		
		// Remap intensity from [0, 1] to [0.25, 1]
		// This increases the ambient 
		intensity = 1 - (intensity * 0.75);

		// subsurface 
		vec3 h = normalize(direction + normal * material.SSS_distortion);
        float I = pow(saturate(dot(normalize(V), -h)), material.SSS_power) * material.SSS_strength;
        dif += color_intensity.rgb * intensity * I;    

		dif += intensity * color_intensity.rgb;
		dif *= color_intensity.a;

		float shadow = CalculateShadow(v_position, normal, i);

        dif *= clamp((1.0 - shadow), 0.25, 1.0);

		// Apply the intensity to the diffuse color
		diffuseColor += vec4(dif, 1.0);
	}

	vec4 terrainColor = pow(texture2D(u_colorMap, texCoord), vec4(2.2));
	terrainColor *= v_lodLevel;
	terrainColor = mix(terrainColor * (1.0 - material.darkEdgeStrength), terrainColor, smoothstep(1.0, 0.0, abs(v_texture0.x - 0.5) * material.darkEdgeRange));
	diffuseColor *= terrainColor;

	float ao = min(v_texture0.y * (1.0 / material.aoRange), 1.0);
	ao = easeOutCubic(min((1.0 - ao) * material.aoStrength * spawnChance, 1.0));
	ao = 1.0 - ao;

	diffuseColor *= ao;

	//fragColor = vec4(v_texture1, 0.0, 1.0);
	fragColor = (diffuseColor * (1.0 - bee_ambientFactor)) + (bee_ambientFactor * terrainColor);

    if(bee_FogColor.w > 0.0)
        ApplyFog(fragColor.rgb, get_specular_sample_ibl(-V, 1).rgb, bee_FogNear, bee_FogFar, distance);

	wsPositionOut = vec4(v_position, 1.0f);
	normalOut = vec4(normal, 1.0f);
}

vec4 get_specular_sample_ibl(vec3 reflection, float lod)
{
    return textureLod(s_ibl_specular, reflection, lod) * u_EnvIntensity; // TODO: Add this
}

float CalculateShadow(vec3 position, vec3 normal, uint lightIndex)
{
	vec4 frag_pos_view = bee_view * vec4(position, 1.0);
	float depthValue = abs(frag_pos_view.z);

	int cascade_index = -1;
	for (int j = 0; j < bee_cascadeCount; ++j)
	{
		if (depthValue < bee_cascadePlaneDistances[j].x)
		{
			cascade_index = j;
			break;
		}
	}
	if (cascade_index == -1)
		cascade_index = bee_cascadeCount;

	vec4 pos_light_coor = bee_directional_lights[lightIndex].shadow_matrices[cascade_index] * vec4(position, 1.0);
	vec3 proj_coords = pos_light_coor.xyz / pos_light_coor.w;
	proj_coords = proj_coords * 0.5f + 0.5f;

	if (proj_coords.z > 1.0)
	{
		return 0.0;
	}

	float bias = max(c_zbias_max * (1.0 - dot(normal, bee_directional_lights[lightIndex].direction)), c_zbias_min);
	if (cascade_index == bee_cascadeCount)
	{
		bias *= 1 / (bee_farPlane * 0.5f);
	}
	else
	{
		bias *= 1 / (bee_cascadePlaneDistances[cascade_index].x * 0.5f);
	}

	float shadow = 0.f;
	vec2 texelSize = 1.0 / vec2(textureSize(s_shadowmaps[lightIndex], 0));
//	for(int x = -1; x <= 1; ++x)
//	{
//		for(int y = -1; y <= 1; ++y)
//		{
//			
//		}
//	}
	float pcfDepth = texture(s_shadowmaps[lightIndex], vec3(proj_coords.xy, cascade_index)).r;
	shadow += (proj_coords.z - bias) > pcfDepth ? 1.0 : 0.0;

	if(proj_coords.z > 1.f)
	{
		shadow = 0.f;
	}
	return shadow;
}

void ApplyFog(inout vec3 pixel_color, in vec3 fog_color, float fog_near, float fog_far, float distance)
{
    float fog_amount = clamp((distance - fog_near) / (fog_far - fog_near), 0.0, 1.0);
    pixel_color = mix(pixel_color, fog_color, fog_amount);
}
