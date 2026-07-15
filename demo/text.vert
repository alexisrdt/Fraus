#version 460

// Vertex rate
// Supposed to be 0 or 1
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inNormal;

// Instance rate
layout(location = 3) in vec2 inOffset;
layout(location = 4) in vec2 inScale;
layout(location = 5) in uint inGlyphId;

layout(location = 0) out vec2 fragmentUV;
flat layout(location = 1) out uint fragmentGlyphID;
flat layout(location = 2) out vec2 fragmentScale;

void main()
{
	const vec2 position = vec2(inPosition.x, 1.0 - inPosition.y);
	gl_Position = vec4(position * inScale + inOffset, 0.0, 1.0);

	fragmentUV = vec2(inTextureCoordinates.x, 1.0 - inTextureCoordinates.y);
	fragmentGlyphID = inGlyphId;
	fragmentScale = inScale;
}
