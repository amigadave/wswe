AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = src/wswe
src_wswe_SOURCES = src/wswe.c
src_wswe_LDADD = $(GTK_LIBS)

dist_pkgdata_DATA = data/main_window_uimanager.ui

AM_CPPFLAGS = $(GTK_CFLAGS) $(MY_CFLAGS)
DISTCHECK_CONFIGURE_FLAGS = --enable-debug
