#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

#define BAD_NUMBER_ARGS 1
#define FSEEK_ERROR 2
#define FREAD_ERROR 3
#define MALLOC_ERROR 4
#define FWRITE_ERROR 5
#define THRESHOLD_VALUE 128
#define PIXEL_WIDTH 3
#define WHITE 0xff
#define BLACK 0x00

/**
 * Parses the command line.
 *
 * argc:      the number of items on the command line (and length of the
 *            argv array) including the executable
 * argv:      the array of arguments as strings (char* array)
 * grayscale: the integer value is set to TRUE if grayscale output indicated
 *            outherwise FALSE for threshold output
 *
 * returns the input file pointer (FILE*)
 **/
FILE *parseCommandLine(int argc, char **argv, int *isGrayscale) {
  if (argc > 2) {
    printf("Usage: %s [-g]\n", argv[0]);
    exit(BAD_NUMBER_ARGS);
  }
  
  if (argc == 2 && strcmp(argv[1], "-g") == 0) {
    *isGrayscale = TRUE;
  } else {
    *isGrayscale = FALSE;
  }

  return stdin;
}

unsigned getFileSizeInBytes(FILE* stream) {
  unsigned fileSizeInBytes = 0;
  
  rewind(stream);
  if (fseek(stream, 0L, SEEK_END) != 0) {
    exit(FSEEK_ERROR);
  }
  fileSizeInBytes = ftell(stream);

  return fileSizeInBytes;
}

void getBmpFileAsBytes(unsigned char* ptr, unsigned fileSizeInBytes, FILE* stream) {
  rewind(stream);
  if (fread(ptr, fileSizeInBytes, 1, stream) != 1) {
#ifdef DEBUG
    printf("feof() = %x\n", feof(stream));
    printf("ferror() = %x\n", ferror(stream));
#endif
    exit(FREAD_ERROR);
  }
}

unsigned char getAverageIntensity(unsigned char blue, unsigned char green, unsigned char red) {	
  #ifdef DEBUG
    printf("Average Intensity for %d, %d, %d is: %d\n", blue, green, red, (blue + green + red) / 3);
  #endif
  return (blue + green + red) / 3;
}

int getRowWidthInBytes(int width) {
  int count = 0;
  
  while((width * 3 + count) % 4 != 0) { //Find padding for multiple of 4
    count++;
  }

  return (width * 3) + count;
}

void setPixelToColor(unsigned char* blue, unsigned char* green, unsigned char* red, unsigned char color) {
  *blue = color;
  *green = color;
  *red = color;
}

void applyGrayscaleToPixel(unsigned char* pixel) {
  int averagePixelValue = getAverageIntensity(*(pixel), *(pixel + 1), *(pixel + 2));
  
  setPixelToColor(pixel, pixel + 1, pixel + 2, averagePixelValue);
}

void applyThresholdToPixel(unsigned char* pixel) {
  if(getAverageIntensity(*(pixel), *(pixel + 1), *(pixel + 2)) >= THRESHOLD_VALUE){
    setPixelToColor(pixel, pixel + 1, pixel + 2, WHITE);
  }
  else {
    setPixelToColor(pixel, pixel + 1, pixel + 2, BLACK);
  }
}

void applyFilterToPixel(unsigned char* pixel, int isGrayscale) {
  if(isGrayscale) {
    applyGrayscaleToPixel(pixel);
  }
  else {
    applyThresholdToPixel(pixel);
  }
}

void applyFilterToRow(unsigned char* row, int width, int isGrayscale) {
  for(int i = 0; i < width; i++) { //Iterate through pixels in a row
    applyFilterToPixel(row + (i * PIXEL_WIDTH), isGrayscale);
  }
}

void applyFilterToPixelArray(unsigned char* pixelArray, int width, int height, int isGrayscale) {
  int rowWidth = getRowWidthInBytes(width);

  for(int i = 0; i < height; i++) { //Iterate through rows in an image
    applyFilterToRow(pixelArray + (i * rowWidth), width, isGrayscale);
  }
}

void parseHeaderAndApplyFilter(unsigned char* bmpFileAsBytes, int isGrayscale) {
  int offsetFirstBytePixelArray = 0;
  int width = 0;
  int height = 0;
  unsigned char* pixelArray = NULL;

  offsetFirstBytePixelArray = *((int *)(bmpFileAsBytes + 10));
  pixelArray = bmpFileAsBytes + offsetFirstBytePixelArray;
  width = *((int *)(bmpFileAsBytes + 18));
  height = *((int *)(bmpFileAsBytes + 22));

#ifdef DEBUG
  printf("offsetFirstBytePixelArray = %u\n", offsetFirstBytePixelArray);
  printf("width = %u\n", width);
  printf("height = %u\n", height);
  printf("pixelArray = %p\n", pixelArray);
#endif

  applyFilterToPixelArray(pixelArray, width, height, isGrayscale);
}

int main(int argc, char **argv) {
  int grayscale = FALSE;
  unsigned fileSizeInBytes = 0;
  unsigned char* bmpFileAsBytes = NULL;
  FILE *stream = NULL;
  
  stream = parseCommandLine(argc, argv, &grayscale);
  fileSizeInBytes = getFileSizeInBytes(stream);

#ifdef DEBUG
  printf("fileSizeInBytes = %u\n", fileSizeInBytes);
#endif

  bmpFileAsBytes = (unsigned char *)malloc(fileSizeInBytes);
  if (bmpFileAsBytes == NULL) {
    exit(MALLOC_ERROR);
  }
  getBmpFileAsBytes(bmpFileAsBytes, fileSizeInBytes, stream);

  parseHeaderAndApplyFilter(bmpFileAsBytes, grayscale);

#ifndef DEBUG
  if (fwrite(bmpFileAsBytes, fileSizeInBytes, 1, stdout) != 1) {
    exit(FWRITE_ERROR);
  }
#endif

  free(bmpFileAsBytes);
  return 0;
}
