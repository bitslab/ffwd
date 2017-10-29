dnl  Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystems, Inc.
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Which version of the canonical setup we're using
AC_DEFUN([PANDORA_CANONICAL_VERSION],[0.175])

AC_DEFUN([PANDORA_BLOCK_BAD_OPTIONS],[
  AS_IF([test "x${prefix}" = "x"],[
    AC_MSG_ERROR([--prefix requires an argument])
  ])
])

dnl The standard setup for how we build Pandora projects
AC_DEFUN([PANDORA_CANONICAL_TARGET],[
  ifdef([m4_define],,[define([m4_define],   defn([define]))])
  ifdef([m4_undefine],,[define([m4_undefine],   defn([undefine]))])
  m4_define([PCT_ALL_ARGS],[$*])
  m4_define([PCT_REQUIRE_CXX],[no])
  m4_define([PCT_FORCE_GCC42],[no])
  m4_define([PCT_DONT_SUPPRESS_INCLUDE],[no])
  m4_define([PCT_NO_VC_CHANGELOG],[no])
  m4_define([PCT_VERSION_FROM_VC],[no])
  m4_define([PCT_USE_VISIBILITY],[yes])
  m4_foreach([pct_arg],[$*],[
    m4_case(pct_arg,
      [require-cxx], [
        m4_undefine([PCT_REQUIRE_CXX])
        m4_define([PCT_REQUIRE_CXX],[yes])
      ],
      [force-gcc42], [
        m4_undefine([PCT_FORCE_GCC42])
        m4_define([PCT_FORCE_GCC42],[yes])
      ],
      [skip-visibility], [
        m4_undefine([PCT_USE_VISIBILITY])
        m4_define([PCT_USE_VISIBILITY],[no])
      ],
      [dont-suppress-include], [
        m4_undefine([PCT_DONT_SUPPRESS_INCLUDE])
        m4_define([PCT_DONT_SUPPRESS_INCLUDE],[yes])
      ],
      [no-vc-changelog], [
        m4_undefine([PCT_NO_VC_CHANGELOG])
        m4_define([PCT_NO_VC_CHANGELOG],[yes])
      ],
      [version-from-vc], [
        m4_undefine([PCT_VERSION_FROM_VC])
        m4_define([PCT_VERSION_FROM_VC],[yes])
    ])
  ])

  PANDORA_BLOCK_BAD_OPTIONS

  # We need to prevent canonical target
  # from injecting -O2 into CFLAGS - but we won't modify anything if we have
  # set CFLAGS on the command line, since that should take ultimate precedence
  AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"],
        [CFLAGS=""])
  AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],
        [CXXFLAGS=""])
  
  AM_INIT_AUTOMAKE(-Wall -Werror -Wno-portability subdir-objects foreign tar-ustar)

  m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

  m4_if(m4_substr(m4_esyscmd(test -d gnulib && echo 0),0,1),0,[
    gl_EARLY
  ],[
    PANDORA_EXTENSIONS 
  ])
  
  AC_REQUIRE([AC_PROG_CC])
  m4_if(PCT_FORCE_GCC42, [yes], [
    AC_REQUIRE([PANDORA_ENSURE_GCC_VERSION])
  ])
  AC_REQUIRE([PANDORA_64BIT])

  m4_if(PCT_NO_VC_CHANGELOG,yes,[
    vc_changelog=no
  ],[
    vc_changelog=yes
  ])
  m4_if(PCT_VERSION_FROM_VC,yes,[
    PANDORA_VC_INFO_HEADER
  ],[
    PANDORA_TEST_VC_DIR

    changequote(<<, >>)dnl
    PANDORA_RELEASE_ID=`echo $VERSION | sed 's/[^0-9]//g'`
    changequote([, ])dnl

    PANDORA_RELEASE_COMMENT=""
    AC_DEFINE_UNQUOTED([PANDORA_RELEASE_VERSION],["$VERSION"],
                       [Version of the software])

    AC_SUBST(PANDORA_RELEASE_COMMENT)
    AC_SUBST(PANDORA_RELEASE_VERSION)
    AC_SUBST(PANDORA_RELEASE_ID)
  ])
  PANDORA_VERSION

  dnl Once we can use a modern autoconf, we can use this
  dnl AC_PROG_CC_C99
  AC_REQUIRE([AC_PROG_CXX])
  PANDORA_EXTENSIONS
  AM_PROG_CC_C_O



  PANDORA_PLATFORM

  dnl autoconf doesn't automatically provide a fail-if-no-C++ macro
  dnl so we check c++98 features and fail if we don't have them, mainly
  dnl for that reason
  PANDORA_CHECK_CXX_STANDARD
  m4_if(PCT_REQUIRE_CXX, [yes], [
    AS_IF([test "$ac_cv_cxx_stdcxx_98" = "no"],[
      AC_MSG_ERROR([No working C++ Compiler has been found. ${PACKAGE} requires a C++ compiler that can handle C++98])
    ])
  ])
  PANDORA_CXX_CSTDINT
  PANDORA_CXX_CINTTYPES
  
  m4_if(m4_substr(m4_esyscmd(test -d gnulib && echo 0),0,1),0,[
    gl_INIT
    AC_CONFIG_LIBOBJ_DIR([gnulib])
  ])

  PANDORA_CHECK_C_VERSION
  PANDORA_CHECK_CXX_VERSION

  AC_HEADER_TIME
  AC_STRUCT_TM
  AC_TYPE_SIZE_T
  AC_SYS_LARGEFILE
  PANDORA_CLOCK_GETTIME

  AC_CHECK_HEADERS(sys/socket.h)

  # off_t is not a builtin type
  AC_CHECK_SIZEOF(off_t, 4)
  AS_IF([test "$ac_cv_sizeof_off_t" -eq 0],[
    AC_MSG_ERROR("${PACKAGE} needs an off_t type.")
  ])

  AC_CHECK_SIZEOF(size_t)
  AS_IF([test "$ac_cv_sizeof_size_t" -eq 0],[
    AC_MSG_ERROR("${PACKAGE} needs an size_t type.")
  ])

  AC_DEFINE_UNQUOTED([SIZEOF_SIZE_T],[$ac_cv_sizeof_size_t],[Size of size_t as computed by sizeof()])
  AC_CHECK_SIZEOF(long long)
  AC_DEFINE_UNQUOTED([SIZEOF_LONG_LONG],[$ac_cv_sizeof_long_long],[Size of long long as computed by sizeof()])
  AC_CACHE_CHECK([if time_t is unsigned], [ac_cv_time_t_unsigned],[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[
#include <time.h>
      ]],
      [[
      int array[(((time_t)-1) > 0) ? 1 : -1];
      ]])
    ],[
      ac_cv_time_t_unsigned=yes
    ],[
      ac_cv_time_t_unsigned=no
    ])
  ])
  AS_IF([test "$ac_cv_time_t_unsigned" = "yes"],[
    AC_DEFINE([TIME_T_UNSIGNED], 1, [Define to 1 if time_t is unsigned])
  ])

  AC_CACHE_CHECK([if system defines RUSAGE_THREAD], [ac_cv_rusage_thread],[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[
#include <sys/time.h>
#include <sys/resource.h>
      ]],[[
      int x= RUSAGE_THREAD;
      ]])
    ],[
      ac_cv_rusage_thread=yes
    ],[
      ac_cv_rusage_thread=no
    ])
  ])
  AS_IF([test "$ac_cv_rusage_thread" = "no"],[
    AC_DEFINE([RUSAGE_THREAD], [RUSAGE_SELF],
      [Define if system doesn't define])
  ])

  AC_CHECK_FUNC(setsockopt, [], [AC_CHECK_LIB(socket, setsockopt)])
  AC_CHECK_FUNC(bind, [], [AC_CHECK_LIB(bind, bind)])



  PANDORA_OPTIMIZE

  PANDORA_HAVE_GCC_ATOMICS

  PANDORA_HEADER_ASSERT

  PANDORA_WARNINGS(PCT_ALL_ARGS)

  PANDORA_ENABLE_DTRACE

  AC_LIB_PREFIX
  PANDORA_HAVE_BETTER_MALLOC

  AC_CHECK_PROGS([PERL], [perl])
  AC_CHECK_PROGS([DPKG_GENSYMBOLS], [dpkg-gensymbols], [:])
  AC_CHECK_PROGS([LCOV], [lcov], [echo lcov not found])
  AC_CHECK_PROGS([LCOV_GENHTML], [genhtml], [echo genhtml not found])

  AC_CHECK_PROGS([SPHINXBUILD], [sphinx-build], [:])
  AS_IF([test "x${SPHINXBUILD}" != "x:"],[
    AC_CACHE_CHECK([if sphinx is new enough],[ac_cv_recent_sphinx],[
    
    ${SPHINXBUILD} -Q -C -b man -d conftest.d . . >/dev/null 2>&1
    AS_IF([test $? -eq 0],[ac_cv_recent_sphinx=yes],
          [ac_cv_recent_sphinx=no])
    rm -rf conftest.d
    ])
  ])

  AM_CONDITIONAL(HAVE_DPKG_GENSYMBOLS,[test "x${DPKG_GENSYMBOLS}" != "x:"])
  AM_CONDITIONAL(HAVE_SPHINX,[test "x${SPHINXBUILD}" != "x:"])
  AM_CONDITIONAL(HAVE_RECENT_SPHINX,[test "x${ac_cv_recent_sphinx}" = "xyes"])

  AS_IF([test "x${gl_LIBOBJS}" != "x"],[
    AS_IF([test "$GCC" = "yes"],[
      AM_CPPFLAGS="-isystem \${top_srcdir}/gnulib -isystem \${top_builddir}/gnulib ${AM_CPPFLAGS}"
    ],[
    AM_CPPFLAGS="-I\${top_srcdir}/gnulib -I\${top_builddir}/gnulib ${AM_CPPFLAGS}"
    ])
  ])
  m4_if(m4_substr(m4_esyscmd(test -d src && echo 0),0,1),0,[
    AM_CPPFLAGS="-I\$(top_srcdir)/src -I\$(top_builddir)/src ${AM_CPPFLAGS}"
  ],[
    AM_CPPFLAGS="-I\$(top_srcdir) -I\$(top_builddir) ${AM_CPPFLAGS}"
  ])

  PANDORA_USE_PIPE

  AM_CFLAGS="-std=c99 ${AM_CFLAGS} ${CC_WARNINGS} ${CC_PROFILING} ${CC_COVERAGE}"
  AM_CXXFLAGS="${AM_CXXFLAGS} ${CXX_WARNINGS} ${CC_PROFILING} ${CC_COVERAGE}"

  AC_SUBST([AM_CFLAGS])
  AC_SUBST([AM_CXXFLAGS])
  AC_SUBST([AM_CPPFLAGS])
  AC_SUBST([AM_LDFLAGS])

])
