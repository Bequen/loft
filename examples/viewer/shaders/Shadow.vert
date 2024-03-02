#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;

struct Light {
    mat4 view;
    vec4 color;
};

layout(set = 2, binding = 0) uniform Lights {
    Light lights[];
} lights;

void main() {
    gl_Position = lights.lights[0].view * vec4(pos * 0.05, 1.0);
}
