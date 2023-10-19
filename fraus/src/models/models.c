#include "../../include/fraus/models/models.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

FrResult frLoadOBJ(const char* pPath, FrModel* pModel)
{
	// Open file
	FILE* file = fopen(pPath, "r");
	if(!file) return FR_ERROR_FILE_NOT_FOUND;

	// Vertices
	uint32_t verticesFakeCount = 256;
	uint32_t vertexCount = 0;
	FrVec3* pVertices = malloc(verticesFakeCount * sizeof(FrVec3));
	if(!pVertices)
	{
		fclose(file);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	// Texture coordinates
	uint32_t textureCoordinatesFakeCount = 256;
	uint32_t textureCoordinateCount = 0;
	FrVec2* pTextureCoordinates = malloc(textureCoordinatesFakeCount * sizeof(FrVec2));
	if(!pTextureCoordinates)
	{
		free(pVertices);
		fclose(file);
		return FR_ERROR_OUT_OF_MEMORY;
	}

	// Normals
	uint32_t normalsFakeCount = 256;
	uint32_t normalCount = 0;
	FrVec3* pNormals = malloc(normalsFakeCount * sizeof(FrVec3));
	if(!pNormals)
	{
		free(pTextureCoordinates);
		free(pVertices);
		fclose(file);
		return FR_ERROR_OUT_OF_MEMORY;
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
				FrVec3* pNewVertices = realloc(pVertices, verticesFakeCount * sizeof(FrVec3));
				if(!pNewVertices)
				{
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pVertices = pNewVertices;
			}

			if(sscanf(
				buffer,
				"v %f %f %f",
				&pVertices[vertexCount].x,
				&pVertices[vertexCount].y,
				&pVertices[vertexCount].z
			) != 3)
			{
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
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
				FrVec2* pNewTextureCoordinates = realloc(pTextureCoordinates, textureCoordinatesFakeCount * sizeof(FrVec2));
				if(!pNewTextureCoordinates)
				{
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pTextureCoordinates = pNewTextureCoordinates;
			}

			if(sscanf(
				buffer,
				"vt %f %f",
				&pTextureCoordinates[textureCoordinateCount].x,
				&pTextureCoordinates[textureCoordinateCount].y
			) != 2)
			{
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}
			pTextureCoordinates[textureCoordinateCount].y = 1.0f - pTextureCoordinates[textureCoordinateCount].y;

			++textureCoordinateCount;
		}

		// Normals
		else if(buffer[0] == 'v' && buffer[1] == 'n')
		{
			if(normalCount == normalsFakeCount)
			{
				normalsFakeCount *= 2;
				FrVec3* pNewNormals = realloc(pNormals, normalsFakeCount * sizeof(FrVec3));
				if(!pNewNormals)
				{
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pNormals = pNewNormals;
			}

			if(sscanf(
				buffer,
				"vn %f %f %f",
				&pNormals[normalCount].x,
				&pNormals[normalCount].y,
				&pNormals[normalCount].z
			) != 3)
			{
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
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
				pModel->pVertices = malloc(vertexCount * sizeof(FrVertex));
				if(!pModel->pVertices)
				{
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pModel->vertexCount = 0;

				pModel->pIndexes = malloc(normalCount * sizeof(uint32_t));
				if(!pModel->pIndexes)
				{
					free(pModel->pVertices);
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pModel->indexCount = 0;

				if(frCreateMap(vertexCount, &map) != FR_SUCCESS)
				{
					free(pModel->pIndexes);
					free(pModel->pVertices);
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
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
				free(pModel->pIndexes);
				free(pModel->pVertices);
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			faceVertex = (FrVertex){
				pVertices[pVertexIndex[0] - 1],
				pTextureCoordinates[pTextureCoordinatesIndex[0] - 1],
				pNormals[pNormalIndex[0] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				pModel->vertexCount,
				&pModel->pIndexes[pModel->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(pModel->pIndexes);
				free(pModel->pVertices);
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
				fclose(file);
				return FR_ERROR_OUT_OF_MEMORY;
			}
			if(pModel->pIndexes[pModel->indexCount] == pModel->vertexCount)
			{
				if(pModel->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* pNewVertices = realloc(pModel->pVertices, vertexCount * sizeof(FrVertex));
					if(!pNewVertices)
					{
						frDestroyMap(&map);
						free(pModel->pIndexes);
						free(pModel->pVertices);
						free(pNormals);
						free(pTextureCoordinates);
						free(pVertices);
						fclose(file);
						return FR_ERROR_OUT_OF_MEMORY;
					}
					pModel->pVertices = pNewVertices;
				}

				pModel->pVertices[pModel->vertexCount] = faceVertex;
				++pModel->vertexCount;
			}

			++pModel->indexCount;
			if(pModel->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* pNewIndexes = realloc(pModel->pIndexes, normalCount * sizeof(uint32_t));
				if(!pNewIndexes)
				{
					frDestroyMap(&map);
					free(pModel->pIndexes);
					free(pModel->pVertices);
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pModel->pIndexes = pNewIndexes;
			}

			faceVertex = (FrVertex){
				pVertices[pVertexIndex[1] - 1],
				pTextureCoordinates[pTextureCoordinatesIndex[1] - 1],
				pNormals[pNormalIndex[1] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				pModel->vertexCount,
				&pModel->pIndexes[pModel->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(pModel->pIndexes);
				free(pModel->pVertices);
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
				fclose(file);
				return FR_ERROR_OUT_OF_MEMORY;
			}
			if(pModel->pIndexes[pModel->indexCount] == pModel->vertexCount)
			{
				if(pModel->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* pNewVertices = realloc(pModel->pVertices, vertexCount * sizeof(FrVertex));
					if (!pNewVertices)
					{
						frDestroyMap(&map);
						free(pModel->pIndexes);
						free(pModel->pVertices);
						free(pNormals);
						free(pTextureCoordinates);
						free(pVertices);
						fclose(file);
						return FR_ERROR_OUT_OF_MEMORY;
					}
					pModel->pVertices = pNewVertices;
				}

				pModel->pVertices[pModel->vertexCount] = faceVertex;
				++pModel->vertexCount;
			}

			++pModel->indexCount;
			if(pModel->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* pNewIndexes = realloc(pModel->pIndexes, normalCount * sizeof(uint32_t));
				if(!pNewIndexes)
				{
					frDestroyMap(&map);
					free(pModel->pIndexes);
					free(pModel->pVertices);
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pModel->pIndexes = pNewIndexes;
			}

			faceVertex = (FrVertex){
				pVertices[pVertexIndex[2] - 1],
				pTextureCoordinates[pTextureCoordinatesIndex[2] - 1],
				pNormals[pNormalIndex[2] - 1]
			};
			if(frGetOrInsertMap(
				&map,
				&faceVertex,
				pModel->vertexCount,
				&pModel->pIndexes[pModel->indexCount]
			) != FR_SUCCESS)
			{
				frDestroyMap(&map);
				free(pModel->pIndexes);
				free(pModel->pVertices);
				free(pNormals);
				free(pTextureCoordinates);
				free(pVertices);
				fclose(file);
				return FR_ERROR_OUT_OF_MEMORY;
			}
			if(pModel->pIndexes[pModel->indexCount] == pModel->vertexCount)
			{
				if(pModel->vertexCount == vertexCount)
				{
					vertexCount *= 2;
					FrVertex* pNewVertices = realloc(pModel->pVertices, vertexCount * sizeof(FrVertex));
					if(!pNewVertices)
					{
						frDestroyMap(&map);
						free(pModel->pIndexes);
						free(pModel->pVertices);
						free(pNormals);
						free(pTextureCoordinates);
						free(pVertices);
						fclose(file);
						return FR_ERROR_OUT_OF_MEMORY;
					}
					pModel->pVertices = pNewVertices;
				}

				pModel->pVertices[pModel->vertexCount] = faceVertex;
				++pModel->vertexCount;
			}

			++pModel->indexCount;
			if(pModel->indexCount == normalCount)
			{
				normalCount *= 2;
				uint32_t* pNewIndexes = realloc(pModel->pIndexes, normalCount * sizeof(uint32_t));
				if(!pNewIndexes)
				{
					frDestroyMap(&map);
					free(pModel->pIndexes);
					free(pModel->pVertices);
					free(pNormals);
					free(pTextureCoordinates);
					free(pVertices);
					fclose(file);
					return FR_ERROR_OUT_OF_MEMORY;
				}
				pModel->pIndexes = pNewIndexes;
			}
		}
	}

	free(pNormals);
	free(pTextureCoordinates);
	free(pVertices);

	fclose(file);

	frDestroyMap(&map);

	FrVertex* pFinalVertices = realloc(pModel->pVertices, pModel->vertexCount * sizeof(FrVertex));
	if(!pFinalVertices)
	{
		free(pModel->pIndexes);
		free(pModel->pVertices);
		return FR_ERROR_OUT_OF_MEMORY;
	}
	pModel->pVertices = pFinalVertices;

	uint32_t* pFinalIndexes = realloc(pModel->pIndexes, pModel->indexCount * sizeof(uint32_t));
	if(!pFinalIndexes)
	{
		free(pModel->pIndexes);
		free(pModel->pVertices);
		return FR_ERROR_OUT_OF_MEMORY;
	}
	pModel->pIndexes = pFinalIndexes;

	return FR_SUCCESS;
}
