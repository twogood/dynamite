dnl $Id$ vim: syntax=config
dnl Check for libdynamite

AC_DEFUN([AM_PATH_LIBDYNAMITE], [

  AC_ARG_WITH(libdynamite,
    AC_HELP_STRING(
      [--with-libdynamite[=DIR]],
      [Search for libdynamite in DIR/include and DIR/lib]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/include"
				LDFLAGS="$LDFLAGS -L${withval}/lib"
			]
    )

  AC_ARG_WITH(libdynamite-include,
    AC_HELP_STRING(
      [--with-libdynamite-include[=DIR]],
      [Search for libdynamite header files in DIR]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}"
			]
    )

  AC_ARG_WITH(libdynamite-lib,
    AC_HELP_STRING(
      [--with-libdynamite-lib[=DIR]],
      [Search for libdynamite library files in DIR]),
      [
				LDFLAGS="$LDFLAGS -L${withval}"
			]
    )

	AC_CHECK_LIB(dynamite,dynamite_explode,,[
		AC_MSG_ERROR([Can't find dynamite library])
		])
	AC_CHECK_HEADERS(libdynamite.h,,[
		AC_MSG_ERROR([Can't find dynamite.h])
		])

])
