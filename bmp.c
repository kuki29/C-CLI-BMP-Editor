#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "bmp.h"

#define BM 0x4D42
#define MB 0x424D

#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))

typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint16_t WORD;

typedef struct bmp_image BMP_IMAGE;
typedef struct bmp_header HEADER;

LONG get_image_size(const HEADER* H);
LONG get_row_size(LONG width);
int get_padding(LONG width);

unsigned int base64_encode(const unsigned char* in, unsigned int inlen, char* out);
unsigned int base64_decode(const char* in, unsigned int inlen, unsigned char* out);

BMP_IMAGE* read_bmp(FILE* stream)
{
	if (stream == NULL)
	{
		return NULL;
	}

	BMP_IMAGE* image = malloc(sizeof(BMP_IMAGE));
	image->header = read_bmp_header(stream);

	if (image->header == NULL || image->header->width == 0 || image->header->height == 0)
	{
		free_bmp_image(image);
		return NULL;
	}

	image->data = read_data(stream, image->header);
	if (image->data == NULL)
	{
		free_bmp_image(image);
		return NULL;
	}

	return image;
}

HEADER* read_bmp_header(FILE* stream)
{
	if (stream == NULL)
	{
		return NULL;
	}

	HEADER* H = malloc(sizeof(HEADER));

	if (stream != NULL && fread(H, sizeof(HEADER), 1, stream) == 1)
	{

		if (H->type == BM || H->type == MB)
		{
			return H;
		}
		else if (*(char*)H == 'Q')
		{
			char* buffer = malloc(72);
			memcpy(buffer, H, sizeof(HEADER));
			fread(buffer + sizeof(HEADER), 72 - sizeof(HEADER), 1, stream);

			if (base64_decode(buffer, 72, (unsigned char*)H) != 54)
			{
				free(H);
				free(buffer);
				return NULL;
			}

			if (H->type == BM || H->type == MB)
			{
				free(buffer);
				return H;
			}
		}
	}

	free(H);
	printf("Error: This is not a BMP file.\n");

	return NULL;
}

struct pixel* read_data(FILE* stream, const HEADER* header)
{
	if (header == NULL || stream == NULL)
	{
		return NULL;

	}

	LONG imageSize = get_image_size(header);
	BYTE* data = malloc((size_t)imageSize);

	if (stream != NULL && fread(data, (size_t)imageSize, 1, stream) == 1)
	{
		if (ftell(stream) != 54 + imageSize)
		{
			char* encoded = malloc((size_t)(imageSize + 2) / 3 * 4);
			memcpy(encoded, data, (size_t)imageSize);
			fread(encoded + imageSize, (size_t)((imageSize + 2) / 3 * 4 - imageSize), 1, stream);

			if (base64_decode(encoded, (unsigned int)(imageSize+2) / 3 * 4, (unsigned char*)data) != imageSize)
			{
				free(encoded);
				free(data);
				return NULL;
			}
			else
			{
				free(encoded);
				return (struct pixel*)data;
			}
		}
		else
		{
			return (struct pixel*)data;
		}
	}
	else
	{
		free(data);
		printf("Error: Corrupted BMP file.\n");

		return NULL;
	}
}

bool write_bmp(FILE* stream, const BMP_IMAGE* image)
{
	if (image == NULL || image->data == NULL || image->header == NULL || stream == NULL)
	{
		return false;
	}

	LONG imageSize = get_image_size(image->header);

	if (stream == stdout)
	{
		char* file = malloc(image->header->size);
		memcpy(file, image->header, sizeof(HEADER));
		memcpy(file + sizeof(HEADER), image->data, (size_t)imageSize);

		char* encoded = malloc(BASE64_ENCODE_OUT_SIZE(image->header->size));
		base64_encode((unsigned char*)file, image->header->size, encoded);
		if (fwrite(encoded, strlen(encoded), 1, stream) == 1)
		{
			free(file);
			free(encoded);
			return true;
		}

		free(encoded);
		free(file);
		return false;
	}
	else
	{
		return (fwrite(image->header, sizeof(HEADER), 1, stream) && \
			fwrite(image->data, (size_t)imageSize, 1, stream));
	}
}

void free_bmp_image(BMP_IMAGE* image)
{
        if (image != NULL)
        {
                if (image->data != NULL)
                {
                        free(image->data);
                }
		if (image->header != NULL)
		{
			free(image->header);
		}

                free(image);
        }
}

LONG get_image_size(const HEADER* H)
{
	if (H->image_size != 0)
	{
		return (LONG)H->image_size;
	}

        return get_row_size((LONG)H->width) * (LONG)H->height;
}

LONG get_row_size(LONG width)
{
        return width * 3 + get_padding(width);
}

int get_padding(LONG width)
{
        return ((4 - (width * 3) % 4) % 4);
}

unsigned int
base64_encode(const unsigned char *in, unsigned int inlen, char *out)
{
	int s;
	unsigned int i;
	unsigned int j;
	unsigned char c;
	unsigned char l;

	s = 0;
	l = 0;

	static const char base64en[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/',
	};

	for (i = j = 0; i < inlen; i++) {
		c = in[i];

		switch (s) {
		case 0:
			s = 1;
			out[j++] = base64en[(c >> 2) & 0x3F];
			break;
		case 1:
			s = 2;
			out[j++] = base64en[((l & 0x3) << 4) | ((c >> 4) & 0xF)];
			break;
		case 2:
			s = 0;
			out[j++] = base64en[((l & 0xF) << 2) | ((c >> 6) & 0x3)];
			out[j++] = base64en[c & 0x3F];
			break;
		}
		l = c;
	}

	switch (s) {
	case 1:
		out[j++] = base64en[(l & 0x3) << 4];
		out[j++] = '=';
		out[j++] = '=';
		break;
	case 2:
		out[j++] = base64en[(l & 0xF) << 2];
		out[j++] = '=';
		break;
	}

	out[j] = 0;

	return j;
}

unsigned int base64_decode(const char *in, unsigned int inlen, unsigned char *out)
{
	unsigned int i;
	unsigned int j;
	unsigned char c;

	if (inlen & 0x3) {
		return 0;
	}

	static const unsigned char base64de[] = {
		/* nul, soh, stx, etx, eot, enq, ack, bel, */
		   255, 255, 255, 255, 255, 255, 255, 255,

		/*  bs,  ht,  nl,  vt,  np,  cr,  so,  si, */
		   255, 255, 255, 255, 255, 255, 255, 255,

		/* dle, dc1, dc2, dc3, dc4, nak, syn, etb, */
		   255, 255, 255, 255, 255, 255, 255, 255,

		/* can,  em, sub, esc,  fs,  gs,  rs,  us, */
		   255, 255, 255, 255, 255, 255, 255, 255,

		/*  sp, '!', '"', '#', '$', '%', '&', ''', */
		   255, 255, 255, 255, 255, 255, 255, 255,

		/* '(', ')', '*', '+', ',', '-', '.', '/', */
		   255, 255, 255,  62, 255, 255, 255,  63,

		/* '0', '1', '2', '3', '4', '5', '6', '7', */
		    52,  53,  54,  55,  56,  57,  58,  59,

		/* '8', '9', ':', ';', '<', '=', '>', '?', */
		    60,  61, 255, 255, 255, 255, 255, 255,

		/* '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', */
		   255,   0,   1,  2,   3,   4,   5,    6,

		/* 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', */
		     7,   8,   9,  10,  11,  12,  13,  14,

		/* 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', */
		    15,  16,  17,  18,  19,  20,  21,  22,

		/* 'X', 'Y', 'Z', '[', '\', ']', '^', '_', */
		    23,  24,  25, 255, 255, 255, 255, 255,

		/* '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', */
		   255,  26,  27,  28,  29,  30,  31,  32,

		/* 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', */
		    33,  34,  35,  36,  37,  38,  39,  40,

		/* 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', */
		    41,  42,  43,  44,  45,  46,  47,  48,

		/* 'x', 'y', 'z', '{', '|', '}', '~', del, */
		    49,  50,  51, 255, 255, 255, 255, 255
	};

	for (i = j = 0; i < inlen; i++) {
		if (in[i] == '=') {
			break;
		}
		if (in[i] < '+' || in[i] > 'z') {
			return 0;
		}

		c = base64de[(unsigned char)in[i]];
		if (c == 255) {
			return 0;
		}

		switch (i & 0x3) {
		case 0:
			out[j] = (unsigned char)((c << 2) & 0xFF);
			break;
		case 1:
			out[j] = (unsigned char)(out[j] | ((c >> 4) & 0x3));
			out[++j] = (unsigned char)((c & 0xF) << 4);
			break;
		case 2:
			out[j] |= (unsigned char)(out[j] | ((c >> 2) & 0xF));
			out[++j] = (unsigned char)((c & 0x3) << 6);
			break;
		case 3:
			out[j++] |= (unsigned char)c;
			break;
		}
	}

	return j;
}
