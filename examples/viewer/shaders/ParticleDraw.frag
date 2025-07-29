#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in float inGradientPos;

layout (location = 0) out vec4 outFragColor;

void main ()
{
	//vec3 color = texture(samplerGradientRamp, vec2(inGradientPos, 0.0)).rgb;
	//outFragColor.rgb = texture(samplerColorMap, gl_PointCoord).rgb * color;
	outFragColor = vec4(inColor.rgb, 1.0f);
}
