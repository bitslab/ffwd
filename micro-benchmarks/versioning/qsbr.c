#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "qsbr.h"
#include "util.h"

#define CACHE_ALIGN 64

#define QSBR_MAX_THREADS 128

static long g_nr_threads = 0;
static qsbr_pthread_data_t *g_threads[QSBR_MAX_THREADS] = {0,};

static volatile pthread_spinlock_t g_update_lock __attribute__ ((__aligned__ (CACHE_ALIGN)));
static volatile int g_epoch __attribute__ ((__aligned__ (CACHE_ALIGN)));

void qsbr_init(void)
{
	g_epoch = 1;
	pthread_spin_init(&g_update_lock, PTHREAD_PROCESS_SHARED);
}

void qsbr_pthread_init(qsbr_pthread_data_t *qsbr_data)
{
	int i;

	qsbr_data->epoch = 0;
	qsbr_data->in_critical = 1;

	for (i = 0; i < QSBR_N_EPOCHS; i++)
		qsbr_data->freelist_count[i] = 0;
	g_threads[FETCH_AND_ADD(&g_nr_threads, 1)] = qsbr_data;
}

static inline void qsbr_free(qsbr_pthread_data_t *qsbr_data, int epoch)
{
	int i;

	MEMBARSTLD();

	for (i = 0; i < qsbr_data->freelist_count[epoch]; i++)
		free(qsbr_data->freelist[epoch][i]);
	qsbr_data->freelist_count[epoch] = 0;
}

static int qsbr_update_epoch()
{
	int i, cur_epoch;

	if (!pthread_spin_trylock(&g_update_lock))
		return 0;

	cur_epoch = g_epoch;
	for (i = 0; i < g_nr_threads; i++) {
		if (g_threads[i]->in_critical == 1 &&
		    g_threads[i]->epoch != cur_epoch) {
			pthread_spin_unlock(&g_update_lock);
			return 0;
		}
	}

	g_epoch = (cur_epoch + 1) % QSBR_N_EPOCHS;

	pthread_spin_unlock(&g_update_lock);

	return 1;
}

void qsbr_quiescent_state(qsbr_pthread_data_t *qsbr_data)
{
	int epoch;

	epoch = g_epoch;
	if (qsbr_data->epoch != epoch) {
		qsbr_free(qsbr_data, epoch);
		qsbr_data->epoch = epoch;
	} else {
		qsbr_data->in_critical = 0;
		if (qsbr_update_epoch()) {
			qsbr_data->in_critical = 1;
			MEMBARSTLD();
			epoch = g_epoch;
			if (qsbr_data->epoch != epoch) {
				qsbr_free(qsbr_data, epoch);
				qsbr_data->epoch = epoch;
			}
			return;
		}
		qsbr_data->in_critical = 1;
		MEMBARSTLD();
	}

	return;
}

void qsbr_free_ptr(void *ptr, qsbr_pthread_data_t *qsbr_data)
{
	// assert(qsbr_data->freelist_count[qsbr_data->epoch] < QSBR_FREELIST_SIZE);
	// qsbr_data->freelist[qsbr_data->epoch][(qsbr_data->freelist_count[qsbr_data->epoch])++] = ptr;
}
