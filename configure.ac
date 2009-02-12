AC_INIT([Where Shall We Eat], [0.1], [davidk@openismus.com], [wswe])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_SRCDIR([src/wswe.c])
AC_PREREQ(2.57)
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall dist-bzip2])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_MSG_CHECKING(whether to build with debugging information)
debugger=no
AC_ARG_ENABLE(debug,AC_HELP_STRING([--enable-debug],[enable debugging symbols and messages (default=no)]),debugger="$enableval")
AC_MSG_RESULT($debugger)
if test x"$debugger" = x"yes"; then
	AC_DEFINE(DEBUG, 1, [Define to enable debugging symbols and messages])
	DEBUG_CFLAGS="-g -Werror"
	AC_SUBST([CFLAGS], $DEBUG_CFLAGS)
fi

MY_CFLAGS="-Wall -DGTK_DISABLE_DEPRECATED=1 -DGDK_DISABLE_DEPRECATED=1 -DGDK_PIXBUF_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1"
AC_SUBST(MY_CFLAGS)

GTK_MODULES="gtk+-2.0 >= 2.12 glib-2.0 >= 2.6"
PKG_CHECK_MODULES(GTK, $GTK_MODULES)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
