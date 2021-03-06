dnl Process this file with autoconf to produce a configure script.

AC_INIT(odyssey, 0.6, vasco-project@lists.gforge.enseeiht.fr)
AC_PREREQ(2.64)
AC_REVISION($Id: configure.ac,v 1.9 2010-11-27 13:25:59 gaufille Exp $)
AC_CONFIG_AUX_DIR(scripts)
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_HEADER(src/config.h)

dnl *** Checks for programs
AC_CANONICAL_HOST
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_RANLIB
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CHECK_PROG([man2html], [man2html], [man2html])
if test "x$man2html" != "xman2html"; then
	AC_MSG_NOTICE(html man pages can not be built without man2html)
fi

dnl *** Checks for libraries

dnl *** Checks for header files
AC_STDC_HEADERS
AC_CHECK_HEADERS(stdio.h errno.h signal.h fcntl.h unistd.h getopt.h sys/select.h sys/ioctl.h \
	sys/time.h,,
	AC_MSG_ERROR(missing the required C header file $ac_header)
)
AC_LANG_PUSH(C++)
AC_CHECK_HEADERS(new exception stdexcept iterator string map vector cstring cstdio cstdlib cerrno csignal cctype,,
	AC_MSG_ERROR(missing the required C++ header file $ac_header)
)
AC_LANG_POP(C++)


dnl *** Checks for typedefs, structures, and compiler characteristics
dnl AC_TYPE_SIZE_T


dnl *** Checks for library functions
AC_CHECK_FUNCS(select snprintf,,
	AC_ERROR([missing a required library function]))



dnl *** Options
AC_ARG_WITH(ppdev,
	[  --without-ppdev         Don't build the Linux ppdev driver],
	check_linux_parport=$withval, check_linux_parport=yes)
ENABLE_LINUX_PPDEV=0
if test x$check_linux_parport != xno; then
	AC_CHECK_HEADERS(linux/parport.h linux/ppdev.h,
		ENABLE_LINUX_PPDEV=1
		AC_DEFINE(ENABLE_LINUX_PPDEV,,[Define to build the LinuxPPDev driver]),
		AC_MSG_NOTICE([*** Not building Linux ppdev driver]))
fi
AM_CONDITIONAL(ENABLE_LINUX_PPDEV, test x$ENABLE_LINUX_PPDEV = x1)

AC_ARG_WITH(directio,
	[  --without-directio      Don't build the Direct I/O driver],
	check_sys_io=$withval, check_sys_io=yes)
ENABLE_DIRECTIO=0
if test x$check_sys_io != xno; then
	AC_CHECK_HEADERS(sys/io.h,
		ENABLE_DIRECTIO=1
		AC_DEFINE(ENABLE_DIRECTIO,,[Define to build the Direct I/O driver]),
		AC_MSG_NOTICE([*** Not building Direct I/O driver]))
fi
AM_CONDITIONAL(ENABLE_DIRECTIO, test x$ENABLE_DIRECTIO = x1)


dnl *** Output files
AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
doc/Makefile
])
