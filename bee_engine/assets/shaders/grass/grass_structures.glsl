
struct GrassInstanceData 
{
	vec4 position;
	vec2 chunkUv;
	vec2 _padding;
};

struct GrassChunkMaterial
{
	vec3 color;
	float minHeight;

	float maxHeight;
	float cutoffLength;
	float bladeBending;
	float distanceNormalBlend;

	float darkEdgeStrength;
	float darkEdgeRange;
	float aoStrength;
	float aoRange;
	
	float SSS_strength;
	float SSS_distortion;
	float SSS_power;
	float _padding;
	
	vec4 lodsAdjustments;
};

layout(std430, binding=GRASS_LOCATION) buffer vertex_buffer 
{
	GrassInstanceData instanceData[];
};

layout(std140, binding=GRASS_MATERIAL_LOCATION) uniform grass_material
{
	GrassChunkMaterial material;
};
