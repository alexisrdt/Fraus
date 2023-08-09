#include "images.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MSBF = Most Significant Byte First
#define FR_MSBF_TO_U16(bytes) \
(((bytes)[0] << 8) | (bytes)[1])

#define FR_MSBF_TO_U32(bytes) \
(((bytes)[0] << 24) | ((bytes)[1] << 16) | ((bytes)[2] << 8) | (bytes)[3])

#define FR_REVERSE_BYTE(byte) \
(((byte & 1) << 7) | ((byte & 2) << 5) | ((byte & 4) << 3) | ((byte & 8) << 1) | ((byte & 16) >> 1) | ((byte & 32) >> 3) | ((byte & 64) >> 5) | ((byte & 128) >> 7))

static uint32_t frCRC(const uint8_t* pData, uint32_t size)
{
	const uint32_t pCRCTable[256] = {
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
		0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
		0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
		0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
		0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
		0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
		0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
		0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
		0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
		0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
		0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
		0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
		0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
		0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
		0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
		0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
		0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
		0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
		0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
		0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
		0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
		0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
		0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
		0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
		0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
		0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
		0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
		0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
		0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
		0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
		0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
		0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
		0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
		0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
		0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
		0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
		0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
		0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
		0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
		0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
		0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
		0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
		0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
		0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
		0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
		0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
		0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
		0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
	};

	uint32_t CRC = 0xFFFFFFFF;

	while(size--)
	{
		CRC = pCRCTable[(CRC & 0xFF) ^ *(pData++)] ^ (CRC >> 8);
	}

	return ~CRC;
}

static uint8_t frPaeth(uint8_t a, uint8_t b, uint8_t c)
{
	const int16_t p = a + b - c;

	const int16_t pa = abs(p - a);
	const int16_t pb = abs(p - b);
	const int16_t pc = abs(p - c);

	if(pa <= pb && pa <= pc) return a;
	if(pb <= pc) return b;
	return c;
}

FrResult frLoadPNG(const char* pPath, FrImage* pImage)
{
	// Check invalid arguments
	if(!pPath || !pImage) return FR_ERROR_INVALID_ARGUMENT;

	// Open file
	FILE* file = fopen(pPath, "rb");
	if(!file) return errno == ENOENT ? FR_ERROR_FILE_NOT_FOUND : FR_ERROR_UNKNOWN;

	// Check PNG signature
	uint8_t signature[8];
	if(fread(signature, 1, 8, file) != 8)
	{
		fclose(file);
		return FR_ERROR_UNKNOWN;
	}
	if(
		signature[0] != 137 ||
		signature[1] != 80  ||
		signature[2] != 78  ||
		signature[3] != 71  ||
		signature[4] != 13  ||
		signature[5] != 10  ||
		signature[6] != 26  ||
		signature[7] != 10
	)
	{
		fclose(file);
		return FR_ERROR_CORRUPTED_FILE;
	}

	bool first = true;
	bool palette = false;
	bool dataChunksStarted = false;
	bool dataChunksFinished = false;
	uint8_t bitDepth;
	uint8_t* pData = NULL;
	size_t dataSize = 0;

	// Loop through PNG chunks
	while(true)
	{
		// Read chunk length
		uint8_t pLengthBuffer[4];
		if(fread(pLengthBuffer, 1, 4, file) != 4)
		{
			free(pData);
			fclose(file);
			if (getc(file) == EOF) return FR_ERROR_CORRUPTED_FILE;
			return FR_ERROR_UNKNOWN;
		}
		uint32_t length = FR_MSBF_TO_U32(pLengthBuffer);
		if(length >= (1 << 0x1F))
		{
			free(pData);
			fclose(file);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// Read chunk type and data
		uint8_t* pTypeAndData = malloc(length + 4);
		if(!pTypeAndData)
		{
			free(pData);
			fclose(file);
			return FR_ERROR_UNKNOWN;
		}
		if(fread(pTypeAndData, 1, length + 4, file) != length + 4)
		{
			free(pTypeAndData);
			free(pData);
			fclose(file);
			return FR_ERROR_UNKNOWN;
		}

		// Read chunk CRC
		uint8_t pCRCBuffer[4];
		if(fread(pCRCBuffer, 1, 4, file) != 4)
		{
			free(pTypeAndData);
			free(pData);
			fclose(file);
			return FR_ERROR_UNKNOWN;
		}
		uint32_t CRC = FR_MSBF_TO_U32(pCRCBuffer);

		// Check chunk CRC
		if(frCRC(pTypeAndData, length + 4) != CRC)
		{
			free(pTypeAndData);
			free(pData);
			fclose(file);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// IHDR chunk
		if(
			pTypeAndData[0] == 'I' &&
			pTypeAndData[1] == 'H' &&
			pTypeAndData[2] == 'D' &&
			pTypeAndData[3] == 'R'
		)
		{
			// IHDR should be first
			if(length != 13 || !first)
			{
				free(pTypeAndData);
				free(pData);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			// Read image dimnesions
			pImage->width = FR_MSBF_TO_U32(pTypeAndData + 4);
			pImage->height = FR_MSBF_TO_U32(pTypeAndData + 8);
			if(pImage->width == 0 || pImage->height == 0)
			{
				free(pTypeAndData);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			// Read bit depth
			bitDepth = pTypeAndData[12];

			// Read color type
			switch(pTypeAndData[13])
			{
				case 0:
					pImage->type = FR_GRAY;
					break;

				case 2:
					pImage->type = FR_RGB;
					break;

				case 3:
					palette = true;
					pImage->type = FR_RGB;
					break;

				case 4:
					pImage->type = FR_GRAY_ALPHA;
					break;

				case 6:
					pImage->type = FR_RGB_ALPHA;
					break;

				default:
					free(pTypeAndData);
					fclose(file);
					return FR_ERROR_CORRUPTED_FILE;
			}

			// Allocate data
			pImage->pData = malloc(pImage->width * pImage->height * pImage->type);
			if(!pImage->pData)
			{
				free(pTypeAndData);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			first = false;
		}

		// Check IHDR chunk was encountered
		if(first)
		{
			free(pTypeAndData);
			free(pData);
			fclose(file);
			return FR_ERROR_CORRUPTED_FILE;
		}

		// IDAT chunk
		if(
			pTypeAndData[0] == 'I' &&
			pTypeAndData[1] == 'D' &&
			pTypeAndData[2] == 'A' &&
			pTypeAndData[3] == 'T'
		)
		{
			// Check length and still in data block
			if(length == 0 || dataChunksFinished)
			{
				free(pTypeAndData);
				free(pData);
				fclose(file);
				return FR_ERROR_UNKNOWN;
			}

			if(!dataChunksStarted) dataChunksStarted = true;

			// Read data
			uint8_t* pNewData = (uint8_t*)realloc(pData, dataSize + length);
			if(!pNewData)
			{
				free(pTypeAndData);
				free(pData);
				fclose(file);
				return FR_ERROR_UNKNOWN;
			}
			pData = pNewData;

			memcpy(pData + dataSize, pTypeAndData + 4, length);

			dataSize += length;
		}

		// IEND chunk
		if(
			pTypeAndData[0] == 'I' &&
			pTypeAndData[1] == 'E' &&
			pTypeAndData[2] == 'N' &&
			pTypeAndData[3] == 'D'
		)
		{
			// Check end of file
			if(length != 0 || getc(file) != EOF)
			{
				free(pTypeAndData);
				free(pData);
				fclose(file);
				return FR_ERROR_CORRUPTED_FILE;
			}

			free(pTypeAndData);

			break;
		}

		// Free chunk data
		free(pTypeAndData);
	}

	// Close file
	fclose(file);

	// Check PNG has some data
	if(dataSize == 0)
	{
		free(pData);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Check zlib header corruption
	if(FR_MSBF_TO_U16(pData) % 31 != 0)
	{
		free(pData);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Check zlib compression method
	if((pData[0] & 0x0F) != 8)
	{
		free(pData);
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Check deflate window size
	uint8_t encoded_window = (pData[0] & 0xF0) >> 4;
	if(encoded_window > 7)
	{
		free(pData);
		return FR_ERROR_CORRUPTED_FILE;
	}
	uint16_t window = 1 << (encoded_window + 8);

	// Check preset dictionnary
	if(pData[1] & 0x20)
	{
		free(pData);
		return FR_ERROR_CORRUPTED_FILE;
	}

	size_t resultSize = (size_t)pImage->height * ((size_t)pImage->width * (size_t)pImage->type + 1);
	uint8_t* pInflateResult = malloc(resultSize);
	if(!pInflateResult)
	{
		free(pData);
		return FR_ERROR_OUT_OF_MEMORY;
	}
	FrResult result = frInflate(pData + 2, dataSize - 6, pInflateResult);
	if(result != FR_SUCCESS)
	{
		free(pData);
		free(pInflateResult);
		return result;
	}

	// Check zlib Adler-32 checksum
	uint32_t s1 = 1, s2 = 0;
	for(size_t i = 0; i < resultSize; ++i)
	{
		s1 = (s1 + pInflateResult[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	if((s2 << 0x10 | s1) != FR_MSBF_TO_U32(pData + dataSize - 4))
	{
		return FR_ERROR_CORRUPTED_FILE;
	}

	// Free compressed data
	free(pData);

	// Undo filtering
	for(uint32_t i = 0; i < pImage->height; ++i)
	{
		switch(pInflateResult[(pImage->width * pImage->type + 1) * i])
		{
			// Same byte
			case 0:
				for(uint32_t j = 0; j < pImage->width * pImage->type; ++j)
				{
					pImage->pData[pImage->width * pImage->type * i + j] = pInflateResult[(pImage->width * pImage->type + 1) * i + j + 1];
				}
				break;

			// Same byte in previous pixel
			case 1:
				for(uint32_t j = 0; j < pImage->width * pImage->type; ++j)
				{
					pImage->pData[pImage->width * pImage->type * i + j] = pInflateResult[(pImage->width * pImage->type + 1) * i + j + 1] + (j < pImage->type ? 0 : pImage->pData[pImage->type * (pImage->width * i - 1) + j]);
				}
				break;

			// Same byte in previous scanline
			case 2:
				for(uint32_t j = 0; j < pImage->width * pImage->type; ++j)
				{
					pImage->pData[pImage->width * pImage->type * i + j] = pInflateResult[(pImage->width * pImage->type + 1) * i + j + 1] + pImage->pData[pImage->width * pImage->type * (i - 1) + j];
				}
				break;

			// Same byte in previous pixel in previous scanline
			case 3:
				for(uint32_t j = 0; j < pImage->width * pImage->type; ++j)
				{
					pImage->pData[pImage->width * pImage->type * i + j] = pInflateResult[(pImage->width * pImage->type + 1) * i + j + 1] + ((j < pImage->type ? 0 : pImage->pData[pImage->type * (pImage->width * i - 1) + j]) + pImage->pData[pImage->width * pImage->type * (i - 1) + j]) / 2;
				}
				break;

			// Paeth
			case 4:
				for(uint32_t j = 0; j < pImage->width * pImage->type; ++j)
				{
					pImage->pData[pImage->width * pImage->type * i + j] =
						pInflateResult[(pImage->width * pImage->type + 1) * i + j + 1] +
						frPaeth(
							j < pImage->type ? 0 : pImage->pData[pImage->type * (pImage->width * i - 1) + j],
							i == 0 ? 0 : (pImage->pData[pImage->width * pImage->type * (i - 1) + j]),
							i == 0 ? 0 : (j < pImage->type ? 0 : pImage->pData[pImage->type * (pImage->width * (i - 1) - 1) + j])
						);
				}
				break;

			// Unknown filtering method
			default:
				return FR_ERROR_CORRUPTED_FILE;
		}
	}

	free(pInflateResult);

	return FR_SUCCESS;
}
