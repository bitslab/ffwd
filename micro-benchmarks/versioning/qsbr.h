#ifndef __QSBR_H
#define __QBSR_H

#define QSBR_N_EPOCHS 3

#define QSBR_PADDING 16

#define QSBR_FREELIST_SIZE 65536

typedef struct qsbr_pthread_data {
	int epoch;
	int in_critical;
	long qsbr_padding[QSBR_PADDING];
	size_t freelist_count[QSBR_N_EPOCHS];
	void *freelist[QSBR_N_EPOCHS][QSBR_FREELIST_SIZE];
} qsbr_pthread_data_t;

void qsbr_init(void);

void qsbr_pthread_init(qsbr_pthread_data_t *qsbr_data);

void qsbr_quiescent_state(qsbr_pthread_data_t *qsbr_data);

void qsbr_free_ptr(void *ptr, qsbr_pthread_data_t *qsbr_data);

#endif
