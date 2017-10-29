/* =============================================================================
 *
 * tm.h
 *
 * Utility defines for transactional memory
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Authors: Chi Cao Minh and Martin Trautmann
 *
 * =============================================================================
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef TM_H
#define TM_H 1

/* =============================================================================
 * Simulator Specific Interface
 *
 * MAIN(argc, argv)
 *     Declare the main function with argc being the identifier for the argument
 *     count and argv being the name for the argument string list
 *
 * MAIN_RETURN(int_val)
 *     Returns from MAIN function
 *
 * GOTO_SIM()
 *     Switch simulator to simulation mode
 *
 * GOTO_REAL()
 *     Switch simulator to non-simulation (real) mode
 *     Note: use in sequential region only
 *
 * IS_IN_SIM()
 *     Returns true if simulator is in simulation mode
 *
 * SIM_GET_NUM_CPU(var)
 *     Assigns the number of simulated CPUs to "var"
 *
 * P_MEMORY_STARTUP
 *     Start up the memory allocator system that handles malloc/free
 *     in parallel regions (but not in transactions)
 *
 * P_MEMORY_SHUTDOWN
 *     Shutdown the memory allocator system that handles malloc/free
 *     in parallel regions (but not in transactions)
 *
 * =============================================================================
 */
#ifdef SIMULATOR

#  include <simapi.h>

#  define MAIN(argc, argv)              void mainX (int argc, \
                                                    const char** argv, \
                                                    const char** envp)
#  define MAIN_RETURN(val)              return /* value is ignored */

#  define GOTO_SIM()                    goto_sim()
#  define GOTO_REAL()                   goto_real()
#  define IS_IN_SIM()                   (inSimulation)

#  define SIM_GET_NUM_CPU(var)          ({ \
                                            if (!IS_IN_SIM()) { \
                                                GOTO_SIM(); \
                                                var = Sim_GetNumCpus(); \
                                                GOTO_REAL(); \
                                            } else { \
                                                var = Sim_GetNumCpus(); \
                                            } \
                                            var; \
                                        })

#  define TM_PRINTF                     Sim_Print
#  define TM_PRINT0                     Sim_Print0
#  define TM_PRINT1                     Sim_Print1
#  define TM_PRINT2                     Sim_Print2
#  define TM_PRINT3                     Sim_Print3

#  include "memory.h"
#  define P_MEMORY_STARTUP(numThread)   do { \
                                            bool_t status; \
                                            status = memory_init((numThread), \
                                                                 1<<24, \
                                                                 2); \
                                            assert(status); \
                                        } while (0) /* enforce comma */
#  define P_MEMORY_SHUTDOWN()           memory_destroy()

#else /* !SIMULATOR */

#  include <stdio.h>

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */

#endif /* !SIMULATOR */


/* =============================================================================
 * Transactional Memory System Interface
 *
 * TM_ARG
 * TM_ARG_ALONE
 * TM_ARGDECL
 * TM_ARGDECL_ALONE
 *     Used to pass TM thread meta data to functions (see Examples below)
 *
 * TM_STARTUP(numThread)
 *     Startup the TM system (call before any other TM calls)
 *
 * TM_SHUTDOWN()
 *     Shutdown the TM system
 *
 * TM_THREAD_ENTER()
 *     Call when thread first enters parallel region
 *
 * TM_THREAD_EXIT()
 *     Call when thread exits last parallel region
 *
 * P_MALLOC(size)
 *     Allocate memory inside parallel region
 *
 * P_FREE(ptr)
 *     Deallocate memory inside parallel region
 *
 * TM_MALLOC(size)
 *     Allocate memory inside atomic block / transaction
 *
 * TM_FREE(ptr)
 *     Deallocate memory inside atomic block / transaction
 *
 * TM_BEGIN()
 *     Begin atomic block / transaction
 *
 * TM_BEGIN_RO()
 *     Begin atomic block / transaction that only reads shared data
 *
 * TM_END()
 *     End atomic block / transaction
 *
 * TM_RESTART()
 *     Restart atomic block / transaction
 *
 * =============================================================================
 *
 * Example Usage: 
 *
 *     MAIN(argc,argv)
 *     {
 *         TM_STARTUP(8);
 *         // create 8 threads and go parallel
 *         TM_SHUTDOWN();
 *     }
 *
 *     void parallel_region ()
 *     {
 *         TM_THREAD_ENTER();
 *         subfunction1(TM_ARG_ALONE);
 *         subfunction2(TM_ARG  1, 2, 3);
 *         TM_THREAD_EXIT();
 *     }
 *
 *     void subfunction1 (TM_ARGDECL_ALONE)
 *     {
 *         TM_BEGIN_RO()
 *         // ... do work that only reads shared data ...
 *         TM_END()
 *
 *         long* array = (long*)P_MALLOC(10 * sizeof(long));
 *         // ... do work ...
 *         P_FREE(array);
 *     }
 *
 *     void subfunction2 (TM_ARGDECL  long a, long b, long c)
 *     {
 *         TM_BEGIN();
 *         long* array = (long*)TM_MALLOC(a * b * c * sizeof(long));
 *         // ... do work that may read or write shared data ...
 *         TM_FREE(array);
 *         TM_END();
 *     }
 * 
 * =============================================================================
 */


/* =============================================================================
 * HTM - Hardware Transactional Memory
 * =============================================================================
 */

#ifdef HTM

#  ifndef SIMULATOR
#    error HTM requries SIMULATOR
#  endif

#  include <assert.h>
#  include <tmapi.h>
#  include "memory.h"
#  include "thread.h"
#  include "types.h"

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */

#  define TM_STARTUP(numThread)         /* nothing */
#  define TM_SHUTDOWN()                 /* nothing */

#  define TM_THREAD_ENTER()             /* nothing */
#  define TM_THREAD_EXIT()              /* nothing */

#  define P_MALLOC(size)                memory_get(thread_getId(), size)
#  define P_FREE(ptr)                   /* TODO: thread local free is non-trivial */
#  define TM_MALLOC(size)               memory_get(thread_getId(), size)
#  define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#  define TM_BEGIN()                    TM_BeginClosed()
#  define TM_BEGIN_RO()                 TM_BeginClosed()
#  define TM_END()                      TM_EndClosed()
#  define TM_RESTART()                  /* nothing */


/* =============================================================================
 * STM - Software Transactional Memory
 * =============================================================================
 */

#elif defined(STM)

#  include <stm.h>
#  include <string.h>
#  include "thread.h"

#  define TM_ARG                        STM_SELF,
#  define TM_ARG_ALONE                  STM_SELF
#  define TM_ARGDECL                    STM_THREAD_T* TM_ARG
#  define TM_ARGDECL_ALONE              STM_THREAD_T* TM_ARG_ALONE

#  ifdef SIMULATOR

#    define TM_STARTUP(numThread)       STM_STARTUP(); \
                                        STM_NEW_THREADS(numThread)
#    define TM_SHUTDOWN()               STM_SHUTDOWN()

#    define TM_THREAD_ENTER()           TM_ARGDECL_ALONE = \
                                            STM_GET_THREAD(thread_getId());
#    define TM_THREAD_EXIT()            STM_FREE_THREAD(TM_ARG_ALONE)

#    define P_MALLOC(size)              memory_get(thread_getId(), size)
#    define P_FREE(ptr)                 /* TODO: thread local free is non-trivial */
#    define TM_MALLOC(size)             memory_get(thread_getId(), size)
#    define TM_FREE(ptr)                /* TODO: thread local free is non-trivial */

#  else /* !SIMULATOR */

#    define TM_STARTUP(numThread)       STM_STARTUP()
#    define TM_SHUTDOWN()               STM_SHUTDOWN()

#    define TM_THREAD_ENTER()           TM_ARGDECL_ALONE = STM_NEW_THREAD(); \
                                        STM_INIT_THREAD(TM_ARG_ALONE, thread_getId())
#    define TM_THREAD_EXIT()            STM_FREE_THREAD(TM_ARG_ALONE)

#    define malloc(size)                tmalloc_reserve(size)
#    define calloc(n, size)             ({ \
                                            size_t numByte = (n) * (size); \
                                            void* ptr = tmalloc_reserve(numByte); \
                                            if (ptr) { \
                                                memset(ptr, 0, numByte); \
                                            } \
                                            ptr; \
                                        })
#    define realloc(ptr, size)          tmalloc_reserveAgain(ptr, size)
#    define free(ptr)                   tmalloc_release(ptr)

#    define P_MALLOC(size)              tmalloc_reserve(size)
#    define P_FREE(ptr)                 tmalloc_release(ptr)
#    define TM_MALLOC(size)             STM_ALLOC(size)
#    define TM_FREE(ptr)                STM_FREE(ptr)

#  endif /* SIMULATOR */

#  define TM_BEGIN()                    STM_BEGIN_WR()
#  define TM_BEGIN_RO()                 STM_BEGIN_RD()
#  define TM_END()                      STM_END()
#  define TM_RESTART()                  STM_RESTART()


/* =============================================================================
 * Sequential execution
 * =============================================================================
 */

#else /* SEQUENTIAL */

#  include <assert.h>

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */

#  define TM_STARTUP(numThread)         /* nothing */
#  define TM_SHUTDOWN()                 /* nothing */

#  define TM_THREAD_ENTER()             /* nothing */
#  define TM_THREAD_EXIT()              /* nothing */

#  define P_MALLOC(size)                malloc(size)
#  define P_FREE(ptr)                   free(ptr)
#  define TM_MALLOC(size)               malloc(size)
#  define TM_FREE(ptr)                  free(ptr)

#  define TM_BEGIN()                    /* nothing */
#  define TM_BEGIN_RO()                 /* nothing */
#  define TM_END()                      /* nothing */
#  define TM_RESTART()                  assert(0)

#endif /* SEQUENTIAL */


/* =============================================================================
 * Transactional Memory System interface for shared memory accesses
 *
 * There are 3 flavors of each function:
 *
 * 1) no suffix: for accessing variables of size "long"
 * 2) _P suffix: for accessing variables of type "pointer"
 * 3) _F suffix: for accessing variables of type "float"
 * =============================================================================
 */
#if defined(STM)

#  define TM_SHARED_READ(var)           STM_READ(var)
#  define TM_SHARED_READ_P(var)         STM_READ_P(var)
#  define TM_SHARED_READ_F(var)         STM_READ_F(var)

#  define TM_SHARED_WRITE(var, val)     STM_WRITE((var), val)
#  define TM_SHARED_WRITE_P(var, val)   STM_WRITE_P((var), val)
#  define TM_SHARED_WRITE_F(var, val)   STM_WRITE_F((var), val)

#  define TM_LOCAL_WRITE(var, val)      STM_LOCAL_WRITE(var, val)
#  define TM_LOCAL_WRITE_P(var, val)    STM_LOCAL_WRITE_P(var, val)
#  define TM_LOCAL_WRITE_F(var, val)    STM_LOCAL_WRITE_F(var, val)

#else /* !STM */

#  define TM_SHARED_READ(var)           (var)
#  define TM_SHARED_READ_P(var)         (var)
#  define TM_SHARED_READ_F(var)         (var)

#  define TM_SHARED_WRITE(var, val)     ({var = val; var;})
#  define TM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_F(var, val)   ({var = val; var;})

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

#endif /* !STM */


#endif /* TM_H */


/* =============================================================================
 *
 * End of tm.h
 *
 * =============================================================================
 */
