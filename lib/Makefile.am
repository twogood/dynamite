## useful flags
AM_CFLAGS = -g -ansi -Wall -Wsign-compare -Wno-long-long -Werror @CFLAGS@ @APPLE_CFLAGS@ -I..

## create this library
lib_LTLIBRARIES = libdynamite.la

## use these sources
libdynamite_la_SOURCES = libdynamite.c

libdynamite_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = libdynamite.h

