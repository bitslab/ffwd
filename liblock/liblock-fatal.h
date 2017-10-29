/* ########################################################################## */
/* (C) UPMC, 2010-2011                                                        */
/*     Authors:                                                               */
/*       Jean-Pierre Lozi <jean-pierre.lozi@lip6.fr>                          */
/*       Gaël Thomas <gael.thomas@lip6.fr>                                    */
/*       Florian David <florian.david@lip6.fr>                                */
/*       Julia Lawall <julia.lawall@lip6.fr>                                  */
/*       Gilles Muller <gilles.muller@lip6.fr>                                */
/* -------------------------------------------------------------------------- */
/* ########################################################################## */
#ifndef _LOCKLIB_FATAL_H_
#define _LOCKLIB_FATAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define echo(msg, ...) do {											\
		fprintf(stderr, "%-15s", "["msg"]: ");	\
		fprintf(stderr, __VA_ARGS__);								\
		fprintf(stderr, "\n");											\
	} while(0)

#define warning(...) echo("warning", __VA_ARGS__)

#define fatal(...) do {																									\
		echo("error", __VA_ARGS__);																					\
		fprintf(stderr, "   at %s::%d (%s)\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
		abort();																														\
	} while(0)

#endif
