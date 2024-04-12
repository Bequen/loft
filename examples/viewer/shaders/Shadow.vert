#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;

layout( push_constant ) uniform constants {
    mat4 view;
} Camera;

void main() {
    gl_Position = Camera.view * vec4(pos * 0.05, 1.0);
}
