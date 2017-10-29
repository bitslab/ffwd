/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef MUBENCH_TM_SPEC_H_
#define MUBENCH_TM_SPEC_H_

#ifdef MUBENCH_WLPDSTM
#include "stm.h"
#elif defined MUBENCH_TANGER
#include "tanger-stm.h"
#endif /* MUBENCH_TANGER */


#ifdef MUBENCH_WLPDSTM

#define START                           BEGIN_TRANSACTION_DESC
#define START_ID(ID)                    BEGIN_TRANSACTION_DESC_ID(ID)
#define START_RO                        START
#define START_RO_ID(ID)                 START_ID(ID)
#define LOAD(addr)                      wlpdstm_read_word_desc(tx, (Word *)(addr))
#define STORE(addr, value)              wlpdstm_write_word_desc(tx, (Word *)addr, (Word)value)
#define COMMIT                          END_TRANSACTION
#define MALLOC(size)                    wlpdstm_tx_malloc_desc(tx, size)
#define FREE(addr, size)                wlpdstm_tx_free_desc(tx, addr, size)

#elif defined MUBENCH_TANGER

#define START                           tanger_begin()
#define START_ID(ID)                    tanger_begin()
#define START_RO                        tanger_begin()
#define START_RO_ID(ID)                 tanger_begin()
#define LOAD(addr)                      (*(addr))
#define STORE(addr, value)              (*(addr) = (value))
#define COMMIT                          tanger_commit()
#define MALLOC(size)                    malloc(size)
#define FREE(addr, size)                free(addr)

#elif defined MUBENCH_SEQUENTIAL

#define START                           /* nothing */
#define START_ID(ID)                    /* nothing */
#define START_RO                        /* nothing */
#define START_RO_ID(ID)                 /* nothing */
#define LOAD(addr)                      (*(addr))
#define STORE(addr, value)              (*(addr) = (value))
#define COMMIT                          /* nothing */
#define MALLOC(size)                    malloc(size)
#define FREE(addr, size)                free(addr)

#endif /* MUBENCH_SEQUENTIAL */



#ifdef  MUBENCH_WLPDSTM
#define TM_ARGDECL_ALONE                tx_desc* tx
#define TM_ARGDECL                      tx_desc* tx,
#define TM_ARG                          tx, 
#define TM_ARG_LAST                     , tx
#define TM_ARG_ALONE                    tx
#define TM_STARTUP()                    wlpdstm_global_init()
#define TM_SHUTDOWN()                   wlpdstm_global_shutdown()
#define TM_THREAD_ENTER()               wlpdstm_thread_init(); \
										tx_desc *tx = wlpdstm_get_tx_desc()
#define TM_THREAD_EXIT()                wlpdstm_thread_shutdown()

#elif defined MUBENCH_TANGER

#define TM_ARGDECL_ALONE                /* nothing */
#define TM_ARGDECL                      /* nothing */
#define TM_ARG                          /* nothing */
#define TM_ARG_ALONE                    /* nothing */
#define TM_ARG_LAST                     /* nothing */
#define TM_STARTUP()                    tanger_init()
#define TM_SHUTDOWN()                   tanger_shutdown()
#define TM_THREAD_ENTER()               tanger_thread_init()
#define TM_THREAD_EXIT()                tanger_thread_shutdown()

#elif defined MUBENCH_SEQUENTIAL

#define TM_ARGDECL_ALONE                /* nothing */
#define TM_ARGDECL                      /* nothing */
#define TM_ARG                          /* nothing */
#define TM_ARG_ALONE                    /* nothing */
#define TM_ARG_LAST                     /* nothing */
#define TM_STARTUP()                    /* nothing */
#define TM_SHUTDOWN()                   /* nothing */
#define TM_THREAD_ENTER()               /* nothing */
#define TM_THREAD_EXIT()                /* nothing */

#endif /* MUBENCH_SEQUENTIAL */

#define TM_SHARED_READ(var)             LOAD(&(var))
#define TM_SHARED_READ_P(var)           LOAD(&(var))

#define TM_SHARED_WRITE(var, val)       STORE(&(var), val)
#define TM_SHARED_WRITE_P(var, val)     STORE(&(var), val)

#define TM_MALLOC(size)                 MALLOC(size)
#define TM_FREE(ptr)                    FREE(ptr, sizeof(*ptr))

#endif /* MUBENCH_TM_SPEC_H_ */
