/* $Id$ */
#ifndef __libdynamite_h__
#define __libdynamite_h__

/*
   She's a dancehall queen for life
   Gonna explode like dynamite
   And she's moving outta sight
   Now she a guh mash up di place like dynamite

   http://www.beenie-man.com/dancehall_queen.htm
 */

#include <stdbool.h>
#include <sys/param.h>

typedef enum 
{
  DYNAMITE_SUCCESS,
  DYNAMITE_NOT_IMPLEMENTED,
  DYNAMITE_BAD_FORMAT,
  DYNAMITE_UNEXPECTED_ERROR,
  DYNAMITE_READ_ERROR,
  DYNAMITE_WRITE_ERROR
} DynamiteResult;

typedef size_t (*dynamite_reader)(void* buffer, size_t size, void* cookie);
typedef size_t (*dynamite_writer)(void* buffer, size_t size, void* cookie);

DynamiteResult dynamite_explode(dynamite_reader reader, dynamite_writer writer, void* cookie);

#endif

