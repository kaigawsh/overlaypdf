#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# autoconf 
#

AC_PREREQ(2.60)
AC_INIT(overlaypdf, 0.2.1, xxx)
AC_CONFIG_SRCDIR([overlaypdf.c])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Use pkg-config to check for the existence of the cairo libraries
PKG_CHECK_MODULES(CAIRO,cairo >= 1.2.6)
# Use pkg-config to check for the existence of the poppler-glib libraries
PKG_CHECK_MODULES(POPPLERGLIB,poppler-glib >= 0.5.4)

# Generate the Makefile
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
