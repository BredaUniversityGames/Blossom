
// Atributes
#define POSITION_LOCATION       0
#define NORMAL_LOCATION         1
#define TEXTURE0_LOCATION       2
#define TEXTURE1_LOCATION       3
#define TANGENT_LOCATION        4
#define DISPLACEMENT_LOCATION   5
#define LOCATION_COUNT          6

// UBOs
#define PER_FRAME_LOCATION                  1
#define PER_MATERIAL_LOCATION               2
#define PER_OBJECT_LOCATION                 3
#define CAMERA_UBO_LOCATION                 4
#define LIGHTS_UBO_LOCATION                 5
#define TRANSFORMS_UBO_LOCATION             6
#define DIRECTIONAL_LIGHTS_UBO_LOCATION     7
#define AMBIENT_WIND_LOCATION				8
#define UBO_LOCATION_COUNT                  9

// Samplers
#define BASE_COLOR_SAMPLER_LOCATION    0
#define NORMAL_SAMPLER_LOCATION        1
#define EMISSIVE_SAMPLER_LOCATION      2
#define ORM_SAMPLER_LOCATION           3
#define OCCLUSION_SAMPLER_LOCATION     4
#define DEPTH_SAMPLER_LOCATION         5
#define IRRADIANCE_LOCATION            6
#define LUT_SAMPER_LOCATION			   7
#define DIFFUSE_SAMPER_LOCATION		   8
#define SPECULAR_SAMPER_LOCATION	   9
#define WIND_SAMPLER_LOCATION		   15
#define TOON_SAMPLER_LOCATION		   16
#define HEIGHTMAP_LOCATION			   17
#define SUBSURFACE_SAMPLER_LOCATION    18		

#define SHADOWMAP_LOCATION			   40 // Dont touch values after this!
 
// Attenuation
#define ATTENUATION_GLTF        0
#define ATTENUATION_BLAST       1
#define ATTENUATION_UNREAL      2
#define ATTENUATION_SMOOTHSTEP  3
