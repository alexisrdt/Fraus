#version 460

layout(location = 0) in vec3 fragmentPosition;
layout(location = 1) in vec2 fragmentTextureCoordinates;
layout(location = 2) in vec3 fragmentNormal;

layout(binding = 1) uniform sampler2D textureSampler;
layout(binding = 2) uniform Factor
{
	vec3 lightPosition;
} f;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 lightDirection = normalize(f.lightPosition - fragmentPosition);
	float brightness = max(dot(fragmentNormal, lightDirection), 0.05);
	outColor = texture(textureSampler, fragmentTextureCoordinates) * brightness;
}
