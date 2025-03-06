#version 460 core

layout(location = 0) out vec4 colorOut;
layout(location = 1) out vec4 brightnessOut;
layout(location = 2) out vec4 wsPositionOut;
layout(location = 3) out vec4 normalOut;

in vec3 v_texCoords;

uniform samplerCube u_skybox;

void main()
{    
    colorOut = brightnessOut = texture(u_skybox, v_texCoords);

    normalOut = vec4(0.0f);
    wsPositionOut = vec4(0.0f);
}
