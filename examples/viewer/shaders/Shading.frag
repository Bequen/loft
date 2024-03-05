#version 450

#define M_PI 3.14159265359

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBloomThreshold;

layout(set = 1, binding = 0) uniform sampler2D inAlbedo;
layout(set = 1, binding = 1) uniform sampler2D inNormal;
layout(set = 1, binding = 2) uniform sampler2D inPosition;
layout(set = 1, binding = 3) uniform sampler2D inPbr;
/* layout(set = 1, binding = 5) uniform sampler2D shadowMap; */
float ggx_normal_distr(float NoH, float roughness) {
	float a = NoH * roughness;
	float k = roughness / (1.0 - NoH * NoH + a * a);
	return k * k * (1.0 / M_PI);
}

vec3 schlick(float VoH, vec3 f0) {
	return f0 + (vec3(1.0) - f0) * pow(1.0 - VoH, 5.0);
}

float ggx_smith_correlated(float NoV, float NoL, float a) {
	float a2 = a * a;
	float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
	float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
	return 0.5 / (GGXV + GGXL + 0.0001);
}


vec3 lambert_diffuse(vec3 albedo) {
	return albedo / M_PI;
}

layout(set = 0, binding = 0) uniform Camera {
	mat4 proj;
	mat4 view;
	vec4 position;
} cam;

struct Light {
	mat4 view;
	vec4 color;
	vec4 position;
};

/* layout(set = 2, binding = 0) uniform Lights {
	Light lights[];
} lights; */

#define AMBIENT 0.1

float linearize_depth(float depth) {
	float n = 1000.0f;
	float f = 1.0f;
	float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));
}

/* float textureProj(vec4 shadowCoord, vec2 off) {
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z )
		{
			shadow = 0.0f;
		}
	}
	return shadow;
} */

/* float filterPCF(vec4 sc) {
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 1.0f;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 4;

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}

	}
	return shadowFactor / count;
} */

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );


/* float get_shadow(Light light, vec4 position) {
	vec4 relativePosition = light.view * position;
	vec4 shadowCoords = relativePosition / relativePosition.w;

	float shadow = filterPCF(biasMat * shadowCoords);

	return shadow;
} */

vec3 bsdf(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 baseColor, float metallic, float roughness) {
	vec3 halfway = normalize(viewDir + lightDir);
	float NoH = max(0.0, dot(normal, halfway));
	float NoL = max(0.0, dot(normal, lightDir));
	float NoV = abs(dot(normal, viewDir)) + 1e-5;
	float VoH = max(0.0, dot(viewDir, halfway));
	float LoH = max(0.0, dot(lightDir, halfway));


	/* color at 0 degree angle */
	vec3 f0 = 0.16 * (1.0 - roughness) * (1.0 - roughness) * (1.0 - metallic) + baseColor * metallic;
	vec3 diffuseColor = (1.0 - metallic) * baseColor.rgb;

	/* normal distribution of the dome */
	float D = ggx_normal_distr(NoH, roughness);
	/* occlusion of microfacets */
	float G = ggx_smith_correlated(NoV, NoL, roughness);
	/* fresnel */
	vec3  F = schlick(VoH, f0);

	vec3 specular = D * G * F; /// (4.0 * NoV * NoL + 0.0001);
	/* light emmited back by material (not reflection) */

	vec3 diffuse = lambert_diffuse(diffuseColor);

	return (diffuse + specular) * NoL * 4.0f;
}

vec3 bloom_threshold(vec3 base) {
	float threshold = 0.8f;
	float knee = 0.5f;
	float intensity = 0.4f;
	vec4 curveThreshold = vec4(threshold - knee, knee * 2.0f, 0.25f / max(1e-5f, knee), threshold);

	/* Pixel brightness */
	float br = max((base).x, max((base).y, (base).z));

	/* Under-threshold part: quadratic curve */
	float rq = clamp(br - curveThreshold.x, 0.0, curveThreshold.y);
	rq = curveThreshold.z * rq * rq;

	/* Combine and apply the brightness response curve. */
	base *= max(rq, br - curveThreshold.w) / max(1e-5, br);

	/* Clamp pixel intensity if clamping enabled */
	if (intensity > 0.0) {
		br = max(1e-5, max((base).x, max((base).y, (base).z)));
		base *= 1.0 - max(0.0, br - intensity) / br;
	}

	return base;
}

void main() {
	vec3 color = texture(inAlbedo, inUV).rgb;
	float metallic = texture(inPbr, inUV).r;
	float roughness = texture(inPbr, inUV).g;
	roughness *= roughness;
	vec3 normal = texture(inNormal, inUV).rgb;
	vec3 position = texture(inPosition, inUV).rgb;

	vec3 viewDir = normalize(-cam.position.xyz - position);

	// vec3 lightDir = normalize(vec3(lights.lights[0].view[0][2], lights.lights[0].view[1][2], lights.lights[0].view[2][2]));
	// lightDir = normalize(lights.lights[0].view[1].xyz);
	vec3 lightDir = normalize(vec3(60.0, 100.0, -30.0) - position);

	float occlusion = 0.0f; //get_shadow(lights.lights[0], vec4(position, 1.0));

	float lightIntensity = 1.0f;
	vec3 Lo = bsdf(normal, viewDir, lightDir, color, metallic, roughness);

	vec3 ambient = vec3(0.2) * color;
	vec3 prd = ambient + Lo * (1.0 - occlusion) * lightIntensity;

	outColor = vec4(prd, 1.0);

	outBloomThreshold = vec4(bloom_threshold(prd), 1.0);
}
