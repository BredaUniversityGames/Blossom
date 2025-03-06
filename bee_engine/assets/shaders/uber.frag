#version 460 core
#extension GL_GOOGLE_include_directive : require
#define PI 3.14159265359 

#include "locations.glsl"
#include "uniforms.glsl"

in vec3 v_position;
in vec3 v_normal;
in mat3x3 v_TBN;
in vec4 v_displacement;
in vec2 v_texture0;
in vec2 v_texture1;

layout(location = BASE_COLOR_SAMPLER_LOCATION) uniform sampler2D s_base_color;
layout(location = NORMAL_SAMPLER_LOCATION)     uniform sampler2D s_normal;
layout(location = EMISSIVE_SAMPLER_LOCATION)   uniform sampler2D s_emissive;
layout(location = ORM_SAMPLER_LOCATION)        uniform sampler2D s_orm;
layout(location = OCCLUSION_SAMPLER_LOCATION)  uniform sampler2D s_occulsion;
layout(location = SUBSURFACE_SAMPLER_LOCATION) uniform sampler2D s_subsurface;
layout(location = LUT_SAMPER_LOCATION)         uniform sampler2D s_ibl_lut;
layout(location = TOON_SAMPLER_LOCATION)       uniform sampler2D s_toon;
layout(location = DIFFUSE_SAMPER_LOCATION)     uniform samplerCube s_ibl_diffuse;
layout(location = SPECULAR_SAMPER_LOCATION)    uniform samplerCube s_ibl_specular;
layout(location = SHADOWMAP_LOCATION)          uniform sampler2DArray[4] s_shadowmaps;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 bright_color;
layout(location = 2) out vec4 wsPositionOut;
layout(location = 3) out vec4 normalOut;

uniform bool is_unlit;
uniform vec2 u_resolution;
uniform bool u_receive_shadows;
uniform bool u_do_toon_shading;
uniform uint u_toon_palette_index;
uniform int u_ibl_specular_mip_count;
uniform bool u_is_ditherable;
uniform float u_dither_distance = 2.0;
uniform bool u_double_sided; 

uniform bool use_base_texture;
uniform bool use_metallic_roughness_texture;
uniform bool use_emissive_texture;
uniform bool use_normal_texture;
uniform bool use_occlusion_texture;
uniform bool use_subsurface_texture;
uniform bool use_alpha_blending;

uniform vec4 base_color_factor;
uniform float metallic_factor;
uniform float roughness_factor;
uniform float subsurface_factor;

uniform float u_SSS_strength;
uniform float u_SSS_distortion;
uniform float u_SSS_power;

uniform bool debug_base_color;
uniform bool debug_normals;
uniform bool debug_normal_map;
uniform bool debug_metallic;
uniform bool debug_roughness;
uniform bool debug_emissive;
uniform bool debug_occlusion;
uniform bool debug_displacement_pivot;
uniform bool debug_wind_mask;

const float c_zbias_min = 0.0000001f;
const float c_zbias_max = 0.0001f;
const float c_gamma = 2.2;
const float c_inv_gamma = 1.0 / c_gamma;
const float u_EnvIntensity = 1.0;
const float c_point_light_tweak = 1.0 / 300.0;
const float c_dir_light_tweak = 1.0 / 200.0;

uniform vec2 u_tiling = vec2(1.0);
uniform vec2 u_offset = vec2(0.0);

const mat4 c_bayerMatrix = mat4(
    0.0, 12.0, 3.0, 15.0,
    8.0, 4.0, 11.0, 7.0,
    2.0, 14.0, 1.0, 13.0,
    10.0, 6.0, 9.0, 5.0
);

#define saturate(value) clamp(value, 0.0, 1.0)
#define DEBUG 1

struct fragment_light
{
    vec3    direction;   
    float   attenuation;
    vec4    color_intensity;
};

struct fragment_material
{
    vec4 albedo;
    vec4 emissive;
    float roughness;
    float metallic;
    float occlusuion;
    vec3 f0;
    vec3 f90;
    vec3 diffuse;
};

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linear_to_sRGB(vec3 color)
{
    return pow(color, vec3(c_inv_gamma));
}

vec3 F_Schlick(float u, vec3 f0) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);    
}

vec3 F_Schlick(vec3 f0, vec3 f90, float VoH) {
    return f0 + (f90 - f0) * pow(clamp(1.0 - VoH, 0.0, 1.0), 5.0);
}

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
float G_Smith(float NoV, float NoL, float roughness)
{
	float k = (roughness * roughness) / 2.0;
	float GGXL = NoL / (NoL * (1.0 - k) + k);
	float GGXV = NoV / (NoV * (1.0 - k) + k);
	return GGXL * GGXV;
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NoV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX(float NdotL, float NoV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NoV * NoV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NoV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (PI * f * f);
}


vec3 BRDF_Lambertian(vec3 f0, vec3 f90, vec3 diffuseColor, float VoH) {
    return (1.0 - F_Schlick(f0, f90, VoH)) * (diffuseColor / PI);
}


vec3 BRDF_SpecularGGX(vec3 f0, vec3 f90, float lin_roughness, float VdotH, float NdotL, float NoV, float NdotH) {
    vec3 F = F_Schlick(f0, f90, VdotH);
    float Vis = V_GGX(NdotL, NoV, lin_roughness);
    float D = D_GGX(NdotH, lin_roughness);
    return F * Vis * D;
}

void apply_light(
    in fragment_light light,
    in fragment_material mt,
    in vec3 position,
    in vec3 normal,
    in vec2 texCoords,
    out vec3 diffuse,
    out vec3 specular)
{ 
    vec3 L = light.direction;                   // Light
    vec3 V = normalize(bee_eyePos - position);  // View
    vec3 N = normal;                            // Normal
    vec3 H = normalize(V + light.direction);    // Half Vector

    vec3 intensity = light.color_intensity.rgb * light.color_intensity.a;
    intensity *= light.attenuation;

    if (u_double_sided)
    {
        vec3 h = normalize(L + N * u_SSS_distortion);
        float I = pow(saturate(dot(V, -h)), u_SSS_power) * u_SSS_strength;

        vec4 subsurfaceColor = mt.albedo;
        if (use_subsurface_texture)
        {
            subsurfaceColor = pow(texture(s_subsurface, texCoords), vec4(2.2));
            if(subsurfaceColor.a > 0.f)
                I *= subsurfaceColor.a;
        }
        I *= subsurface_factor;
            
        diffuse += subsurfaceColor.rgb * light.color_intensity.rgb * I;
    }

    float NoV = saturate(dot(N, V));
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));
    
    specular += intensity * NoL * BRDF_SpecularGGX(mt.f0, mt.f90, mt.roughness, VoH, NoL, NoV, NoH);
    diffuse += intensity * NoL * BRDF_Lambertian(mt.f0,  mt.f90, mt.diffuse, VoH);
}

void apply_toon_light(
    in fragment_light light,
    in fragment_material mt,
    in vec3 position,
    in vec3 normal,
    out vec3 diffuse,
    out vec3 specular)
{
    vec3 L = light.direction;                   // Light
    vec3 V = normalize(bee_eyePos - position);  // View
    vec3 N = normal;                            // Normal
    vec3 H = normalize(V + light.direction);    // Half Vector

    float NoV = saturate(dot(N, V));
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float LoH = saturate(dot(L, H));
    float VoH = saturate(dot(V, H));

    vec3 intensity = light.color_intensity.rgb * light.color_intensity.a;
    intensity *= light.attenuation;
    specular += intensity * NoL * BRDF_SpecularGGX(mt.f0, mt.f90, mt.roughness, VoH, NoL, NoV, NoH);

    float colorPal = 0.2 * float(u_toon_palette_index) + 0.01; // +0.01 because of sampling issues (somehow the image gets magnified and the index is a little incorrect)
    diffuse += texture(s_toon, vec2(NoL, colorPal)).xyz  * intensity * mt.albedo.xyz;
}

vec3 get_diffuse_light_ibl(vec3 n)
{
    return texture(s_ibl_diffuse, n).rgb * u_EnvIntensity; // TODO: Add this
}

vec4 get_specular_sample_ibl(vec3 reflection, float lod)
{
    return textureLod(s_ibl_specular, reflection, lod) * u_EnvIntensity; // TODO: Add this
}

vec3 getIBLRadianceLambertian(vec3 n, vec3 v, float roughness, vec3 diffuseColor, vec3 F0)
{
    float NoV = saturate(dot(n, v));
    vec2 brdfSamplePoint = clamp(vec2(NoV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    vec2 f_ab = texture(s_ibl_lut, brdfSamplePoint).rg;

    vec3 irradiance = get_diffuse_light_ibl(n);

    // see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
    // Roughness dependent fresnel, from Fdez-Aguera

    vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
    vec3 k_S = F0 + Fr * pow(1.0 - NoV, 5.0);
    vec3 FssEss = k_S * f_ab.x + f_ab.y; // <--- GGX / specular light contribution

    // Multiple scattering, from Fdez-Aguera
    float Ems = (1.0 - (f_ab.x + f_ab.y));
    vec3 F_avg = (F0 + (1.0 - F0) / 21.0);
    vec3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);
    vec3 k_D = diffuseColor * (1.0 - FssEss + FmsEms); // we use +FmsEms as indicated by the formula in the blog post (might be a typo in the implementation)

    return (FmsEms + k_D) * irradiance;
}

vec3 getIBLRadianceGGX(vec3 n, vec3 v, float roughness, vec3 F0)
{
    float NoV = saturate(dot(n, v));
    float lod = roughness * float(u_ibl_specular_mip_count - 1);
    vec3 reflection = normalize(reflect(-v, n));

    vec2 brdfSamplePoint = clamp(vec2(NoV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    vec2 f_ab = texture(s_ibl_lut, brdfSamplePoint).rg;
    vec4 specularSample = get_specular_sample_ibl(reflection, lod);

    vec3 specularLight = specularSample.rgb;

    // see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
    // Roughness dependent fresnel, from Fdez-Aguera
    vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
    vec3 k_S = F0 + Fr * pow(1.0 - NoV, 5.0);
    vec3 FssEss = k_S * f_ab.x + f_ab.y;
    return specularLight * FssEss;
}

float attenuation(float distance, float range)
{
    // GLTF 2.0 uses quadratic attenuation, but with range
    float distance2 = distance * distance;    
    return max(min(1.0 - pow(distance/range, 4), 1), 0) / distance2;
}

void apply_fog(inout vec3 pixel_color, in vec3 fog_color, float fog_near, float fog_far, float distance)
{
    float fog_amount = saturate((distance - fog_near) / (fog_far - fog_near));
    pixel_color = mix(pixel_color, fog_color, fog_amount);
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
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(s_shadowmaps[lightIndex],
            vec3(proj_coords.xy + vec2(x,y) * texelSize, cascade_index)).r;
			shadow += (proj_coords.z - bias) > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0f;
	if(proj_coords.z > 1.f)
	{
		shadow = 0.f;
	}
    
	return shadow;
}

void main()
{       
    // Collect all the properties in this struct (like g-buffer) 
    fragment_material mat;

    vec2 texCoord = v_texture0 * u_tiling + u_offset;

    vec3 V = bee_eyePos - v_position;
    float distance = length(V);

    // Dithering
    float normalizedDistance = clamp(distance / u_dither_distance, 0.0, 1.0);

    // Calculate screen coordinates in 4x4 grid
    ivec2 screenPos = ivec2(gl_FragCoord.xy) % 4;
    
    // Determine the threshold from the Bayer matrix
    float threshold = c_bayerMatrix[screenPos.x][screenPos.y] / 16.0;

    // Compare the normalized distance with the threshold
    if (normalizedDistance < threshold && u_is_ditherable) {
        discard; // Discard the fragment
    }

    if(use_base_texture)
        mat.albedo = pow(texture(s_base_color, texCoord), vec4(2.2));
    else
        mat.albedo = vec4(1.0, 1.0, 1.0, 1.0);
    mat.albedo *= base_color_factor;

    // Alpha clipping (unless alpha blending is enabled)
    if(!use_alpha_blending && mat.albedo.a < 0.2f) // TODO: Bring this from material
    {
       discard;
    }
        
    if(is_unlit)
    {
        frag_color = vec4(linear_to_sRGB(mat.albedo.rgb), mat.albedo.a);
        return;
    }

    mat.emissive = vec4(0.0);
    if(use_emissive_texture)
    {
        mat.emissive = pow(texture(s_emissive, texCoord), vec4(2.2));
    }

    if(use_metallic_roughness_texture)
    {
        vec4 tex_orm = texture(s_orm, texCoord);
        mat.occlusuion = tex_orm.r;
        mat.roughness = tex_orm.g * roughness_factor;        
        mat.metallic = tex_orm.b * metallic_factor;
    }
    else
    {
        mat.occlusuion = 1.0;
        mat.roughness = roughness_factor;
        mat.metallic = metallic_factor;
    }

    if(use_occlusion_texture)
    {
        float tex_occlusuion = texture(s_occulsion, texCoord).r;
        mat.occlusuion = tex_occlusuion;
    }

    vec3 normal = normalize(v_normal);
    if(use_normal_texture)
    {
        normal = texture(s_normal, texCoord).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(v_TBN * normal);
    }

    // Go from perceptual to linear(ish) roughness. 
    mat.f0 = mix(vec3(0.04), mat.albedo.rgb, mat.metallic);
    mat.diffuse = mix(mat.albedo.rgb,  vec3(0), mat.metallic);
    mat.f90 = vec3(1.0);    // all materials are reflective at grazing angles
    float reflectance = max(max(mat.f0.r, mat.f0.g), mat.f0.b);

    V = normalize(V);                               // View  

    vec3 specular = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 ibl_spec = vec3(0.0);
    vec3 ibl_diff = vec3(0.0);

    // Double sided shading
    if (u_double_sided)
    {
        if(!gl_FrontFacing) 
            normal *= -1;
    }

    ibl_spec = getIBLRadianceGGX(normal, V, mat.roughness, mat.f0);
    ibl_diff = getIBLRadianceLambertian(normal, V, mat.roughness, mat.diffuse, mat.f0);

    // To alpha roughness
    mat.roughness = mat.roughness * mat.roughness;

    for(int i = 0; i < bee_directionalLightsCount; i++) // Dir lights
    {        
        fragment_light light;
        light.direction = bee_directional_lights[i].direction;
        light.color_intensity = vec4(   bee_directional_lights[i].color,
                                        bee_directional_lights[i].intensity);
        light.attenuation = c_dir_light_tweak;
        vec3 dif = vec3(0.0);
        vec3 spc = vec3(0.0);
        
        if(u_do_toon_shading)
        {
            apply_toon_light(light, mat, v_position, normal, dif, spc);
        }
        else
        {
            apply_light(light, mat, v_position, normal, texCoord, dif, spc);
        }
        
        if(u_receive_shadows)
        {
            float shadow = CalculateShadow(v_position, normal, i);

            dif *= (1.0f - shadow);
            spc *= (1.0f - shadow);
        }

        diffuse += dif;
        specular += spc;
    }

    for(int i = 0; i < bee_pointLightsCount; i++) // Point lights
    {        
        fragment_light light;        
        light.direction = bee_point_lights[i].position - v_position;
        float distance = length(light.direction);
        light.direction /= distance;
        light.color_intensity = vec4(   bee_point_lights[i].color,
                                        bee_point_lights[i].intensity);
        light.attenuation = attenuation(distance, bee_point_lights[i].range);
        light.attenuation *= c_point_light_tweak;
        vec3 dif = vec3(0.0);
        vec3 spc = vec3(0.0);
        if(u_do_toon_shading)
        {
            apply_toon_light(light, mat, v_position, normal, dif, spc);
        }
        else
        {
            apply_light(light, mat, v_position, normal, texCoord, dif, spc);
        }      
        diffuse += dif;
        specular += spc;
    }

    vec3 direct_lighting = (specular + diffuse) + mat.emissive.rgb;

    vec3 iblDiff = mix(ibl_diff, ibl_diff * mat.occlusuion, 1);
	vec3 iblSpec = mix(ibl_spec, ibl_spec * mat.occlusuion, 1);

    vec3 ambient_lighting = iblDiff + iblSpec;

    vec3 color = mix(direct_lighting, ambient_lighting, bee_ambientFactor);

    frag_color = vec4(color, 1.0);

    if(bee_FogColor.w > 0.0)
        apply_fog(frag_color.rgb, get_specular_sample_ibl(-V, 1).rgb, bee_FogNear, bee_FogFar, distance);

    wsPositionOut = vec4(v_position, 1.0);
    normalOut = vec4(normal, 1.0);

#if DEBUG   
    if(debug_base_color)
        frag_color = vec4(mat.albedo.rgb, 1.0);
    if(debug_normals)
        frag_color = vec4(normal, 1.0);
    if(debug_normal_map && use_normal_texture)
        frag_color = texture(s_normal, texCoord);
    if(debug_metallic)
        frag_color = vec4(vec3(mat.metallic), 1.0);
    if(debug_roughness)
        frag_color = vec4(vec3(mat.roughness), 1.0);
    if(debug_metallic && debug_roughness)
        frag_color = vec4(0.0, mat.roughness, mat.metallic, 1.0);
    if(debug_occlusion)
        frag_color = vec4(vec3(mat.occlusuion), 1.0);
    if(debug_emissive)
        frag_color = vec4(vec3(mat.emissive), 1.0);
    if(debug_displacement_pivot)
        frag_color = vec4(v_displacement.xyz, 1.0);
    if(debug_wind_mask)
        frag_color = vec4(vec3(v_displacement.w), 1.0);
#endif
}