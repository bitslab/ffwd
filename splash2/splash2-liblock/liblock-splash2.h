#ifndef _LIBLOCK_SPLASH2_H_
#define _LIBLOCK_SPLASH2_H_

#undef liblock_execute_operation
#define liblock_execute_operation(lock, val, pending) liblock_exec(lock, pending, val)

extern const char*  liblock_lock_name;

extern struct core* get_server_core_1();
extern struct core* get_server_core_2();

#define TYPE_NOINFO liblock_lock_name
#define TYPE_POSIX      "posix"
#define TYPE_RCL        "rcl"
#define TYPE_MCS        "mcs"
#define TYPE_SPINLOCK   "spinlock"
#define TYPE_FLAT       "flat"

#define TYPE_EXPERIENCE (liblock_lock_name)
//#define DEFAULT_ARG     0

#define DEFAULT_ARG get_server_core_1()
#define ARG_NOINFO DEFAULT_ARG

#include "liblock-config.h"

#endif
