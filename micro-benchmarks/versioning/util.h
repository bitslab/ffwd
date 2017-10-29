#ifndef __UTIL_H
#define __UTIL_H

#define CAS(addr, oldv, newv) __sync_bool_compare_and_swap((addr), (oldv), (newv))

#define MEMBARSTLD() __sync_synchronize()

#define FETCH_AND_ADD(addr, v) __sync_fetch_and_add((addr), (v))

#endif
