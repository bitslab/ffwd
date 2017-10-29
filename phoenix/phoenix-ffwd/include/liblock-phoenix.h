#ifndef LIBLOCK_PHOENIX_H
#define LIBLOCK_PHOENIX_H

#define TYPE_POSIX "posix"
#define TYPE_RCL        "rcl"
//#define TYPE_MCS        "mcs"
//#define TYPE_SPINLOCK   "spinlock"
//#define TYPE_FLAT       "flat"


#define DEFAULT_ARG get_server_core_1()
//#define TYPE_NOINFO TYPE_POSIX
//#define TYPE_NOINFO TYPE_RCL
#define TYPE_NOINFO liblock_lock_name
#define ARG_NOINFO DEFAULT_ARG

#undef liblock_execute_operation 
#define liblock_execute_operation(lock, ctx, fct) liblock_exec(lock, (void* (*)(void *))(fct), ctx)

extern const char*  liblock_lock_name;

extern struct core* get_server_core_1();
extern struct core* get_server_core_2();
/* extern struct core* get_server_core_3(); */

/* #define TYPE_POSIX      "posix" */
/* //#define TYPE_RCL        "rcl" */
/* //#define TYPE_MCS        "mcs" */
/* //#define TYPE_SPINLOCK   "spinlock" */
/* //#define TYPE_FLAT       "flat" */

/* #define TYPE_EXPERIENCE (liblock_lock_name) */
/* #define DEFAULT_ARG     get_server_core_1() */
/* #define ARG_CORE_1      get_server_core_1() */
/* #define ARG_CORE_2      get_server_core_2() */
/* #define ARG_CORE_3      get_server_core_3() */

#endif

