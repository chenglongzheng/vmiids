dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([vmiids], [0.1], [kittel@fs.tum.de])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall])

#Include CPPUnit
AM_PATH_CPPUNIT(1.12.1)

AC_CONFIG_SRCDIR([src/Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

# Checks for programs.
# store current user given compiler flags to avoid default setup via AC_PROG_CXX
OLD_CXXFLAGS=$CXXFLAGS
# check for C++ preprocessor and compiler and the library compiler
# (might change the compiler flags)
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXXCPP
# reset compiler flags to initial flags
CXXFLAGS=$OLD_CXXFLAGS

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN

# use the C++ compiler for the following checks
AC_LANG([C++])


# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([iostream])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

##libconfig
PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.3],,
	AC_MSG_ERROR([libconfig++ 1.3 or newer not found.])
)

##libgcrypt
AM_PATH_LIBGCRYPT(1.4.5,
                  , AC_MSG_ERROR([libgcrypt 1.4 or newer not found.]))


if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

######################################################################
# DOXYGEN SUPPORT
######################################################################

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

DX_INIT_DOXYGEN([$PACKAGE_NAME],[doxygen.cfg], [doc])

######################################################################

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(src/vmiids/Makefile)
AC_CONFIG_FILES(src/vmiids/util/Makefile)
AC_CONFIG_FILES(src/vmiids/modules/Makefile)
AC_CONFIG_FILES(src/vmiids/modules/detection/Makefile)
AC_CONFIG_FILES(src/vmiids/modules/notification/Makefile)
AC_CONFIG_FILES(src/vmiids/modules/sensor/Makefile)
AC_CONFIG_FILES(src/vmiids/rpc/Makefile)
AC_CONFIG_FILES(src/clientApps/Makefile)

##########################################################################
# debug compilation support
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
    AM_CXXFLAGS="$AM_CXXFLAGS -ggdb -O0 -pg"
    AM_LDFLAGS="$AM_LDFLAGS -pg"
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
    AM_CXXFLAGS="$AM_CXXFLAGS -O3"
fi

##########################################################################

##########################################################################
## compile the VmiIDS Gui, iff qt is available
##########################################################################

AC_MSG_CHECKING([checking for qt])
bnv_try_1="pkg-config --cflags QtCore QtGui"
AC_TRY_EVAL(bnv_try_1)
AM_CONDITIONAL([COMPILEGUI], [test x"$ac_status" != x0])
if test x"$ac_status" != x0; then
   AC_MSG_ERROR([qt not found! Won´t compile Gui])
else
AM_CONDITIONAL([COMPILEGUI], [true])
AC_CONFIG_FILES(src/gui/Makefile)

QT_CXXFLAGS=`pkg-config --cflags QtCore QtGui`
QT_LDFLAGS=`pkg-config --libs QtCore QtGui`
AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDFLAGS)
AC_MSG_RESULT([yes])
fi

##########################################################################
# adding the memtool library
##########################################################################

# introduce the optional configure parameter for a non-standard install prefix of XXX
AC_ARG_WITH([memtool],
    [AS_HELP_STRING([--with-memtool=prefix],
        [try this for a non-standard install prefix of the memtool library])],
    [MEMTOOLPATHSET=1],
    [MEMTOOLPATHSET=0])
	
# if optional parameter used, extend path flags for compliler and linker
if test $MEMTOOLPATHSET = 1 ; then
    # extend the compiler and linker flags according to the path set
    MEMTOOL_CXXFLAGS="-I$with_memtool/include"
    MEMTOOL_LDFLAGS="-L$with_memtool/ -lmemtool"
    MEMTOOL_PATH=$with_memtool
else 
    # extend the compiler and linker flags according to the path set
    MEMTOOL_LDFLAGS="-lmemtool"
    MEMTOOL_PATH="/usr/lib"
fi

##########################################################################
##########################################################################
# check for memtool library
##########################################################################

# store current *FLAGS and merge with AM_*FLAGS for compilation and linker check   
OLD_CXXFLAGS=$CXXFLAGS;
OLD_LDFLAGS=$LDFLAGS;
CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS $MEMTOOL_CXXFLAGS $QT_CXXFLAGS"
LDFLAGS="$AM_LDFLAGS $LDFLAGS $MEMTOOL_LDFLAGS $QT_LDFLAGS"

# ensure the library to check for is covered by the LIBS variable
OLD_LIBS=$LIBS
LIBS="$LIBS -lmemtool"

# check for XXX library headers   
AC_MSG_CHECKING([for the memtool library headers])
# try to compile a file that includes a header of the library memtool
AC_COMPILE_IFELSE([[#include <memtool/memtool.h>]],
    [AC_MSG_RESULT([found])
        # try to link the memtool constructor out of library memtool
        AC_MSG_CHECKING([whether the memtool library can be linked])
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([[#include <memtool/memtool.h>]],
                [[Memtool memtool;]])],
            [AC_MSG_RESULT([yes])
		        FOUND_MEMTOOL=1;],
            [AC_MSG_RESULT([no])
                LIBS=$OLD_LIBS; dnl reset to old value since memtool was not found
                FOUND_MEMTOOL=0;])],
    [AC_MSG_RESULT([not found])
        FOUND_MEMTOOL=0;])

# reset original *FLAGS
CXXFLAGS=$OLD_CXXFLAGS
LDFLAGS=$OLD_LDFLAGS
LIBS=$OLD_LIBS

# handle results
if test $FOUND_MEMTOOL != 1; then
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([The memtool library was not found!])
    if test  $MEMTOOLPATHSET = 1 ; then
        AC_MSG_NOTICE([ The install prefix '$with_memtool' for the memtool library was set.])
        AC_MSG_NOTICE([ --> Maybe wrong ???])
    else
        AC_MSG_NOTICE([ No non-standard install prefix was set.])
        AC_MSG_NOTICE([ --> You might want to use '--with-memtool=PREFIX' ?!?])
    fi
    AC_MSG_NOTICE([])
    AC_MSG_ERROR([memtool library is an essential dependency : cannot build and stop here !])
fi

##########################################################################

# distribute additional compiler and linker flags
# --> set these variables instead of CXXFLAGS or LDFLAGS
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST(MEMTOOL_CXXFLAGS)
AC_SUBST(MEMTOOL_LDFLAGS)
AC_SUBST(MEMTOOL_PATH)
AC_SUBST([LIBS])

AC_CONFIG_FILES(
	[vmiids.sh:src/vmiids/vmiids.sh.in],
	[chmod +x vmiids.sh])


AC_OUTPUT
