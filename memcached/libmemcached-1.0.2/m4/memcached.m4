AX_WITH_PROG(MEMCACHED_BINARY,memcached)
AS_IF([test -f "$ac_cv_path_MEMCACHED_BINARY"],
      [
        AC_DEFINE([HAVE_MEMCACHED_BINARY], [1], [If Memcached binary is available])
        AC_DEFINE_UNQUOTED([MEMCACHED_BINARY], "$ac_cv_path_MEMCACHED_BINARY", [Name of the memcached binary used in make test])
       ],
       [
        AC_DEFINE([HAVE_MEMCACHED_BINARY], [0], [If Memcached binary is available])
        AC_DEFINE([MEMCACHED_BINARY], [0], [Name of the memcached binary used in make test])
      ])
