#include "../../include/fraus/models/models.h"

#include <stdio.h>
#include <stdlib.h>

FrResult frLoadOBJ(const char* const path, FrModel* const model)
{
	// Open file
	FILE* const file = fopen(path, "r");
	if(!file)
	{
		return FR_ERROR_FILE_NOT_FOUND;
	}

	// Vertices
	uint32_t verticesFakeCount = 256;
	uint32_t vertexCount = 0;
	FrVec3* vertices = malloc(verticesFakeCount * sizeof(vertices[0]));
	if(!vertices)
	{
		fclose(file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Texture coordinates
	uint32_t textureCoordinatesFakeCount = 256;
	uint32_t textureCoordinateCount = 0;
	FrVec2* textureCoordinates = malloc(textureCoordinatesFakeCount * sizeof(textureCoordinates[0]));
	if(!textureCoordinates)
	{
		free(vertices);
		fclose(file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	float textureCoordinateMax = 0.f;

	// Normals
	uint32_t normalsFakeCount = 256;
	uint32_t normalCount = 0;
	FrVec3* normals = malloc(normalsFakeCount * sizeof(normals[0]));
	if(!normals)
	{
		free(textureCoordinates);
		free(vertices);
		fclose(file);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Read lines
	char buffer[256];
	uint32_t
		vertexIndex[3],
		textureCoordinatesIndex[3],
		normalIndex[3];
	FrVertex faceVertex;
	bool firstFace = true;
	FrMap map;
	while(fgets(buffer, sizeof(buffer), file))
	{
		// Vertices
		if(buffer[0] == 'v' && buffer[1] == ' ')
		{
			if(vertexCount == verticesFakeCount)
			{
				verticesFakeCount *= 2;
				FrVec3* const newVertices = realloc(vertices, verticesFakeCount * sizeof(newVertices[0]));
				if(!newVertices)
				{
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				vertices = newVertices;
			}

			if(sscanf(
				buffer,
				"v %f %f %f",
				&vertices[vertexCount].x,
				&vertices[vertexCount].y,
				&vertices[vertexCount].z
			) != 3)
			{
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			++vertexCount;
		}

		// Texture coordinates
		else if(buffer[0] == 'v' && buffer[1] == 't')
		{
			if(textureCoordinateCount == textureCoordinatesFakeCount)
			{
				textureCoordinatesFakeCount *= 2;
				FrVec2* const newTextureCoordinates = realloc(textureCoordinates, textureCoordinatesFakeCount * sizeof(newTextureCoordinates[0]));
				if(!newTextureCoordinates)
				{
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				textureCoordinates = newTextureCoordinates;
			}

			if(sscanf(
				buffer,
				"vt %f %f",
				&textureCoordinates[textureCoordinateCount].x,
				&textureCoordinates[textureCoordinateCount].y
			) != 2)
			{
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}
			if(textureCoordinates[textureCoordinateCount].y > textureCoordinateMax)
			{
				textureCoordinateMax = textureCoordinates[textureCoordinateCount].y;
			}

			++textureCoordinateCount;
		}

		// Normals
		else if(buffer[0] == 'v' && buffer[1] == 'n')
		{
			if(normalCount == normalsFakeCount)
			{
				normalsFakeCount *= 2;
				FrVec3* const newNormals = realloc(normals, normalsFakeCount * sizeof(newNormals[0]));
				if(!newNormals)
				{
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				normals = newNormals;
			}

			if(sscanf(
				buffer,
				"vn %f %f %f",
				&normals[normalCount].x,
				&normals[normalCount].y,
				&normals[normalCount].z
			) != 3)
			{
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			++normalCount;
		}

		// Faces
		else if(buffer[0] == 'f')
		{
			if(firstFace)
			{
				model->vertices = malloc(vertexCount * sizeof(model->vertices[0]));
				if(!model->vertices)
				{
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				model->vertexCount = 0;

				model->indexes = malloc(normalCount * sizeof(model->indexes[0]));
				if(!model->indexes)
				{
					free(model->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				model->indexCount = 0;

				if(frCreateMap(vertexCount, &map) != FR_SUCCESS)
				{
					free(model->indexes);
					free(model->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}

				firstFace = false;
			}

			if(sscanf(
				buffer,
				"f %u/%u/%u %u/%u/%u %u/%u/%u",
				&vertexIndex[0],
				&textureCoordinatesIndex[0],
				&normalIndex[0],
				&vertexIndex[1],
				&textureCoordinatesIndex[1],
				&normalIndex[1],
				&vertexIndex[2],
				&textureCoordinatesIndex[2],
				&normalIndex[2]
			) != 9)
			{
				frDestroyMap(&map);
				free(model->indexes);
				free(model->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			faceVertex = (FrVertex){
				vertices[vertexIndex[0] - 1],
				textureCoordinates[textureCoordinatesIndex[0] - 1],
				normals[normalIndex[0] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				model->vertexCount,
				&model->indexes[model->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(model->indexes);
				free(model->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			if(model->indexes[model->indexCount] == model->vertexCount)
			{
				if(model->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* const newVertices = realloc(model->vertices, vertexCount * sizeof(newVertices[0]));
					if(!newVertices)
					{
						frDestroyMap(&map);
						free(model->indexes);
						free(model->vertices);
						free(normals);
						free(textureCoordinates);
						free(vertices);
						fclose(file);
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					model->vertices = newVertices;
				}

				model->vertices[model->vertexCount] = faceVertex;
				++model->vertexCount;
			}

			++model->indexCount;
			if(model->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* const newIndexes = realloc(model->indexes, normalCount * sizeof(newIndexes[0]));
				if(!newIndexes)
				{
					frDestroyMap(&map);
					free(model->indexes);
					free(model->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				model->indexes = newIndexes;
			}

			faceVertex = (FrVertex){
				vertices[vertexIndex[1] - 1],
				textureCoordinates[textureCoordinatesIndex[1] - 1],
				normals[normalIndex[1] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				model->vertexCount,
				&model->indexes[model->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(model->indexes);
				free(model->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			if(model->indexes[model->indexCount] == model->vertexCount)
			{
				if(model->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* const newVertices = realloc(model->vertices, vertexCount * sizeof(newVertices[0]));
					if(!newVertices)
					{
						frDestroyMap(&map);
						free(model->indexes);
						free(model->vertices);
						free(normals);
						free(textureCoordinates);
						free(vertices);
						fclose(file);
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					model->vertices = newVertices;
				}

				model->vertices[model->vertexCount] = faceVertex;
				++model->vertexCount;
			}

			++model->indexCount;
			if(model->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* const newIndexes = realloc(model->indexes, normalCount * sizeof(newIndexes[0]));
				if(!newIndexes)
				{
					frDestroyMap(&map);
					free(model->indexes);
					free(model->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				model->indexes = newIndexes;
			}

			faceVertex = (FrVertex){
				vertices[vertexIndex[2] - 1],
				textureCoordinates[textureCoordinatesIndex[2] - 1],
				normals[normalIndex[2] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				model->vertexCount,
				&model->indexes[model->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(model->indexes);
				free(model->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			if(model->indexes[model->indexCount] == model->vertexCount)
			{
				if(model->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* const newVertices = realloc(model->vertices, vertexCount * sizeof(newVertices[0]));
					if(!newVertices)
					{
						frDestroyMap(&map);
						free(model->indexes);
						free(model->vertices);
						free(normals);
						free(textureCoordinates);
						free(vertices);
						fclose(file);
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					model->vertices = newVertices;
				}

				model->vertices[model->vertexCount] = faceVertex;
				++model->vertexCount;
			}

			++model->indexCount;
			if(model->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* const newIndexes = realloc(model->indexes, normalCount * sizeof(newIndexes[0]));
				if(!newIndexes)
				{
					frDestroyMap(&map);
					free(model->indexes);
					free(model->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				model->indexes = newIndexes;
			}
		}
	}

	free(normals);
	free(textureCoordinates);
	free(vertices);

	fclose(file);

	frDestroyMap(&map);

	FrVertex* const finalVertices = realloc(model->vertices, model->vertexCount * sizeof(finalVertices[0]));
	if(!finalVertices)
	{
		free(model->indexes);
		free(model->vertices);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	model->vertices = finalVertices;
	for(uint32_t vertexIndex = 0; vertexIndex < model->vertexCount; ++vertexIndex)
	{
		model->vertices[vertexIndex].textureCoordinates.y = textureCoordinateMax -  model->vertices[vertexIndex].textureCoordinates.y;
	}

	uint32_t* const finalIndexes = realloc(model->indexes, model->indexCount * sizeof(finalIndexes[0]));
	if(!finalIndexes)
	{
		free(model->indexes);
		free(model->vertices);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	model->indexes = finalIndexes;

	return FR_SUCCESS;
}
