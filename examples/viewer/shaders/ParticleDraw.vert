#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inGradientPos;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outGradientPos;

layout(set = 0, binding = 0) uniform Camera {
	mat4 proj;
	mat4 view;
} cam;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main ()
{
    gl_PointSize = 8.0;
    outColor = vec4(1.0);
    outGradientPos = inGradientPos.x;
    gl_Position = cam.proj * cam.view * vec4(inPos.xy * 0.01, 1.0, 1.0);
}
