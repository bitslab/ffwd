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

#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ulocks.h>
#include <sys/time.h>
#include <sys/sysmp.h>

static char *shmfile = 0;
static int shmid;
static void *shmvaddr;
static void *ap;

void
anl_shmem_init(size_t shmsize)
{
	key_t shmkey;
	usptr_t *usptr;

	shmkey = (key_t)getpid();
	shmkey = (shmkey<<16)|shmkey;

	if((shmid=shmget(shmkey, shmsize, IPC_CREAT|SHM_R|SHM_W|044)) == -1){
		perror("shmget");
		exit(1);
	}

	/*
	 * The segment needs to be attached only once; all
	 * attached shared memory segments are inherited by 
	 * forked off processes.
	 */

	if((shmvaddr=shmat(shmid, (void *)0, 0)) == (void *)(-1)){
		perror("shmat");
		exit(1);
	}

	shmfile = mktemp("/tmp/shmXXXXXX");
	if((usptr=usinit(shmfile)) == NULL){
		perror("usinit");
		exit(1);
	}

	if((ap=acreate(shmvaddr, shmsize, MEM_SHARED, usptr, NULL)) == NULL){
		perror("acreate");
		exit(1);
	}

	printf("shmid 0x%x shmvaddr 0x%x\n", shmid, shmvaddr);
}

void *
anl_malloc(size_t size)
{
	return( amalloc(size, ap) );
}

void
anl_free(void *ptr)
{
	afree(ptr, ap);
}

void
anl_shmem_end()
{
	adelete(ap);
	shmctl(shmid, IPC_RMID);
	unlink(shmfile);
}

