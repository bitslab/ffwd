#ifndef LIBLOCK_CONFIG
#define LIBLOCK_CONFIG

//#define TYPE_NOINFO TYPE_POSIX
#define TYPE_NOINFO TYPE_EXPERIENCE
#define ARG_NOINFO DEFAULT_ARG

#define liblock_execute_operation(lock, ctx, fct) liblock_exec(lock, (void* (*)(void *))(fct), ctx)

extern const char*  liblock_lock_name;
extern struct core* liblock_server_core_1;
 
#define TYPE_POSIX      "posix"
#define TYPE_RCL        "rcl"
#define TYPE_MCS        "mcs"
#define TYPE_SPINLOCK   "spinlock"
#define TYPE_FLAT       "flat"
 
#define TYPE_EXPERIENCE (liblock_lock_name)
#define DEFAULT_ARG     (liblock_server_core_1)

#endif
