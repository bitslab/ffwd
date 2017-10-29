#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdio.h>
#include <pthread.h>

#if defined (__sun)
#    include <sys/pset.h>  /* P_PID, processor_bind() */
#    include <sys/types.h>
#    include <sys/processor.h>
#    include <sys/procset.h>
#    include <unistd.h>    /* getpid() */
#elif (defined (__gnu_linux__) || defined (__gnu__linux) || defined (__linux__))
#    include <sched.h>     /* CPU_SET, CPU_ZERO, cpu_set_t, sched_setaffinity() */
#endif

#include "config.h"


__thread int32_t __thread_id = -1;

inline static void setThreadId(int32_t id) {
    __thread_id = id;
}

inline static int32_t getThreadId(void) {
    return __thread_id;
}

int _thread_pin(unsigned int cpu_id)
{
    pthread_setconcurrency(USE_CPUS);

#if defined(__sun) || defined(sun)
    int ret;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
    ret = processor_bind(P_LWPID, cpu_id + 1, (((cpu_id%2) * 64) + (cpu_id/2))% USE_CPUS, NULL);
    if (ret == -1)
        perror("processor_bind");

    return ret;
    
#elif defined (__gnu_linux__) || defined (__gnu__linux) || defined (__linux__)
    int ret = 0;
    cpu_set_t mask;  
    unsigned int len = sizeof(mask);

    CPU_ZERO(&mask);
    CPU_SET(cpu_id % USE_CPUS, &mask);
    ret = sched_setaffinity(0, len, &mask);
    if (ret == -1)
        perror("sched_setaffinity");

    return ret;
#endif
}

#endif

