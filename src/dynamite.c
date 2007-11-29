/* $Id$ */
#include "libdynamite.h"
#include <stdio.h>
#include <stdlib.h>

#define FCLOSE(file)    if (file) { fclose(file); file = NULL; }

typedef struct _Cookie
{
  FILE* input_file;
  FILE* output_file;
} Cookie;

static size_t reader(void* buffer, size_t size, void* cookie)
{
  return fread(buffer, 1, size, ((Cookie*)cookie)->input_file);
}

static size_t writer(void* buffer, size_t size, void* cookie)
{
  return fwrite(buffer, 1, size, ((Cookie*)cookie)->output_file);
}

int main(int argc, char** argv)
{
  int result = -1;
  Cookie cookie;

  if (argc < 3)
  {
    fprintf(stderr, "usage: dynamite <input_file> <output_file> [input_offset]\n");
    return 1;
  }

  if (!(cookie.input_file   = fopen(argv[1], "r"))) {
    fprintf(stderr, "dynamite: error opening file %s for input: ", argv[1]);
    perror(NULL);
    return 1;
  }

  if (!(cookie.output_file  = fopen(argv[2], "w"))) {
    fprintf(stderr, "dynamite: error opening file %s for output: ", argv[2]);
    perror(NULL);
    return 1;
  }

  if (argc >= 4)
  {
    fseek(cookie.input_file, strtol(argv[3], NULL, 0), SEEK_SET);
  }

  result = dynamite_explode(reader, writer, &cookie);

  FCLOSE(cookie.input_file);
  FCLOSE(cookie.output_file);
  return result;
}
