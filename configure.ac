#
# General declarations
#

AC_PREREQ([2.69])
AC_INIT(stu, m4_esyscmd([tr -d '\n' <VERSION]), kunegis@gmail.com)

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.9 foreign])

AM_MAINTAINER_MODE([disable])

# C++
AC_PROG_CXX
AC_LANG(C++)

# Headers
AC_CHECK_HEADERS([fcntl.h limits.h string.h sys/time.h unistd.h])

# Types
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# functions
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime dup2 gettimeofday memset munmap strchr strstr strtol])

#
# Custom checks
#

AC_MSG_CHECKING([for CLOCK_REALTIME_COARSE])
AC_COMPILE_IFELSE( [AC_LANG_PROGRAM([[#include <time.h>]],
                                    [[struct timespec t; int r= clock_gettime(CLOCK_REALTIME_COARSE, &t);]])],
                   [
                      AC_MSG_RESULT([yes])
                      AC_DEFINE_UNQUOTED([HAVE_CLOCK_REALTIME_COARSE], 1, [Define to 1 if you have CLOCK_REALTIME_COARSE.])
                    ],
                    [
                      AC_MSG_RESULT([no])
                      AC_DEFINE_UNQUOTED([HAVE_CLOCK_REALTIME_COARSE], 0, [Define to 1 if you have CLOCK_REALTIME_COARSE.])
                    ]
                 )

#
# Output
#
AC_OUTPUT(Makefile)
