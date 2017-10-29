AX_WITH_PROG(MEMCACHED_SASL_BINARY,memcached_sasl)
AS_IF([test -f "$ac_cv_path_MEMCACHED_SASL_BINARY"],
      [
        AC_DEFINE([HAVE_MEMCACHED_SASL_BINARY], [1], [Name of the memcached_sasl binary used in make test])
        AC_DEFINE_UNQUOTED([MEMCACHED_SASL_BINARY], "$ac_cv_path_MEMCACHED_SASL_BINARY", [Name of the memcached_sasl binary used in make test])
       ],
       [
        AC_DEFINE([HAVE_MEMCACHED_SASL_BINARY], [0], [Name of the memcached_sasl binary used in make test])
        AC_DEFINE([MEMCACHED_SASL_BINARY], [0], [Name of the memcached_sasl binary used in make test])
      ])
