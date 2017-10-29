#include <getopt.h>
#include <sys/time.h>
#include <stdint.h>

#ifdef FFWD
	#include "ffwd.h"
	#include "macro.h"
#endif

#include "benchmark_list_move.h"

static void barrier_init(barrier_t *b, int n)
{
	pthread_cond_init(&b->complete, NULL);
	pthread_mutex_init(&b->mutex, NULL);
	b->count = n;
	b->crossing = 0;
}

static void barrier_cross(barrier_t *b)
{
	pthread_mutex_lock(&b->mutex);
	b->crossing++;
	if (b->crossing < b->count)
		pthread_cond_wait(&b->complete, &b->mutex);
	else {
		pthread_cond_broadcast(&b->complete);
		b->crossing = 0;
	}
	pthread_mutex_unlock(&b->mutex);
}

#define DEFAULT_DURATION 1000
#define DEFAULT_NTHREADS 1
#define DEFAULT_ISIZE    256
#define DEFAULT_VRANGE   512

void print_args(void)
{
	printf("list benchmark:\n");
	printf("options:\n");
	printf("  -h: print help message\n");
	printf("  -d: test running time in milliseconds (default %d)\n", DEFAULT_DURATION);
	printf("  -n: number of threads (default %d)\n", DEFAULT_NTHREADS);
	printf("  -i: initial size of the list (default %d)\n", DEFAULT_ISIZE);
	printf("  -r: range of value (default %d)\n", DEFAULT_VRANGE);
}

static volatile int stop;

static void *bench_thread(void *data)
{
	int from;
	int key;
	int i;
	pthread_data_t *d = (pthread_data_t *)data;

	// thread_init

	barrier_cross(d->barrier);
	while (stop == 0) {
		from = rand_r(&d->seed) % 2;
		key = rand_r(&d->seed) % d->range;
		list_move(key, d, from);
		d->nr_move++;
	}

	// thread cleanup

	return NULL;
}

int main(int argc, char *argv[])
{
	struct option bench_options[] = {
		{"help",           no_argument,       NULL, 'h'},
		{"duration",       required_argument, NULL, 'd'},
		{"num-of-threads", required_argument, NULL, 'n'},
		{"initial-size",   required_argument, NULL, 'i'},
		{"range",          required_argument, NULL, 'r'},
		{0,                0,                 0,    0  }
	};

	int i, c;
	int duration = DEFAULT_DURATION;
	int nr_threads = DEFAULT_NTHREADS;
	int init_size = DEFAULT_ISIZE;
	int value_range = DEFAULT_VRANGE;
	barrier_t barrier;
	pthread_attr_t attr;
	pthread_t *threads;
	pthread_data_t **data;
	struct timeval start, end;
	struct timespec timeout;
	unsigned long nr_move, nr_txn;
	void *list;

	#ifdef FFWD
		ffwd_init();
		launch_servers(1);
	#endif

	stop = 0;

	while (1) {
		c = getopt_long(argc, argv, "hd:n:i:r:", bench_options, &i);

		if (c == -1)
			break;

		if (c == 0 && bench_options[i].flag == 0)
			c = bench_options[i].val;

		switch(c) {
		case 'h':
			print_args();
			goto out;
		case 'd':
			duration = atoi(optarg);
			break;
		case 'n':
			nr_threads = atoi(optarg);
			break;
		case 'i':
			init_size = atoi(optarg);
			break;
		case 'r':
			value_range = atoi(optarg);
			break;
		default:
			printf("Error while processing options.\n");
			goto out;
		}
	}

	if (duration <= 0) {
		printf("invalid test time\n");
		goto out;
	}
	if (nr_threads <= 0) {
		printf("invalid thread number\n");
		goto out;
	}
	if (init_size + init_size > value_range) {
		printf("list initial size should not be larger than half value range\n");
		goto out;
	}

	printf("List move benchmark\n");
	printf("Test time:     %d\n", duration);
	printf("Thread number: %d\n", nr_threads);
	printf("Initial size:  %d\n", init_size);
	printf("Value range:   %d\n", value_range);

	timeout.tv_sec = duration / 1000;
	timeout.tv_nsec = (duration % 1000) * 1000000;

	if ((threads = (pthread_t *)malloc(nr_threads * sizeof(pthread_t))) == NULL) {
		printf("failed to malloc pthread_t\n");
		goto out;
	}
	if ((data = (pthread_data_t **)malloc(nr_threads * sizeof(pthread_data_t *))) == NULL) {
		printf("failed to malloc pthread_data_t\n");
		goto out;
	}
	for (i = 0; i < nr_threads; i++) {
		if ((data[i] = alloc_pthread_data()) == NULL) {
			printf("failed to malloc pthread_data_t %d\n", i);
			goto out;
		}
	}

	srand(time(0));
	// global init
	if ((list = list_global_init(init_size, value_range)) == NULL) {
		printf("failed to do list_global_init\n");
		goto out;
	}

	barrier_init(&barrier, nr_threads + 1);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	for (i = 0; i < nr_threads; i++) {
		data[i]->id = i;
		data[i]->nr_move = 0;
		data[i]->nr_txn = 0;
		data[i]->range = value_range;
		data[i]->seed = rand();
		data[i]->barrier = &barrier;
		data[i]->list = list;
		if (list_thread_init(data[i], data, nr_threads)) {
			printf("failed to do list_thread_init\n");
			goto out;
		}
		#ifdef FFWD
			ffwd_thread_create(&threads[i],0, bench_thread,(void *)(data[i]));
		#else
			if (pthread_create(&threads[i], &attr, bench_thread, (void *)(data[i])) != 0) {
				printf("failed to create thread %d\n", i);
				goto out;
			}
		#endif
	}
	pthread_attr_destroy(&attr);

	printf("STARTING THREADS...\n");
	barrier_cross(&barrier);

	gettimeofday(&start, NULL);
	nanosleep(&timeout, NULL);
	stop = 1;
	gettimeofday(&end, NULL);
	printf("STOPPING THREADS...\n");


	for (i = 0; i < nr_threads; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			printf("failed to join child thread %d\n", i);
			goto out;
		}
	}

	duration = (end.tv_sec * 1000 + end.tv_usec / 1000) -
	           (start.tv_sec * 1000 + start.tv_usec / 1000);
	nr_move = 0;
	nr_txn = 0;
	for (i = 0;  i < nr_threads; i++) {
		printf("Thread %d: move %lu\n", i, data[i]->nr_move);
		nr_move += (data[i]->nr_move);
		nr_txn += (data[i]->nr_txn);
	}

	printf("List benchmark ends:\n");
	printf("duration: %d ms\n", duration);
	printf("  ops:      %lu (%f/s)\n", nr_move, (nr_move) * 1000.0 / duration);
	printf("  txns:     %lu (%f/s)\n", nr_txn, (nr_txn) * 1000.0 / duration);

	for (i = 0; i < nr_threads; i++)
		free_pthread_data(data[i]);
	list_global_exit(list);
	free(data);
	free(threads);
	#ifdef FFWD
		ffwd_shutdown();
	#endif

out:
	return 0;
}
