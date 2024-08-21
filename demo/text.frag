#version 460

layout(location = 0) in vec2 fragmentTextureCoordinates;
flat layout(location = 1) in uint fragmentGlyphId;

layout(binding = 0) readonly buffer GlyphOffsets
{
	uint glyphOffsets[];
};
layout(binding = 1) readonly buffer GlyphContours
{
	uint glyphContours[];
};
layout(binding = 2) readonly buffer GlyphPoints
{
	vec2 glyphPoints[];
};

layout(location = 0) out vec4 fragColor;

void main()
{
	uint intersectionCount = 0;

	const uint contourOffset = glyphOffsets[2 * fragmentGlyphId];
	const uint pointsOffset = glyphOffsets[2 * fragmentGlyphId + 1];

	const uint contourCount = glyphContours[contourOffset];
	uint firstPoint = 0;
	for(uint contourIndex = 0; contourIndex < contourCount; ++contourIndex)
	{
		const uint pointCount = glyphContours[contourOffset + contourIndex + 1];
		const uint lastPoint = firstPoint + pointCount - 1;

		for(uint pointIndex = firstPoint; pointIndex <= lastPoint; pointIndex += 2)
		{
			const uint pointOffset = pointsOffset + pointIndex;

			const vec2 p0 = glyphPoints[pointOffset];
			const vec2 p1 = glyphPoints[pointOffset + 1];
			const vec2 p2 = glyphPoints[pointIndex == lastPoint - 1 ? pointsOffset + firstPoint : pointOffset + 2];

			// p(t) = (1 - t)^2 * p0 + 2 * (1 - t) * t * p1 + t^2 * p2
			// p(t) = (p0 - 2 * p1 + p2) * t^2 + 2 * (p1 - p0) * t + p0

			const float a = p0.y - 2.0 * p1.y + p2.y;
			const float b = 2.0 * (p1.y - p0.y);
			const float c = p0.y - fragmentTextureCoordinates.y;

			float t0;
			float t1;
			if(abs(a) < 1e-6)
			{
				if(abs(b) < 1e-6)
				{
					if(abs(c) <= 1e-6)
					{
						t0 = 0.5;
						t1 = 0.5;
					}
					else
					{
						t0 = -1.0;
						t1 = -1.0;
					}
				}
				else
				{
					t0 = -c / b;
					t1 = -c / b;
				}
			}
			else
			{
				const float dis = b * b - 4.0 * a * c;
				if(dis < 0.0)
				{
					t0 = -1.0;
					t1 = -1.0;
				}
				else
				{
					const float d = sqrt(dis);
					t0 = (-b + d) / (2.0 * a);
					t1 = (-b - d) / (2.0 * a);
				}
			}

			if(
				(t0 >= 0.0 && t0 <= 1.0 && fragmentTextureCoordinates.x <= (1.0-t0)*(1.0-t0)*p0.x + 2.0*(1.0-t0)*t0*p1.x + t0*t0*p2.x) ||
				(t1 >= 0.0 && t1 <= 1.0 && fragmentTextureCoordinates.x <= (1.0-t1)*(1.0-t1)*p0.x + 2.0*(1.0-t1)*t1*p1.x + t1*t1*p2.x)
			)
			{
				++intersectionCount;
			}
		}

		firstPoint = lastPoint + 1;
	}

	if(intersectionCount % 2 != 0)
	{
		fragColor = vec4(1.0, 1.0, 1.0, 1.0);
	}
	else if(fragmentTextureCoordinates.x < 0.01 || fragmentTextureCoordinates.x > 0.99 || fragmentTextureCoordinates.y < 0.01 || fragmentTextureCoordinates.y > 0.99)
	{
		fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else
	{
		fragColor = vec4(0.0, 0.0, 0.0, 0.0);
	}
}
