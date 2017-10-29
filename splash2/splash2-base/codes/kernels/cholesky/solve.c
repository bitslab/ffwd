
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

/*************************************************************************/
/*                                                                       */
/*  Sparse Cholesky Factorization (Fan-Out with no block copy-across)    */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*  -pP : P = number of processors.                                      */
/*  -Bb : Use a postpass partition size of b.                            */
/*  -Cc : Cache size in bytes.                                           */
/*  -s  : Print individual processor timing statistics.                  */
/*  -t  : Test output.                                                   */
/*  -h  : Print out command line options.                                */
/*                                                                       */
/*  Note: This version works under both the FORK and SPROC models        */
/*                                                                       */
/*************************************************************************/


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


#include <math.h>
#include "matrix.h"

#define SH_MEM_AMT   100000000
#define DEFAULT_PPS         32
#define DEFAULT_CS       16384
#define DEFAULT_P            1

double CacheSize = DEFAULT_CS;
double CS;
long BS = 45;

struct GlobalMemory *Global;

long *T, *nz, *node, *domain, *domains, *proc_domains;

long *PERM, *INVP;

long solution_method = FAN_OUT*10+0;

long distribute = -1;

long target_partition_size = 0;
long postpass_partition_size = DEFAULT_PPS;
long permutation_method = 1;
long join = 1; /* attempt to amalgamate supernodes */
long scatter_decomposition = 0;

long P=DEFAULT_P;
long iters = 1;
SMatrix M;      /* input matrix */

char probname[80];

extern struct Update *freeUpdate[MAX_PROC];
extern struct Task *freeTask[MAX_PROC];
extern long *firstchild, *child;
extern BMatrix LB;
extern char *optarg;

struct gpid {
  long pid;
  unsigned long initdone;
  unsigned long finish;
} *gp;

long do_test = 0;
long do_stats = 0;

int main(int argc, char *argv[])
{
  double *b, *x;
  double norm;
  long i;
  long c;
  long *assigned_ops, num_nz, num_domain, num_alloc, ps;
  long *PERM2;
  extern double *work_tree;
  extern long *partition;
  unsigned long start;
  double mint, maxt, avgt;

  {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(start) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
}

  while ((c = getopt(argc, argv, "B:C:p:D:sth")) != -1) {
    switch(c) {
    case 'B': postpass_partition_size = atoi(optarg); break;  
    case 'C': CacheSize = (double) atoi(optarg); break;  
    case 'p': P = atol(optarg); break;  
    case 's': do_stats = 1; break;  
    case 't': do_test = 1; break;  
    case 'h': printf("Usage: CHOLESKY <options> file\n\n");
              printf("options:\n");
              printf("  -Bb : Use a postpass partition size of b.\n");
              printf("  -Cc : Cache size in bytes.\n");
              printf("  -pP : P = number of processors.\n");
              printf("  -s  : Print individual processor timing statistics.\n");
              printf("  -t  : Test output.\n");
              printf("  -h  : Print out command line options.\n\n");
              printf("Default: CHOLESKY -p%1d -B%1d -C%1d\n",
                     DEFAULT_P,DEFAULT_PPS,DEFAULT_CS);
              exit(0);
              break;
    }
  }

  CS = CacheSize / 8.0;
  CS = sqrt(CS);
  BS = (long) floor(CS+0.5);

  {;}

  gp = (struct gpid *) valloc(sizeof(struct gpid));;
  gp->pid = 0;
  Global = (struct GlobalMemory *)
    valloc(sizeof(struct GlobalMemory));;
  {pthread_barrier_init(&(Global->start), NULL, P);}
  pthread_mutex_init(&(Global->waitLock), NULL);
  pthread_mutex_init(&(Global->memLock), NULL);

  MallocInit(P);  

  i = 0;
  while (++i < argc && argv[i][0] == '-')
    ;
  M = ReadSparse(argv[i], probname);

  distribute = LB_DOMAINS*10 + EMBED;

  printf("\n");
  printf("Sparse Cholesky Factorization\n");
  printf("     Problem: %s\n",probname);
  printf("     %ld Processors\n",P);
  printf("     Postpass partition size: %ld\n",postpass_partition_size);
  printf("     %0.0f byte cache\n",CacheSize);
  printf("\n");
  printf("\n");

  printf("true partitions\n");

  printf("Fan-out, ");
  printf("no block copy-across\n");

  printf("LB domain, "); 
  printf("embedded ");
  printf("distribution\n");

  printf("No ordering\n");

  PERM = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  INVP = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);

  CreatePermutation(M.n, PERM, NO_PERM);

  InvertPerm(M.n, PERM, INVP);

  T = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  EliminationTreeFromA(M, T, PERM, INVP);

  firstchild = (long *) MyMalloc((M.n+2)*sizeof(long), DISTRIBUTED);
  child = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  ParentToChild(T, M.n, firstchild, child);

  nz = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  ComputeNZ(M, T, nz, PERM, INVP);

  work_tree = (double *) MyMalloc((M.n+1)*sizeof(double), DISTRIBUTED);
  ComputeWorkTree(M, nz, work_tree);

  node = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  FindSupernodes(M, T, nz, node);

  Amalgamate2(1, M, T, nz, node, (long *) NULL, 1);


  assigned_ops = (long *) malloc(P*sizeof(long));
  domain = (long *) MyMalloc(M.n*sizeof(long), DISTRIBUTED);
  domains = (long *) MyMalloc(M.n*sizeof(long), DISTRIBUTED);
  proc_domains = (long *) MyMalloc((P+1)*sizeof(long), DISTRIBUTED);
  printf("before partition\n");
  fflush(stdout);
  Partition(M, P, T, assigned_ops, domain, domains, proc_domains);
  free(assigned_ops);

  {
    long i, tot_domain_updates, tail_length;

    tot_domain_updates = 0;
    for (i=0; i<proc_domains[P]; i++) {
      tail_length = nz[domains[i]]-1;
      tot_domain_updates += tail_length*(tail_length+1)/2;
    }
    printf("%ld total domain updates\n", tot_domain_updates);
  }

  num_nz = num_domain = 0;
  for (i=0; i<M.n; i++) {
    num_nz += nz[i];
    if (domain[i])
      num_domain += nz[i];
  }
  
  ComputeTargetBlockSize(M, P);

  printf("Target partition size %ld, postpass size %ld\n",
	 target_partition_size, postpass_partition_size);

  NoSegments(M);

  PERM2 = (long *) malloc((M.n+1)*sizeof(long));
  CreatePermutation(M.n, PERM2, permutation_method);
  ComposePerm(PERM, PERM2, M.n);
  free(PERM2);

  InvertPerm(M.n, PERM, INVP);

  b = CreateVector(M);

  ps = postpass_partition_size;
  num_alloc = num_domain + (num_nz-num_domain)*10/ps/ps;
  CreateBlockedMatrix2(M, num_alloc, T, firstchild, child, PERM, INVP,
		       domain, partition);

  FillInStructure(M, firstchild, child, PERM, INVP);

  AssignBlocksNow();

  AllocateNZ();

  FillInNZ(M, PERM, INVP);
  FreeMatrix(M);

  InitTaskQueues(P);

  PreAllocate1FO();
  ComputeRemainingFO();
  ComputeReceivedFO();

  {
	long	i, Error;

	for (i = 0; i < (P) - 1; i++) {
		Error = pthread_create(&PThreadTable[i], NULL, (void * (*)(void *))(Go), NULL);
		if (Error != 0) {
			printf("Error in pthread_create().\n");
			exit(-1);
		}
	}

	Go();
};
  {
	long	i, Error;
	for (i = 0; i < (P) - 1; i++) {
		Error = pthread_join(PThreadTable[i], NULL);
		if (Error != 0) {
			printf("Error in pthread_join().\n");
			exit(-1);
		}
	}
};

  printf("%.0f operations for factorization\n", work_tree[M.n]);

  printf("\n");
  printf("                            PROCESS STATISTICS\n");
  printf("              Total\n");
  printf(" Proc         Time \n");
  printf("    0    %10.0ld\n", Global->runtime[0]);
  if (do_stats) {
    maxt = avgt = mint = Global->runtime[0];
    for (i=1; i<P; i++) {
      if (Global->runtime[i] > maxt) {
        maxt = Global->runtime[i];
      }
      if (Global->runtime[i] < mint) {
        mint = Global->runtime[i];
      }
      avgt += Global->runtime[i];
    }
    avgt = avgt / P;
    for (i=1; i<P; i++) {
      printf("  %3ld    %10ld\n",i,Global->runtime[i]);
    }
    printf("  Avg    %10.0f\n",avgt);
    printf("  Min    %10.0f\n",mint);
    printf("  Max    %10.0f\n",maxt);
    printf("\n");
  }

  printf("                            TIMING INFORMATION\n");
  printf("Start time                        : %16lu\n",
          start);
  printf("Initialization finish time        : %16lu\n",
          gp->initdone);
  printf("Overall finish time               : %16lu\n",
          gp->finish);
  printf("Total time with initialization    : %16lu\n",
          gp->finish-start);
  printf("Total time without initialization : %16lu\n",
          gp->finish-gp->initdone);
  printf("\n");

  if (do_test) {
    printf("                             TESTING RESULTS\n");
    x = TriBSolve(LB, b, PERM);
    norm = ComputeNorm(x, LB.n);
    if (norm >= 0.0001) {
      printf("Max error is %10.9f\n", norm);
    } else {
      printf("PASSED\n");
    }
  }

  {exit(0);}
}


void Go()
{
  long MyNum;
  struct LocalCopies *lc;

  {pthread_mutex_lock(&(Global->waitLock));}
    MyNum = gp->pid;
    gp->pid++;
  {pthread_mutex_unlock(&(Global->waitLock));}

  {;};
/* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
   processors to avoid migration */

  lc =(struct LocalCopies *) valloc(sizeof(struct LocalCopies)+2*PAGE_SIZE);
  lc->freeUpdate = NULL;
  lc->freeTask = NULL;
  lc->runtime = 0;

  PreAllocateFO(MyNum,lc);

    /* initialize - put original non-zeroes in L */

  PreProcessFO(MyNum);

  {
	pthread_barrier_wait(&(Global->start));
};

/* POSSIBLE ENHANCEMENT:  Here is where one might reset the
   statistics that one is measuring about the parallel execution */

  if ((MyNum == 0) || (do_stats)) {
    {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(lc->rs) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
  }

  BNumericSolveFO(MyNum,lc);

  {
	pthread_barrier_wait(&(Global->start));
};

  if ((MyNum == 0) || (do_stats)) {
    {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(lc->rf) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
    lc->runtime += (lc->rf-lc->rs);
  }

  if (MyNum == 0) {
    CheckRemaining();
    CheckReceived();
  }

  {
	pthread_barrier_wait(&(Global->start));
};

  if ((MyNum == 0) || (do_stats)) {
    Global->runtime[MyNum] = lc->runtime;
  }
  if (MyNum == 0) {
    gp->initdone = lc->rs;
    gp->finish = lc->rf;
  } 

  {
	pthread_barrier_wait(&(Global->start));
};

}


void PlaceDomains(long P)
{
  long p, d, first;
  char *range_start, *range_end;

  for (p=P-1; p>=0; p--)
    for (d=LB.proc_domains[p]; d<LB.proc_domains[p+1]; d++) {
      first = LB.domains[d];
      while (firstchild[first] != firstchild[first+1])
        first = child[firstchild[first]];

      /* place indices */
      range_start = (char *) &LB.row[LB.col[first]];
      range_end = (char *) &LB.row[LB.col[LB.domains[d]+1]];
      MigrateMem(&LB.row[LB.col[first]],
		 (LB.col[LB.domains[d]+1]-LB.col[first])*sizeof(long),
		 p);

      /* place non-zeroes */
      range_start = (char *) &BLOCK(LB.col[first]);
      range_end = (char *) &BLOCK(LB.col[LB.domains[d]+1]);
      MigrateMem(&BLOCK(LB.col[first]),
		 (LB.col[LB.domains[d]+1]-LB.col[first])*sizeof(double),
		 p);
    }
}



/* Compute result of first doing PERM1, then PERM2 (placed back in PERM1) */

void ComposePerm(long *PERM1, long *PERM2, long n)
{
  long i, *PERM3;

  PERM3 = (long *) malloc((n+1)*sizeof(long));

  for (i=0; i<n; i++)
    PERM3[i] = PERM1[PERM2[i]];

  for (i=0; i<n; i++)
    PERM1[i] = PERM3[i];

  free(PERM3);
}

