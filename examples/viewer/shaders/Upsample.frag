#version 450

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D source;
layout(set = 1, binding = 1) uniform sampler2D upscaled;

layout( push_constant ) uniform constants
{
    ivec2 resolution;
} PushConstants;

vec4 upscale() {
    vec2 srcTexelSize = 1.0f / PushConstants.resolution * 0.5f;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    vec3
    color  = texture(upscaled, vec2(inUV.x + x, inUV.y + y)).rgb;
    color += texture(upscaled, vec2(inUV.x + x, inUV.y - y)).rgb;
    color += texture(upscaled, vec2(inUV.x - x, inUV.y + y)).rgb;
    color += texture(upscaled, vec2(inUV.x - x, inUV.y - y)).rgb;

    color += 2.0f * texture(upscaled, vec2(inUV.x + x, inUV.y)).rgb;
    color += 2.0f * texture(upscaled, vec2(inUV.x - x, inUV.y)).rgb;
    color += 2.0f * texture(upscaled, vec2(inUV.x, inUV.y + y)).rgb;
    color += 2.0f * texture(upscaled, vec2(inUV.x, inUV.y - y)).rgb;

    color += 4.0f * texture(upscaled, inUV).rgb;

    return vec4(color * (1.0f / 16.0f), 1.0f);
}

void main() {
    fragColor = upscale();
}