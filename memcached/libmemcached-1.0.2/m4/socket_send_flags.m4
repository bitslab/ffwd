dnl  Copyright (C) 2011 Trond Norbye
dnl This file is free software; Trond Norbye
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl ---------------------------------------------------------------------------
dnl Macro: SOCKET_SEND_FLAGS
dnl ---------------------------------------------------------------------------

AC_DEFUN([SOCKET_SEND_FLAGS],
[
  AC_CACHE_CHECK([for MSG_NOSIGNAL], [ac_cv_msg_nosignal], [
    AC_LANG_PUSH([C])
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I${srcdir}"
    AC_TRY_LINK([
#include <sys/socket.h>
                   ], [
int flags= MSG_NOSIGNAL;
                   ],
                   [ ac_cv_msg_nosignal=yes ],
                   [ ac_cv_msg_nosignal=no ])
   CFLAGS="$save_CFLAGS"
   AC_LANG_POP
  ])

  AC_CACHE_CHECK([for MSG_DONTWAIT], [ac_cv_msg_dontwait], [
    AC_LANG_PUSH([C])
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I${srcdir}"
    AC_TRY_LINK([
#include <sys/socket.h>
                   ], [
int flags= MSG_DONTWAIT;
                   ],
                   [ ac_cv_msg_dontwait=yes ],
                   [ ac_cv_msg_dontwait=no ])
   CFLAGS="$save_CFLAGS"
   AC_LANG_POP
  ])

  AC_CACHE_CHECK([for MSG_MORE], [ac_cv_msg_more], [
    AC_LANG_PUSH([C])
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I${srcdir}"
    AC_TRY_LINK([
#include <sys/socket.h>
                   ], [
int flags= MSG_MORE;
                   ],
                   [ ac_cv_msg_more=yes ],
                   [ ac_cv_msg_more=no ])
   CFLAGS="$save_CFLAGS"
   AC_LANG_POP
  ])

  AS_IF([test "x$ac_cv_msg_nosignal" = "xyes"],[
        AC_DEFINE(HAVE_MSG_NOSIGNAL, 1, [Define to 1 if you have a MSG_NOSIGNAL])])
  AS_IF([test "x$ac_cv_msg_dontwait" = "xyes"],[
        AC_DEFINE(HAVE_MSG_DONTWAIT, 1, [Define to 1 if you have a MSG_DONTWAIT])])
  AS_IF([test "x$ac_cv_msg_more" = "xyes"],[
        AC_DEFINE(HAVE_MSG_MORE, 1, [Define to 1 if you have a HAVE_MSG_MORE])])
])

dnl ---------------------------------------------------------------------------
dnl End Macro: SOCKET_SEND_FLAGS
dnl ---------------------------------------------------------------------------
