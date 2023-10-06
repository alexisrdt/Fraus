#version 450

layout(location = 0) in vec2 fragmentTextureCoordinates;

layout(binding = 1) uniform sampler2D textureSampler;
layout(binding = 2) uniform Factor
{
	vec3 factor;
} f;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(textureSampler, fragmentTextureCoordinates);
	outColor.r *= f.factor.r;
	outColor.g *= f.factor.g;
	outColor.b *= f.factor.b;
}
