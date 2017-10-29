#include <numa.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include "macro.h"

#define MAX_THREADS 128
#define MAX_SOCK 4
#define MAX_NUM_OF_SERVERS 4
#define THREADS_PER_RESPONSE NCLIENTS 
#define INDEX_DIFF 2
#define ATOMIC_INC __asm__ __volatile__ ("lock xaddl %0, %1 \n\t" \
			 					: "=r" (val), "=m" (atomic_variable) \
             					: "0" (val), "m" (atomic_variable) \
              					: "memory"); \

struct server_response
{
	uint64_t return_values[THREADS_PER_RESPONSE];
	uint64_t flags;

} __attribute__((packed));

struct server_set{
	struct server_response* server_responses[MAX_SOCK * 4];
};

struct request{
	uint64_t flag;
	uint32_t argc;
	uint64_t (*fptr)(int);
	uint64_t argv[5];
	char make_it_64_bytes[4];
} __attribute__((packed));

struct ffwd_context {
	int id;
	int dead;
	int id_in_chip;
	uint64_t mask;
	void *(* initfunc)(void*);
	void *initvalue;
	uint64_t local_client_flag[MAX_NUM_OF_SERVERS];
	struct request* request[MAX_NUM_OF_SERVERS];
	volatile struct server_response* server_response[MAX_NUM_OF_SERVERS];
};

struct server_args{
	int server_core;
	struct request* chip0;
	struct request* chip1;
	struct request* chip2;
	struct request* chip3;
	struct server_set* server_response;
};

#define SERVER_CODE_IMP(id_in_chip, request, socket_id, flag_id) \
		if(((old_client_flags##socket_id ) ^ (request[id_in_chip].flag)) & ((uint64_t)1 << id_in_chip)){ \
		__asm__ __volatile__ (	"movq 	%4, %%r8 \n\t" \
								"movq 	%5, %%rcx \n\t" \
								"movq 	%6, %%rdx \n\t" \
								"movq 	%7, %%rsi \n\t" \
								"movq 	%8, %%rdi \n\t" \
								"call 	*%2 \n\t" \
								"movq 	%%rax, %0 \n\t" \
								"movq 	$1, %%rax \n\t" \
								"movl 	$"#id_in_chip", %%ecx \n\t" \
								"shl 	%%cl, %%rax \n\t" \
								"xor 	%%rax, %1 \n\t" \
								: "=m" (local_return_values##flag_id[(id_in_chip) % NCLIENTS]), \
								  "+r" (socket_client_flags##socket_id) \
								: "m"(request[id_in_chip].fptr), \
								  "m" (request[id_in_chip].argc), \
								  "m" (request[id_in_chip].argv[4]), \
								  "m" (request[id_in_chip].argv[3]), \
								  "m" (request[id_in_chip].argv[2]), \
								  "m" (request[id_in_chip].argv[1]), \
								  "m" (request[id_in_chip].argv[0]) \
								: "memory", "rax", "rdi", "rsi", "r8", "r9", "r10", "r11", "rcx", "rdx"); \
} \

#define READ_SPEC \
	"NODES=/sys/devices/system/node; \
	for F in `ls -d $NODES/node*`; do \
		for i in $(cat $F/cpulist | sed \"s/,/ /g\"); do \
			seq -w -s ' ' $(echo $i | tr -s '-' ' '); \
		done; \
	done;"

#define FIND_PLATFORM \
	"if cat /proc/cpuinfo | grep -q 'Opteron' ; then \
		echo Opteron; \
	else \
		if cat /proc/cpuinfo | grep -q 'Xeon' ; then \
			echo Xeon; \
		else \
			echo not supported; \
		fi; \
	fi;"

extern inline void prepare_request(struct request* myrequest, int arg_count, ...);

#define FFWD_EXEC(server_no, function, ret, ...) \
 	context->request[server_no]->fptr = function; \
 	prepare_request(context->request[server_no], __VA_ARGS__); \
 	context->local_client_flag[server_no] ^= context->mask; \
 	context->request[server_no]->flag = context->local_client_flag[server_no]; \
 	while(((context->server_response[server_no]->flags ^ context->local_client_flag[server_no]) & context->mask)){ \
 		__asm__ __volatile__("rep;nop": : :"memory"); \
 	} \
 	ret = context->server_response[server_no]->return_values[((context->id_in_chip)) % NCLIENTS]; \

#define GET_CONTEXT() \
	struct ffwd_context *context = ffwd_get_context();

extern int   num_sockets;
extern int   active_threads_per_socket;
extern int   cores_per_socket;
extern int   num_threads;
extern char* platform;
extern volatile int num_of_server_launched;

void ffwd_init();
void launch_server(int);
void ffwd_shutdown();
struct ffwd_context* ffwd_get_context();
void ffwd_thread_create(pthread_t *thread, pthread_attr_t *client_attr, void *(* func) (void *),void* value);
void move_to_core(int core_id);

