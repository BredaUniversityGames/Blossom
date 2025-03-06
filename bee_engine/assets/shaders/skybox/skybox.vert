#version 460 core
layout (location = 0) in vec3 a_position;

#include "../locations.glsl"
#include "../uniforms.glsl"
#include "../matrices.glsl"

out vec3 v_texCoords;

void main()
{
    v_texCoords = a_position;
    mat4 view = mat4(mat3(bee_view));
    vec4 pos = bee_projection * view * vec4(a_position, 1.0);
    gl_Position = pos.xyww;
}  


