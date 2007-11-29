## Makefile.am -- Process this file with automake to produce Makefile.in
@SET_MAKE@

## don't complain about missing AUTHORS, NEWS, ChangeLog, etc
AUTOMAKE_OPTIONS = foreign 1.4

pcfiles = libdynamite.pc

all_local: $(pcfiles)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)

EXTRA_DIST = libdynamite.pc.in

CLEANFILES = $(pcfiles)

SUBDIRS = lib src man
