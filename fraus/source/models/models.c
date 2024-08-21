#include "../../include/fraus/models/models.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

FrResult frLoadOBJ(const char* path, FrModel* pModel)
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
		pVertexIndex[3],
		pTextureCoordinatesIndex[3],
		pNormalIndex[3];
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
				pModel->vertices = malloc(vertexCount * sizeof(pModel->vertices[0]));
				if(!pModel->vertices)
				{
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				pModel->vertexCount = 0;

				pModel->indexes = malloc(normalCount * sizeof(pModel->indexes[0]));
				if(!pModel->indexes)
				{
					free(pModel->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				pModel->indexCount = 0;

				if(frCreateMap(vertexCount, &map) != FR_SUCCESS)
				{
					free(pModel->indexes);
					free(pModel->vertices);
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
				&pVertexIndex[0],
				&pTextureCoordinatesIndex[0],
				&pNormalIndex[0],
				&pVertexIndex[1],
				&pTextureCoordinatesIndex[1],
				&pNormalIndex[1],
				&pVertexIndex[2],
				&pTextureCoordinatesIndex[2],
				&pNormalIndex[2]
			) != 9)
			{
				frDestroyMap(&map);
				free(pModel->indexes);
				free(pModel->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			faceVertex = (FrVertex){
				vertices[pVertexIndex[0] - 1],
				textureCoordinates[pTextureCoordinatesIndex[0] - 1],
				normals[pNormalIndex[0] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				pModel->vertexCount,
				&pModel->indexes[pModel->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(pModel->indexes);
				free(pModel->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			if(pModel->indexes[pModel->indexCount] == pModel->vertexCount)
			{
				if(pModel->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* const newVertices = realloc(pModel->vertices, vertexCount * sizeof(newVertices[0]));
					if(!newVertices)
					{
						frDestroyMap(&map);
						free(pModel->indexes);
						free(pModel->vertices);
						free(normals);
						free(textureCoordinates);
						free(vertices);
						fclose(file);
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					pModel->vertices = newVertices;
				}

				pModel->vertices[pModel->vertexCount] = faceVertex;
				++pModel->vertexCount;
			}

			++pModel->indexCount;
			if(pModel->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* const newIndexes = realloc(pModel->indexes, normalCount * sizeof(newIndexes[0]));
				if(!newIndexes)
				{
					frDestroyMap(&map);
					free(pModel->indexes);
					free(pModel->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				pModel->indexes = newIndexes;
			}

			faceVertex = (FrVertex){
				vertices[pVertexIndex[1] - 1],
				textureCoordinates[pTextureCoordinatesIndex[1] - 1],
				normals[pNormalIndex[1] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				pModel->vertexCount,
				&pModel->indexes[pModel->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(pModel->indexes);
				free(pModel->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			if(pModel->indexes[pModel->indexCount] == pModel->vertexCount)
			{
				if(pModel->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* const newVertices = realloc(pModel->vertices, vertexCount * sizeof(newVertices[0]));
					if(!newVertices)
					{
						frDestroyMap(&map);
						free(pModel->indexes);
						free(pModel->vertices);
						free(normals);
						free(textureCoordinates);
						free(vertices);
						fclose(file);
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					pModel->vertices = newVertices;
				}

				pModel->vertices[pModel->vertexCount] = faceVertex;
				++pModel->vertexCount;
			}

			++pModel->indexCount;
			if(pModel->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* const newIndexes = realloc(pModel->indexes, normalCount * sizeof(newIndexes[0]));
				if(!newIndexes)
				{
					frDestroyMap(&map);
					free(pModel->indexes);
					free(pModel->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				pModel->indexes = newIndexes;
			}

			faceVertex = (FrVertex){
				vertices[pVertexIndex[2] - 1],
				textureCoordinates[pTextureCoordinatesIndex[2] - 1],
				normals[pNormalIndex[2] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				pModel->vertexCount,
				&pModel->indexes[pModel->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(pModel->indexes);
				free(pModel->vertices);
				free(normals);
				free(textureCoordinates);
				free(vertices);
				fclose(file);
				return FR_ERROR_OUT_OF_HOST_MEMORY;
			}
			if(pModel->indexes[pModel->indexCount] == pModel->vertexCount)
			{
				if(pModel->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* const newVertices = realloc(pModel->vertices, vertexCount * sizeof(newVertices[0]));
					if(!newVertices)
					{
						frDestroyMap(&map);
						free(pModel->indexes);
						free(pModel->vertices);
						free(normals);
						free(textureCoordinates);
						free(vertices);
						fclose(file);
						return FR_ERROR_OUT_OF_HOST_MEMORY;
					}
					pModel->vertices = newVertices;
				}

				pModel->vertices[pModel->vertexCount] = faceVertex;
				++pModel->vertexCount;
			}

			++pModel->indexCount;
			if(pModel->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* const newIndexes = realloc(pModel->indexes, normalCount * sizeof(newIndexes[0]));
				if(!newIndexes)
				{
					frDestroyMap(&map);
					free(pModel->indexes);
					free(pModel->vertices);
					free(normals);
					free(textureCoordinates);
					free(vertices);
					fclose(file);
					return FR_ERROR_OUT_OF_HOST_MEMORY;
				}
				pModel->indexes = newIndexes;
			}
		}
	}

	free(normals);
	free(textureCoordinates);
	free(vertices);

	fclose(file);

	frDestroyMap(&map);

	FrVertex* const finalVertices = realloc(pModel->vertices, pModel->vertexCount * sizeof(finalVertices[0]));
	if(!finalVertices)
	{
		free(pModel->indexes);
		free(pModel->vertices);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	pModel->vertices = finalVertices;
	for(uint32_t vertexIndex = 0; vertexIndex < pModel->vertexCount; ++vertexIndex)
	{
		pModel->vertices[vertexIndex].textureCoordinates.y = textureCoordinateMax -  pModel->vertices[vertexIndex].textureCoordinates.y;
	}

	uint32_t* const finalIndexes = realloc(pModel->indexes, pModel->indexCount * sizeof(finalIndexes[0]));
	if(!finalIndexes)
	{
		free(pModel->indexes);
		free(pModel->vertices);
		return FR_ERROR_OUT_OF_HOST_MEMORY;
	}
	pModel->indexes = finalIndexes;

	return FR_SUCCESS;
}
