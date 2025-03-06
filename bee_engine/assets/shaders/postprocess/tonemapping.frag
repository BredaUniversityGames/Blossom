#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_hdrBuffer;
uniform float u_exposure;

const float c_white = 0.998; // constant for now
const float c_exposure_bias = 2.0;

#define saturate(value) clamp(value, 0.0, 1.0)

// part of CIE xyY color space
// coeff derived from: https://en.wikipedia.org/wiki/Luminous_efficiency_function
float Luminance(vec3 hdrColor)
{
    return dot(hdrColor, vec3(0.2125f, 0.7154f, 0.0721f));
}

// from: https://www-old.cs.utah.edu/docs/techreports/2002/pdf/UUCS-02-001.pdf
vec3 Reinhard(vec3 hdrColor)
{
    vec3 col = u_exposure * hdrColor;
    float lum = Luminance(col);
    float r_lum = lum / (1.0 + lum);
    vec3 fin_col = col / lum * r_lum;
    return fin_col;
}

vec3 ReinhardExtended(vec3 hdrColor)
{
    vec3 col = u_exposure * hdrColor;
    float lum = Luminance(col);
    float l_white = c_white * u_exposure;
    float r_lum = (lum * (1.0 + lum / (l_white * l_white))) / (1.0 + lum);
    vec3 fin_col = col / lum * r_lum;
    return fin_col;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 Aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float Aces(float x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{             
    vec3 hdrColor = texture(u_hdrBuffer, TexCoords).rgb;

    // no tonemapping (clamb rgb)
    vec3 result = saturate(hdrColor);

    // reinhard
    //result = Reinhard(hdrColor);
    //result = ReinhardExtended(hdrColor);

    // aces
    result = Aces(hdrColor);

    // gamma correct after everything
    const float gamma = 2.2;
    result = pow(result, vec3(1.0 / gamma));

    FragColor = vec4(result, 1.0);
}