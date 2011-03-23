/* $Id$ */
#include "libdynamite.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
   This code probably looks really bad to anyone that is experienced with
   (de)compression algorithms. But it works.
 */

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#endif

#define BUFFER_SIZE             0x8000
#define MAX_DICTIONARY_SIZE     0x1000

#define BIT_0   1
#define BIT_8   0x100

#define FREE(ptr)       { if (ptr) { free(ptr); ptr = NULL; } }

typedef struct _Dynamite
{
  DynamiteResult    status;
  
  /* external connections */
  dynamite_reader   reader;
  dynamite_writer   writer;
  void*             cookie;
  
  /* bit reader */
  uint8_t*          buffer;
  uint8_t           byte;
  int               bit;

  
  /* dictionary */
  uint8_t*          dictionary;
  int               dictionary_bits;
  int               dictionary_size;
  int               window_offset;
  
  /* debug */
  size_t            bytes_written;
} Dynamite;

static size_t dynamite_read(Dynamite* dynamite, void* buffer, size_t size)/*{{{*/
{
  return dynamite->reader(buffer, size, dynamite->cookie);
}/*}}}*/

static size_t dynamite_write(Dynamite* dynamite, void* buffer, size_t size)/*{{{*/
{
  return dynamite->writer(buffer, size, dynamite->cookie);
}/*}}}*/

static bool dynamite_read_header(Dynamite* dynamite)/*{{{*/
{
  bool success = false;
  uint8_t header[2];

  if (sizeof(header) != dynamite_read(dynamite, header, sizeof(header)))
    goto exit;

  dynamite->dictionary_bits = header[1];

  switch (header[0])
  {
    case 0:
      /* ok! */
      break;

    case 1:
      /* encoded literal representation, not (yet) supported, fall through */
      dynamite->status = DYNAMITE_NOT_IMPLEMENTED;
#if DEBUG
    fprintf(stderr, "Encoded literal representation not yet supported.\n");
#endif
      goto exit;

    default:
      /* not compressed with this algorithm! */
      dynamite->status = DYNAMITE_BAD_FORMAT;
#if DEBUG
    fprintf(stderr, "Unknown format: %i\n", header[0]);
#endif
      goto exit;
  }

  switch (dynamite->dictionary_bits)
  {
    case 4:
      dynamite->dictionary_size = 0x400;
      break;
    case 5:
      dynamite->dictionary_size = 0x800;
      break;
    case 6:
      dynamite->dictionary_size = 0x1000;
      break;
    default:
      /* not compressed with this algorithm! */
      dynamite->status = DYNAMITE_BAD_FORMAT;
#if DEBUG
    fprintf(stderr, "Unknown dictionary bits: %i\n", dynamite->dictionary_bits);
#endif
      goto exit;
  }

  success = true;

exit:
  return success;
}/*}}}*/

static unsigned dynamite_read_bit(Dynamite* dynamite)/*{{{*/
{
  /* XXX: this could probably be optimized */
  unsigned result;

  if (8 == dynamite->bit)
  {
    dynamite_read(dynamite, &dynamite->byte, 1);
    dynamite->bit = 0;
  }
  
  result = dynamite->byte & 1;
  dynamite->byte >>= 1;
  dynamite->bit++;

  return result;
}/*}}}*/

static unsigned dynamite_read_bits_little_endian(Dynamite* dynamite, int count)/*{{{*/
{
  /* XXX: this could probably be optimized */
  int i;
  unsigned result = 0;
  unsigned bit = 1;

  for (i = 0; i < count; i++)
  {
    if (dynamite_read_bit(dynamite))
      result |= bit;
    bit <<= 1;
  }

  return result;
}/*}}}*/

static unsigned dynamite_read_bits_big_endian(Dynamite* dynamite, int count)/*{{{*/
{
  /* XXX: this could probably be optimized */
  int i;
  unsigned result = 0;

  for (i = 0; i < count; i++)
  {
    result <<= 1;
    result |= dynamite_read_bit(dynamite);
  }

  return result;
}/*}}}*/

static int dynamite_get_copy_length(Dynamite* dynamite)/*{{{*/
{
  unsigned bits;
  bits = dynamite_read_bits_big_endian(dynamite, 2);

  switch (bits)
  {
    case 0:       /* 00 */
      bits = dynamite_read_bits_big_endian(dynamite, 2);

      switch (bits)/*{{{*/
      {
        case 0:   /* 0000 */
          bits = dynamite_read_bits_big_endian(dynamite, 2);
          switch (bits)
          {
            case 0:   /* 000000 */
              if (dynamite_read_bit(dynamite)) /* 0000001 */
              {
                return 136 + dynamite_read_bits_little_endian(dynamite, 7); 
              }
              else                      /* 0000000 */
              {
                return 264 + dynamite_read_bits_little_endian(dynamite, 8); 
              }
            case 1:   /* 000001 */
              return 72 + dynamite_read_bits_little_endian(dynamite, 6); 
            case 2:   /* 000010 */
              return 40 + dynamite_read_bits_little_endian(dynamite, 5); 
            case 3:   /* 000011 */
              return 24 + dynamite_read_bits_little_endian(dynamite, 4);
            default:
              abort();
          }

        case 1:   /* 0001 */
          if (dynamite_read_bit(dynamite))
            return 12 + dynamite_read_bits_little_endian(dynamite, 2);
          else
            return 16 + dynamite_read_bits_little_endian(dynamite, 3);

        case 2:   /* 0010 */
          if (dynamite_read_bit(dynamite))   /* 00101 */
            return 9;
          else                        /* 00100 */
            return 10 + dynamite_read_bit(dynamite);

        case 3:   /* 0011 */
          return 8;
        default:
          abort();
      }/*}}}*/

    case 1:       /* 01 */
      if (dynamite_read_bit(dynamite))
        return 5; /* 011 */
      else        /* 010 */
      {
        if (dynamite_read_bit(dynamite))
          return 6;   /* 0101 */
        else
          return 7;   /* 0100 */
      }
      
    case 2:       /* 10 */
      if (dynamite_read_bit(dynamite))
        return 2;   /* 101 */
      else
        return 4;   /* 100 */
      
    case 3:
      return 3;

    default:
      abort();
  }
}/*}}}*/

static unsigned dynamite_get_upper_offset_bits(Dynamite* dynamite)/*{{{*/
{
  unsigned bits;
  
  bits = dynamite_read_bits_big_endian(dynamite, 2);

  switch (bits)
  {
    case 0:   /* 00 */
      if (dynamite_read_bit(dynamite))     /* 001 */
      {
        bits = dynamite_read_bits_big_endian(dynamite, 4);
        return 0x27 - bits;
      }
      else                          /* 000 */
      {
        if (dynamite_read_bit(dynamite))   /* 0001 */
        {
          bits = dynamite_read_bits_big_endian(dynamite, 3);
          return 0x2f - bits;
        }
        else                        /* 0000 */
        {
          bits = dynamite_read_bits_big_endian(dynamite, 4);
          return 0x3f - bits;
        }
      }

    case 1:   /* 01 */
      bits = dynamite_read_bits_big_endian(dynamite, 4);
      if (bits)
        return 0x16 - bits;
      else
        return 0x17 - dynamite_read_bit(dynamite);

    case 2:   /* 10 */
      bits = dynamite_read_bit(dynamite) << 1;
      bits |= dynamite_read_bit(dynamite);
      switch (bits)
      {
        case 0: /* 1000 */
          if (dynamite_read_bit(dynamite))
            return 5;
          else
            return 6;

        case 1: /* 1001 */
          if (dynamite_read_bit(dynamite))
            return 3;
          else
            return 4;

        case 2: /* 1010 */
          return 2;
        case 3: /* 1011 */
          return 1;

        default:
          abort();
      }

    case 3: /* 11 */
      return 0;

    default:
      abort();
  }
}/*}}}*/

static int dynamite_get_offset(Dynamite* dynamite, int length)/*{{{*/
{
  unsigned lower_bit_count;
  int result = 0;

  if (2 == length)
    lower_bit_count = 2;
  else
    lower_bit_count = dynamite->dictionary_bits;

  result = dynamite_get_upper_offset_bits(dynamite) << lower_bit_count;
  result |= dynamite_read_bits_little_endian(dynamite, lower_bit_count);
 
  return result; 
}/*}}}*/

static void dynamite_add_to_dictionary(Dynamite* dynamite, uint8_t byte)/*{{{*/
{
  dynamite->window_offset = (dynamite->window_offset + 1) % dynamite->dictionary_size;
  dynamite->dictionary[dynamite->window_offset] = byte;
}/*}}}*/

static uint8_t dynamite_get_from_dictionary(Dynamite* dynamite, unsigned offset)/*{{{*/
{
  size_t window_offset = dynamite->window_offset;
  if (offset > window_offset)
  {
    window_offset += dynamite->dictionary_size;
  }

  return dynamite->dictionary[window_offset - offset];
}/*}}}*/

static bool dynamite_write_byte(Dynamite* dynamite, uint8_t byte)/*{{{*/
{
#if DEBUG
  fprintf(stderr, "%02x ", byte);
#endif
  dynamite_add_to_dictionary(dynamite, byte);
  if (1 == dynamite_write(dynamite, &byte, 1))
  {
    dynamite->bytes_written++;
    return true;
  }
  else
  {
    dynamite->status = DYNAMITE_WRITE_ERROR;
    return false;
  }
}/*}}}*/

DynamiteResult dynamite_explode(dynamite_reader reader, dynamite_writer writer, void* cookie)
{
  Dynamite dynamite;

  memset(&dynamite, 0, sizeof(Dynamite));

  dynamite.status = DYNAMITE_UNEXPECTED_ERROR;
  dynamite.reader = reader;
  dynamite.writer = writer;
  dynamite.cookie = cookie;
  dynamite.buffer = malloc(BUFFER_SIZE);

  if (!dynamite_read_header(&dynamite))
    goto exit;

  dynamite.bit        = 8; /* causes update of byte */
  dynamite.status     = DYNAMITE_SUCCESS;
  dynamite.dictionary = malloc(dynamite.dictionary_size);
  memset(dynamite.dictionary, 0, dynamite.dictionary_size);

  while (DYNAMITE_SUCCESS == dynamite.status)
  {
#if DEBUG
    fprintf(stderr, "%08x: ", dynamite.bytes_written);
#endif

    if (dynamite_read_bit(&dynamite))
    {
      /* Control token */
      int length = dynamite_get_copy_length(&dynamite);

      assert(length <= 519);

      if (519 == length)
      {
        /* finished */
        break;
      }
      else
      {
        int offset = dynamite_get_offset(&dynamite, length);

#if DEBUG
        fprintf(stderr, "Copying %i bytes from offset %i\n", length, offset);
#endif

        while (length)
        {
          dynamite_write_byte(&dynamite, dynamite_get_from_dictionary(&dynamite, offset));
          length--;
        }
      }
    }
    else
    {
      /* 
         Literal token
       */

      /* TODO: handle encoded representation too */
      uint8_t byte = dynamite_read_bits_little_endian(&dynamite, 8) /*& 0xff*/;
      dynamite_write_byte(&dynamite, byte);
    }

#if DEBUG
    if (dynamite.bytes_written > 0x20)
      abort();
#endif

#if DEBUG
    fprintf(stderr, "\n");
#endif
  }

exit:
  FREE(dynamite.buffer);
  FREE(dynamite.dictionary);
  return dynamite.status;
}

