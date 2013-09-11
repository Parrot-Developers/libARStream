#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([libARStreaming], [0.1.0], [ardrone@ardrone.org])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(m4)

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
    CFLAGS+=" -g -Wall -Werror -Wno-uninitialized -O0"
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
    CFLAGS+=" -O3"
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
# Define libraries directories
##########################################################################

# get the directories of libARSAL
AC_ARG_WITH([libARSALInstallDir],
            AS_HELP_STRING([--with-libARSALInstallDir=DIR],[directory of the libARSAL installation @<:@default=PREFIX@:>@]),
            [if test x$withval = xno; then
              libARSALInstallDir=""
            else
              libARSALInstallDir="$withval"
            fi], 
            [if test x$prefix = xNONE; then
              libARSALInstallDir=""
            else
              libARSALInstallDir="$prefix"
            fi])

AC_ARG_WITH([libARSALIncludeDir],
            AS_HELP_STRING([--with-libARSALIncludeDir=DIR],[directory of the libARSAL headers @<:@default=ARSALInstallDir/include@:>@]),
            [if test x$withval = xno; then
              [if test ! -z $libARSALInstallDir; then
                 libARSALIncludeDir="$libARSALInstallDir/include"
              else
                 libARSALIncludeDir=""
              fi]
            else
              libARSALIncludeDir="$withval"
            fi],
              [if test ! -z $libARSALInstallDir; then
                 libARSALIncludeDir="$libARSALInstallDir/include"
              else
                 libARSALIncludeDir=""
              fi])

AC_ARG_WITH([libARSALLibDir],
            AS_HELP_STRING([--with-libARSALLibDir=DIR],[directory of the libARSAL libraries @<:@default=ARSALInstallDir/lib@:>@]),
            [if test x$withval = xno; then
              [if test ! -z $libARSALInstallDir; then
                libARSALLibDir="$libARSALInstallDir/lib"
              else
                libARSALLibDir=""
              fi]
            else
              libARSALLibDir="$withval"
            fi],
              [if test ! -z $libARSALInstallDir; then
                libARSALLibDir="$libARSALInstallDir/lib"
              else
                libARSALLibDir=""
              fi])

# get the directories of libARNetwork
AC_ARG_WITH([libARNetworkInstallDir],
            AS_HELP_STRING([--with-libARNetworkInstallDir=DIR],[directory of the libARNetwork installation @<:@default=PREFIX@:>@]),
            [if test x$withval = xno; then
              libARNetworkInstallDir=""
            else
              libARNetworkInstallDir="$withval"
            fi], 
            [if test x$prefix = xNONE; then
              libARNetworkInstallDir=""
            else
              libARNetworkInstallDir="$prefix"
            fi])

AC_ARG_WITH([libARNetworkIncludeDir],
            AS_HELP_STRING([--with-libARNetworkIncludeDir=DIR],[directory of the libARNetwork headers @<:@default=ARNetworkInstallDir/include@:>@]),
            [if test x$withval = xno; then
              [if test ! -z $libARNetworkInstallDir; then
                 libARNetworkIncludeDir="$libARNetworkInstallDir/include"
              else
                 libARNetworkIncludeDir=""
              fi]
            else
              libARNetworkIncludeDir="$withval"
            fi],
              [if test ! -z $libARNetworkInstallDir; then
                 libARNetworkIncludeDir="$libARNetworkInstallDir/include"
              else
                 libARNetworkIncludeDir=""
              fi])

AC_ARG_WITH([libARNetworkLibDir],
            AS_HELP_STRING([--with-libARNetworkLibDir=DIR],[directory of the libARNetwork libraries @<:@default=ARNetworkInstallDir/lib@:>@]),
            [if test x$withval = xno; then
              [if test ! -z $libARNetworkInstallDir; then
                libARNetworkLibDir="$libARNetworkInstallDir/lib"
              else
                libARNetworkLibDir=""
              fi]
            else
              libARNetworkLibDir="$withval"
            fi],
              [if test ! -z $libARNetworkInstallDir; then
                libARNetworkLibDir="$libARNetworkInstallDir/lib"
              else
                libARNetworkLibDir=""
              fi])

# get the directories of libARNetworkAL
AC_ARG_WITH([libARNetworkALInstallDir],
            AS_HELP_STRING([--with-libARNetworkALInstallDir=DIR],[directory of the libARNetworkAL installation @<:@default=PREFIX@:>@]),
            [if test x$withval = xno; then
              libARNetworkALInstallDir=""
            else
              libARNetworkALInstallDir="$withval"
            fi], 
            [if test x$prefix = xNONE; then
              libARNetworkALInstallDir=""
            else
              libARNetworkALInstallDir="$prefix"
            fi])

AC_ARG_WITH([libARNetworkALIncludeDir],
            AS_HELP_STRING([--with-libARNetworkALIncludeDir=DIR],[directory of the libARNetworkAL headers @<:@default=ARNetworkALInstallDir/include@:>@]),
            [if test x$withval = xno; then
              [if test ! -z $libARNetworkALInstallDir; then
                 libARNetworkALIncludeDir="$libARNetworkALInstallDir/include"
              else
                 libARNetworkALIncludeDir=""
              fi]
            else
              libARNetworkALIncludeDir="$withval"
            fi],
              [if test ! -z $libARNetworkALInstallDir; then
                 libARNetworkALIncludeDir="$libARNetworkALInstallDir/include"
              else
                 libARNetworkALIncludeDir=""
              fi])

AC_ARG_WITH([libARNetworkALLibDir],
            AS_HELP_STRING([--with-libARNetworkALLibDir=DIR],[directory of the libARNetworkAL libraries @<:@default=ARNetworkALInstallDir/lib@:>@]),
            [if test x$withval = xno; then
              [if test ! -z $libARNetworkALInstallDir; then
                libARNetworkALLibDir="$libARNetworkALInstallDir/lib"
              else
                libARNetworkALLibDir=""
              fi]
            else
              libARNetworkALLibDir="$withval"
            fi],
              [if test ! -z $libARNetworkALInstallDir; then
                libARNetworkALLibDir="$libARNetworkALInstallDir/lib"
              else
                libARNetworkALLibDir=""
              fi])

##########################################################################
# Define includes directories
##########################################################################

# include local headers first
CFLAGS+=" -I$srcdir/../Includes -I$srcdir/../Sources"
# add the headers directory of libARSAL
if test ! -z $libARSALIncludeDir; then
  CFLAGS+=" -I$libARSALIncludeDir"
fi
# add the headers directory of libARNetwork
if test ! -z $libARNetworkIncludeDir; then
  CFLAGS+=" -I$libARNetworkIncludeDir"
fi
# add the headers directory of libARNetworkAL
if test ! -z $libARNetworkALIncludeDir; then
  CFLAGS+=" -I$libARNetworkALIncludeDir"
fi

CPPFLAGS="$CFLAGS"
##########################################################################
# Define binaries directories
##########################################################################

# add the binaries directory of libARSAL
if test ! -z $libARSALLibDir; then
  LDFLAGS+=" -L$libARSALLibDir"
fi
# add the binaries directory of libARNetwork
if test ! -z $libARNetworkLibDir; then
  LDFLAGS+=" -L$libARNetworkLibDir"
fi
# add the binaries directory of libARNetworkAL
if test ! -z $libARNetworkALLibDir; then
  LDFLAGS+=" -L$libARNetworkALLibDir"
fi

##########################################################################
# Check headers
##########################################################################

# Checks for generic headers
AC_CHECK_HEADERS([inttypes.h],,AC_MSG_ERROR(The inttypes.h header is required in order to build the library!))
AC_CHECK_HEADERS([stdlib.h],,AC_MSG_ERROR(The stdlib.h header is required in order to build the library!))
AC_CHECK_HEADERS([unistd.h],,AC_MSG_ERROR(The unistd.h header is required in order to build the library!))

# Checks for libARSAL headers
AC_CHECK_HEADERS([libARSAL/ARSAL_Print.h libARSAL/ARSAL_Mutex.h libARSAL/ARSAL_Endianness.h libARSAL/ARSAL_Sem.h],,
	AC_MSG_ERROR(The libARSAL headers are required in order to build the library!
- Use --with-libARSALInstallDir of --with-libARSALIncludeDir to indicate a specific include path.))
# Checks for libARNetwork headers
AC_CHECK_HEADERS([libARNetwork/ARNETWORK_Manager.h libARNetwork/ARNETWORK_IOBufferParam.h],,
	AC_MSG_ERROR(The libARNetwork headers are required in order to build the library !
- Use --with-libARNetworkInstallDir of --with-libARNetworkIncludeDir to indicate a specific include path.))

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
DX_INIT_DOXYGEN([libARStreaming], [doxygen.cfg], [$srcdir/../Doc])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

# Check for python and run prebuild PY script
AM_PATH_PYTHON(,, AC_MSG_ERROR(Python is required to generate source files for this library))

# Run prebuild PY script
AC_MSG_NOTICE([Executing python prebuild commands])
AM_CONDITIONAL([PYTHON_RUN_OK], [$PYTHON $srcdir/../../ARBuildUtils/Utils/Python/ARSDK_PrebuildActions.py $srcdir])
AM_COND_IF([PYTHON_RUN_OK], , AC_MSG_ERROR(Python processing failed))

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

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

# Checks for library functions.


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
