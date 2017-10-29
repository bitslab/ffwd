AX_CHECK_LIBRARY([LIBGEARMAN], [libgearman/gearmand.h], [gearman], 
                  [
                    AM_CONDITIONAL(HAVE_LIBGEARMAN, true)
                    AC_DEFINE([HAVE_LIBGEARMAN], [1], [Enables libgearman Support])
                  ], 
                  [
                    AM_CONDITIONAL(HAVE_LIBGEARMAN, false)
                    AC_DEFINE([HAVE_LIBGEARMAN], [0], [Enables libgearman Support])
                  ])
