#include "macro.h"

#ifdef FRAMES
	#define CORES_PER_SOCKET 8
	#ifdef ALL
		#define THREADS_PER_SOCKET 14
	#else
		#define THREADS_PER_SOCKET 7
	#endif

#elif NODES
	#define CORES_PER_SOCKET 16
	#ifdef ALL
		#define THREADS_PER_SOCKET 30
	#elif YIELD
		#define THREADS_PER_SOCKET 60
	#else
		#define THREADS_PER_SOCKET 15
	#endif
#endif

#define LABEL_MAX 100

#define CREATE_LABELS0(i, ...) CREATE_LABELS_IMP0(i)
#define CREATE_LABELS_IMP0(i) extern const char after_call_0_##i[]; \
							  extern const char return_from_yield_0_##i[];

#define CREATE_LABELS1(i, ...) CREATE_LABELS_IMP1(i)
#define CREATE_LABELS_IMP1(i) extern const char after_call_1_##i[]; \
							  extern const char return_from_yield_1_##i[];

#define CREATE_LABELS2(i, ...) CREATE_LABELS_IMP2(i)
#define CREATE_LABELS_IMP2(i) extern const char after_call_2_##i[]; \
							  extern const char return_from_yield_2_##i[];

#define CREATE_LABELS3(i, ...) CREATE_LABELS_IMP3(i)
#define CREATE_LABELS_IMP3(i) extern const char after_call_3_##i[]; \
							  extern const char return_from_yield_3_##i[];

extern const char yield_label[];

EVAL(REPEAT(THREADS_PER_SOCKET, UNROLL, CREATE_LABELS0, ~))
EVAL(REPEAT(THREADS_PER_SOCKET, UNROLL, CREATE_LABELS1, ~))
EVAL(REPEAT(THREADS_PER_SOCKET, UNROLL, CREATE_LABELS2, ~))
EVAL(REPEAT(THREADS_PER_SOCKET, UNROLL, CREATE_LABELS3, ~))

extern volatile char* current_client_label;
