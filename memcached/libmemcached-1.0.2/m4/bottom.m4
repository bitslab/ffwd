AC_DEFUN([CONFIG_EXTRA], [

AH_BOTTOM([

#if defined(__cplusplus) 
#include CSTDINT_H 
#include CINTTYPES_H 
#else 
#include <stdint.h> 
#include <inttypes.h> 
#endif

#if !defined(HAVE_ULONG) && !defined(__USE_MISC)
# define HAVE_ULONG 1
typedef unsigned long int ulong;
#endif

])

AH_BOTTOM([
#ifdef WIN32
#define _WIN32_WINNT 0x0501
#endif

/* To hide the platform differences between MS Windows and Unix, I am
 * going to use the Microsoft way and #define the Microsoft-specific
 * functions to the unix way. Microsoft use a separate subsystem for sockets,
 * but Unix normally just use a filedescriptor on the same functions. It is
 * a lot easier to map back to the unix way with macros than going the other
 * way without side effect ;-)
 */
#ifdef WIN32
#include "win32/wrappers.h"
#define get_socket_errno() WSAGetLastError()
#else
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(a) close(a)
#define get_socket_errno() errno
#endif

#ifndef HAVE_MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef HAVE_MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

#ifndef HAVE_MSG_MORE
#define MSG_MORE 0
#endif

])
])dnl CONFIG_EXTRA
