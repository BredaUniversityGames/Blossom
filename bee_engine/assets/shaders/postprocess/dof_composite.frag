#version 460 core

in vec2 TexCoords;

out vec4 FragColor;

layout(location = 0) uniform sampler2D u_blurred_image;
layout(location = 1) uniform sampler2D u_original_image;
layout(location = 2) uniform sampler2D u_wsposition_image;

uniform vec3 u_camera_pos;
uniform float u_focus_distance;
uniform float u_focus_fallof_distance;

void main()
{             
    vec3 result = vec3(0.0f);
    vec3 blurred_sample = texture(u_blurred_image, TexCoords).rgb;
    vec3 original_sample = texture(u_original_image, TexCoords).rgb;
    vec4 ws_pos_full = texture(u_wsposition_image, TexCoords);
    vec3 ws_pos = ws_pos_full.xyz;
    float alpha = ws_pos_full.a;

    float dist_to_pos = length(ws_pos - u_camera_pos);

    float blur = smoothstep(
        u_focus_distance,
        u_focus_distance + u_focus_fallof_distance,
        dist_to_pos
    );

    // Alpha of 0 means the skybox
    // We default to the full blur for this one
    if(alpha < 0.9f)
        blur = 1.0f;

    FragColor = vec4(mix(original_sample, blurred_sample, blur), 1.0);
}