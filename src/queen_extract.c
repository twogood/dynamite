/* $Id$ */

/*
   Example code for libdynamite.

   This code extracts the files from the installer to the adventure game "Flight
   of the Amazon Queen" from 1995.

   It takes the installation data files QUEEN._[1-6] and extracts these files:

   QUEEN.1
   DOS4GW.EXE
   SETUP.EXE
   SETUP.SB
   SETUP.MUS
   SETUP.RL
   QUEEN.EXE
   AQ.BAT
 */

#define _BSD_SOURCE 1
#include "libdynamite.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_SIZE   30

#define FCLOSE(file)    if (file) { fclose(file); file = NULL; }

typedef struct _Cookie
{
  int index;
  FILE* input_file;
  FILE* output_file;
} Cookie;

static void next_file(Cookie* cookie)
{
  char filename[16];
  FCLOSE(cookie->input_file);
  cookie->index++;
  snprintf(filename, sizeof(filename), "QUEEN._%i", cookie->index);
  cookie->input_file = fopen(filename, "r");

  if (!cookie->input_file && cookie->index <= 6)
  {
    fprintf(stderr, "Failed to open file '%s'\n", filename);
  }
}

static size_t reader(void* buffer, size_t size, void* cookie)
{
  size_t result = fread(buffer, 1, size, ((Cookie*)cookie)->input_file);

  if (result < size)
  {
    next_file((Cookie*)cookie);
    if (((Cookie*)cookie)->input_file)
    {
      buffer += result;
      result += fread(buffer, 1, size - result, ((Cookie*)cookie)->input_file);
    }
  }

  return result;
}

static size_t writer(void* buffer, size_t size, void* cookie)
{
  return fwrite(buffer, 1, size, ((Cookie*)cookie)->output_file);
}

int main(int argc, char** argv)
{
  int result = -1;
  Cookie cookie;

  memset(&cookie, 0, sizeof(Cookie));

  next_file(&cookie);

  for (;;)
  {
    uint8_t name_size;
    char* name = NULL;

#if 0
    printf("Volume %i, offset 0x%08lx\n", cookie.index, ftell(cookie.input_file));
#endif
    
    if (sizeof(name_size) != reader(&name_size, sizeof(name_size), &cookie))
      break;

    name = malloc(MAX_FILENAME_SIZE);

    if (MAX_FILENAME_SIZE != reader(name, MAX_FILENAME_SIZE, &cookie))
    {
      fprintf(stderr, "read name failed\n");
      break;
    }

    name[name_size] = '\0';
    printf("extracting: %s\n", name);

    cookie.output_file = fopen(name, "w");

    result = dynamite_explode(reader, writer, &cookie);

    FCLOSE(cookie.output_file);

    if (DYNAMITE_SUCCESS != result)
    {
      fprintf(stderr, "Error %i\n", result);
      break;
    }
  }

  FCLOSE(cookie.input_file);
  FCLOSE(cookie.output_file);
  return result;
}
