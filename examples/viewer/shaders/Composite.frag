#version 450


layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D inHdr;
layout(set = 1, binding = 1) uniform sampler2D inBloom;

const mat3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = transpose(ACESInputMat) * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = transpose(ACESOutputMat) * color;

    // Clamp to [0, 1]
    color = clamp(color, vec3(0.0f), vec3(1.0f));

    return color;
}

void main() {
    fragColor = vec4(ACESFitted(texture(inHdr, inUV).rgb + texture(inBloom, inUV).rgb), 1.0f);
}