#version 450

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D source;
layout(set = 1, binding = 1) uniform sampler2D upscaled;

layout( push_constant ) uniform constants
{
    ivec2 resolution;
} PushConstants;

/* ? - e - ?
 * | a | b |
 * f - ? - g
 * | c | d |
 * ? - h - ?
 */
vec4 upscale() {
    vec2 srcTexelSize = 1.0f / PushConstants.resolution * 2.0f;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    vec3 color = vec3(0.0f);
    vec3 a = texture(upscaled, vec2(inUV.x - x, inUV.y + y)).rgb;
    vec3 b = texture(upscaled, vec2(inUV.x + x, inUV.y + y)).rgb;

    vec3 c = texture(upscaled, vec2(inUV.x - x, inUV.y - y)).rgb;
    vec3 d = texture(upscaled, vec2(inUV.x + x, inUV.y - y)).rgb;

    vec3 e = texture(upscaled, vec2(inUV.x, inUV.y + 2.0*y)).rgb;
    vec3 h = texture(upscaled, vec2(inUV.x, inUV.y - 2.0*y)).rgb;

    vec3 f = texture(upscaled, vec2(inUV.x - 2.0*x, inUV.y)).rgb;
    vec3 g = texture(upscaled, vec2(inUV.x + 2.0*x, inUV.y)).rgb;

    return vec4((a + b + c + d) * (1.0f/6.0f) + (e + f + g + h) * (1.0f/12.0f), 1.0);

    /* vec3
    color  = texture(upscaled, vec2(inUV.x + x, inUV.y + y)).rgb;
    color += texture(upscaled, vec2(inUV.x + x, inUV.y - y)).rgb;
    color += texture(upscaled, vec2(inUV.x - x, inUV.y + y)).rgb;
    color += texture(upscaled, vec2(inUV.x - x, inUV.y - y)).rgb;

    color += 2.0f * texture(upscaled, vec2(inUV.x + x, inUV.y)).rgb;
    color += 2.0f * texture(upscaled, vec2(inUV.x - x, inUV.y)).rgb;
    color += 2.0f * texture(upscaled, vec2(inUV.x, inUV.y + y)).rgb;
    color += 2.0f * texture(upscaled, vec2(inUV.x, inUV.y - y)).rgb;

    color += 4.0f * texture(upscaled, inUV).rgb;

    return vec4(color * (1.0f / 16.0f), 1.0f); */
}

void main() {
    fragColor = texture(source, inUV) + upscale();
}