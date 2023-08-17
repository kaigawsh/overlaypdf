# Makefile.am
#
# automake Makefile
#
# $Id:$
#

@SET_MAKE@

# Define the binaraies (programs) to build 
bin_PROGRAMS = overlaypdf

# For each binary define the sources that it depends on, the C flags and any librarys
# N.B. POPPLERGLIB_ should be set by the configure script either using pkg-config, 
# by guessing where the librarys may be, or by the user specifying.
overlaypdf_SOURCES = overlaypdf.c
overlaypdf_CFLAGS = $(CAIRO_CFLAGS) $(POPPLERGLIB_CFLAGS) 
overlaypdf_LDADD = $(CAIRO_LIBS) $(POPPLERGLIB_LIBS) 

MAINTAINERCLEANFILES = Makefile.in
