#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([displaytest], [0.01], [unknown@unknown])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/display/display.hh])
AC_CONFIG_HEADERS([config.h])

# Add picky CXXFLAGS
CXX14_FLAGS="-std=c++14 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Weffc++ -Werror"
AC_SUBST([CXX14_FLAGS])
AC_SUBST([PICKY_CXXFLAGS])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
PKG_CHECK_MODULES([XCB], [xcb])
PKG_CHECK_MODULES([XCBPRESENT], [xcb-present])
PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 src/display/Makefile
		 src/examples/Makefile])
AC_OUTPUT
