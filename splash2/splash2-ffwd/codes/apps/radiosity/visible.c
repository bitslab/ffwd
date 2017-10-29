
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

/**************************************************************
 *
 *	Visibility testing
 *
 *
 ***************************************************************/

#include	<stdio.h>
#include        <math.h>


#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include "ffwd.h"
extern pthread_t PThreadTable[];
;


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


/*  This file contains many constant definitions that control the execution
of the program, as well as lobal data structure declarations */

#ifndef _RADIOSITY_H
#define _RADIOSITY_H

#include <math.h>

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

/**************************************************************
*
*       Definitions relevant to parallel processing
*
***************************************************************/

#ifndef _PARALLEL_H
#define _PARALLEL_H



/***************************************************************************
*
*    Shared lock variable
*
*    Some machines provide only a limited number of lock variables. This
*    data structure allows sharing of these lock variables.
*    The shared locks are divided into 2 segments so that different types of
*    objects are given different locks.
*
****************************************************************************/

typedef struct
{
    pthread_mutex_t lock;
} Shared_Lock ;

#define SHARED_LOCK_SEG_SIZE (MAX_SHARED_LOCK / 2)

#define SHARED_LOCK_SEG0 (0)
#define SHARED_LOCK_SEG1 (1)
#define SHARED_LOCK_SEGANY (2)

/****************************************************************************
*
*    Memory Consistency Model of the machine
*
*    Some macro changes its behavior based on the memory consistency model
*
*
*****************************************************************************/

/* Set one(1) to the model used in the machine.  Set only one of these
at a time */

#define MEM_CONSISTENCY_RELEASE    (0)
#define MEM_CONSISTENCY_WEAK       (0)
#define MEM_CONSISTENCY_PROCESSOR  (1)

#endif



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


#ifndef _PATCH_H
#define _PATCH_H

#include "structs.H"

/************************************************************************
*
*     Constants
*
*************************************************************************/

#define F_COPLANAR  (5.0e-2)     /* H(P) < F_COPLANAR then P is on the plane */
#define N_VISIBILITY_TEST_RAYS  (10)	/* number of "random", "magic" rays fired
between patches to test visibility */

#define FF_GEOMETRY_ERROR (1.0)		/* FF relative error due to Fdf approx
and cosine approx of angle */
#define FF_GEOMETRY_VARIANCE (1.0)	/* FF relative varance with in elem */
#define FF_VISIBILITY_ERROR (1.0 / N_VISIBILITY_TEST_RAYS)



/************************************************************************
*
*     Intersection code
*
*************************************************************************/

#define POINT_POSITIVE_SIDE   (1)
#define POINT_NEGATIVE_SIDE   (2)
#define POINT_ON_PLANE        (0)

#define P1_POSITIVE    (1)
#define P1_NEGATIVE    (2)
#define P2_POSITIVE    (4)
#define P2_NEGATIVE    (8)
#define P3_POSITIVE    (16)
#define P3_NEGATIVE    (32)
#define ANY_POSITIVE   (P1_POSITIVE | P2_POSITIVE | P3_POSITIVE)
#define ANY_NEGATIVE   (P1_NEGATIVE | P2_NEGATIVE | P3_NEGATIVE)
#define POSITIVE_SIDE(code) (((code) & ANY_NEGATIVE) == 0)
#define NEGATIVE_SIDE(code) (((code) & ANY_POSITIVE) == 0)
#define INTERSECTING(code)  (   ((code) & ANY_NEGATIVE) \
&& ((code) & ANY_POSITIVE) )
#define P1_CODE(code)  (code & 3)
#define P2_CODE(code)  ((code >> 2) & 3)
#define P3_CODE(code)  ((code >> 4) & 3)

/************************************************************************
*
*     Visibility Testing
*
*************************************************************************/

#define      VISIBILITY_UNDEF      ((float)-1.0)
#define      PATCH_CACHE_SIZE      (2)        /* The first two cache entries
covers about 95% of the total cache hits, so using
more doesn't help too much. */

/************************************************************************
*
*     Refinement Advice
*
*************************************************************************/

#define _NO_INTERACTION          (1)
#define _NO_REFINEMENT_NECESSARY (2)
#define _REFINE_PATCH_1          (4)
#define _REFINE_PATCH_2          (8)
#define _NO_VISIBILITY_NECESSARY (16)

#define NO_INTERACTION(c)          ((c) & _NO_INTERACTION)
#define NO_REFINEMENT_NECESSARY(c) ((c) & _NO_REFINEMENT_NECESSARY)
#define REFINE_PATCH_1(c)          ((c) & _REFINE_PATCH_1)
#define REFINE_PATCH_2(c)          ((c) & _REFINE_PATCH_2)
#define NO_VISIBILITY_NECESSARY(c) ((c) & _NO_VISIBILITY_NECESSARY)


/************************************************************************
*
*     Element Vertex
*
*     ElementVertex represents a vertex of an element. A vertex structure
*     is shared by those elements which contain the vertex as part of their
*     vertex list.
*
*************************************************************************/

typedef struct _elemvertex {
    Vertex p ;			  /* Coordinate of the vertex */
    Rgb    col ;			  /* Color of the vertex */
    float  weight ;			  /* weight */
    Shared_Lock *ev_lock ;
} ElemVertex ;


#define N_ELEMVERTEX_ALLOCATE (16)

/************************************************************************
*
*     Edge
*
*     Edge represents each edge of the element. Two adjacent elements
*     share the same edge. As an element is subdivided, the edge is also
*     subdivided. The edges form a binary tree, which can be viewed as a
*     projection of the element subdivision along an edge of the element.
*     In other words, the edge structure binds elements at the same height.
*     Note that the vertices may appear in reverse order in the edge structure
*     with respect to the order in the patch/element definition.
*
*************************************************************************/

typedef struct _edge {
    ElemVertex   *pa, *pb ;
    struct _edge *ea, *eb ;		  /* Edge (A-center) and (center-B) */
    Shared_Lock  *edge_lock ;	          /* Use segment0 */
} Edge ;


#define N_EDGE_ALLOCATE (16)

#define _LEAF_EDGE(e) ((e)->ea == 0)
#define EDGE_REVERSE(e,a,b) ((e)->pa == (b))


/************************************************************************
*
*     Planar equation
*
*     Plane equation (in implicit form) of the triangle patch.
*     A point P on the plane satisfies
*         (N.P) + C = 0
*     where N is the normal vector of the patch, C is a constant which
*     is the distance of the plane from the origin scaled by -|N|.
*
*************************************************************************/

typedef struct {
    Vertex  n ;		          /* Normal vector (normalized) */
    float  c ;			  /* Constant */
    /* Nx*x + Ny*y + Nz*z + C = 0 */
} PlaneEqu ;


/************************************************************************
*
*     Patch (also a node of the BSP tree)
*
*     The Patch represents a triangular patch (input polygon) of the given
*     geometric model (i.e., room scene). The Patch contains 'per-patch'
*     information such as the plane equation, area, and color. The Patch also
*     serves as a node of the BSP tree which is used to test patch-patch
*     visibility. The Patch points to the root level of the element quad-tree.
*     Geometrically speaking, the Patch and the root represent the same
*     triangle.
*     Although coordinates of the vertices are given by the Edge structure,
*     copies are stored in the Patch to allow fast access to the coordinates
*     during the visibility test.
*     For cost based task distribution another structure, Patch_Cost, is
*     also used. This structure is made separate from the Patch structure
*     since gathering cost statistics is a frequently read/write operation.
*     If it were in the Patch structure, updating a cost would result in
*     invalidation of the Patch structure and cause cache misses during
*     BSP traversal.
*
*************************************************************************/

struct _element ;

typedef struct _patch {
    ElemVertex *ev1, *ev2, *ev3 ;	  /* ElemVertecies of the patch */
    Edge    *e12, *e23, *e31 ;          /* Edges of the patch */
    Vertex   p1, p2, p3 ;		  /* Vertices of the patch */
    PlaneEqu plane_equ ;		  /* Plane equation H(x,y,z) */
    float    area ;			  /* Area of the patch */
    Rgb      color ;			  /* Diffuse color of the patch */
    /*       (reflectance) */
    Rgb      emittance ;	          /* Radiant emmitence */

    struct _patch  *bsp_positive ;	  /* BSP tree H(x,y,z) >= 0 */
    struct _patch  *bsp_negative ;	  /*          H(x,y,z) <  0 */
    struct _patch  *bsp_parent ;        /* BSP backpointer to the parent*/

    struct _element *el_root ;	  /* Root of the element tree */
    long      seq_no ;		          /* Patch sequence number */
} Patch ;


typedef struct {
    Patch    *patch ;
    Shared_Lock *cost_lock ;		  /* Cost variable lock */
    long      n_bsp_node ;	          /* Number of BSP nodes visited */
    long      n_total_inter ;	          /* Total number of interactions */
    long      cost_estimate ;            /* Cost estimate */
    long      cost_history[11] ;	  /* Cost history */
} Patch_Cost ;

/* Patch cost:
Visiting a node in BSP tree:  150 cyc (overall)
Gathering ray per interaction: 50 cyc (overall avg) */

#define PATCH_COST(p)          ((p)->n_bsp_node * 3 + (p)->n_total_inter)
#define PATCH_COST_ESTIMATE(p)  ((p)->cost_history[0] \
+ ((p)->cost_history[1] >> 1)\
+ ((p)->cost_history[2] >> 2) )


/************************************************************************
*
*     Element
*
*     The Element represents each node of the quad-tree generated by the
*     hierarchical subdivision. The Element structure consists of:
*      - pointers to maintain the tree structure
*      - a linear list of interacting elements
*      - radiosity value of the element
*      - pointer to the vertex and edge data structures
*
*     To allow smooth radiosity interpolation across elements, an element
*     shares edges and vertices with adjacent elements.
*
*************************************************************************/

struct _interact ;

typedef struct _element {
    Shared_Lock *elem_lock ;	          /* Element lock variable (seg 1) */
    Patch *patch ;			  /* Original patch of the element */

    struct _element *parent ;		  /* Quad tree (parent)          */
    struct _element *center ;		  /*           (center triangle) */
    struct _element *top ;		  /*           (top)             */
    struct _element *left ;		  /*           (left)            */
    struct _element *right ;		  /*           (right)           */

    struct _interact *interactions ;	  /* Top of light interaction list */
    long  n_interactions ;		  /* Total # of interactions */
    struct _interact *vis_undef_inter ; /* Top of visibility undef list */
    long  n_vis_undef_inter ;		  /* # of interactions whose visibility
    is not yet calculated */
    Rgb  rad ;			  /* Radiosity of this element
    (new guess of B) */
    Rgb  rad_in ;			  /* Sum of anscestor's radiosity */
    Rgb  rad_subtree ;		  /* Area weighted sum of subtree's
    radiosity (includes this elem) */
    long  join_counter ;		  /* # of unfinished subprocesses */

    ElemVertex *ev1, *ev2, *ev3 ;	  /* Vertices of the element */
    Edge       *e12, *e23, *e31 ;	  /* Edges of the element */
    float area ;		          /* Area of the element */
} Element ;


#define _LEAF_ELEMENT(e) ((e)->center == 0)

#if MEM_CONSISTENCY_PROCESSOR
#define LEAF_ELEMENT(e)  _LEAF_ELEMENT((e))
#endif

#if (MEM_CONSISTENCY_RELEASE || MEM_CONSISTENCY_WEAK)
extern long leaf_element() ;
#define LEAF_ELEMENT(e) (leaf_element((e)))
#endif


/************************************************************************
*
*     Interaction
*
*************************************************************************/

typedef struct _interact {
    struct _interact *next ;		  /* Next entry of the list */
    Element *destination ;	          /* Partner of the interaction */
    float   formfactor_out ;		  /* Form factor from this patch  */
    float   formfactor_err ;            /* Error of FF */
    float   area_ratio ;		  /* Area(this) / Area(dest) */
    float   visibility ;		  /* Visibility (0 - 1.0) */
} Interaction ;


void foreach_patch_in_bsp(void (*func)(), long arg1, long process_id);
void foreach_depth_sorted_patch(Vertex *sort_vec, void (*func)(), long arg1, long process_id);
void define_patch(Patch *patch, Patch *root, long process_id);
void split_patch(Patch *patch, Patch *node, long xing_code, long process_id);
void attach_element(Patch *patch, long process_id);
void refine_newpatch(Patch *patch, long newpatch, long process_id);
Patch *get_patch(long process_id);
void init_patchlist(long process_id);
void print_patch(Patch *patch, long process_id);
void print_bsp_tree(long process_id);
void _pr_patch(Patch *patch, long dummy, long process_id);
float plane_equ(PlaneEqu *plane, Vertex *point, long process_id);
float comp_plane_equ(PlaneEqu *pln, Vertex *p1, Vertex *p2, Vertex *p3, long process_id);
long point_intersection(PlaneEqu *plane, Vertex *point, long process_id);
long patch_intersection(PlaneEqu *plane, Vertex *p1, Vertex *p2, Vertex *p3, long process_id);
void print_plane_equ(PlaneEqu *peq, long process_id);

#endif


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

/* Header file for model data structures and definitions */

#ifndef _MODEL_H
#define _MODEL_H


/************************************************************************
*
*     Constants
*
*************************************************************************/

#define MODEL_TRIANGLE  (0)
#define MODEL_RECTANGLE (1)
#define MODEL_NULL      (-1)

#define MODEL_TEST_DATA (0)
#define MODEL_ROOM_DATA (1)
#define MODEL_LARGEROOM_DATA (2)


/************************************************************************
*
*     Model descriptor
*
*************************************************************************/

/* General structure of the model descriptor */
typedef struct {
    Rgb   color ;			/* Diffuse color */
    Rgb   emittance ;		        /* Radiant emittance */
    Vertex _dummy[4] ;
} Model ;

/* Triangle */
typedef struct {
    Rgb   color ;			/* Diffuse color */
    Rgb   emittance ;		        /* Radiant emittance */
    Vertex p1, p2, p3 ;
} Model_Triangle ;

typedef Model_Triangle Model_Rectangle ;


typedef struct {
    long type ;
    Model model ;
} ModelDataBase ;

/*
 * modelman.C
 */
void init_modeling_tasks(long process_id);
void process_model(Model *model, long type, long process_id);

extern long model_selector ;

#endif




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


#ifndef _TASK_H
#define _TASK_H


/************************************************************************
*
*     Constants
*
*************************************************************************/

#define PAGE_SIZE 8192   /* page size of system, used for padding to
allow page placement of some logically
per-process data structures */

/*** Task types ***/
#define TASK_MODELING      (1)
#define TASK_BSP           (2)
#define TASK_FF_REFINEMENT (4)
#define TASK_RAY           (8)
#define TASK_RAD_AVERAGE   (16)
#define TASK_VISIBILITY    (32)


/*** Controling parallelism ***/

#define MAX_TASKGET_RETRY (32)	    /* Max # of retry get_task() can make */
#define N_ALLOCATE_LOCAL_TASK (8)   /* get_task() and free_task() transfer
this # of task objects to/from the
global shared queue at a time */


/************************************************************************
*
*     Task Descriptors
*
*************************************************************************/

/* Decompose modeling object into patches (B-reps) */
typedef struct {
    long   type ;		     /* Object type */
    Model *model ;		     /* Object to be decomposed */
} Modeling_Task ;


/* Insert a new patch to the BSP tree */
typedef struct {
    Patch *patch ;                 /* Patch to be inserted */
    Patch *parent ;		     /* Parent node in the BSP tree */
} BSP_Task ;


/* Refine element interaction based on FF value or BF value */
typedef struct {
    Element *e1, *e2 ;	     /* Interacting elements */
    float   visibility ;           /* Visibility of parent */
    long level ;		     /* Path length from the root element */
} Refinement_Task ;


typedef struct {
    long  ray_type ;
    Element *e ;		     /* The element we are interested in */
} Ray_Task ;


typedef struct {
    Element *e ;		     /* The element we are interested in */
    Interaction *inter ;	     /* Top of interactions */
    long   n_inter ;		     /* Number of interactions */
    void  (*k)() ;		     /* Continuation */
} Visibility_Task ;

/* Radiosity averaging task */

#define RAD_AVERAGING_MODE (0)
#define RAD_NORMALIZING_MODE (1)

typedef struct {
    Element *e ;
    long level ;
    long mode ;
} RadAvg_Task ;



/************************************************************************
*
*     Task Definition
*
*************************************************************************/


typedef struct _task {
    long task_type ;
    struct _task *next ;
    union {
        Modeling_Task   model ;
        BSP_Task        bsp ;
        Refinement_Task ref ;
        Ray_Task        ray ;
        Visibility_Task vis ;
        RadAvg_Task     rad ;
    } task ;
} Task ;


typedef struct {
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
    pthread_mutex_t q_lock;
    Task  *top, *tail ;
    long   n_tasks ;
    pthread_mutex_t f_lock;
    long   n_free ;
    Task  *free ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
} Task_Queue ;


#define TASK_APPEND (0)
#define TASK_INSERT (1)

#define taskq_length(q)   (q->n_tasks)
#define taskq_top(q)      (q->top)
#define taskq_too_long(q)  ((q)->n_tasks > n_tasks_per_queue)

/*
 * taskman.C
 */
void process_tasks(long process_id);
long _process_task_wait_loop(void);
void create_modeling_task(Model *model, long type, long process_id);
void create_bsp_task(Patch *patch, Patch *parent, long process_id);
void create_ff_refine_task(Element *e1, Element *e2, long level, long process_id);
void create_ray_task(Element *e, long process_id);
void enqueue_ray_task(long qid, Element *e, long mode, long process_id);
void create_visibility_tasks(Element *e, void (*k)(), long process_id);
void create_radavg_task(Element *e, long mode, long process_id);
void enqueue_radavg_task(long qid, Element *e, long mode, long process_id);
void enqueue_task(long qid, Task *task, long mode);
Task *dequeue_task(long qid, long max_visit, long process_id);
Task *get_task(long process_id);
void free_task(Task *task, long process_id);
void init_taskq(long process_id);
long check_task_counter(void);
long assign_taskq(long process_id);
void print_task(Task *task);
void print_taskq(Task_Queue *tq);

#endif


#include "glib.h"
#include "pslib.h"


/****************************************
*
*    Configuration Parameters
*
*****************************************/

/*************************************************************************
*
*    Task scheduling & Load balancing (1)
*       --- Assignment of the patches to the processors
*
*    This macro specifies how patches are assigned to the task queues (ie,
*    processors).
*    - PATCH_ASSIGNMENT_STATIC assigns the same set of patches to the same
*    queue repeatedly over iterations.
*    - PATCH_ASSIGNMENT_COSTBASED assigns patches to queues based on the
*    work associated with those patches in previous iterations, in order
*    to try to balance the initial workload assignment among processors
*    and hence reduce task stealing.
*
**************************************************************************/

#define PATCH_ASSIGNMENT_STATIC    (1)
#define PATCH_ASSIGNMENT_COSTBASED (3)

#if !defined(PATCH_ASSIGNMENT)
#define PATCH_ASSIGNMENT PATCH_ASSIGNMENT_STATIC
#endif


/****************************************
*
*    Constants
*
*****************************************/


#define F_ZERO  (1.0e-6)

#if defined(SIMULATOR)
#define MAX_PROCESSORS (128)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (128)	      /* Maximum number of task queues */
#define MAX_TASKS    (32768)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (80000)	      /* # of available element objects */
#define MAX_INTERACTIONS (640000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (65536)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#if defined(DASH)
#define MAX_PROCESSORS (64)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (64)	      /* Maximum number of task queues */
#define MAX_TASKS    (32768)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (80000)	      /* # of available element objects */
#define MAX_INTERACTIONS (640000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (65536)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#if defined(SGI_GL)
#define MAX_PROCESSORS (8)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (8)	      /* Maximum number of task queues */
#define MAX_TASKS    (8192)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (40000)	      /* # of available element objects */
#define MAX_INTERACTIONS (320000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (16384)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#if defined(SUN4)
#define MAX_PROCESSORS (1)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (1)	      /* Maximum number of task queues */
#define MAX_TASKS    (1024)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (20000)	      /* # of available element objects */
#define MAX_INTERACTIONS (160000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (16384)     /* # of available ElemVertex objs */
#define MAX_EDGES  (32768)            /* # of available Edge objs */
#endif

#if (!defined(SIMULATOR) && !defined(DASH) && !defined(SGI_GL) && !defined(SUN4))
#define MAX_PROCESSORS (128)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (128)	      /* Maximum number of task queues */
#define MAX_TASKS    (32768)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (80000)	      /* # of available element objects */
#define MAX_INTERACTIONS (640000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (65536)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#define MAX_SHARED_LOCK (3900)	      /* Maximum locks allocated. Objects
share these locks */

#if defined(SGI_GL) || defined(DASH) || defined(SIMULATOR)
#define CLOCK_MAX_VAL (2048*1000000)  /* ANL macro clock max value */
#elif defined(SUN4)
#define CLOCK_MAX_VAL (65536*1000000)  /* ANL macro clock max value */
#else
#define CLOCK_MAX_VAL (2048*1000000)  /* ANL macro clock max value */
#endif



/****************************************
*
*    System defaults
*
*****************************************/

#define DEFAULT_N_PROCESSORS (1)
#define DEFAULT_N_TASKQUEUES (1)
#define DEFAULT_N_TASKS_PER_QUEUE (200)
/* Create new tasks if number of tasks currently
in the queue is less than this number */
#define DEFAULT_N_INTER_PARALLEL_BF_REFINEMENT (5)
/* If the number of interactions is greater than
or equal to this value, BF-refinement is
performed in parallel */
#define DEFAULT_N_VISIBILITY_PER_TASK (4)
/* Number of visibility computations per
visibility task */
#define DEFAULT_AREA_EPSILON (2000.0)
/* If element is smaller than this value,
no further subdivision takes place */
#define DEFAULT_ENERGY_EPSILON (0.005)
/* Terminate radiosity iteration if the
difference of total energy is less than this
value. */
#define DEFAULT_BFEPSILON (0.015)
/* BF refinement threshold level. If the estimated
error of BF (due to FF error and error due to
constant approximation within an element) is
larger than this value, then subdivide */

#define DFLT_VIEW_ROT_X (10.0)
#define DFLT_VIEW_ROT_Y (0.0)
#define DFLT_VIEW_DIST  (8000.0)
#define DFLT_VIEW_ZOOM  (1.0)


/****************************************
*
*    Display mode
*
*****************************************/

#define DISPLAY_FILLED   (0)
#define DISPLAY_SHADED   (1)
#define DISPLAY_EDGEONLY (2)

#define DISPLAY_ALL_INTERACTIONS  (0)
#define DISPLAY_HALF_INTERACTIONS (1)



/****************************************
*
*    Statistical Measure
*
*****************************************/

#define MAX_ITERATION_INFO (16)

struct _element ;

typedef struct
{
    long visibility_comp ;
    long ray_intersect_test ;
    long tasks_from_myq ;
    long tasks_from_otherq ;
    long process_tasks_wait ;
    struct _element *last_pr_task ;
} PerIterationInfo ;


typedef struct
{
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
    long total_modeling_tasks ;
    long total_def_patch_tasks ;
    long total_ff_ref_tasks ;
    long total_ray_tasks ;
    long total_radavg_tasks ;
    long total_direct_radavg_tasks ;
    long total_interaction_comp ;
    long total_visibility_comp ;
    long partially_visible ;
    long total_ray_intersect_test ;
    long total_patch_cache_check ;
    long total_patch_cache_hit ;
    long patch_cache_hit[PATCH_CACHE_SIZE] ;
    PerIterationInfo per_iteration[ MAX_ITERATION_INFO ] ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
} StatisticalInfo ;

/****************************************
*
*    Shared data structure definition.
*
*****************************************/

typedef struct
{
    long rad_start, rad_time, refine_time, wait_time, vertex_time;
} Timing;

typedef struct
{

    /* Task queue */
    /* ***** */ long index;
    /* ***** */	pthread_mutex_t index_lock;
    Task_Queue task_queue[ MAX_TASKQUEUES ] ;
    Task task_buf[ MAX_TASKS ] ;

    /* BSP tree root */
    pthread_mutex_t bsp_tree_lock;
    Patch *bsp_root ;

    /* Average radiosity value */
    pthread_mutex_t avg_radiosity_lock;
    long   converged ;
    Rgb   prev_total_energy ;
    Rgb   total_energy ;
    float total_patch_area ;
    long   iteration_count ;

    /* Computation cost estimate */
    pthread_mutex_t cost_sum_lock;
    long cost_sum ;
    long cost_estimate_sum ;
    Patch_Cost patch_cost[ MAX_PATCHES ] ;

    /* Barrier */
    
pthread_barrier_t	(barrier);


    /* Private varrier */
    long pbar_count ;
    pthread_mutex_t pbar_lock;

    /* Task initializer counter */
    long task_counter ;
    pthread_mutex_t task_counter_lock;

    /* Resource buffers */
    pthread_mutex_t free_patch_lock;
    Patch *free_patch ;
    long   n_total_patches ;
    long   n_free_patches ;
    Patch patch_buf[ MAX_PATCHES ] ;

    pthread_mutex_t free_element_lock;
    Element *free_element ;
    long     n_free_elements ;
    Element element_buf[ MAX_ELEMENTS ] ;

    pthread_mutex_t free_interaction_lock;
    Interaction *free_interaction ;
    long         n_free_interactions ;
    Interaction interaction_buf[ MAX_INTERACTIONS ] ;

    pthread_mutex_t free_elemvertex_lock;
    long         free_elemvertex ;
    ElemVertex  elemvertex_buf[ MAX_ELEMVERTICES ] ;

    pthread_mutex_t free_edge_lock;
    long   free_edge ;
    Edge  edge_buf[ MAX_EDGES ] ;

    Shared_Lock sh_lock[ MAX_SHARED_LOCK ] ;

    StatisticalInfo stat_info[ MAX_PROCESSORS ] ;

} Global ;


/****************************************
*
*    Global variables
*
*****************************************/

extern Timing **timing ;
extern Global *global ;
extern long    n_processors ;
extern long    n_taskqueues ;
extern long    n_tasks_per_queue ;

extern long    N_inter_parallel_bf_refine ;
extern long    N_visibility_per_task ;
extern float  Area_epsilon ;
extern float  Energy_epsilon ;
extern float  BFepsilon ;

extern long batch_mode, verbose_mode ;
extern long taskqueue_id[] ;

extern long time_rad_start, time_rad_end, time_process_start[] ;


/****************************************
*
*    Global function names & types
*
*****************************************/

/*
 * display.C
 */
void radiosity_averaging(Element *elem, long mode, long process_id);
void setup_view(float rot_x, float rot_y, float dist, float zoom, long process_id);
void display_scene(long fill_sw, long patch_sw, long mesh_sw, long interaction_sw, long process_id);
void display_patch(Patch *patch, long mode, long process_id);
void display_patches_in_bsp_tree(long mode, long process_id);
void display_element(Element *element, long mode, long process_id);
void display_elements_in_patch(Patch *patch, long mode, long process_id);
void display_elements_in_bsp_tree(long mode, long process_id);
void display_interactions_in_element(Element *elem, long mode, long process_id);
void display_interactions_in_patch(Patch *patch, long mode, long process_id);
void display_interactions_in_bsp_tree(long process_id);
void ps_display_scene(long fill_sw, long patch_sw, long mesh_sw, long interaction_sw, long process_id);
void ps_display_patch(Patch *patch, long mode, long process_id);
void ps_display_patches_in_bsp_tree(long mode, long process_id);
void ps_display_element(Element *element, long mode, long process_id);
void ps_display_elements_in_patch(Patch *patch, long mode, long process_id);
void ps_display_elements_in_bsp_tree(long mode, long process_id);
void ps_display_interactions_in_element(Element *elem, long mode, long process_id);
void ps_display_interactions_in_patch(Patch *patch, long mode, long process_id);
void ps_display_interactions_in_bsp_tree(long process_id);

/*
 * elemman.C
 */
void foreach_element_in_patch(Patch *patch, void (*func)(), long arg1, long process_id);
void foreach_leaf_element_in_patch(Patch *patch, void (*func)(), long arg1, long process_id);
void ff_refine_elements(Element *e1, Element *e2, long level, long process_id);
long error_analysis(Element *e1, Element *e2, Interaction *inter12, Interaction *inter21, long process_id);
void bf_error_analysis_list(Element *elem, Interaction *i_list, long process_id);
long bf_error_analysis(Element *elem, Interaction *inter, long process_id);
long radiosity_converged(long process_id);
void subdivide_element(Element *e, long process_id);
void process_rays(Element *e, long process_id);
long element_completely_invisible(Element *e1, Element *e2, long process_id);
Element *get_element(long process_id);
long leaf_element(Element *elem, long process_id);
void init_elemlist(long process_id);
void print_element(Element *elem, long process_id);
void foreach_interaction_in_element(Element *elem, void (*func)(), long arg1, long process_id);
void compute_formfactor(Element *e_src, Element *e_dst, Interaction *inter, long process_id);
void compute_interaction(Element *e_src, Element *e_dst, Interaction *inter, long subdiv, long process_id);
void insert_interaction(Element *elem, Interaction *inter, long process_id);
void delete_interaction(Element *elem, Interaction *prev, Interaction *inter, long process_id);
void insert_vis_undef_interaction(Element *elem, Interaction *inter, long process_id);
void delete_vis_undef_interaction(Element *elem, Interaction *prev, Interaction *inter, long process_id);
Interaction *get_interaction(long process_id);
void free_interaction(Interaction *interaction, long process_id);
void init_interactionlist(long process_id);
void print_interaction(Interaction *inter, long process_id);

/*
 * rad_main.C
 */
void start_radiosity(long val);
void change_display(long val);
void change_view_x(long val);
void change_view_y(long val);
void change_view_zoom(long val);
void change_BFepsilon(long val);
void change_area_epsilon(long val);
void select_model(long val);
void utility_tools(long val);
void radiosity(void);
long init_ray_tasks(long process_id);
void init_radavg_tasks(long mode, long process_id);
void init_global(long process_id);
void print_usage(void);

/*
 * rad_tools.C
 */
void print_statistics(FILE *fd, long process_id);
void print_per_process_info(FILE *fd, long process);
void get_patch_stat(Patch *patch, long dummy, long process_id);
void get_elem_stat(Element *elem, long dummy, long process_id);
void count_interaction(Element *es, Element *e1, Element *e2, Element *e3, long *c3, long *c2, long *c1, long *c0, long process_id);
long search_intearction(Interaction *int_list, Interaction *inter, long process_id);
void print_running_time(long process_id);
void print_fork_time(long process_id);
void init_stat_info(long process_id);
void clear_radiosity(long process_id);
void clear_patch_radiosity(Patch *patch, long dummy, long process_id);

/*
 * smallobj.C
 */
float vector_length(Vertex *v);
float distance(Vertex *p1, Vertex *p2);
float normalize_vector(Vertex *v1, Vertex *v2);
float inner_product(Vertex *v1, Vertex *v2);
void cross_product(Vertex *vc, Vertex *v1, Vertex *v2);
float plane_normal(Vertex *vc, Vertex *p1, Vertex *p2, Vertex *p3);
void center_point(Vertex *p1, Vertex *p2, Vertex *p3, Vertex *pc);
void four_center_points(Vertex *p1, Vertex *p2, Vertex *p3, Vertex *pc, Vertex *pc1, Vertex *pc2, Vertex *pc3);
void print_point(Vertex *point);
void print_rgb(Rgb *rgb);
ElemVertex *create_elemvertex(Vertex *p, long process_id);
ElemVertex *get_elemvertex(long process_id);
void init_elemvertex(long process_id);
void foreach_leaf_edge(Edge *edge, long reverse, void (*func)(), long arg1, long arg2, long process_id);
Edge *create_edge(ElemVertex *v1, ElemVertex *v2, long process_id);
void subdivide_edge(Edge *e, float a_ratio, long process_id);
Edge *get_edge(long process_id);
void init_edge(long process_id);
void init_sharedlock(long process_id);
Shared_Lock *get_sharedlock(long segment, long process_id);

/*
 * visible.C
 */
void init_visibility_module(long process_id);
void get_test_rays(Vertex *p_src, Ray *v, long no, long process_id);
long v_intersect(Patch *patch, Vertex *p, Ray *ray, float t);
long traverse_bsp(Patch *src_node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id);
long traverse_subtree(Patch *node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id);
long intersection_type(Patch *patch, Vertex *p, Ray *ray, float *tval, float range_min, float range_max);
long test_intersection(Patch *patch, Vertex *p, Ray *ray, float tval, long process_id);
void update_patch_cache(Patch *patch, long process_id);
long check_patch_cache(Vertex *p, Ray *ray, float r_min, float r_max, long process_id);
void init_patch_cache(long process_id);
long patch_tested(Patch *p, long process_id);
float visibility(Element *e1, Element *e2, long n_rays, long process_id);
void compute_visibility_values(Element *elem, Interaction *inter, long n_inter, long process_id);
void visibility_task(Element *elem, Interaction *inter, long n_inter, void (*k)(), long process_id);

#endif


#define VIS_RANGE_MARGIN (0.01)

#define		VISI_RAYS_MAX   (16)
#define         VISI_POOL_NO    (16)

#define FABS(x)  (((x) < 0)?-(x):(x))


float rand_ray1[VISI_RAYS_MAX][2], rand_ray2[VISI_RAYS_MAX][2] ;

struct v_struct {
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
                                   and allow page-placement */
    Patch *v_src_patch, *v_dest_patch ;
    Vertex v_src_p1,   v_dest_p1 ;
    Vertex v_src_v12,  v_src_v13 ;
    Vertex v_dest_v12, v_dest_v13 ;

    long bsp_nodes_visited, total_bsp_nodes_visited ;
    Ray ray_pool[VISI_POOL_NO];
    Vertex point_pool[VISI_POOL_NO];
    long pool_dst_hits;	/* Number of rays that hit the destination  */
    Patch *patch_cache[PATCH_CACHE_SIZE] ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
                                   and allow page-placement */
} vis_struct[MAX_PROCESSORS];


/*************************************************************
 *
 *	void init_visibility_module()
 *
 *       initialize the random test rays array.
 *
 *       Test ray parameters are precomputed as follows.
 *       (1) The triangles are divided into 16 small triangles.
 *       (2) Each small triangle shoots a ray to a small triangle on the
 *           destination. If N-rays are requested by get_test_ray(),
 *           N small triangles are chosen on the source and the destination
 *           and a ray is shot between N pairs of the small triangles.
 *       (3) Current scheme selects pairs of the small triangles in a static
 *           manner. The pairs are chosen such that rays are equally
 *           distributed.
 *
 *************************************************************/

void init_visibility_module(long process_id)
{
#define TTICK (1.0/12.0)

    /* Three corner triangles. P(i) -- Q(i) */
    rand_ray1[0][0] = TTICK ;      rand_ray1[0][1] = TTICK ;
    rand_ray1[1][0] = TTICK ;      rand_ray1[1][1] = TTICK * 10 ;
    rand_ray1[2][0] = TTICK * 10 ; rand_ray1[2][1] = TTICK ;
    rand_ray2[0][0] = TTICK ;      rand_ray2[0][1] = TTICK ;
    rand_ray2[1][0] = TTICK ;      rand_ray2[1][1] = TTICK * 10 ;
    rand_ray2[2][0] = TTICK * 10 ; rand_ray2[2][1] = TTICK ;

    /* Three triangles adjacent to the corners. RotLeft P(i)--> Q(i+1) */
    rand_ray1[3][0] = TTICK * 2 ;  rand_ray1[3][1] = TTICK * 2 ;
    rand_ray1[4][0] = TTICK * 2 ;  rand_ray1[4][1] = TTICK * 8 ;
    rand_ray1[5][0] = TTICK * 8 ;  rand_ray1[5][1] = TTICK * 2 ;
    rand_ray2[4][0] = TTICK * 2 ;  rand_ray2[4][1] = TTICK * 2 ;
    rand_ray2[5][0] = TTICK * 2 ;  rand_ray2[5][1] = TTICK * 8 ;
    rand_ray2[3][0] = TTICK * 8 ;  rand_ray2[3][1] = TTICK * 2 ;

    /* Three triangles adjacent to the center. RotRight P(i)--> Q(i-1) */
    rand_ray1[6][0] = TTICK * 2 ;  rand_ray1[6][1] = TTICK * 5 ;
    rand_ray1[7][0] = TTICK * 5 ;  rand_ray1[7][1] = TTICK * 5 ;
    rand_ray1[8][0] = TTICK * 5 ;  rand_ray1[8][1] = TTICK * 2 ;
    rand_ray2[8][0] = TTICK * 2 ;  rand_ray2[8][1] = TTICK * 5 ;
    rand_ray2[6][0] = TTICK * 5 ;  rand_ray2[6][1] = TTICK * 5 ;
    rand_ray2[7][0] = TTICK * 5 ;  rand_ray2[7][1] = TTICK * 2 ;

    /* Center triangle. Straight P(i) --> Q(i) */
    rand_ray1[9][0] = TTICK * 4 ;  rand_ray1[9][1] = TTICK * 4 ;
    rand_ray2[9][0] = TTICK * 4 ;  rand_ray2[9][1] = TTICK * 4 ;

    /* Along the pelimeter. RotRight P(i)--> Q(i-1) */
    rand_ray1[10][0] = TTICK * 1 ;  rand_ray1[10][1] = TTICK * 7 ;
    rand_ray1[11][0] = TTICK * 5 ;  rand_ray1[11][1] = TTICK * 4 ;
    rand_ray1[12][0] = TTICK * 4 ;  rand_ray1[12][1] = TTICK * 1 ;
    rand_ray2[12][0] = TTICK * 1 ;  rand_ray2[12][1] = TTICK * 7 ;
    rand_ray2[10][0] = TTICK * 5 ;  rand_ray2[10][1] = TTICK * 4 ;
    rand_ray2[11][0] = TTICK * 4 ;  rand_ray2[11][1] = TTICK * 1 ;

    /* Along the pelimeter. RotLeft P(i)--> Q(i+1) */
    rand_ray1[13][0] = TTICK * 1 ;  rand_ray1[13][1] = TTICK * 4 ;
    rand_ray1[14][0] = TTICK * 4 ;  rand_ray1[14][1] = TTICK * 7 ;
    rand_ray1[15][0] = TTICK * 7 ;  rand_ray1[15][1] = TTICK * 1 ;
    rand_ray2[14][0] = TTICK * 1 ;  rand_ray2[14][1] = TTICK * 4 ;
    rand_ray2[15][0] = TTICK * 4 ;  rand_ray2[15][1] = TTICK * 7 ;
    rand_ray2[13][0] = TTICK * 7 ;  rand_ray2[13][1] = TTICK * 1 ;

    /* Initialize patch cache */
    init_patch_cache(process_id) ;
}


/*************************************************************
 *
 *	void get_test_ray(): get a randomized ray vector and start point.
 *
 *	Place: main loop.
 *
 *	Storage: must keep in the invidiual processor.
 *
 *************************************************************/

void get_test_rays(Vertex *p_src, Ray *v, long no, long process_id)
{
    long g_index, i ;
    Vertex p_dst ;
    float fv1, fv2 ;

    if( no > VISI_RAYS_MAX )
        no = VISI_RAYS_MAX ;

    for (i = 0, g_index = 0 ; i < no; i++, g_index++) {

        fv1 = rand_ray1[ g_index ][0] ;
        fv2 = rand_ray1[ g_index ][1] ;
        p_src->x = vis_struct[process_id].v_src_p1.x + vis_struct[process_id].v_src_v12.x * fv1 + vis_struct[process_id].v_src_v13.x * fv2 ;
        p_src->y = vis_struct[process_id].v_src_p1.y + vis_struct[process_id].v_src_v12.y * fv1 + vis_struct[process_id].v_src_v13.y * fv2 ;
        p_src->z = vis_struct[process_id].v_src_p1.z + vis_struct[process_id].v_src_v12.z * fv1 + vis_struct[process_id].v_src_v13.z * fv2 ;

        fv1 = rand_ray2[ g_index ][0] ;
        fv2 = rand_ray2[ g_index ][1] ;
        p_dst.x = vis_struct[process_id].v_dest_p1.x + vis_struct[process_id].v_dest_v12.x * fv1 + vis_struct[process_id].v_dest_v13.x * fv2 ;
        p_dst.y = vis_struct[process_id].v_dest_p1.y + vis_struct[process_id].v_dest_v12.y * fv1 + vis_struct[process_id].v_dest_v13.y * fv2 ;
        p_dst.z = vis_struct[process_id].v_dest_p1.z + vis_struct[process_id].v_dest_v12.z * fv1 + vis_struct[process_id].v_dest_v13.z * fv2 ;

        v->x = p_dst.x - p_src->x;
        v->y = p_dst.y - p_src->y;
        v->z = p_dst.z - p_src->z;

        p_src++;
        v++;
    }
}


/************************************************************************
 *
 *	long v_intersect(): check if the ray intersects with the polygon.
 *
 *************************************************************************/


long v_intersect(Patch *patch, Vertex *p, Ray *ray, float t)
{
    float px, py, pz;
    float nx, ny, nz;
    float rx, ry, rz;
    float x, y, x1, x2, x3, y1, y2, y3;
    float a, b, c;
    long nc, sh, nsh;

    nx = patch->plane_equ.n.x;
    ny = patch->plane_equ.n.y;
    nz = patch->plane_equ.n.z;

    rx = ray->x;
    ry = ray->y;
    rz = ray->z;

    px = p->x;
    py = p->y;
    pz = p->z;


    a = FABS(nx); b = FABS(ny); c = FABS(nz);

    if (a > b)
        if (a > c) {
            x  = py + t * ry; y = pz + t * rz;
            x1 = patch->p1.y; y1 = patch->p1.z;
            x2 = patch->p2.y; y2 = patch->p2.z;
            x3 = patch->p3.y; y3 = patch->p3.z;
        } else {
            x  = px + t * rx; y = py + t * ry;
            x1 = patch->p1.x; y1 = patch->p1.y;
            x2 = patch->p2.x; y2 = patch->p2.y;
            x3 = patch->p3.x; y3 = patch->p3.y;
        }
    else if (b > c) {
        x  = px + t * rx; y = pz + t * rz;
        x1 = patch->p1.x; y1 = patch->p1.z;
        x2 = patch->p2.x; y2 = patch->p2.z;
        x3 = patch->p3.x; y3 = patch->p3.z;
    } else {
        x  = px + t * rx; y = py + t * ry;
        x1 = patch->p1.x; y1 = patch->p1.y;
        x2 = patch->p2.x; y2 = patch->p2.y;
        x3 = patch->p3.x; y3 = patch->p3.y;
    }


    x1 -= x; y1 -= y;
    x2 -= x; y2 -= y;
    x3 -= x; y3 -= y;

    nc = 0;

    if (y1 >= 0.0)
        sh = 1;
    else
        sh = -1;

    if (y2 >= 0.0)
        nsh = 1;
    else
        nsh = -1;

    if (sh != nsh) {
        if ((x1 >= 0.0) && (x2 >= 0.0))
            nc++;
        else if ((x1 >= 0.0) || (x2 >= 0.0))
            if ((x1 - y1 * (x2 - x1) / (y2 - y1)) > 0.0)
                nc++;
        sh = nsh;
    }

    if (y3 >= 0.0)
        nsh = 1;
    else
        nsh = -1;

    if (sh != nsh) {
        if ((x2 >= 0.0) && (x3 >= 0.0))
            nc++;
        else if ((x2 >= 0.0) || (x3 >= 0.0))
            if ((x2 - y2 * (x3 - x2) / (y3 - y2)) > 0.0)
                nc++;
        sh = nsh;
    }

    if (y1 >= 0.0)
        nsh = 1;
    else
        nsh = -1;

    if (sh != nsh) {
        if ((x3 >= 0.0) && (x1 >= 0.0))
            nc++;
        else if ((x3 >= 0.0) || (x1 >= 0.0))
            if ((x1 - y1 * (x1 - x3) / (y1 - y3)) > 0.0)
                nc++;
        sh = nsh;
    }

    if ((nc % 2) == 0)
        return(0);
    else {
        return(1);
    }

}




#define DEST_FOUND (1)
#define DEST_NOT_FOUND (0)

#define ON_THE_PLANE          (0)
#define POSITIVE_SUBTREE_ONLY (1)
#define NEGATIVE_SUBTREE_ONLY (2)
#define POSITIVE_SIDE_FIRST   (3)
#define NEGATIVE_SIDE_FIRST   (4)



/****************************************************************************
 *
 *    traverse_bsp()
 *    traverse_subtree()
 *
 *    Traverse the BSP tree. The traversal is in-order. Since the traversal
 *    of the BSP tree begins at the middle of the BSP tree (i.e., the source
 *    node), the traversal is performed as follows.
 *      1) Traverse the positive subtee of the start (source) node.
 *      2) For each ancestor node of the source node, visit it (immediate
 *         parent first).
 *         2.1) Test if the node intercepts the ray.
 *         2.2) Traverse the subtree of the node (i.e., the subtree that the
 *              source node does not belong to.
 *
 *    traverse_bsp() takes care of the traversal of ancestor nodes. Ordinary
 *    traversal of a subtree is done by traverse_subtree().
 *
 *****************************************************************************/


long traverse_bsp(Patch *src_node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id)
{
    float t = 0.0 ;
    Patch *parent, *visited_child ;
    long advice ;


    /* (1) Check patch cache */
    if( check_patch_cache( p, ray, r_min, r_max, process_id ) )
        return( 1 ) ;

    /* (2) Check S+(src_node) */
    if( traverse_subtree( src_node->bsp_positive, p, ray, r_min, r_max, process_id ) )
        return( 1 ) ;

    /* (3) Continue in-order traversal till root is encountered */
    for( parent = src_node->bsp_parent, visited_child = src_node ;
        parent ;
        visited_child = parent, parent = parent->bsp_parent )
        {
            /* Check intersection at this node */
            advice = intersection_type( parent, p, ray, &t, r_min, r_max ) ;
            if( (advice != POSITIVE_SUBTREE_ONLY) && (advice != NEGATIVE_SUBTREE_ONLY) )
                {
                    if( test_intersection( parent, p, ray, t, process_id ) )
                        return( 1 ) ;

                    r_min = t - VIS_RANGE_MARGIN ;
                }

            /* Traverse unvisited subtree of the node */
            if(   (parent->bsp_positive == visited_child) && (advice != POSITIVE_SUBTREE_ONLY) )
                {
                    if( traverse_subtree( parent->bsp_negative, p, ray, r_min, r_max, process_id ) )
                        return( 1 ) ;
                }
            else if( (parent->bsp_positive != visited_child) && (advice != NEGATIVE_SUBTREE_ONLY) )
                {
                    if( traverse_subtree( parent->bsp_positive, p, ray, r_min, r_max, process_id ) )
                        return( 1 ) ;
                }
        }

    return( 0 ) ;
}




long traverse_subtree(Patch *node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id)
  /*
   *    To minimize the length of the traversal of the BSP tree, a pruning
   *    algorithm is incorporated.
   *    One possibility (not used in this version) is to prune one of the
   *	 subtrees if the node in question intersects the ray outside of the
   *	 range defined by the source and the destination patches.
   *    Another possibility (used here) is more aggressive pruning. Like the above
   *    method, the intersection point is checked against the range to prune the
   *    subtree. But instead of using a constant source-destination range,
   *    the range itself is recursively subdivided so that the minimum range is
   *    applied the possibility of pruning maximized.
   */
{
    float t ;
    long advice ;


    if( node == 0 )
        return( 0 ) ;

    advice = intersection_type( node, p, ray, &t, r_min, r_max ) ;
    if( advice == POSITIVE_SIDE_FIRST )
        {
            /* The ray is approaching from the positive side of the patch */
            if( traverse_subtree( node->bsp_positive, p, ray,
                                 r_min, t + VIS_RANGE_MARGIN, process_id ) )
                return( 1 ) ;

            if( test_intersection( node, p, ray, t, process_id ) )
                return( 1 ) ;
            return( traverse_subtree( node->bsp_negative, p, ray,
                                     t - VIS_RANGE_MARGIN, r_max, process_id ) ) ;
        }
    else if( advice == NEGATIVE_SIDE_FIRST )
        {
            /* The ray is approaching from the negative side of the patch */
            if( traverse_subtree( node->bsp_negative, p, ray,
                                 r_min, t + VIS_RANGE_MARGIN, process_id ) )
                return( 1 ) ;
            if( test_intersection( node, p, ray, t, process_id ) )
                return( 1 ) ;

            return( traverse_subtree( node->bsp_positive, p, ray,
                                     t - VIS_RANGE_MARGIN, r_max, process_id ) ) ;
        }

    else if( advice == POSITIVE_SUBTREE_ONLY )
        return( traverse_subtree( node->bsp_positive, p, ray,
                                 r_min, r_max, process_id ) ) ;
    else if( advice == NEGATIVE_SUBTREE_ONLY )
        return( traverse_subtree( node->bsp_negative, p, ray,
                                 r_min, r_max, process_id ) ) ;
    else
        /* On the plane */
        return( 1 ) ;
}



/**************************************************************************
 *
 *	intersection_type()
 *
 *       Compute intersection coordinate as the barycentric coordinate
 *       w.r.t the ray vector. This value is returned to the caller through
 *       the variable passed by reference.
 *       intersection_type() also classifies the intersection type and
 *       returns the type as the "traversal advice" code.
 *       Possible types are:
 *       1) the patch and the ray are parallel
 *          --> POSITIVE_SUBTREE_ONLY, NEGATIVE_SUBTREE_ONLY, or ON_THE_PLANE
 *       2) intersects the ray outside of the specified range
 *          --> POSITIVE_SUBTREE_ONLY, NEGATIVE_SUBTREE_ONLY
 *       3) intersects within the range
 *          --> POSITIVE_SIDE_FIRST, NEGATIVE_SIDE_FIRST
 *
 ***************************************************************************/

long intersection_type(Patch  *patch, Vertex *p, Ray *ray, float *tval, float range_min, float range_max)
{
    float r_dot_n ;
    float dist ;
    float t ;
    float nx, ny, nz ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    vis_struct[process_id].bsp_nodes_visited++ ;
#endif

    /* (R.N) */
    nx = patch->plane_equ.n.x ;
    ny = patch->plane_equ.n.y ;
    nz = patch->plane_equ.n.z ;

    r_dot_n = nx * ray->x + ny * ray->y + nz * ray->z ;
    dist = patch->plane_equ.c  +  p->x * nx  +  p->y * ny  +  p->z * nz ;

    if( (-(float)F_ZERO < r_dot_n) && (r_dot_n < (float)F_ZERO) )
        {
            if( dist > (float)F_COPLANAR )
                return( POSITIVE_SUBTREE_ONLY ) ;
            else if( dist < -F_COPLANAR )
                return( NEGATIVE_SUBTREE_ONLY ) ;

            return( ON_THE_PLANE ) ;
        }

    t = -dist / r_dot_n ;
    *tval = t ;

    if( t < range_min )
        {
            if( r_dot_n >= 0 )
                return( POSITIVE_SUBTREE_ONLY ) ;
            else
                return( NEGATIVE_SUBTREE_ONLY ) ;
        }
    else if ( t > range_max )
        {
            if( r_dot_n >= 0 )
                return( NEGATIVE_SUBTREE_ONLY ) ;
            else
                return( POSITIVE_SUBTREE_ONLY ) ;
        }
    else
        {
            if( r_dot_n >= 0 )
                return( NEGATIVE_SIDE_FIRST ) ;
            else
                return( POSITIVE_SIDE_FIRST ) ;
        }
}


/*************************************************************
 *
 *	test_intersection()
 *
 *************************************************************/

long test_intersection(Patch *patch, Vertex *p, Ray *ray, float tval, long process_id)
{
    /* Rays always hit the destination. Note that (R.Ndest) is already
       checked by visibility() */

    if( patch == vis_struct[process_id].v_dest_patch )
        {
            vis_struct[process_id].pool_dst_hits++ ;
            return( 1 ) ;
        }

    if( patch_tested( patch, process_id ) )
        return( 0 ) ;

    if( v_intersect( patch, p, ray, tval ) )
        {
            /* Store it in the patch-cache */
            update_patch_cache( patch, process_id ) ;
            return( 1 ) ;
        }

    return( 0 ) ;
}



/*************************************************************
 *
 *	patch_cache
 *
 *       update_patch_cache()
 *       check_patch_cache()
 *       init_patch_cache()
 *
 *    To exploit visibility coherency, a patch cache is used.
 *    Before traversing the BSP tree, the cache contents are tested to see
 *    if they intercept the ray in question. The size of the patch cache is
 *    defined by PATCH_CACHE_SIZE (in patch.H). Since the first two
 *    entries of the cache
 *    usually cover about 95% of the cache hits, increasing the cache size
 *    does not help much. Nevertheless, the program is written so that
 *    increasing cache size does not result in additional ray-intersection
 *    test.
 *
 *************************************************************/

void update_patch_cache(Patch *patch, long process_id)
{
    long i ;

    /* Shift current contents */
    for( i = PATCH_CACHE_SIZE-1 ; i > 0  ; i-- )
        vis_struct[process_id].patch_cache[i] = vis_struct[process_id].patch_cache[i-1] ;

    /* Store the new patch data */
    vis_struct[process_id].patch_cache[0] = patch ;
}



long check_patch_cache(Vertex *p, Ray *ray, float r_min, float r_max, long process_id)
{
    long i ;
    float t ;
    Patch *temp ;
    long advice ;

    for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        {
            if(   (vis_struct[process_id].patch_cache[i] == 0)
               || (vis_struct[process_id].patch_cache[i] == vis_struct[process_id].v_dest_patch)
               || (vis_struct[process_id].patch_cache[i] == vis_struct[process_id].v_src_patch) )
                continue ;

            advice = intersection_type( vis_struct[process_id].patch_cache[i],  p, ray, &t, r_min, r_max ) ;

            /* If no intersection, then skip */
            if(   (advice == POSITIVE_SUBTREE_ONLY)
               || (advice == NEGATIVE_SUBTREE_ONLY) )
                continue ;

            if(   (advice == ON_THE_PLANE) || v_intersect( vis_struct[process_id].patch_cache[i], p, ray, t ) )
                {
                    /* Change priority */
                    if( i > 0 )
                        {
                            temp = vis_struct[process_id].patch_cache[ i-1 ] ;
                            vis_struct[process_id].patch_cache[ i-1 ] = vis_struct[process_id].patch_cache[ i ] ;
                            vis_struct[process_id].patch_cache[ i ] = temp ;
                        }

                    return( 1 ) ;
                }
        }


    return( 0 ) ;
}



void init_patch_cache(long process_id)
{
    long i ;

    for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        vis_struct[process_id].patch_cache[ i ] = 0 ;
}


long patch_tested(Patch *p, long process_id)
{
    long i ;

    for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        {
            if( p == vis_struct[process_id].patch_cache[i] )
                return( 1 ) ;
        }

    return( 0 ) ;
}


/*************************************************************
 *
 *	float visibility(): checking if two patches are mutually invisible.
 *
 *************************************************************/


float visibility(Element *e1, Element *e2, long n_rays, long process_id)
{
    float range_max, range_min ;
    long i;
    Ray *r;
    long ray_reject ;

    vis_struct[process_id].v_src_patch  = e1->patch;
    vis_struct[process_id].v_dest_patch = e2->patch;

    vis_struct[process_id].v_src_p1 = e1->ev1->p ;
    vis_struct[process_id].v_src_v12.x = e1->ev2->p.x - vis_struct[process_id].v_src_p1.x ;
    vis_struct[process_id].v_src_v12.y = e1->ev2->p.y - vis_struct[process_id].v_src_p1.y ;
    vis_struct[process_id].v_src_v12.z = e1->ev2->p.z - vis_struct[process_id].v_src_p1.z ;
    vis_struct[process_id].v_src_v13.x = e1->ev3->p.x - vis_struct[process_id].v_src_p1.x ;
    vis_struct[process_id].v_src_v13.y = e1->ev3->p.y - vis_struct[process_id].v_src_p1.y ;
    vis_struct[process_id].v_src_v13.z = e1->ev3->p.z - vis_struct[process_id].v_src_p1.z ;

    vis_struct[process_id].v_dest_p1 = e2->ev1->p ;
    vis_struct[process_id].v_dest_v12.x = e2->ev2->p.x - vis_struct[process_id].v_dest_p1.x ;
    vis_struct[process_id].v_dest_v12.y = e2->ev2->p.y - vis_struct[process_id].v_dest_p1.y ;
    vis_struct[process_id].v_dest_v12.z = e2->ev2->p.z - vis_struct[process_id].v_dest_p1.z ;
    vis_struct[process_id].v_dest_v13.x = e2->ev3->p.x - vis_struct[process_id].v_dest_p1.x ;
    vis_struct[process_id].v_dest_v13.y = e2->ev3->p.y - vis_struct[process_id].v_dest_p1.y ;
    vis_struct[process_id].v_dest_v13.z = e2->ev3->p.z - vis_struct[process_id].v_dest_p1.z ;

    get_test_rays( vis_struct[process_id].point_pool, vis_struct[process_id].ray_pool, n_rays, process_id ) ;

    range_min = -VIS_RANGE_MARGIN ;
    range_max =  1.0 + VIS_RANGE_MARGIN ;

    vis_struct[process_id].pool_dst_hits = 0 ;
    ray_reject = 0 ;
    for ( i = 0 ; i < n_rays ; i++ )
        {
            r = &(vis_struct[process_id].ray_pool[i]);

            if (  (inner_product( (Vertex *)r, &(vis_struct[process_id].v_src_patch)->plane_equ.n ) <= 0.0 )
                ||(inner_product( (Vertex *)r, &(vis_struct[process_id].v_dest_patch)->plane_equ.n ) >= 0.0 ) )
                {
                    ray_reject++ ;
                    continue ;
                }

            traverse_bsp( vis_struct[process_id].v_src_patch, &vis_struct[process_id].point_pool[i], r, range_min, range_max, process_id ) ;
        }

    if (ray_reject == n_rays) {
        /* All rays have been trivially rejected. This can occur
           if no rays are shot between visible portion of the elements.
           Return partial visibility (1/No-of-rays). */

        /* Return partially visible result */
        vis_struct[process_id].pool_dst_hits = 1 ;
    }

    return( (float)vis_struct[process_id].pool_dst_hits / (float)n_rays ) ;
}



/*****************************************************************
 *
 *    compute_visibility_values()
 *
 *    Apply visibility() function to an interaction list.
 *
 ******************************************************************/

void compute_visibility_values(Element *elem, Interaction *inter, long n_inter, long process_id)
{
    for( ; n_inter > 0 ; inter = inter->next, n_inter-- )
        {
            if( inter->visibility != VISIBILITY_UNDEF )
                continue ;

            vis_struct[process_id].bsp_nodes_visited = 0 ;

            inter->visibility
                = visibility( elem, inter->destination,
                             N_VISIBILITY_TEST_RAYS, process_id ) ;

            vis_struct[process_id].total_bsp_nodes_visited += vis_struct[process_id].bsp_nodes_visited ;
        }
}

struct context13 {
  Element *elem;
  long n_inter;
  long new_vis_undef_count;
};

void func13(struct context13 *ctx13);
void func13(struct context13 *ctx13)
{
  ctx13->elem->n_vis_undef_inter -= ctx13->n_inter ;
  ctx13->new_vis_undef_count = ctx13->elem->n_vis_undef_inter ;
}

struct context14 {
  Patch_Cost *pc;
  long process_id;
};

void func14(struct context14 *ctx14);
void func14(struct context14 *ctx14)
{
  ctx14->pc->n_bsp_node += vis_struct[ctx14->process_id].total_bsp_nodes_visited ;
}

/*****************************************************************
 *
 *    visibility_task()
 *
 *    Compute visibility values and then call continuation when all
 *    the undefined visibility values have been computed.
 *
 ******************************************************************/

void visibility_task(Element *elem, Interaction *inter, long n_inter, void (*k)(), long process_id)
{
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    Patch_Cost *pc ;
#endif
    long new_vis_undef_count ;

    /* Compute visibility */
    vis_struct[process_id].total_bsp_nodes_visited = 0 ;
    compute_visibility_values( elem, inter, n_inter, process_id ) ;

    /* Change visibility undef count */
//#ifdef FFWD
#if 0
    GET_CONTEXT()
    uint64_t return_value = 0;
    struct context13 ctx13;
    ctx13.elem = elem;
    ctx13.n_inter = n_inter;
    ctx13.new_vis_undef_count = new_vis_undef_count;
    FFWD_EXEC(0, &func13, return_value, 1, &ctx13)
    elem = ctx13.elem;
    n_inter = ctx13.n_inter;
    new_vis_undef_count = ctx13.new_vis_undef_count;
#else
    {pthread_mutex_lock(&(elem->elem_lock->lock));};
    elem->n_vis_undef_inter -= n_inter ;
    new_vis_undef_count = elem->n_vis_undef_inter ;
    {pthread_mutex_unlock(&(elem->elem_lock->lock));};
#endif

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    pc = &global->patch_cost[ elem->patch->seq_no ] ;
#ifdef FFWD
    struct context14 ctx14;
    ctx14.pc = pc;
    ctx14.process_id = process_id;
    FFWD_EXEC(0, &func14, return_value, 1, &ctx14)
    pc = ctx14.pc;
#else
    {pthread_mutex_lock(&(pc->cost_lock->lock));};
    pc->n_bsp_node += vis_struct[process_id].total_bsp_nodes_visited ;
    {pthread_mutex_unlock(&(pc->cost_lock->lock));};
#endif
#endif

    /* Call continuation if this is the last task finished. */
    if( new_vis_undef_count == 0 )
        k( elem, process_id ) ;
}
