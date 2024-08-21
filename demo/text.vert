#version 460

// Vertex rate
// Supposed to be 0 or 1
layout(location = 0) in vec2 inPosition;

// Instance rate
layout(location = 1) in vec2 inOffset;
layout(location = 2) in vec2 inScale;
layout(location = 3) in uint inGlyphId;

layout(location = 0) out vec2 fragmentUV;
flat layout(location = 1) out uint fragmentGlyphID;

void main()
{
	const vec2 position = vec2(inPosition.x, 1.0 - inPosition.y);
	gl_Position = vec4(position * inScale + inOffset, 0.0, 1.0);

	fragmentUV = inPosition;
	fragmentGlyphID = inGlyphId;
}
