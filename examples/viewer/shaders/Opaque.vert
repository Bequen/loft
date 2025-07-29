#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 tangent;

layout(set = 0, binding = 0) uniform Camera {
	mat4 proj;
	mat4 view;
} cam;

struct SceneNode {
	uint transformIdx;
	uint meshIdx;
};

layout(set = 2, binding = 1) uniform ObjectInfo {
	SceneNode nodes[];
} objectInfo;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out mat3 outTBN;

layout( push_constant ) uniform constants
{
	uint transformIdx;
	uint materialIdx;
} PushConstants;


void main() {
	vec3 bitangent = cross(norm, tangent.xyz) * tangent.w;
	outTBN = mat3(tangent.xyz, bitangent, norm);

	outPos = pos.xyz * 0.05;
	outNormal = norm.xyz;
	outUV = uv;
    gl_Position = cam.proj * cam.view * vec4(outPos, 1.0);
}
