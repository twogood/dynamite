/* $Id$ */
#include "libdynamite.h"
#include <stdio.h>

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

  cookie.input_file   = fopen(argv[1], "r");
  cookie.output_file  = fopen(argv[2], "w");

  result = dynamite_explode(reader, writer, &cookie);

  FCLOSE(cookie.input_file);
  FCLOSE(cookie.output_file);
  return result;
}
