
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#include <float.h>
#include "defs.h"
#include "memory.h"


#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
/** +EDIT */
//#define MAX_THREADS 32
#define MAX_THREADS 48
/** -EDIT */
pthread_t PThreadTable[MAX_THREADS];


g_mem *G_Memory;
local_memory Local[MAX_PROCS];

/*
 *  InitGlobalMemory ()
 *
 *  Args : none.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Allocates all the global storage for G_Memory.
 *
 */
void
InitGlobalMemory ()
{
   G_Memory = (g_mem *) valloc(sizeof(g_mem));;
   G_Memory->i_array = (long *) valloc(Number_Of_Processors * sizeof(long));;
   G_Memory->d_array = (double *) valloc(Number_Of_Processors * sizeof(double));;
   if (G_Memory == NULL) {
      printf("Ran out of global memory in InitGlobalMemory\n");
      exit(-1);
   }
   G_Memory->count = 0;
   G_Memory->id = 0;
   pthread_mutex_init(&(G_Memory->io_lock), NULL);;
   pthread_mutex_init(&(G_Memory->mal_lock), NULL);;
   pthread_mutex_init(&(G_Memory->single_lock), NULL);;
   pthread_mutex_init(&(G_Memory->count_lock), NULL);;
   
{
	unsigned long	i, Error;

	for (i = 0; i < MAX_LOCKS; i++) {
		Error = pthread_mutex_init(&G_Memory->lock_array[i], NULL);
		if (Error != 0) {
			printf("Error while initializing array of locks.\n");
			exit(-1);
		}
	}
}
;
   {pthread_barrier_init(&(G_Memory->synch), NULL, Number_Of_Processors);};
   G_Memory->max_x = -MAX_REAL;
   G_Memory->min_x = MAX_REAL;
   G_Memory->max_y = -MAX_REAL;
   G_Memory->min_y = MAX_REAL;
}


