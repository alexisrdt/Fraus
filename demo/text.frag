#version 460

layout(location = 0) in vec2 fragmentTextureCoordinates;
flat layout(location = 1) in uint fragmentGlyphId;
flat layout(location = 2) in vec2 fragmentScale;

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
layout(binding = 3) readonly uniform Extent
{
	vec2 extent;
};

layout(location = 0) out vec4 fragColor;

vec4 horizontalRoots(vec2 p0, vec2 p1, vec2 p2)
{
	const float a = p0.y - 2.0 * p1.y + p2.y;
	const float b = p1.y - p0.y;
	const float c = p0.y - fragmentTextureCoordinates.y;

	float t0, t1;

	if(abs(a) < 1e-6)
	{
		if(abs(b) < 1e-6)
		{
			if(abs(c) < 1e-6)
			{
				t0 = t1 = 0.5;
			}
			else
			{
				t0 = t1 = -1.0;
			}
		}
		else
		{
			const float i = 0.5 / b;
			t0 = t1 = -c * i;
		}
	}
	else
	{
		const float i = 1.0 / a;

		const float d = sqrt(max(b * b - a * c, 0.0));

		t0 = (-b - d) * i;
		t1 = (-b + d) * i;
	}

	return vec4(t0, t1, (1.0 - t0) * (1.0 - t0) * p0.x + 2.0 * (1.0 - t0) * t0 * p1.x + t0 * t0 * p2.x, (1.0 - t1) * (1.0 - t1) * p0.x + 2.0 * (1.0 - t1) * t1 * p1.x + t1 * t1 * p2.x);
}

vec4 verticalRoots(vec2 p0, vec2 p1, vec2 p2)
{
	const float a = p0.x - 2.0 * p1.x + p2.x;
	const float b = p1.x - p0.x;
	const float c = p0.x - fragmentTextureCoordinates.x;

	float t0, t1;

	if(abs(a) < 1e-6)
	{
		if(abs(b) < 1e-6)
		{
			if(abs(c) < 1e-6)
			{
				t0 = t1 = 0.5;
			}
			else
			{
				t0 = t1 = -1.0;
			}
		}
		else
		{
			const float i = 0.5 / b;
			t0 = t1 = -c * i;
		}
	}
	else
	{
		const float i = 1.0 / a;

		const float d = sqrt(max(b * b - a * c, 0.0));

		t0 = (-b - d) * i;
		t1 = (-b + d) * i;
	}

	return vec4(t0, t1, (1.0 - t0) * (1.0 - t0) * p0.y + 2.0 * (1.0 - t0) * t0 * p1.y + t0 * t0 * p2.y, (1.0 - t1) * (1.0 - t1) * p0.y + 2.0 * (1.0 - t1) * t1 * p1.y + t1 * t1 * p2.y);
}

float coverage(float horizontalCoverage, float verticalCoverage, float horizontalWeight, float verticalWeight)
{
	float coverage = max(abs(horizontalCoverage * horizontalWeight + verticalCoverage * verticalWeight) / max(horizontalWeight + verticalWeight, 1e-6), min(abs(horizontalCoverage), abs(verticalCoverage)));

	return clamp(abs(coverage), 0.0, 1.0);
}

void main()
{
	const uint lookup = 0x2e74;

	float horizontalCoverage = 0.0, horizontalWeight = 0.0, verticalCoverage = 0.0, verticalWeight = 0.0;

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

			uint contributions = lookup >> (
				(p0.y > fragmentTextureCoordinates.y ? 2 : 0) +
				(p1.y > fragmentTextureCoordinates.y ? 4 : 0) +
				(p2.y > fragmentTextureCoordinates.y ? 8 : 0)
			);

			if(contributions > 0)
			{
				const vec4 roots = horizontalRoots(p0, p1, p2);

				if((contributions & 1) > 0 && roots.x >= 0.0 && roots.x <= 1.0)
				{
					const float diff = (roots.z - fragmentTextureCoordinates.x) * fragmentScale.x * extent.x;
					horizontalCoverage += clamp(diff + 0.5, 0.0, 1.0);
					horizontalWeight = max(horizontalWeight, clamp(1.0 - abs(diff) * 2.0, 0.0, 1.0));
				}

				if((contributions & 2) > 0 && roots.y >= 0.0 && roots.y <= 1.0)
				{
					const float diff = (roots.w - fragmentTextureCoordinates.x) * fragmentScale.x * extent.x;
					horizontalCoverage -= clamp(diff + 0.5, 0.0, 1.0);
					horizontalWeight = max(horizontalWeight, clamp(1.0 - abs(diff) * 2.0, 0.0, 1.0));
				}
			}

			contributions = lookup >> (
				(p0.x > fragmentTextureCoordinates.x ? 2 : 0) +
				(p1.x > fragmentTextureCoordinates.x ? 4 : 0) +
				(p2.x > fragmentTextureCoordinates.x ? 8 : 0)
			);

			if(contributions > 0)
			{
				const vec4 roots = verticalRoots(p0, p1, p2);

				if((contributions & 1) > 0 && roots.x >= 0.0 && roots.x <= 1.0)
				{
					const float diff = (roots.z - fragmentTextureCoordinates.y) * fragmentScale.y * extent.y;
					verticalCoverage += clamp(diff + 0.5, 0.0, 1.0);
					verticalWeight = max(verticalWeight, clamp(1.0 - abs(diff) * 2.0, 0.0, 1.0));
				}

				if((contributions & 2) > 0 && roots.y >= 0.0 && roots.y <= 1.0)
				{
					const float diff = (roots.w - fragmentTextureCoordinates.y) * fragmentScale.y * extent.y;
					verticalCoverage -= clamp(diff + 0.5, 0.0, 1.0);
					verticalWeight = max(verticalWeight, clamp(1.0 - abs(diff) * 2.0, 0.0, 1.0));
				}
			}
		}

		firstPoint = lastPoint + 1;
	}

	if(fragmentTextureCoordinates.x <= 0.01 || fragmentTextureCoordinates.x >= 0.99 || fragmentTextureCoordinates.y <= 0.01 || fragmentTextureCoordinates.y >= 0.99)
	{
		fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else
	{
		fragColor = vec4(1.0, 1.0, 1.0, coverage(horizontalCoverage, verticalCoverage, horizontalWeight, verticalWeight));
	}
}
