#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(stu, 1.15.23, kunegis@gmail.com)

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX

AC_LANG(C++)

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime dup2 gettimeofday memset munmap strchr strstr strtol])

AC_OUTPUT(Makefile)

