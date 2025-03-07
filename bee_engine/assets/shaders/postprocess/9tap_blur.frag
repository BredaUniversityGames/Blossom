#version 460 core
  
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D u_image;
uniform vec2 u_blur_direction;

// Gaussian kernel
const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(u_image, 0); // gets size of single texel
    vec3 result = texture(u_image, TexCoords).rgb * weight[0]; // current fragment's contribution

    for(int i = 1; i < 5; ++i)
    {
        result += texture(u_image, TexCoords + tex_offset * u_blur_direction).rgb * weight[i];
        result += texture(u_image, TexCoords - tex_offset * u_blur_direction).rgb * weight[i];
    }
    
    FragColor = vec4(result, 1.0);
}