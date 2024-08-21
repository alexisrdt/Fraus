#version 460

layout(binding = 0) uniform UniformObject {
	mat4 viewProjection;
} ubo;

layout(push_constant) uniform constants
{
	mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragmentPosition;
layout(location = 1) out vec2 fragmentTextureCoordinates;
layout(location = 2) out vec3 fragmentNormal;

void main()
{
	gl_Position = ubo.viewProjection * pc.model * vec4(inPosition, 1.0);
	fragmentPosition = (pc.model * vec4(inPosition, 1.0)).xyz;
	fragmentTextureCoordinates = inTextureCoordinates;
	fragmentNormal = inNormal;
}
