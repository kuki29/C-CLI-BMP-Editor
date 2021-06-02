#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "transformations.h"

typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint16_t WORD;

LONG get_image_size(const struct bmp_header* H);
LONG get_row_size(LONG width);
int get_padding(LONG width);

struct bmp_image* flip_horizontally(const struct bmp_image* image)
{
	if (image == NULL || image->header == NULL || image->data == NULL)
	{
		return NULL;
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	DWORD width = result->header->width;
	DWORD height = result->header->height;

	result->data = malloc((size_t)get_image_size(result->header));

	for (int i = 0; i < height; i++)
	{
		for(int j = 0; j < width; j++)
		{
			memcpy(((BYTE*)result->data + get_row_size((LONG)width) * i + j * 3), \
				((BYTE*)image->data + get_row_size((LONG)width) * i + ((LONG)width - j) * 3), 3);
		}
	}

	return result;

}

struct bmp_image* flip_vertically(const struct bmp_image* image)
{
	if (image == NULL || image->header == NULL || image->data == NULL)
	{
		return NULL;
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	DWORD width = result->header->width;
	DWORD height = result->header->height;

	result->data = malloc((size_t)get_image_size(result->header));

	for (int i = 0; i < height; i++)
	{
		memcpy(((BYTE*)result->data + get_row_size((LONG)width) * i), \
			((BYTE*)image->data + get_row_size((LONG)width) * ((LONG)height - i)), (size_t)(width * 3));
	}

	return result;
}

struct bmp_image* rotate_right(const struct bmp_image* image)
{
	if (image == NULL || image->header == NULL || image->data == NULL)
	{
		return NULL;
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	DWORD width = result->header->height;
	DWORD height = result->header->width;

	result->header->width = width;
	result->header->height = height;

	result->data = malloc((size_t)get_image_size(result->header));

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			memcpy(((BYTE*)result->data + get_row_size((LONG)width) * i + j * 3), \
				((BYTE*)image->data + get_row_size((LONG)height) * j + ((LONG)height - i) * 3), 3);
		}
	}

	return result;
}

struct bmp_image* rotate_left(const struct bmp_image* image)
{
	if (image == NULL || image->header == NULL || image->data == NULL)
	{
		return NULL;
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	DWORD width = result->header->height;
	DWORD height = result->header->width;

	result->header->width = width;
	result->header->height = height;

	result->data = malloc((size_t)get_image_size(result->header));

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			memcpy(((BYTE*)result->data + get_row_size((LONG)width) * i + j * 3), \
				((BYTE*)image->data + get_row_size((LONG)height) * ((LONG)width - j) + i * 3), 3);
		}
	}

	return result;
}

struct bmp_image* crop(const struct bmp_image* image, const uint32_t x, const uint32_t y, const uint32_t h, const uint32_t w)
{
	if (image == NULL || image->header == NULL || image->data == NULL)
	{
		return NULL;
	}

	if (x + w >= image->header->width || y + h >= image->header->height || \
		x < 0 || y < 0 || h < 1 || w < 1)
	{
		return NULL;
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	result->header->width = w;
	result->header->height = h;
	result->header->image_size = 0;
	result->header->image_size = (DWORD)get_image_size(result->header);
	result->header->size = (DWORD)sizeof(struct bmp_header) + result->header->image_size;

	result->data = malloc((size_t)get_image_size(result->header));

	for (int i = 0; i < h; i++)
	{
		memcpy(((BYTE*)result->data + i * get_row_size((LONG)w)), \
			((BYTE*)image->data + (i + (int)y) * get_row_size((LONG)image->header->width) + x * 3), w * 3);
	}

	return result;

}

struct bmp_image* scale(const struct bmp_image* image, float scaleFactor)
{
	if (image == NULL || image->header == NULL || image->data == NULL || scaleFactor <= 0)
	{
		return NULL;
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	if (scaleFactor == 1)
	{
		result->data = malloc((size_t)get_image_size(result->header));
		memcpy(result->data, image->data, (size_t)get_image_size(result->header));

		return result;
	}

	DWORD width = (DWORD)round((float)result->header->width * scaleFactor);
	DWORD height = (DWORD)round((float)result->header->height * scaleFactor);

	result->header->width = width;
	result->header->height = height;
	result->header->image_size = (DWORD)get_image_size(result->header);
	result->header->size = (DWORD)sizeof(struct bmp_header) + (DWORD)get_image_size(result->header);

	result->data = malloc((size_t)get_image_size(result->header));
	memset(result->data, 0, (size_t)get_image_size(result->header));

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			memcpy(((BYTE*)result->data + i * get_row_size((LONG)width) + j * 3), \
				((BYTE*)image->data + (LONG)round((float)i / scaleFactor) * get_row_size((LONG)image->header->width) + (LONG)round((float)j / scaleFactor) * 3), 3);
		}
	}

	return result;

}

struct bmp_image* extract(const struct bmp_image* image, const char* color_to_keep)
{
	if (image == NULL || image->header == NULL || image->data == NULL || color_to_keep == NULL)
	{
		return NULL;
	}

	bool saveColor[3] = {false, false, false};
	bool keepAll = true;

	int i = 0;
	bool run = true;
	while (run)
	{
		switch (color_to_keep[i++])
		{
		case 'b':
			saveColor[0] = true;
			keepAll = false;
			break;

		case 'g':
			saveColor[1] = true;
			keepAll = false;
			break;

		case 'r':
			saveColor[2] = true;
			keepAll = false;
			break;

		case '\0':
			run = false;
			break;

		default:
			return NULL;
		}
	}

	struct bmp_image* result = malloc(sizeof(struct bmp_image));
	result->header = malloc(sizeof(struct bmp_header));
	memcpy(result->header, image->header, sizeof(struct bmp_header));

	if (keepAll)
	{
		result->data = malloc((size_t)get_image_size(result->header));
		memcpy(result->data, image->data, (size_t)get_image_size(result->header));

		return result;
	}

	DWORD width = result->header->height;
	DWORD height = result->header->width;

	result->data = malloc((size_t)get_image_size(result->header));
	memset(((BYTE*)result->data), 0, (size_t)get_image_size(result->header));

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				if (saveColor[k])
				{
					*(((BYTE*)result->data) + (i * get_row_size((LONG)width) + j * 3 + k)) = *((BYTE*)image->data + (i * get_row_size((LONG)width) + j * 3 + k));
				}
				else
				{
					*(((BYTE*)result->data) + (i * get_row_size((LONG)width) + j * 3 + k)) = 0x00;
				}
			}
		}
	}

	return result;
}
