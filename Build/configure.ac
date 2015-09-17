#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([libARStream], [3.1.0], [ardrone@ardrone.org])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(m4)

m4_include([../../ARSDKBuildUtils/Utils/m4/ARSDK.m4])

# automake initialisation and check for minimal automake API version 1.9
AM_INIT_AUTOMAKE

# use the C compiler for the following checks
AC_LANG([C])

##########################################################################
# Debug compilation support
##########################################################################
AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
    [enable debug data generation (def=no)])],
    [debugit="$enableval"],
    [debugit=no])
AC_MSG_RESULT([$debugit])

if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    LOCAL_CFLAGS+=" -g -Wall -Werror -Wno-uninitialized -O0"
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
    LOCAL_CFLAGS+=" -O3"
fi
AM_CONDITIONAL([DEBUG_MODE], [test "$debugit" = "yes"])

##########################################################################
# Non versionned .so compilation support (for Android)
##########################################################################
AC_MSG_CHECKING([whether to put version number in .so files])
AC_ARG_ENABLE([so-version],
    [AS_HELP_STRING([--disable-so-version],
    [disable version names in .so files [default=enabled]])],
    [sover="$enableval"],
    [sover=yes])
AC_MSG_RESULT([$sover])


##########################################################################
# Define includes directories
##########################################################################

# include local headers first
LOCAL_CFLAGS+=" -I$srcdir/../Includes -I$srcdir/../Sources"

CPPFLAGS+="$LOCAL_CFLAGS"
CFLAGS+="$LOCAL_CFLAGS"
OBJCFLAGS+="$LOCAL_CFLAGS"

# Add libARxxx deps

AR_DEPENDS([libARSAL], [Y], [libARSAL/ARSAL.h])
AR_DEPENDS([libARNetworkAL], [Y], [libARNetworkAL/ARNetworkAL.h])
AR_DEPENDS([libARNetwork], [Y], [libARNetwork/ARNetwork.h])

##########################################################################
# Check headers
##########################################################################

# Check for SO_NOSIGPIPE macros
AC_CHECK_DECLS([SO_NOSIGPIPE, MSG_NOSIGNAL],[],[],[[#include <sys/socket.h>]])

# Checks for generic headers
AC_CHECK_HEADERS([inttypes.h],,AC_MSG_ERROR(The inttypes.h header is required in order to build the library!))
AC_CHECK_HEADERS([stdlib.h],,AC_MSG_ERROR(The stdlib.h header is required in order to build the library!))
AC_CHECK_HEADERS([unistd.h],,AC_MSG_ERROR(The unistd.h header is required in order to build the library!))

##########################################################################
# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_PROGS([DOXYGEN], [doxygen])
m4_include(doxygen.ac)
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([libARStream], [doxygen.cfg], [$srcdir/../Doc])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_LIB([m],[sqrt])

# Generate libtool version info number
if test x"$sover" = x"yes"; then
	LDFLAGS+=" -release $PACKAGE_VERSION"
else
    LDFLAGS+=" -avoid-version"
fi

# distribute additional compiler and linker flags
# --> set these variables instead of CFLAGS or LDFLAGS
AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])

# Generates Makefile
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debugit
  C Compiler.....: $CC $CFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
  Doxygen........: ${DOXYGEN:-NONE}
"
