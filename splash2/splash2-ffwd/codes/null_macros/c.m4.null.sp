divert(-1)
define(NEWPROC,) dnl

define(BARRIER, `{
	pthread_barrier_wait(&($1));
}')

define(BARDEC, `
pthread_barrier_t	($1);
')

define(BARINIT, `{
	pthread_barrier_init(&($1), NULL, $2);
}')

define(BAREXCLUDE, `{;}')

define(BARINCLUDE, `{;}')

define(GSDEC, `long ($1);')
define(GSINIT, `{ ($1) = 0; }')
define(GETSUB, `{
  if (($1)<=($3))
    ($2) = ($1)++;
  else {
    ($2) = -1;
    ($1) = 0;
  }
}')

define(NU_GSDEC, `long ($1);')
define(NU_GSINIT, `{ ($1) = 0; }')
define(NU_GETSUB, `GETSUB($1,$2,$3,$4)')

define(ADEC, `long ($1);')
define(AINIT, `{;}')
define(PROBEND, `{;}')

define(LOCKDEC, `pthread_mutex_t ($1);')
define(LOCKINIT, `{pthread_mutex_init(&($1), NULL); printf("@$1: %p\n", &$1);}')
define(LOCK, `{pthread_mutex_lock(&($1));}')
define(UNLOCK, `{pthread_mutex_unlock(&($1));}')

define(SPINLOCKDEC, `volatile cache_block_t *($1); ')
define(SPINLOCKINIT, `{
    int result;

	result = posix_memalign((void **)&($1),
                            CACHE_BLOCK_SIZE,
                            CACHE_BLOCK_SIZE);
	
    if (result != 0) {
    	fprintf(stderr, "Error: posix_memalign");
        exit(-1);
    }

    memset((void *)($1), 0, CACHE_BLOCK_SIZE);
	
	*((uint32_t *)($1)) = 0;
}')
define(SPINLOCK, `{
// 	asm volatile("" ::: "memory");
// 	while (!__sync_bool_compare_and_swap_4(&(($1)[0]), 0, 1))
//	__asm__ __volatile__ ("rep; nop" : : );
//	asm volatile("" ::: "memory");
	lock_spin($1);
}')
define(SPINUNLOCK, `{
	unlock_spin($1);
//	asm volatile("" ::: "memory");
//	*((uint32_t *)($1)) = 0;
//	asm volatile("" ::: "memory");
}')


define(NLOCKDEC, `long ($1);')
define(NLOCKINIT, `{;}')
define(NLOCK, `{;}')
define(NUNLOCK, `{;}')

define(ALOCKDEC, `pthread_mutex_t $1[$2];')
define(ALOCKINIT, `{
	unsigned long	i, Error;

	for (i = 0; i < $2; i++) {
		Error = pthread_mutex_init(&$1[i], NULL);
		if (Error != 0) {
			printf("Error while initializing array of locks.\n");
			exit(-1);
		}
	}
}')
define(ALOCK, `{pthread_mutex_lock(&$1[$2]);}')
define(AULOCK, `{pthread_mutex_unlock(&$1[$2]);}')

define(PAUSEDEC, `
struct {
	pthread_mutex_t	Mutex;
	pthread_cond_t	CondVar;
	unsigned long	Flag;
} $1;
')
define(PAUSEINIT, `{
	pthread_mutex_init(&$1.Mutex, NULL);
	pthread_cond_init(&$1.CondVar, NULL);
	$1.Flag = 0;
}
')
define(CLEARPAUSE, `{
	$1.Flag = 0;
	pthread_mutex_unlock(&$1.Mutex);}
')
define(SETPAUSE, `{
	pthread_mutex_lock(&$1.Mutex);
	$1.Flag = 1;
	pthread_cond_broadcast(&$1.CondVar);
	pthread_mutex_unlock(&$1.Mutex);}
')
define(EVENT, `{;}')
define(WAITPAUSE, `{
	pthread_mutex_lock(&$1.Mutex);
	if ($1.Flag == 0) {
		pthread_cond_wait(&$1.CondVar, &$1.Mutex);
	}
}')
define(PAUSE, `{;}')

define(AUG_ON, ` ')
define(AUG_OFF, ` ')
define(TRACE_ON, ` ')
define(TRACE_OFF, ` ')
define(REF_TRACE_ON, ` ')
define(REF_TRACE_OFF, ` ')
define(DYN_TRACE_ON, `;')
define(DYN_TRACE_OFF, `;')
define(DYN_REF_TRACE_ON, `;')
define(DYN_REF_TRACE_OFF, `;')
define(DYN_SIM_ON, `;')
define(DYN_SIM_OFF, `;')
define(DYN_SCHED_ON, `;')
define(DYN_SCHED_OFF, `;')
define(AUG_SET_LOLIMIT, `;')
define(AUG_SET_HILIMIT, `;')

define(MENTER, `{;}')
define(DELAY, `{;}')
define(CONTINUE, `{;}')
define(MEXIT, `{;}')
define(MONINIT, `{;}')

define(WAIT_FOR_END, `{
	long	i, Error;
	for (i = 0; i < ($1) - 1; i++) {
		Error = pthread_join(PThreadTable[i], NULL);
		if (Error != 0) {
			printf("Error in pthread_join().\n");
			exit(-1);
		}
	}
}')

define(CREATE, `{
	long	i, Error;

	for (i = 0; i < ($2) - 1; i++) {
		Error = pthread_create(&PThreadTable[i], NULL, (void * (*)(void *))($1), NULL);
		if (Error != 0) {
			printf("Error in pthread_create().\n");
			exit(-1);
		}
	}

	$1();
}')

define(MAIN_INITENV, `{;}')
define(MAIN_END, `{exit(0);}')

define(MAIN_ENV,`
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#define MAX_THREADS 48
#define CACHE_BLOCK_SIZE 128
pthread_t PThreadTable[MAX_THREADS];
')

define(ENV, ` ')
define(EXTERN_ENV, `
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
extern pthread_t PThreadTable[];
')

define(G_MALLOC, `valloc($1);')
define(G_FREE, `;')
define(G_MALLOC_F, `valloc($1)')
define(NU_MALLOC, `valloc($1);')
define(NU_FREE, `;')
define(NU_MALLOC_F, `valloc($1)')

define(GET_HOME, `{($1) = 0;}')
define(GET_PID, `{($1) = 0;}')
define(AUG_DELAY, `{sleep ($1);}')
define(ST_LOG, `{;}')
define(SET_HOME, `{;}')
define(CLOCK, `{
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	($1) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
}')
divert(0)
