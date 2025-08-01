#version 450

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D inHdr;

layout( push_constant ) uniform constants
{
    ivec2 resolution;
} PushConstants;

/* a - ? - b
 * |   |   |
 * ? - c - ?
 * |   |   |
 * d - ? - e
 */
vec4 downscale() {
    vec2 srcTexelSize = 1.0f / PushConstants.resolution;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    vec3 color = vec3(0.0f);
    vec3 a = texture(inHdr, vec2(inUV.x - 2*x, inUV.y + 2*y)).rgb;
    vec3 b = texture(inHdr, vec2(inUV.x + 2*x, inUV.y + 2*y)).rgb;

    vec3 c = texture(inHdr, inUV).rgb;

    vec3 d = texture(inHdr, vec2(inUV.x - 2*x, inUV.y - 2*y)).rgb;
    vec3 e = texture(inHdr, vec2(inUV.x + 2*x, inUV.y - 2*y)).rgb;



    return vec4((a + b + c + d) * (1.0f/8.0f) + c * 1.0f/2.0f, 1.0);

    /* vec3 a = texture(inHdr, vec2(inUV.x - 2*x, inUV.y + 2*y)).rgb;
    vec3 b = texture(inHdr, vec2(inUV.x,       inUV.y + 2*y)).rgb;
    vec3 c = texture(inHdr, vec2(inUV.x + 2*x, inUV.y + 2*y)).rgb;

    vec3 d = texture(inHdr, vec2(inUV.x - 2*x, inUV.y)).rgb;
    vec3 e = texture(inHdr, vec2(inUV.x,       inUV.y)).rgb;
    vec3 f = texture(inHdr, vec2(inUV.x + 2*x, inUV.y)).rgb;

    vec3 g = texture(inHdr, vec2(inUV.x - 2*x, inUV.y - 2*y)).rgb;
    vec3 h = texture(inHdr, vec2(inUV.x,       inUV.y - 2*y)).rgb;
    vec3 i = texture(inHdr, vec2(inUV.x + 2*x, inUV.y - 2*y)).rgb;

    vec3 j = texture(inHdr, vec2(inUV.x - x, inUV.y + y)).rgb;
    vec3 k = texture(inHdr, vec2(inUV.x + x, inUV.y + y)).rgb;
    vec3 l = texture(inHdr, vec2(inUV.x - x, inUV.y - y)).rgb;
    vec3 m = texture(inHdr, vec2(inUV.x + x, inUV.y - y)).rgb;

    vec3 color = e * 0.125f;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;
    return vec4(color, 1.0f); */


}

void main() {
    fragColor = downscale();
}
