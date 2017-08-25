#
# Stu uses Autoconf for configuration.  Autoconf is run by the
# developers to generate 'configure', 'Makefile' and other files.  These
# can then be used to compile Stu, even without having Autoconf
# installed. 
#
# The main purpose of using Autoconf (as opposed to having a simple
# POSIX-compliant Makefile), is to recognize capabilities of the system
# to enable and disable certain features.
#
# In principle, Stu should use only POSIX features, and Autoconf would
# be unnecessary, as a single POSIX-compliant codebase would compile on
# all platforms.  In practice, this fails in two ways:
#
#  - Some systems don't follow POSIX, but working around that is easy,
#    so missing out on these systems is not worth it. 
#  - Some nonstandard features are too useful to not use, but not having
#    them still allows a very useful version of Stu.
#
# However, we try to keep these configuration options to a strict
# minimum.  At the moment there is just one:  using
# CLOCK_REALTIME_COARSE on Linux, which enables nanosecond-precision
# timestamps in line with the filesystem. 
#

#
# General declarations
#

AC_PREREQ([2.69])
AC_INIT(stu, m4_esyscmd([tr -d ' \n' <VERSION]), kunegis@gmail.com, stu, https://github.com/kunegis/stu)

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.9 foreign])

AM_MAINTAINER_MODE([disable])

AC_PROG_CXX
AC_LANG(C++)

#
# Checks for specific features.  None of these is enabled at the moment,
# as all work on the platforms we used Stu on.  Also, they are all
# POSIX-compliant.  If a platform does not have these, then compile
# itself will fail, as it should. 
#

# Other checks
AC_SYS_LARGEFILE

#
# Custom checks.  These check for some nonstandard features that we
# might use. 
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
