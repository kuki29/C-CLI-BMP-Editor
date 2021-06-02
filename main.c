#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/select.h>

#include "bmp.h"
#include "transformations.h"

extern char* optarg;
extern int optind, opterr, optopt;

int getopt(int argc, char* const argv[], const char* optstring);

void help()
{
	printf("Usage: bmp [OPTION]... [FILE]...\n");
	printf("Simple BMP transformation tool.\n\n");
	printf("With no FILE, read from standard input or write to standard output.\n\n");
	printf("\t-r\t\trotate image right\n");
	printf("\t-l\t\trotate image left\n");
	printf("\t-h\t\tflip image horizontally\n");
	printf("\t-v\t\tflip image vertically\n");
	printf("\t-c y,x,h,w\tcrop image from position [y,x] of giwen height and width\n");
	printf("\t-s factor\tscale image by factor\n");
	printf("\t-e string\textract colors\n");
	printf("\t-o file\twrite output to file\n");
	printf("\t-i file\tread input from the file\n");
}

int main(int argc, char* argv[])
{
	static const char* optString = "rlhvc:s:e:o:i:";

	bool rotateRight = false;
	bool rotateLeft = false;
	bool flipHorizontally = false;
	bool flipVertically = false;
	bool cropImage = false;
	char* cropParams = NULL;
	float scaleFactor = 0;
	char* extractColors = NULL;
	char* inputFilename = NULL;
	char* outputFilename = NULL;

	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, optString)) != -1)
	{
		switch(c)
		{
		case 'r':
			rotateRight = true;
			break;

		case 'l':
			rotateLeft = true;
			break;

		case 'h':
			flipHorizontally = true;
			break;

		case 'v':
			flipVertically = true;
			break;

		case 'c':
			cropParams = optarg;
			cropImage = true;
			break;

		case 's':
			scaleFactor = (float)atof(optarg);
			break;

		case 'e':
			extractColors = optarg;
			break;

		case 'o':
			outputFilename = optarg;
			break;

		case 'i':
			inputFilename = optarg;
			break;

		case '?':
		default:
			help();
			return 1;
		}
	}

	struct bmp_image* image = NULL;

	if (inputFilename != NULL)
	{
		FILE* file = fopen(inputFilename, "rb");

		if (file != NULL)
		{
			image = read_bmp(file);
		}

		fclose(file);
	}
	else
	{
		fd_set rfds;
                struct timeval tv;
                int retval;

                FD_ZERO(&rfds);
                FD_SET(0, &rfds);

                tv.tv_sec = 1;
                tv.tv_usec = 0;

                retval = select(1, &rfds, NULL, NULL, &tv);

                if (retval == -1 || !retval)
                {
                        return 0;
                }

		image = read_bmp(stdin);
	}


	if (flipHorizontally)
	{
		struct bmp_image* tmp = flip_horizontally(image);
		free_bmp_image(image);
		image = tmp;
	}
	if (flipVertically)
	{
		struct bmp_image* tmp = flip_vertically(image);
		free_bmp_image(image);
		image = tmp;
	}
	if (rotateRight)
	{
		struct bmp_image* tmp = rotate_right(image);
		free_bmp_image(image);
		image = tmp;
	}
	if (rotateLeft)
	{
		struct bmp_image* tmp = rotate_left(image);
		free_bmp_image(image);
		image = tmp;
	}
	if (cropImage)
	{
		uint32_t params[4];
		int paramsIndex = 0;

		char* tmpBuf = malloc(strlen(cropParams));
		int tmpBufIndex = 0;
		memset(tmpBuf, 0, strlen(cropParams));

		for (int i = 0; i < strlen(cropParams); i++)
		{
			if (isdigit(cropParams[i]))
			{
				tmpBuf[tmpBufIndex++] = cropParams[i];
			}
			else if(cropParams[i] == ',')
			{
				if (paramsIndex > 4)
				{
					free(tmpBuf);
					help();
					return 1;
				}

				params[paramsIndex++] = (uint32_t)atoi(tmpBuf);

				memset(tmpBuf, 0, strlen(cropParams) + 1);
				tmpBufIndex = 0;
			}
			else
			{
				free(tmpBuf);
				help();
				return 0;
			}
		}

		if (paramsIndex == 3 && tmpBufIndex != 0)
		{
			params[paramsIndex++] = (uint32_t)atoi(tmpBuf);

			memset(tmpBuf, 0, strlen(cropParams) + 1);
			tmpBufIndex = 0;
		}
		else
		{
			free(tmpBuf);
			help();
			return 0;
		}

		free(tmpBuf);

		struct bmp_image* tmp = crop(image, params[0], params[1], params[2], params[3]);
		free_bmp_image(image);
		image = tmp;
	}
	if (scaleFactor != 0)
	{
		struct bmp_image* tmp = scale(image, scaleFactor);
		free_bmp_image(image);
		image = tmp;
	}
	if (extractColors != NULL)
	{
		struct bmp_image* tmp = extract(image, extractColors);
		free_bmp_image(image);
		image = tmp;
	}


	if (outputFilename != NULL)
	{
		FILE* file = fopen(outputFilename, "w+b");
		if (file != NULL && !write_bmp(file, image))
		{
			printf("Error: Unable to save %s", outputFilename);
		}

		fclose(file);
	}
	else
	{
		if (!write_bmp(stdout, image))
		{
			printf("Error: Unable to save");
		}
	}

	free_bmp_image(image);

	return 0;
}
