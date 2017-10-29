
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
 *       Parallel Hierarchical Radiosity
 *
 *
 *       Room model data base.
 *
 *       Courtesy of Pat Hanrahan.
 *
 ***************************************************************/


#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
extern pthread_t PThreadTable[];
;

#include <stdio.h>


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



ModelDataBase room_model[] =
{

    /** Patch 0 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, 700.000000 },
                { -847.000000, -800.002014, 700.000000 },
                { 847.000000, 800.002014, 700.000000 } } } },

    /** Patch 1 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, -800.002014, -1001.000000 },
                { 847.000000, -800.002014, -1001.000000 },
                { -847.000000, 800.002014, -1001.000000 } } } },

    /** Patch 2 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, -1001.000000 },
                { 847.000000, -800.002014, 700.000000 },
                { 847.000000, 800.002014, -1001.000000 } } } },

    /** Patch 3 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, -800.002014, 700.000000 },
                { -847.000000, -800.002014, -1001.000000 },
                { -847.000000, 800.002014, 700.000000 } } } },

    /** Patch 4 **/
    { MODEL_RECTANGLE,
        { { 0.603921, 0.603921, 0.603921 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, 800.002014, 700.000000 },
                { -847.000000, 800.002014, 700.000000 },
                { 847.000000, 800.002014, -1001.000000 } } } },

    /** Patch 5 **/
    { MODEL_RECTANGLE,
        { { 0.501961, 0.501961, 0.501961 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, -1001.000000 },
                { -847.000000, -800.002014, -1001.000000 },
                { 847.000000, -800.002014, 700.000000 } } } },

    /** Patch 6 **/
    { MODEL_RECTANGLE,
        { { 0.576470, 0.521569, 0.521568 }, { 0.000000, 0.000000, 0.000000 },
            { { -269.149994, -216.666794, -1001.000000 },
                { 165.895096, -216.666794, -1001.000000 },
                { -269.149994, 281.666687, -1001.000000 } } } },

    /** Patch 7 **/
    { MODEL_RECTANGLE,
        { { 0.098034, 0.086275, 0.074510 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -159.999695, 285.980103 },
                { -840.000061, -159.999695, -404.214966 },
                { -840.000061, 298.332977, 285.980103 } } } },

    /** Patch 8 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -84.000000, -350.000000, -999.999023 },
                { -840.000061, -350.000000, -159.998962 },
                { -84.000000, -299.999695, -999.999023 } } } },

    /** Patch 9 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -350.000000, -159.998962 },
                { -84.000000, -350.000000, -999.999023 },
                { -336.000000, -350.000000, 155.000305 } } } },

    /** Patch 10 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -336.000000, -299.999695, 155.000305 },
                { 420.000031, -299.999695, -684.999023 },
                { -840.000061, -299.999695, -159.998962 } } } },

    /** Patch 11 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -336.000000, -350.000000, 155.000305 },
                { 420.000031, -350.000000, -684.999023 },
                { -336.000000, -299.999695, 155.000305 } } } },

    /** Patch 12 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, -350.000000, -684.999023 },
                { -84.000000, -350.000000, -999.999023 },
                { 420.000031, -299.999695, -684.999023 } } } },

    /** Patch 13 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -350.000000, -159.998962 },
                { -336.000000, -350.000000, 155.000305 },
                { -840.000061, -299.999695, -159.998962 } } } },

    /** Patch 14 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -306.000092, -800.002014, 80.000183 },
                { -786.001953, -800.002014, -220.003052 },
                { -306.000092, -350.000000, 80.000183 } } } },

    /** Patch 15 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -341.999695, -800.002014, 120.000305 },
                { -306.000092, -800.002014, 80.000183 },
                { -341.999695, -350.000000, 120.000305 } } } },

    /** Patch 16 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -786.001953, -800.002014, -220.003052 },
                { -822.002930, -800.002014, -179.998047 },
                { -786.001953, -350.000000, -220.003052 } } } },

    /** Patch 17 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -822.002930, -800.002014, -179.998047 },
                { -341.999695, -800.002014, 120.000305 },
                { -822.002930, -350.000000, -179.998047 } } } },

    /** Patch 18 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -533.332825, -345.001953 },
                { 420.000031, -533.332825, -684.999023 },
                { 113.999901, -500.000214, -345.001953 } } } },

    /** Patch 19 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, -533.332825, -684.999023 },
                { -84.000000, -533.332825, -999.999023 },
                { 420.000031, -500.000214, -684.999023 } } } },

    /** Patch 20 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -500.000214, -345.001953 },
                { 420.000031, -500.000214, -684.999023 },
                { -390.000092, -500.000214, -660.001953 } } } },

    /** Patch 21 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -533.332825, -660.001953 },
                { -84.000000, -533.332825, -999.999023 },
                { 113.999901, -533.332825, -345.001953 } } } },

    /** Patch 22 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -84.000000, -533.332825, -999.999023 },
                { -390.000092, -533.332825, -660.001953 },
                { -84.000000, -500.000214, -999.999023 } } } },

    /** Patch 23 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -533.332825, -660.001953 },
                { 113.999901, -533.332825, -345.001953 },
                { -390.000092, -500.000214, -660.001953 } } } },

    /** Patch 24 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -700.000000, -345.001953 },
                { 420.000031, -700.000000, -684.999023 },
                { 113.999901, -666.666687, -345.001953 } } } },

    /** Patch 25 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, -700.000000, -684.999023 },
                { -84.000000, -700.000000, -999.999023 },
                { 420.000031, -666.666687, -684.999023 } } } },

    /** Patch 26 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -666.666687, -345.001953 },
                { 420.000031, -666.666687, -684.999023 },
                { -390.000092, -666.666687, -660.001953 } } } },

    /** Patch 27 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -700.000000, -660.001953 },
                { -84.000000, -700.000000, -999.999023 },
                { 113.999901, -700.000000, -345.001953 } } } },

    /** Patch 28 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -84.000000, -700.000000, -999.999023 },
                { -390.000092, -700.000000, -660.001953 },
                { -84.000000, -666.666687, -999.999023 } } } },

    /** Patch 29 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -700.000000, -660.001953 },
                { 113.999901, -700.000000, -345.001953 },
                { -390.000092, -666.666687, -660.001953 } } } },

    /** Patch 30 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -138.000107, -500.000214, -940.001953 },
                { 341.999695, -500.000214, -640.002930 },
                { -138.000107, -350.000000, -940.001953 } } } },

    /** Patch 31 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -500.000214, -640.002930 },
                { 378.000000, -500.000214, -680.000977 },
                { 341.999695, -350.000000, -640.002930 } } } },

    /** Patch 32 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 378.000000, -500.000214, -680.000977 },
                { -101.999802, -500.000214, -980.000122 },
                { 378.000000, -350.000000, -680.000977 } } } },

    /** Patch 33 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -101.999802, -500.000214, -980.000122 },
                { -138.000107, -500.000214, -940.001953 },
                { -101.999802, -350.000000, -980.000122 } } } },

    /** Patch 34 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -138.000107, -666.666687, -940.001953 },
                { 341.999695, -666.666687, -640.002930 },
                { -138.000107, -533.332825, -940.001953 } } } },

    /** Patch 35 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -666.666687, -640.002930 },
                { 378.000000, -666.666687, -680.000977 },
                { 341.999695, -533.332825, -640.002930 } } } },

    /** Patch 36 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 378.000000, -666.666687, -680.000977 },
                { -101.999802, -666.666687, -980.000122 },
                { 378.000000, -533.332825, -680.000977 } } } },

    /** Patch 37 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -101.999802, -666.666687, -980.000122 },
                { -138.000107, -666.666687, -940.001953 },
                { -101.999802, -533.332825, -980.000122 } } } },

    /** Patch 38 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -138.000107, -800.002014, -940.001953 },
                { 341.999695, -800.002014, -640.002930 },
                { -138.000107, -700.000000, -940.001953 } } } },

    /** Patch 39 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -800.002014, -640.002930 },
                { 378.000000, -800.002014, -680.000977 },
                { 341.999695, -700.000000, -640.002930 } } } },

    /** Patch 40 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 378.000000, -800.002014, -680.000977 },
                { -101.999802, -800.002014, -980.000122 },
                { 378.000000, -700.000000, -680.000977 } } } },

    /** Patch 41 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -101.999802, -800.002014, -980.000122 },
                { -138.000107, -800.002014, -940.001953 },
                { -101.999802, -700.000000, -980.000122 } } } },

    /** Patch 42 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 108.000198, -500.000214, -380.002075 },
                { 120.000298, -500.000214, -393.337036 },
                { 108.000198, -350.000000, -380.002075 } } } },

    /** Patch 43 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 104.000397, -500.000214, -403.333008 },
                { 91.999603, -500.000214, -389.998047 },
                { 104.000397, -350.000000, -403.333008 } } } },

    /** Patch 44 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 120.000298, -500.000214, -393.337036 },
                { 104.000397, -500.000214, -403.333008 },
                { 120.000298, -350.000000, -393.337036 } } } },

    /** Patch 45 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 91.999603, -500.000214, -389.998047 },
                { 108.000198, -500.000214, -380.002075 },
                { 91.999603, -350.000000, -389.998047 } } } },

    /** Patch 46 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 108.000198, -666.666687, -380.002075 },
                { 120.000298, -666.666687, -393.337036 },
                { 108.000198, -533.332825, -380.002075 } } } },

    /** Patch 47 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 104.000397, -666.666687, -403.333008 },
                { 91.999603, -666.666687, -389.998047 },
                { 104.000397, -533.332825, -403.333008 } } } },

    /** Patch 48 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 120.000298, -666.666687, -393.337036 },
                { 104.000397, -666.666687, -403.333008 },
                { 120.000298, -533.332825, -393.337036 } } } },

    /** Patch 49 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 91.999603, -666.666687, -389.998047 },
                { 108.000198, -666.666687, -380.002075 },
                { 91.999603, -533.332825, -389.998047 } } } },

    /** Patch 50 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -308.000000, -500.000214, -640.002930 },
                { -295.999207, -500.000214, -653.331055 },
                { -308.000000, -350.000000, -640.002930 } } } },

    /** Patch 51 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -295.999207, -500.000214, -653.331055 },
                { -311.999786, -500.000214, -663.333984 },
                { -295.999207, -350.000000, -653.331055 } } } },

    /** Patch 52 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -311.999786, -500.000214, -663.333984 },
                { -323.999908, -500.000214, -649.999023 },
                { -311.999786, -350.000000, -663.333984 } } } },

    /** Patch 53 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -323.999908, -500.000214, -649.999023 },
                { -308.000000, -500.000214, -640.002930 },
                { -323.999908, -350.000000, -649.999023 } } } },

    /** Patch 54 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -308.000000, -666.666687, -640.002930 },
                { -295.999207, -666.666687, -653.331055 },
                { -308.000000, -533.332825, -640.002930 } } } },

    /** Patch 55 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -295.999207, -666.666687, -653.331055 },
                { -311.999786, -666.666687, -663.333984 },
                { -295.999207, -533.332825, -653.331055 } } } },

    /** Patch 56 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -311.999786, -666.666687, -663.333984 },
                { -323.999908, -666.666687, -649.999023 },
                { -311.999786, -533.332825, -663.333984 } } } },

    /** Patch 57 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -323.999908, -666.666687, -649.999023 },
                { -308.000000, -666.666687, -640.002930 },
                { -323.999908, -533.332825, -649.999023 } } } },

    /** Patch 58 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 433.708832, -566.666809, -144.088013 },
                { 298.470917, -566.666809, 97.593323 },
                { 424.914001, -534.328918, -147.503967 } } } },

    /** Patch 59 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 298.470917, -566.666809, 97.593323 },
                { 509.488007, -518.146973, 179.592041 },
                { 289.676117, -534.328918, 94.175232 } } } },

    /** Patch 60 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 644.725891, -518.146973, -62.090027 },
                { 433.708832, -566.666809, -144.088013 },
                { 635.931091, -485.809113, -65.505981 } } } },

    /** Patch 61 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 433.708832, -566.666809, -144.088013 },
                { 644.725891, -518.146973, -62.090027 },
                { 298.470917, -566.666809, 97.593323 } } } },

    /** Patch 62 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 509.488007, -518.146973, 179.592041 },
                { 644.725891, -518.146973, -62.090027 },
                { 500.693207, -485.809113, 176.174622 } } } },

    /** Patch 63 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 289.676117, -534.328918, 94.175232 },
                { 500.693207, -485.809113, 176.174622 },
                { 424.914001, -534.328918, -147.503967 } } } },

    /** Patch 64 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 188.395203, -633.333435, -257.802979 },
                { 356.457520, -695.767822, -192.500000 },
                { 36.252300, -633.333435, 14.087463 } } } },

    /** Patch 65 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 36.252300, -633.333435, 14.087463 },
                { 204.315292, -695.767822, 79.394714 },
                { 49.833000, -602.427002, 19.364807 } } } },

    /** Patch 66 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 356.457520, -695.767822, -192.500000 },
                { 188.395203, -633.333435, -257.802979 },
                { 370.038208, -664.861389, -187.221985 } } } },

    /** Patch 67 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 204.315292, -695.767822, 79.394714 },
                { 356.457520, -695.767822, -192.500000 },
                { 217.895294, -664.861389, 84.671997 } } } },

    /** Patch 68 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 188.395203, -633.333435, -257.802979 },
                { 36.252300, -633.333435, 14.087463 },
                { 201.975204, -602.427002, -252.524963 } } } },

    /** Patch 69 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 49.833000, -602.427002, 19.364807 },
                { 217.895294, -664.861389, 84.671997 },
                { 201.975204, -602.427002, -252.524963 } } } },

    /** Patch 70 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 650.722107, -800.002014, 10.894836 },
                { 580.434387, -800.002014, 136.506287 },
                { 533.348206, -541.608887, -85.043030 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 416.794708, -541.608887, 123.246887 },
                { 533.348206, -541.608887, -85.043030 },
                { 580.434387, -800.002014, 136.506287 } } } },

    /** Patch 71 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 650.722107, -800.002014, 10.894836 },
                { 533.348206, -541.608887, -85.043030 },
                { 689.917908, -800.002014, 29.029724 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 568.779419, -533.325073, -69.986023 },
                { 689.917908, -800.002014, 29.029724 },
                { 533.348206, -541.608887, -85.043030 } } } },

    /** Patch 72 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 416.794708, -541.608887, 123.246887 },
                { 580.434387, -800.002014, 136.506287 },
                { 453.411713, -533.325073, 136.186401 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 622.298584, -800.002014, 149.870728 },
                { 453.411713, -533.325073, 136.186401 },
                { 580.434387, -800.002014, 136.506287 } } } },

    /** Patch 73 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 622.298584, -800.002014, 149.870728 },
                { 689.917908, -800.002014, 29.029724 },
                { 453.411713, -533.325073, 136.186401 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 568.779419, -533.325073, -69.986023 },
                { 453.411713, -533.325073, 136.186401 },
                { 689.917908, -800.002014, 29.029724 } } } },

    /** Patch 74 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 67.053001, -800.002014, -25.025024 },
                { 172.239197, -800.002014, -213.003052 },
                { 147.604797, -670.852722, 34.824341 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 279.033295, -670.852722, -200.052979 },
                { 147.604797, -670.852722, 34.824341 },
                { 172.239197, -800.002014, -213.003052 } } } },

    /** Patch 75 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 279.033295, -670.852722, -200.052979 },
                { 172.239197, -800.002014, -213.003052 },
                { 244.124313, -658.100830, -212.344971 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 126.785400, -800.002014, -226.576050 },
                { 244.124313, -658.100830, -212.344971 },
                { 172.239197, -800.002014, -213.003052 } } } },

    /** Patch 76 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 67.053001, -800.002014, -25.025024 },
                { 147.604797, -670.852722, 34.824341 },
                { 25.356800, -800.002014, -45.317932 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.862701, -658.100830, 20.442810 },
                { 25.356800, -800.002014, -45.317932 },
                { 147.604797, -670.852722, 34.824341 } } } },

    /** Patch 77 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 126.785400, -800.002014, -226.576050 },
                { 25.356800, -800.002014, -45.317932 },
                { 244.124313, -658.100830, -212.344971 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.862701, -658.100830, 20.442810 },
                { 244.124313, -658.100830, -212.344971 },
                { 25.356800, -800.002014, -45.317932 } } } },

    /** Patch 78 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 185.036591, -766.667969, -183.266968 },
                { 102.610901, -766.667969, -35.966003 },
                { 623.555115, -766.667969, 20.008118 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 571.348389, -766.667969, 113.306213 },
                { 623.555115, -766.667969, 20.008118 },
                { 102.610901, -766.667969, -35.966003 } } } },

    /** Patch 79 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 641.077515, -800.002014, 28.130920 },
                { 161.664307, -800.002014, -194.102966 },
                { 623.555115, -766.667969, 20.008118 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 185.036591, -766.667969, -183.266968 },
                { 623.555115, -766.667969, 20.008118 },
                { 161.664307, -800.002014, -194.102966 } } } },

    /** Patch 80 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 77.628601, -800.002014, -43.924988 },
                { 590.078308, -800.002014, 119.270935 },
                { 102.610901, -766.667969, -35.966003 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 571.348389, -766.667969, 113.306213 },
                { 102.610901, -766.667969, -35.966003 },
                { 590.078308, -800.002014, 119.270935 } } } },

    /** Patch 81 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -66.666603, -999.999023 },
                { -840.000061, -66.666603, -999.999023 },
                { -360.000183, -66.666603, -700.000000 } } } },

    /** Patch 82 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -233.333084, -999.999023 },
                { -840.000061, -233.333084, -999.999023 },
                { -479.999817, -233.333084, -599.997925 } } } },

    /** Patch 83 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -266.666412, -599.997925 },
                { -840.000061, -266.666412, -599.997925 },
                { -479.999817, -266.666412, -999.999023 } } } },

    /** Patch 84 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -266.666412, -999.999023 },
                { -840.000061, -266.666412, -999.999023 },
                { -479.999817, -233.333084, -999.999023 } } } },

    /** Patch 85 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -266.666412, -999.999023 },
                { -840.000061, -266.666412, -599.997925 },
                { -840.000061, -233.333084, -999.999023 } } } },

    /** Patch 86 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -266.666412, -599.997925 },
                { -479.999817, -266.666412, -599.997925 },
                { -840.000061, -233.333084, -599.997925 } } } },

    /** Patch 87 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -266.666412, -599.997925 },
                { -479.999817, -266.666412, -999.999023 },
                { -479.999817, -233.333084, -599.997925 } } } },

    /** Patch 88 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -800.002014, -999.999023 },
                { -840.000061, -800.002014, -999.999023 },
                { -800.002014, -266.666412, -999.999023 } } } },

    /** Patch 89 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -800.002014, -999.999023 },
                { -840.000061, -800.002014, -966.665039 },
                { -840.000061, -266.666412, -999.999023 } } } },

    /** Patch 90 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -800.002014, -966.665039 },
                { -800.002014, -800.002014, -966.665039 },
                { -840.000061, -266.666412, -966.665039 } } } },

    /** Patch 91 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -800.002014, -966.665039 },
                { -800.002014, -800.002014, -999.999023 },
                { -800.002014, -266.666412, -966.665039 } } } },

    /** Patch 92 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -99.999901, -700.000000 },
                { -840.000061, -99.999901, -700.000000 },
                { -360.000183, -99.999901, -999.999023 } } } },

    /** Patch 93 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -99.999901, -999.999023 },
                { -840.000061, -99.999901, -999.999023 },
                { -360.000183, -66.666603, -999.999023 } } } },

    /** Patch 94 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -99.999901, -999.999023 },
                { -840.000061, -99.999901, -700.000000 },
                { -840.000061, -66.666603, -999.999023 } } } },

    /** Patch 95 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -99.999901, -700.000000 },
                { -360.000183, -99.999901, -700.000000 },
                { -840.000061, -66.666603, -700.000000 } } } },

    /** Patch 96 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -99.999901, -700.000000 },
                { -360.000183, -99.999901, -999.999023 },
                { -360.000183, -66.666603, -700.000000 } } } },

    /** Patch 97 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -233.333084, -999.999023 },
                { -840.000061, -233.333084, -999.999023 },
                { -800.002014, -99.999901, -999.999023 } } } },

    /** Patch 98 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -233.333084, -999.999023 },
                { -840.000061, -233.333084, -966.665039 },
                { -840.000061, -99.999901, -999.999023 } } } },

    /** Patch 99 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -233.333084, -966.665039 },
                { -800.002014, -233.333084, -966.665039 },
                { -840.000061, -99.999901, -966.665039 } } } },

    /** Patch 100 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -233.333084, -966.665039 },
                { -800.002014, -233.333084, -999.999023 },
                { -800.002014, -99.999901, -966.665039 } } } },

    /** Patch 101 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 400.000305, -999.999023 },
                { -840.000061, 400.000305, -999.999023 },
                { -800.002014, 400.000305, -966.665039 } } } },

    /** Patch 102 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 99.999901, -999.999023 },
                { -840.000061, 99.999901, -999.999023 },
                { -479.999817, 99.999901, -599.997925 } } } },

    /** Patch 103 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 66.666603, -599.997925 },
                { -840.000061, 66.666603, -599.997925 },
                { -479.999817, 66.666603, -999.999023 } } } },

    /** Patch 104 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 66.666603, -999.999023 },
                { -840.000061, 66.666603, -999.999023 },
                { -479.999817, 99.999901, -999.999023 } } } },

    /** Patch 105 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 66.666603, -999.999023 },
                { -840.000061, 66.666603, -599.997925 },
                { -840.000061, 99.999901, -999.999023 } } } },

    /** Patch 106 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 66.666603, -599.997925 },
                { -479.999817, 66.666603, -599.997925 },
                { -840.000061, 99.999901, -599.997925 } } } },

    /** Patch 107 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 66.666603, -599.997925 },
                { -479.999817, 66.666603, -999.999023 },
                { -479.999817, 99.999901, -599.997925 } } } },

    /** Patch 108 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -66.666603, -999.999023 },
                { -840.000061, -66.666603, -999.999023 },
                { -800.002014, 66.666603, -999.999023 } } } },

    /** Patch 109 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -66.666603, -999.999023 },
                { -840.000061, -66.666603, -966.665039 },
                { -840.000061, 66.666603, -999.999023 } } } },

    /** Patch 110 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -66.666603, -966.665039 },
                { -800.002014, -66.666603, -966.665039 },
                { -840.000061, 66.666603, -966.665039 } } } },

    /** Patch 111 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -66.666603, -966.665039 },
                { -800.002014, -66.666603, -999.999023 },
                { -800.002014, 66.666603, -966.665039 } } } },

    /** Patch 112 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 233.333084, -700.000000 },
                { -360.000183, 233.333084, -999.999023 },
                { -360.000183, 266.666412, -700.000000 } } } },

    /** Patch 113 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 266.666412, -999.999023 },
                { -840.000061, 266.666412, -999.999023 },
                { -360.000183, 266.666412, -700.000000 } } } },

    /** Patch 114 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 233.333084, -700.000000 },
                { -840.000061, 233.333084, -700.000000 },
                { -360.000183, 233.333084, -999.999023 } } } },

    /** Patch 115 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 233.333084, -999.999023 },
                { -840.000061, 233.333084, -999.999023 },
                { -360.000183, 266.666412, -999.999023 } } } },

    /** Patch 116 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 233.333084, -999.999023 },
                { -840.000061, 233.333084, -700.000000 },
                { -840.000061, 266.666412, -999.999023 } } } },

    /** Patch 117 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 233.333084, -700.000000 },
                { -360.000183, 233.333084, -700.000000 },
                { -840.000061, 266.666412, -700.000000 } } } },

    /** Patch 118 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 99.999901, -999.999023 },
                { -840.000061, 99.999901, -999.999023 },
                { -800.002014, 233.333084, -999.999023 } } } },

    /** Patch 119 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 99.999901, -999.999023 },
                { -840.000061, 99.999901, -966.665039 },
                { -840.000061, 233.333084, -999.999023 } } } },

    /** Patch 120 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 99.999901, -966.665039 },
                { -800.002014, 99.999901, -966.665039 },
                { -840.000061, 233.333084, -966.665039 } } } },

    /** Patch 121 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 99.999901, -966.665039 },
                { -800.002014, 99.999901, -999.999023 },
                { -800.002014, 233.333084, -966.665039 } } } },

    /** Patch 122 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 266.666412, -999.999023 },
                { -840.000061, 266.666412, -999.999023 },
                { -800.002014, 400.000305, -999.999023 } } } },

    /** Patch 123 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 266.666412, -999.999023 },
                { -840.000061, 266.666412, -966.665039 },
                { -840.000061, 400.000305, -999.999023 } } } },

    /** Patch 124 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 266.666412, -966.665039 },
                { -800.002014, 266.666412, -966.665039 },
                { -840.000061, 400.000305, -966.665039 } } } },

    /** Patch 125 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 266.666412, -966.665039 },
                { -800.002014, 266.666412, -999.999023 },
                { -800.002014, 400.000305, -966.665039 } } } },

    /** Patch 126 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 392.043396, 243.630798, -351.567993 },
                { 242.694885, 183.257202, -315.875000 },
                { 510.173309, 243.630798, -8.260010 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 398.162109, 198.350601, 18.507324 },
                { 510.173309, 243.630798, -8.260010 },
                { 242.694885, 183.257202, -315.875000 } } } },

    /** Patch 127 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 396.395996, 235.538788, -352.604004 },
                { 512.350281, 239.584793, -8.778015 },
                { 247.047501, 175.165207, -316.917969 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 400.338379, 194.304596, 17.987915 },
                { 247.047501, 175.165207, -316.917969 },
                { 512.350281, 239.584793, -8.778015 } } } },

    /** Patch 128 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 356.604492, 243.630798, -454.558960 },
                { 238.473892, 243.630798, -797.867065 },
                { 207.255997, 183.257202, -418.873047 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 126.462692, 198.350601, -771.098877 },
                { 207.255997, 183.257202, -418.873047 },
                { 238.473892, 243.630798, -797.867065 } } } },

    /** Patch 129 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 360.957123, 235.538788, -455.595093 },
                { 211.608612, 175.165207, -419.908936 },
                { 240.650208, 239.584793, -798.384888 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 128.639008, 194.304596, -771.616943 },
                { 240.650208, 239.584793, -798.384888 },
                { 211.608612, 175.165207, -419.908936 } } } },

    /** Patch 130 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -332.520996 },
                { 461.685730, 114.158798, -368.207031 },
                { 467.804382, 68.877899, 1.866211 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 579.816284, 114.158798, -24.899048 },
                { 467.804382, 68.877899, 1.866211 },
                { 461.685730, 114.158798, -368.207031 } } } },

    /** Patch 131 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 308.116211, 114.158798, -814.505981 },
                { 426.246765, 114.158798, -471.197998 },
                { 196.104996, 68.877899, -787.744995 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 276.898285, 53.784500, -435.511963 },
                { 196.104996, 68.877899, -787.744995 },
                { 426.246765, 114.158798, -471.197998 } } } },

    /** Patch 132 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 765.387024, 266.184113, -515.088013 },
                { 777.203064, 266.184113, -480.753052 },
                { 426.246765, 114.158798, -471.197998 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 461.685730, 114.158798, -368.207031 },
                { 426.246765, 114.158798, -471.197998 },
                { 777.203064, 266.184113, -480.753052 } } } },

    /** Patch 133 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 777.203064, 266.184113, -480.753052 },
                { 759.794006, 298.552094, -476.594971 },
                { 461.685730, 114.158798, -368.207031 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 426.864197, 178.894806, -359.884033 },
                { 461.685730, 114.158798, -368.207031 },
                { 759.794006, 298.552094, -476.594971 } } } },

    /** Patch 134 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 759.794006, 298.552094, -476.594971 },
                { 747.977966, 298.552094, -510.922974 },
                { 426.864197, 178.894806, -359.884033 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 391.425293, 178.894806, -462.881958 },
                { 426.864197, 178.894806, -359.884033 },
                { 747.977966, 298.552094, -510.922974 } } } },

    /** Patch 135 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 747.977966, 298.552094, -510.922974 },
                { 765.387024, 266.184113, -515.088013 },
                { 391.425293, 178.894806, -462.881958 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 426.246765, 114.158798, -471.197998 },
                { 391.425293, 178.894806, -462.881958 },
                { 765.387024, 266.184113, -515.088013 } } } },

    /** Patch 136 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 765.387024, 266.184113, -515.088013 },
                { 747.977966, 298.552094, -510.922974 },
                { 777.203064, 266.184113, -480.753052 } } } },

    /** Patch 137 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 426.246765, 114.158798, -471.197998 },
                { 276.898285, 53.784500, -435.511963 },
                { 391.425293, 178.894806, -462.881958 } } } },

    /** Patch 138 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -332.520996 },
                { 461.685730, 114.158798, -368.207031 },
                { 277.516418, 118.520500, -324.197998 } } } },

    /** Patch 139 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 391.425293, 178.894806, -462.881958 },
                { 242.076797, 118.520500, -427.188965 },
                { 426.864197, 178.894806, -359.884033 } } } },

    /** Patch 140 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 461.685730, 114.158798, -368.207031 },
                { 312.337189, 53.784500, -332.520996 },
                { 426.246765, 114.158798, -471.197998 } } } },

    /** Patch 141 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.669006, 54.875099, -390.179932 },
                { 254.482208, 54.875099, -355.844971 },
                { 225.257904, 87.243103, -386.021973 } } } },

    /** Patch 142 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.076797, 118.520500, -427.188965 },
                { 225.257904, 87.243103, -386.021973 },
                { 277.516418, 118.520500, -324.197998 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 237.071091, 87.243103, -351.687012 },
                { 277.516418, 118.520500, -324.197998 },
                { 225.257904, 87.243103, -386.021973 } } } },

    /** Patch 143 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.669006, 54.875099, -390.179932 },
                { 276.898285, 53.784500, -435.511963 },
                { 254.482208, 54.875099, -355.844971 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -332.520996 },
                { 254.482208, 54.875099, -355.844971 },
                { 276.898285, 53.784500, -435.511963 } } } },

    /** Patch 144 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.076797, 118.520500, -427.188965 },
                { 276.898285, 53.784500, -435.511963 },
                { 225.257904, 87.243103, -386.021973 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.669006, 54.875099, -390.179932 },
                { 225.257904, 87.243103, -386.021973 },
                { 276.898285, 53.784500, -435.511963 } } } },

    /** Patch 145 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -332.520996 },
                { 277.516418, 118.520500, -324.197998 },
                { 254.482208, 54.875099, -355.844971 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 237.071091, 87.243103, -351.687012 },
                { 254.482208, 54.875099, -355.844971 },
                { 277.516418, 118.520500, -324.197998 } } } },

    /** Patch 146 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 356.604492, 243.630798, -454.558960 },
                { 207.255997, 183.257202, -418.873047 },
                { 392.043396, 243.630798, -351.567993 } } } },

    /** Patch 147 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 752.408997, 298.552094, -498.050049 },
                { 675.556702, 272.411285, -479.688965 },
                { 717.583984, 363.288818, -489.727051 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 680.249512, 348.194702, -480.808960 },
                { 717.583984, 363.288818, -489.727051 },
                { 675.556702, 272.411285, -479.688965 } } } },

    /** Patch 148 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 678.510010, 272.411285, -471.106934 },
                { 755.362976, 298.552094, -489.468018 },
                { 683.202820, 348.194702, -472.227051 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 720.538025, 363.288818, -481.145020 },
                { 683.202820, 348.194702, -472.227051 },
                { 755.362976, 298.552094, -489.468018 } } } },

    /** Patch 149 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 758.855957, 278.322113, -513.526978 },
                { 681.231567, 248.135315, -504.265991 },
                { 723.421997, 278.322113, -616.518066 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 686.083313, 263.228699, -607.593018 },
                { 723.421997, 278.322113, -616.518066 },
                { 681.231567, 248.135315, -504.265991 } } } },

    /** Patch 150 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 676.879028, 256.227295, -503.223022 },
                { 754.509033, 286.414093, -512.484009 },
                { 681.730652, 271.320679, -606.557007 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 719.068054, 286.414093, -615.474976 },
                { 681.730652, 271.320679, -606.557007 },
                { 754.509033, 286.414093, -512.484009 } } } },

    /** Patch 151 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 766.317993, 286.414093, -478.156006 },
                { 694.598816, 256.227295, -451.730957 },
                { 801.759033, 286.414093, -375.164917 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 764.421021, 271.320679, -366.239990 },
                { 801.759033, 286.414093, -375.164917 },
                { 694.598816, 256.227295, -451.730957 } } } },

    /** Patch 152 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 698.951416, 248.135315, -452.767090 },
                { 770.671997, 278.322113, -479.192017 },
                { 768.775024, 263.228699, -367.283081 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 806.112976, 278.322113, -376.201050 },
                { 768.775024, 263.228699, -367.283081 },
                { 770.671997, 278.322113, -479.192017 } } } },

    /** Patch 153 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 577.639282, 118.204796, -24.380981 },
                { 457.333099, 122.250793, -367.164062 },
                { 465.628113, 72.923897, 2.386292 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 307.984589, 61.876499, -331.477905 },
                { 465.628113, 72.923897, 2.386292 },
                { 457.333099, 122.250793, -367.164062 } } } },

    /** Patch 154 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 272.545715, 61.876499, -434.468994 },
                { 421.894196, 122.250793, -470.161987 },
                { 193.928696, 72.923897, -787.219971 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 305.939880, 118.204796, -813.987915 },
                { 193.928696, 72.923897, -787.219971 },
                { 421.894196, 122.250793, -470.161987 } } } },

    /** Patch 155 **/
    { MODEL_RECTANGLE,
        { { 1.000000, 1.000000, 1.000000 }, { 300.000000, 300.000000, 300.000000 },
            { { -229.925507, 290.945190, -923.173950 },
                { -297.808014, 290.945190, -866.606934 },
                { -258.613617, 218.439896, -947.078979 } } } },

    /** Patch 156 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -329.024506, 295.358704, -920.905884 },
                { -343.368195, 259.105713, -932.854980 },
                { -297.808014, 290.945190, -866.606934 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -326.496094, 218.439896, -890.512085 },
                { -297.808014, 290.945190, -866.606934 },
                { -343.368195, 259.105713, -932.854980 } } } },

    /** Patch 157 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -343.368195, 259.105713, -932.854980 },
                { -309.427307, 259.105713, -961.141968 },
                { -326.496094, 218.439896, -890.512085 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -258.613617, 218.439896, -947.078979 },
                { -326.496094, 218.439896, -890.512085 },
                { -309.427307, 259.105713, -961.141968 } } } },

    /** Patch 158 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -309.427307, 259.105713, -961.141968 },
                { -295.082886, 295.358704, -949.185913 },
                { -258.613617, 218.439896, -947.078979 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -229.925507, 290.945190, -923.173950 },
                { -258.613617, 218.439896, -947.078979 },
                { -295.082886, 295.358704, -949.185913 } } } },

    /** Patch 159 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -295.082886, 295.358704, -949.185913 },
                { -329.024506, 295.358704, -920.905884 },
                { -229.925507, 290.945190, -923.173950 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -297.808014, 290.945190, -866.606934 },
                { -229.925507, 290.945190, -923.173950 },
                { -329.024506, 295.358704, -920.905884 } } } },

    /** Patch 160 **/
    { MODEL_RECTANGLE,
        { { 1.000000, 1.000000, 1.000000 }, { 300.000000, 300.000000, 300.000000 },
            { { -651.277222, 370.474304, -517.432983 },
                { -691.848511, 370.474304, -444.927002 },
                { -694.779419, 301.192505, -534.338013 } } } },

    /** Patch 161 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -742.811951, 379.820679, -486.801025 },
                { -764.567993, 345.179810, -495.250000 },
                { -691.848511, 370.474304, -444.927002 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -735.350037, 301.192505, -461.832031 },
                { -691.848511, 370.474304, -444.927002 },
                { -764.567993, 345.179810, -495.250000 } } } },

    /** Patch 162 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -764.567993, 345.179810, -495.250000 },
                { -744.281982, 345.179810, -531.502930 },
                { -735.350037, 301.192505, -461.832031 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -694.779419, 301.192505, -534.338013 },
                { -735.350037, 301.192505, -461.832031 },
                { -744.281982, 345.179810, -531.502930 } } } },

    /** Patch 163 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -744.281982, 345.179810, -531.502930 },
                { -722.525940, 379.820679, -523.046997 },
                { -694.779419, 301.192505, -534.338013 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -651.277222, 370.474304, -517.432983 },
                { -694.779419, 301.192505, -534.338013 },
                { -722.525940, 379.820679, -523.046997 } } } },

    /** Patch 164 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -722.525940, 379.820679, -523.046997 },
                { -742.811951, 379.820679, -486.801025 },
                { -651.277222, 370.474304, -517.432983 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -691.848511, 370.474304, -444.927002 },
                { -651.277222, 370.474304, -517.432983 },
                { -742.811951, 379.820679, -486.801025 } } } },

    /** Patch 165 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, 654.000183, -299.999023 },
                { 420.000031, 654.000183, -299.999023 },
                { -420.000031, 600.000061, -299.999023 } } } },

    /** Patch 166 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, 654.000183, -299.999023 },
                { 420.000031, 654.000183, 0.000000 },
                { 420.000031, 600.000061, -299.999023 } } } },

    /** Patch 167 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, 654.000183, 0.000000 },
                { -420.000031, 654.000183, 0.000000 },
                { 420.000031, 600.000061, 0.000000 } } } },

    /** Patch 168 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, 654.000183, 0.000000 },
                { -420.000031, 654.000183, -299.999023 },
                { -420.000031, 600.000061, 0.000000 } } } },

    /** Patch 169 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 120.000000, 120.000000, 120.000000 },
            { { -264.000092, 628.000122, -150.002991 },
                { -264.000092, 628.000122, -190.001038 },
                { -215.999710, 628.000122, -150.002991 } } } },

    /** Patch 170 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 60.000000, 60.000000, 60.000000 },
            { { -264.000092, 631.333496, -190.001038 },
                { -264.000092, 631.333496, -150.002991 },
                { -215.999710, 631.333496, -190.001038 } } } },

    /** Patch 171 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 120.000000, 120.000000, 120.000000 },
            { { -24.000200, 628.000122, -150.002991 },
                { -24.000200, 628.000122, -190.001038 },
                { 24.000200, 628.000122, -150.002991 } } } },

    /** Patch 172 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 60.000000, 60.000000, 60.000000 },
            { { -24.000200, 631.333496, -190.001038 },
                { -24.000200, 631.333496, -150.002991 },
                { 24.000200, 631.333496, -190.001038 } } } },

    /** Patch 173 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 120.000000, 120.000000, 120.000000 },
            { { 215.999710, 628.000122, -150.002991 },
                { 215.999710, 628.000122, -190.001038 },
                { 264.000092, 628.000122, -150.002991 } } } },

    /** Patch 174 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 60.000000, 60.000000, 60.000000 },
            { { 215.999710, 631.333496, -190.001038 },
                { 215.999710, 631.333496, -150.002991 },
                { 264.000092, 631.333496, -190.001038 } } } },




    { MODEL_NULL } } ;




ModelDataBase largeroom_model[] =
{

    /** Patch 0 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, 0.000000 },
                { 840.000061, -800.002014, 0.000000 },
                { 847.000000, -793.001953, 0.000000 } } } },

    /** Patch 1 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, -800.002014, -1701.000000 },
                { 847.000000, -800.002014, -1701.000000 },
                { -847.000000, 800.002014, -1701.000000 } } } },

    /** Patch 2 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, -1701.000000 },
                { 847.000000, -800.002014, 0.000000 },
                { 847.000000, 800.002014, -1701.000000 } } } },

    /** Patch 3 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, -800.002014, 0.000000 },
                { -847.000000, -800.002014, -1701.000000 },
                { -847.000000, 800.002014, 0.000000 } } } },

    /** Patch 4 **/
    { MODEL_RECTANGLE,
        { { 0.603921, 0.603921, 0.603921 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, 800.002014, 0.000000 },
                { -847.000000, 800.002014, 0.000000 },
                { 847.000000, 800.002014, -1701.000000 } } } },

    /** Patch 5 **/
    { MODEL_RECTANGLE,
        { { 0.501961, 0.501961, 0.501961 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, -1701.000000 },
                { -847.000000, -800.002014, -1701.000000 },
                { 847.000000, -800.002014, 0.000000 } } } },

    /** Patch 6 **/
    { MODEL_RECTANGLE,
        { { 0.576470, 0.521569, 0.521568 }, { 0.000000, 0.000000, 0.000000 },
            { { -269.149994, -216.666794, -1701.000000 },
                { 165.895096, -216.666794, -1701.000000 },
                { -269.149994, 281.666687, -1701.000000 } } } },

    /** Patch 7 **/
    { MODEL_RECTANGLE,
        { { 0.098034, 0.086275, 0.074510 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -159.999695, -414.019897 },
                { -840.000061, -159.999695, -1104.214966 },
                { -840.000061, 298.332977, -414.019897 } } } },

    /** Patch 8 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -84.000000, -350.000000, -1699.999023 },
                { -840.000061, -350.000000, -859.998962 },
                { -84.000000, -299.999695, -1699.999023 } } } },

    /** Patch 9 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -350.000000, -859.998962 },
                { -84.000000, -350.000000, -1699.999023 },
                { -336.000000, -350.000000, -544.999695 } } } },

    /** Patch 10 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -336.000000, -299.999695, -544.999695 },
                { 420.000031, -299.999695, -1384.999023 },
                { -840.000061, -299.999695, -859.998962 } } } },

    /** Patch 11 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -336.000000, -350.000000, -544.999695 },
                { 420.000031, -350.000000, -1384.999023 },
                { -336.000000, -299.999695, -544.999695 } } } },

    /** Patch 12 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, -350.000000, -1384.999023 },
                { -84.000000, -350.000000, -1699.999023 },
                { 420.000031, -299.999695, -1384.999023 } } } },

    /** Patch 13 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -350.000000, -859.998962 },
                { -336.000000, -350.000000, -544.999695 },
                { -840.000061, -299.999695, -859.998962 } } } },

    /** Patch 14 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -306.000092, -800.002014, -619.999817 },
                { -786.001953, -800.002014, -920.003052 },
                { -306.000092, -350.000000, -619.999817 } } } },

    /** Patch 15 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -341.999695, -800.002014, -579.999695 },
                { -306.000092, -800.002014, -619.999817 },
                { -341.999695, -350.000000, -579.999695 } } } },

    /** Patch 16 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -786.001953, -800.002014, -920.003052 },
                { -822.002930, -800.002014, -879.998047 },
                { -786.001953, -350.000000, -920.003052 } } } },

    /** Patch 17 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -822.002930, -800.002014, -879.998047 },
                { -341.999695, -800.002014, -579.999695 },
                { -822.002930, -350.000000, -879.998047 } } } },

    /** Patch 18 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -533.332825, -1045.001953 },
                { 420.000031, -533.332825, -1384.999023 },
                { 113.999901, -500.000214, -1045.001953 } } } },

    /** Patch 19 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, -533.332825, -1384.999023 },
                { -84.000000, -533.332825, -1699.999023 },
                { 420.000031, -500.000214, -1384.999023 } } } },

    /** Patch 20 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -500.000214, -1045.001953 },
                { 420.000031, -500.000214, -1384.999023 },
                { -390.000092, -500.000214, -1360.001953 } } } },

    /** Patch 21 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -533.332825, -1360.001953 },
                { -84.000000, -533.332825, -1699.999023 },
                { 113.999901, -533.332825, -1045.001953 } } } },

    /** Patch 22 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -84.000000, -533.332825, -1699.999023 },
                { -390.000092, -533.332825, -1360.001953 },
                { -84.000000, -500.000214, -1699.999023 } } } },

    /** Patch 23 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -533.332825, -1360.001953 },
                { 113.999901, -533.332825, -1045.001953 },
                { -390.000092, -500.000214, -1360.001953 } } } },

    /** Patch 24 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -700.000000, -1045.001953 },
                { 420.000031, -700.000000, -1384.999023 },
                { 113.999901, -666.666687, -1045.001953 } } } },

    /** Patch 25 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, -700.000000, -1384.999023 },
                { -84.000000, -700.000000, -1699.999023 },
                { 420.000031, -666.666687, -1384.999023 } } } },

    /** Patch 26 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.999901, -666.666687, -1045.001953 },
                { 420.000031, -666.666687, -1384.999023 },
                { -390.000092, -666.666687, -1360.001953 } } } },

    /** Patch 27 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -700.000000, -1360.001953 },
                { -84.000000, -700.000000, -1699.999023 },
                { 113.999901, -700.000000, -1045.001953 } } } },

    /** Patch 28 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -84.000000, -700.000000, -1699.999023 },
                { -390.000092, -700.000000, -1360.001953 },
                { -84.000000, -666.666687, -1699.999023 } } } },

    /** Patch 29 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -390.000092, -700.000000, -1360.001953 },
                { 113.999901, -700.000000, -1045.001953 },
                { -390.000092, -666.666687, -1360.001953 } } } },

    /** Patch 30 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -138.000107, -500.000214, -1640.001953 },
                { 341.999695, -500.000214, -1340.002930 },
                { -138.000107, -350.000000, -1640.001953 } } } },

    /** Patch 31 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -500.000214, -1340.002930 },
                { 378.000000, -500.000214, -1380.000977 },
                { 341.999695, -350.000000, -1340.002930 } } } },

    /** Patch 32 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 378.000000, -500.000214, -1380.000977 },
                { -101.999802, -500.000214, -1680.000122 },
                { 378.000000, -350.000000, -1380.000977 } } } },

    /** Patch 33 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -101.999802, -500.000214, -1680.000122 },
                { -138.000107, -500.000214, -1640.001953 },
                { -101.999802, -350.000000, -1680.000122 } } } },

    /** Patch 34 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -138.000107, -666.666687, -1640.001953 },
                { 341.999695, -666.666687, -1340.002930 },
                { -138.000107, -533.332825, -1640.001953 } } } },

    /** Patch 35 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -666.666687, -1340.002930 },
                { 378.000000, -666.666687, -1380.000977 },
                { 341.999695, -533.332825, -1340.002930 } } } },

    /** Patch 36 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 378.000000, -666.666687, -1380.000977 },
                { -101.999802, -666.666687, -1680.000122 },
                { 378.000000, -533.332825, -1380.000977 } } } },

    /** Patch 37 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -101.999802, -666.666687, -1680.000122 },
                { -138.000107, -666.666687, -1640.001953 },
                { -101.999802, -533.332825, -1680.000122 } } } },

    /** Patch 38 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -138.000107, -800.002014, -1640.001953 },
                { 341.999695, -800.002014, -1340.002930 },
                { -138.000107, -700.000000, -1640.001953 } } } },

    /** Patch 39 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -800.002014, -1340.002930 },
                { 378.000000, -800.002014, -1380.000977 },
                { 341.999695, -700.000000, -1340.002930 } } } },

    /** Patch 40 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 378.000000, -800.002014, -1380.000977 },
                { -101.999802, -800.002014, -1680.000122 },
                { 378.000000, -700.000000, -1380.000977 } } } },

    /** Patch 41 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -101.999802, -800.002014, -1680.000122 },
                { -138.000107, -800.002014, -1640.001953 },
                { -101.999802, -700.000000, -1680.000122 } } } },

    /** Patch 42 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 108.000198, -500.000214, -1080.002075 },
                { 120.000298, -500.000214, -1093.337036 },
                { 108.000198, -350.000000, -1080.002075 } } } },

    /** Patch 43 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 104.000397, -500.000214, -1103.333008 },
                { 91.999603, -500.000214, -1089.998047 },
                { 104.000397, -350.000000, -1103.333008 } } } },

    /** Patch 44 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 120.000298, -500.000214, -1093.337036 },
                { 104.000397, -500.000214, -1103.333008 },
                { 120.000298, -350.000000, -1093.337036 } } } },

    /** Patch 45 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 91.999603, -500.000214, -1089.998047 },
                { 108.000198, -500.000214, -1080.002075 },
                { 91.999603, -350.000000, -1089.998047 } } } },

    /** Patch 46 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 108.000198, -666.666687, -1080.002075 },
                { 120.000298, -666.666687, -1093.337036 },
                { 108.000198, -533.332825, -1080.002075 } } } },

    /** Patch 47 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 104.000397, -666.666687, -1103.333008 },
                { 91.999603, -666.666687, -1089.998047 },
                { 104.000397, -533.332825, -1103.333008 } } } },

    /** Patch 48 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 120.000298, -666.666687, -1093.337036 },
                { 104.000397, -666.666687, -1103.333008 },
                { 120.000298, -533.332825, -1093.337036 } } } },

    /** Patch 49 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 91.999603, -666.666687, -1089.998047 },
                { 108.000198, -666.666687, -1080.002075 },
                { 91.999603, -533.332825, -1089.998047 } } } },

    /** Patch 50 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -308.000000, -500.000214, -1340.002930 },
                { -295.999207, -500.000214, -1353.331055 },
                { -308.000000, -350.000000, -1340.002930 } } } },

    /** Patch 51 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -295.999207, -500.000214, -1353.331055 },
                { -311.999786, -500.000214, -1363.333984 },
                { -295.999207, -350.000000, -1353.331055 } } } },

    /** Patch 52 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -311.999786, -500.000214, -1363.333984 },
                { -323.999908, -500.000214, -1349.999023 },
                { -311.999786, -350.000000, -1363.333984 } } } },

    /** Patch 53 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -323.999908, -500.000214, -1349.999023 },
                { -308.000000, -500.000214, -1340.002930 },
                { -323.999908, -350.000000, -1349.999023 } } } },

    /** Patch 54 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -308.000000, -666.666687, -1340.002930 },
                { -295.999207, -666.666687, -1353.331055 },
                { -308.000000, -533.332825, -1340.002930 } } } },

    /** Patch 55 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -295.999207, -666.666687, -1353.331055 },
                { -311.999786, -666.666687, -1363.333984 },
                { -295.999207, -533.332825, -1353.331055 } } } },

    /** Patch 56 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -311.999786, -666.666687, -1363.333984 },
                { -323.999908, -666.666687, -1349.999023 },
                { -311.999786, -533.332825, -1363.333984 } } } },

    /** Patch 57 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -323.999908, -666.666687, -1349.999023 },
                { -308.000000, -666.666687, -1340.002930 },
                { -323.999908, -533.332825, -1349.999023 } } } },

    /** Patch 58 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 433.708832, -566.666809, -844.088013 },
                { 298.470917, -566.666809, -602.406677 },
                { 424.914001, -534.328918, -847.503967 } } } },

    /** Patch 59 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 298.470917, -566.666809, -602.406677 },
                { 509.488007, -518.146973, -520.407959 },
                { 289.676117, -534.328918, -605.824768 } } } },

    /** Patch 60 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 644.725891, -518.146973, -762.090027 },
                { 433.708832, -566.666809, -844.088013 },
                { 635.931091, -485.809113, -765.505981 } } } },

    /** Patch 61 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 433.708832, -566.666809, -844.088013 },
                { 644.725891, -518.146973, -762.090027 },
                { 298.470917, -566.666809, -602.406677 } } } },

    /** Patch 62 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 509.488007, -518.146973, -520.407959 },
                { 644.725891, -518.146973, -762.090027 },
                { 500.693207, -485.809113, -523.825378 } } } },

    /** Patch 63 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 289.676117, -534.328918, -605.824768 },
                { 500.693207, -485.809113, -523.825378 },
                { 424.914001, -534.328918, -847.503967 } } } },

    /** Patch 64 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 188.395203, -633.333435, -957.802979 },
                { 356.457520, -695.767822, -892.500000 },
                { 36.252300, -633.333435, -685.912537 } } } },

    /** Patch 65 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 36.252300, -633.333435, -685.912537 },
                { 204.315292, -695.767822, -620.605286 },
                { 49.833000, -602.427002, -680.635193 } } } },

    /** Patch 66 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 356.457520, -695.767822, -892.500000 },
                { 188.395203, -633.333435, -957.802979 },
                { 370.038208, -664.861389, -887.221985 } } } },

    /** Patch 67 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 204.315292, -695.767822, -620.605286 },
                { 356.457520, -695.767822, -892.500000 },
                { 217.895294, -664.861389, -615.328003 } } } },

    /** Patch 68 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 188.395203, -633.333435, -957.802979 },
                { 36.252300, -633.333435, -685.912537 },
                { 201.975204, -602.427002, -952.524963 } } } },

    /** Patch 69 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { 49.833000, -602.427002, -680.635193 },
                { 217.895294, -664.861389, -615.328003 },
                { 201.975204, -602.427002, -952.524963 } } } },

    /** Patch 70 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 650.722107, -800.002014, -689.105164 },
                { 580.434387, -800.002014, -563.493713 },
                { 533.348206, -541.608887, -785.043030 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 416.794708, -541.608887, -576.753113 },
                { 533.348206, -541.608887, -785.043030 },
                { 580.434387, -800.002014, -563.493713 } } } },

    /** Patch 71 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 650.722107, -800.002014, -689.105164 },
                { 533.348206, -541.608887, -785.043030 },
                { 689.917908, -800.002014, -670.970276 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 568.779419, -533.325073, -769.986023 },
                { 689.917908, -800.002014, -670.970276 },
                { 533.348206, -541.608887, -785.043030 } } } },

    /** Patch 72 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 416.794708, -541.608887, -576.753113 },
                { 580.434387, -800.002014, -563.493713 },
                { 453.411713, -533.325073, -563.813599 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 622.298584, -800.002014, -550.129272 },
                { 453.411713, -533.325073, -563.813599 },
                { 580.434387, -800.002014, -563.493713 } } } },

    /** Patch 73 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 622.298584, -800.002014, -550.129272 },
                { 689.917908, -800.002014, -670.970276 },
                { 453.411713, -533.325073, -563.813599 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 568.779419, -533.325073, -769.986023 },
                { 453.411713, -533.325073, -563.813599 },
                { 689.917908, -800.002014, -670.970276 } } } },

    /** Patch 74 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 67.053001, -800.002014, -725.025024 },
                { 172.239197, -800.002014, -913.003052 },
                { 147.604797, -670.852722, -665.175659 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 279.033295, -670.852722, -900.052979 },
                { 147.604797, -670.852722, -665.175659 },
                { 172.239197, -800.002014, -913.003052 } } } },

    /** Patch 75 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 279.033295, -670.852722, -900.052979 },
                { 172.239197, -800.002014, -913.003052 },
                { 244.124313, -658.100830, -912.344971 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 126.785400, -800.002014, -926.576050 },
                { 244.124313, -658.100830, -912.344971 },
                { 172.239197, -800.002014, -913.003052 } } } },

    /** Patch 76 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 67.053001, -800.002014, -725.025024 },
                { 147.604797, -670.852722, -665.175659 },
                { 25.356800, -800.002014, -745.317932 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.862701, -658.100830, -679.557190 },
                { 25.356800, -800.002014, -745.317932 },
                { 147.604797, -670.852722, -665.175659 } } } },

    /** Patch 77 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 126.785400, -800.002014, -926.576050 },
                { 25.356800, -800.002014, -745.317932 },
                { 244.124313, -658.100830, -912.344971 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 113.862701, -658.100830, -679.557190 },
                { 244.124313, -658.100830, -912.344971 },
                { 25.356800, -800.002014, -745.317932 } } } },

    /** Patch 78 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 185.036591, -766.667969, -883.266968 },
                { 102.610901, -766.667969, -735.966003 },
                { 623.555115, -766.667969, -679.991882 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 571.348389, -766.667969, -586.693787 },
                { 623.555115, -766.667969, -679.991882 },
                { 102.610901, -766.667969, -735.966003 } } } },

    /** Patch 79 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 641.077515, -800.002014, -671.869080 },
                { 161.664307, -800.002014, -894.102966 },
                { 623.555115, -766.667969, -679.991882 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 185.036591, -766.667969, -883.266968 },
                { 623.555115, -766.667969, -679.991882 },
                { 161.664307, -800.002014, -894.102966 } } } },

    /** Patch 80 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 77.628601, -800.002014, -743.924988 },
                { 590.078308, -800.002014, -580.729065 },
                { 102.610901, -766.667969, -735.966003 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { 571.348389, -766.667969, -586.693787 },
                { 102.610901, -766.667969, -735.966003 },
                { 590.078308, -800.002014, -580.729065 } } } },

    /** Patch 81 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -66.666603, -1699.999023 },
                { -840.000061, -66.666603, -1699.999023 },
                { -360.000183, -66.666603, -1400.000000 } } } },

    /** Patch 82 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -233.333084, -1699.999023 },
                { -840.000061, -233.333084, -1699.999023 },
                { -479.999817, -233.333084, -1299.997925 } } } },

    /** Patch 83 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -266.666412, -1299.997925 },
                { -840.000061, -266.666412, -1299.997925 },
                { -479.999817, -266.666412, -1699.999023 } } } },

    /** Patch 84 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -266.666412, -1699.999023 },
                { -840.000061, -266.666412, -1699.999023 },
                { -479.999817, -233.333084, -1699.999023 } } } },

    /** Patch 85 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -266.666412, -1699.999023 },
                { -840.000061, -266.666412, -1299.997925 },
                { -840.000061, -233.333084, -1699.999023 } } } },

    /** Patch 86 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -266.666412, -1299.997925 },
                { -479.999817, -266.666412, -1299.997925 },
                { -840.000061, -233.333084, -1299.997925 } } } },

    /** Patch 87 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, -266.666412, -1299.997925 },
                { -479.999817, -266.666412, -1699.999023 },
                { -479.999817, -233.333084, -1299.997925 } } } },

    /** Patch 88 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -800.002014, -1699.999023 },
                { -840.000061, -800.002014, -1699.999023 },
                { -800.002014, -266.666412, -1699.999023 } } } },

    /** Patch 89 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -800.002014, -1699.999023 },
                { -840.000061, -800.002014, -1666.665039 },
                { -840.000061, -266.666412, -1699.999023 } } } },

    /** Patch 90 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -800.002014, -1666.665039 },
                { -800.002014, -800.002014, -1666.665039 },
                { -840.000061, -266.666412, -1666.665039 } } } },

    /** Patch 91 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -800.002014, -1666.665039 },
                { -800.002014, -800.002014, -1699.999023 },
                { -800.002014, -266.666412, -1666.665039 } } } },

    /** Patch 92 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -99.999901, -1400.000000 },
                { -840.000061, -99.999901, -1400.000000 },
                { -360.000183, -99.999901, -1699.999023 } } } },

    /** Patch 93 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -99.999901, -1699.999023 },
                { -840.000061, -99.999901, -1699.999023 },
                { -360.000183, -66.666603, -1699.999023 } } } },

    /** Patch 94 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -99.999901, -1699.999023 },
                { -840.000061, -99.999901, -1400.000000 },
                { -840.000061, -66.666603, -1699.999023 } } } },

    /** Patch 95 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -99.999901, -1400.000000 },
                { -360.000183, -99.999901, -1400.000000 },
                { -840.000061, -66.666603, -1400.000000 } } } },

    /** Patch 96 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, -99.999901, -1400.000000 },
                { -360.000183, -99.999901, -1699.999023 },
                { -360.000183, -66.666603, -1400.000000 } } } },

    /** Patch 97 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -233.333084, -1699.999023 },
                { -840.000061, -233.333084, -1699.999023 },
                { -800.002014, -99.999901, -1699.999023 } } } },

    /** Patch 98 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -233.333084, -1699.999023 },
                { -840.000061, -233.333084, -1666.665039 },
                { -840.000061, -99.999901, -1699.999023 } } } },

    /** Patch 99 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -233.333084, -1666.665039 },
                { -800.002014, -233.333084, -1666.665039 },
                { -840.000061, -99.999901, -1666.665039 } } } },

    /** Patch 100 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -233.333084, -1666.665039 },
                { -800.002014, -233.333084, -1699.999023 },
                { -800.002014, -99.999901, -1666.665039 } } } },

    /** Patch 101 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 400.000305, -1699.999023 },
                { -840.000061, 400.000305, -1699.999023 },
                { -800.002014, 400.000305, -1666.665039 } } } },

    /** Patch 102 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 99.999901, -1699.999023 },
                { -840.000061, 99.999901, -1699.999023 },
                { -479.999817, 99.999901, -1299.997925 } } } },

    /** Patch 103 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 66.666603, -1299.997925 },
                { -840.000061, 66.666603, -1299.997925 },
                { -479.999817, 66.666603, -1699.999023 } } } },

    /** Patch 104 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 66.666603, -1699.999023 },
                { -840.000061, 66.666603, -1699.999023 },
                { -479.999817, 99.999901, -1699.999023 } } } },

    /** Patch 105 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 66.666603, -1699.999023 },
                { -840.000061, 66.666603, -1299.997925 },
                { -840.000061, 99.999901, -1699.999023 } } } },

    /** Patch 106 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 66.666603, -1299.997925 },
                { -479.999817, 66.666603, -1299.997925 },
                { -840.000061, 99.999901, -1299.997925 } } } },

    /** Patch 107 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -479.999817, 66.666603, -1299.997925 },
                { -479.999817, 66.666603, -1699.999023 },
                { -479.999817, 99.999901, -1299.997925 } } } },

    /** Patch 108 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -66.666603, -1699.999023 },
                { -840.000061, -66.666603, -1699.999023 },
                { -800.002014, 66.666603, -1699.999023 } } } },

    /** Patch 109 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -66.666603, -1699.999023 },
                { -840.000061, -66.666603, -1666.665039 },
                { -840.000061, 66.666603, -1699.999023 } } } },

    /** Patch 110 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, -66.666603, -1666.665039 },
                { -800.002014, -66.666603, -1666.665039 },
                { -840.000061, 66.666603, -1666.665039 } } } },

    /** Patch 111 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, -66.666603, -1666.665039 },
                { -800.002014, -66.666603, -1699.999023 },
                { -800.002014, 66.666603, -1666.665039 } } } },

    /** Patch 112 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 233.333084, -1400.000000 },
                { -360.000183, 233.333084, -1699.999023 },
                { -360.000183, 266.666412, -1400.000000 } } } },

    /** Patch 113 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 266.666412, -1699.999023 },
                { -840.000061, 266.666412, -1699.999023 },
                { -360.000183, 266.666412, -1400.000000 } } } },

    /** Patch 114 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 233.333084, -1400.000000 },
                { -840.000061, 233.333084, -1400.000000 },
                { -360.000183, 233.333084, -1699.999023 } } } },

    /** Patch 115 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -360.000183, 233.333084, -1699.999023 },
                { -840.000061, 233.333084, -1699.999023 },
                { -360.000183, 266.666412, -1699.999023 } } } },

    /** Patch 116 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 233.333084, -1699.999023 },
                { -840.000061, 233.333084, -1400.000000 },
                { -840.000061, 266.666412, -1699.999023 } } } },

    /** Patch 117 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 233.333084, -1400.000000 },
                { -360.000183, 233.333084, -1400.000000 },
                { -840.000061, 266.666412, -1400.000000 } } } },

    /** Patch 118 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 99.999901, -1699.999023 },
                { -840.000061, 99.999901, -1699.999023 },
                { -800.002014, 233.333084, -1699.999023 } } } },

    /** Patch 119 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 99.999901, -1699.999023 },
                { -840.000061, 99.999901, -1666.665039 },
                { -840.000061, 233.333084, -1699.999023 } } } },

    /** Patch 120 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 99.999901, -1666.665039 },
                { -800.002014, 99.999901, -1666.665039 },
                { -840.000061, 233.333084, -1666.665039 } } } },

    /** Patch 121 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 99.999901, -1666.665039 },
                { -800.002014, 99.999901, -1699.999023 },
                { -800.002014, 233.333084, -1666.665039 } } } },

    /** Patch 122 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 266.666412, -1699.999023 },
                { -840.000061, 266.666412, -1699.999023 },
                { -800.002014, 400.000305, -1699.999023 } } } },

    /** Patch 123 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 266.666412, -1699.999023 },
                { -840.000061, 266.666412, -1666.665039 },
                { -840.000061, 400.000305, -1699.999023 } } } },

    /** Patch 124 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -840.000061, 266.666412, -1666.665039 },
                { -800.002014, 266.666412, -1666.665039 },
                { -840.000061, 400.000305, -1666.665039 } } } },

    /** Patch 125 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -800.002014, 266.666412, -1666.665039 },
                { -800.002014, 266.666412, -1699.999023 },
                { -800.002014, 400.000305, -1666.665039 } } } },

    /** Patch 126 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 392.043396, 243.630798, -1051.567993 },
                { 242.694885, 183.257202, -1015.875000 },
                { 510.173309, 243.630798, -708.260010 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 398.162109, 198.350601, -681.492676 },
                { 510.173309, 243.630798, -708.260010 },
                { 242.694885, 183.257202, -1015.875000 } } } },

    /** Patch 127 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 396.395996, 235.538788, -1052.604004 },
                { 512.350281, 239.584793, -708.778015 },
                { 247.047501, 175.165207, -1016.917969 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 400.338379, 194.304596, -682.012085 },
                { 247.047501, 175.165207, -1016.917969 },
                { 512.350281, 239.584793, -708.778015 } } } },

    /** Patch 128 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 356.604492, 243.630798, -1154.558960 },
                { 238.473892, 243.630798, -1497.867065 },
                { 207.255997, 183.257202, -1118.873047 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 126.462692, 198.350601, -1471.098877 },
                { 207.255997, 183.257202, -1118.873047 },
                { 238.473892, 243.630798, -1497.867065 } } } },

    /** Patch 129 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 360.957123, 235.538788, -1155.595093 },
                { 211.608612, 175.165207, -1119.908936 },
                { 240.650208, 239.584793, -1498.384888 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 128.639008, 194.304596, -1471.616943 },
                { 240.650208, 239.584793, -1498.384888 },
                { 211.608612, 175.165207, -1119.908936 } } } },

    /** Patch 130 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -1032.520996 },
                { 461.685730, 114.158798, -1068.207031 },
                { 467.804382, 68.877899, -698.133789 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 579.816284, 114.158798, -724.899048 },
                { 467.804382, 68.877899, -698.133789 },
                { 461.685730, 114.158798, -1068.207031 } } } },

    /** Patch 131 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 308.116211, 114.158798, -1514.505981 },
                { 426.246765, 114.158798, -1171.197998 },
                { 196.104996, 68.877899, -1487.744995 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 276.898285, 53.784500, -1135.511963 },
                { 196.104996, 68.877899, -1487.744995 },
                { 426.246765, 114.158798, -1171.197998 } } } },

    /** Patch 132 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 765.387024, 266.184113, -1215.088013 },
                { 777.203064, 266.184113, -1180.753052 },
                { 426.246765, 114.158798, -1171.197998 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 461.685730, 114.158798, -1068.207031 },
                { 426.246765, 114.158798, -1171.197998 },
                { 777.203064, 266.184113, -1180.753052 } } } },

    /** Patch 133 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 777.203064, 266.184113, -1180.753052 },
                { 759.794006, 298.552094, -1176.594971 },
                { 461.685730, 114.158798, -1068.207031 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 426.864197, 178.894806, -1059.884033 },
                { 461.685730, 114.158798, -1068.207031 },
                { 759.794006, 298.552094, -1176.594971 } } } },

    /** Patch 134 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 759.794006, 298.552094, -1176.594971 },
                { 747.977966, 298.552094, -1210.922974 },
                { 426.864197, 178.894806, -1059.884033 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 391.425293, 178.894806, -1162.881958 },
                { 426.864197, 178.894806, -1059.884033 },
                { 747.977966, 298.552094, -1210.922974 } } } },

    /** Patch 135 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 747.977966, 298.552094, -1210.922974 },
                { 765.387024, 266.184113, -1215.088013 },
                { 391.425293, 178.894806, -1162.881958 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 426.246765, 114.158798, -1171.197998 },
                { 391.425293, 178.894806, -1162.881958 },
                { 765.387024, 266.184113, -1215.088013 } } } },

    /** Patch 136 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 765.387024, 266.184113, -1215.088013 },
                { 747.977966, 298.552094, -1210.922974 },
                { 777.203064, 266.184113, -1180.753052 } } } },

    /** Patch 137 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 426.246765, 114.158798, -1171.197998 },
                { 276.898285, 53.784500, -1135.511963 },
                { 391.425293, 178.894806, -1162.881958 } } } },

    /** Patch 138 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -1032.520996 },
                { 461.685730, 114.158798, -1068.207031 },
                { 277.516418, 118.520500, -1024.197998 } } } },

    /** Patch 139 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 391.425293, 178.894806, -1162.881958 },
                { 242.076797, 118.520500, -1127.188965 },
                { 426.864197, 178.894806, -1059.884033 } } } },

    /** Patch 140 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 461.685730, 114.158798, -1068.207031 },
                { 312.337189, 53.784500, -1032.520996 },
                { 426.246765, 114.158798, -1171.197998 } } } },

    /** Patch 141 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.669006, 54.875099, -1090.179932 },
                { 254.482208, 54.875099, -1055.844971 },
                { 225.257904, 87.243103, -1086.021973 } } } },

    /** Patch 142 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.076797, 118.520500, -1127.188965 },
                { 225.257904, 87.243103, -1086.021973 },
                { 277.516418, 118.520500, -1024.197998 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 237.071091, 87.243103, -1051.687012 },
                { 277.516418, 118.520500, -1024.197998 },
                { 225.257904, 87.243103, -1086.021973 } } } },

    /** Patch 143 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.669006, 54.875099, -1090.179932 },
                { 276.898285, 53.784500, -1135.511963 },
                { 254.482208, 54.875099, -1055.844971 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -1032.520996 },
                { 254.482208, 54.875099, -1055.844971 },
                { 276.898285, 53.784500, -1135.511963 } } } },

    /** Patch 144 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.076797, 118.520500, -1127.188965 },
                { 276.898285, 53.784500, -1135.511963 },
                { 225.257904, 87.243103, -1086.021973 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 242.669006, 54.875099, -1090.179932 },
                { 225.257904, 87.243103, -1086.021973 },
                { 276.898285, 53.784500, -1135.511963 } } } },

    /** Patch 145 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 312.337189, 53.784500, -1032.520996 },
                { 277.516418, 118.520500, -1024.197998 },
                { 254.482208, 54.875099, -1055.844971 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 237.071091, 87.243103, -1051.687012 },
                { 254.482208, 54.875099, -1055.844971 },
                { 277.516418, 118.520500, -1024.197998 } } } },

    /** Patch 146 **/
    { MODEL_RECTANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 356.604492, 243.630798, -1154.558960 },
                { 207.255997, 183.257202, -1118.873047 },
                { 392.043396, 243.630798, -1051.567993 } } } },

    /** Patch 147 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 752.408997, 298.552094, -1198.050049 },
                { 675.556702, 272.411285, -1179.688965 },
                { 717.583984, 363.288818, -1189.727051 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 680.249512, 348.194702, -1180.808960 },
                { 717.583984, 363.288818, -1189.727051 },
                { 675.556702, 272.411285, -1179.688965 } } } },

    /** Patch 148 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 678.510010, 272.411285, -1171.106934 },
                { 755.362976, 298.552094, -1189.468018 },
                { 683.202820, 348.194702, -1172.227051 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 720.538025, 363.288818, -1181.145020 },
                { 683.202820, 348.194702, -1172.227051 },
                { 755.362976, 298.552094, -1189.468018 } } } },

    /** Patch 149 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 758.855957, 278.322113, -1213.526978 },
                { 681.231567, 248.135315, -1204.265991 },
                { 723.421997, 278.322113, -1316.518066 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 686.083313, 263.228699, -1307.593018 },
                { 723.421997, 278.322113, -1316.518066 },
                { 681.231567, 248.135315, -1204.265991 } } } },

    /** Patch 150 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 676.879028, 256.227295, -1203.223022 },
                { 754.509033, 286.414093, -1212.484009 },
                { 681.730652, 271.320679, -1306.557007 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 719.068054, 286.414093, -1315.474976 },
                { 681.730652, 271.320679, -1306.557007 },
                { 754.509033, 286.414093, -1212.484009 } } } },

    /** Patch 151 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 766.317993, 286.414093, -1178.156006 },
                { 694.598816, 256.227295, -1151.730957 },
                { 801.759033, 286.414093, -1075.164917 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 764.421021, 271.320679, -1066.239990 },
                { 801.759033, 286.414093, -1075.164917 },
                { 694.598816, 256.227295, -1151.730957 } } } },

    /** Patch 152 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 698.951416, 248.135315, -1152.767090 },
                { 770.671997, 278.322113, -1179.192017 },
                { 768.775024, 263.228699, -1067.283081 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 806.112976, 278.322113, -1076.201050 },
                { 768.775024, 263.228699, -1067.283081 },
                { 770.671997, 278.322113, -1179.192017 } } } },

    /** Patch 153 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 577.639282, 118.204796, -724.380981 },
                { 457.333099, 122.250793, -1067.164062 },
                { 465.628113, 72.923897, -697.613708 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 307.984589, 61.876499, -1031.477905 },
                { 465.628113, 72.923897, -697.613708 },
                { 457.333099, 122.250793, -1067.164062 } } } },

    /** Patch 154 **/
    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 272.545715, 61.876499, -1134.468994 },
                { 421.894196, 122.250793, -1170.161987 },
                { 193.928696, 72.923897, -1487.219971 } } } },

    { MODEL_TRIANGLE,
        { { 0.941176, 0.125490, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 305.939880, 118.204796, -1513.987915 },
                { 193.928696, 72.923897, -1487.219971 },
                { 421.894196, 122.250793, -1170.161987 } } } },

    /** Patch 155 **/
    { MODEL_RECTANGLE,
        { { 1.000000, 1.000000, 1.000000 }, { 400.000000, 400.000000, 400.000000 },
            { { -229.925507, 290.945190, -1623.173950 },
                { -297.808014, 290.945190, -1566.606934 },
                { -258.613617, 218.439896, -1647.078979 } } } },

    /** Patch 156 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -329.024506, 295.358704, -1620.905884 },
                { -343.368195, 259.105713, -1632.854980 },
                { -297.808014, 290.945190, -1566.606934 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -326.496094, 218.439896, -1590.512085 },
                { -297.808014, 290.945190, -1566.606934 },
                { -343.368195, 259.105713, -1632.854980 } } } },

    /** Patch 157 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -343.368195, 259.105713, -1632.854980 },
                { -309.427307, 259.105713, -1661.141968 },
                { -326.496094, 218.439896, -1590.512085 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -258.613617, 218.439896, -1647.078979 },
                { -326.496094, 218.439896, -1590.512085 },
                { -309.427307, 259.105713, -1661.141968 } } } },

    /** Patch 158 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -309.427307, 259.105713, -1661.141968 },
                { -295.082886, 295.358704, -1649.185913 },
                { -258.613617, 218.439896, -1647.078979 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -229.925507, 290.945190, -1623.173950 },
                { -258.613617, 218.439896, -1647.078979 },
                { -295.082886, 295.358704, -1649.185913 } } } },

    /** Patch 159 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -295.082886, 295.358704, -1649.185913 },
                { -329.024506, 295.358704, -1620.905884 },
                { -229.925507, 290.945190, -1623.173950 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -297.808014, 290.945190, -1566.606934 },
                { -229.925507, 290.945190, -1623.173950 },
                { -329.024506, 295.358704, -1620.905884 } } } },

    /** Patch 160 **/
    { MODEL_RECTANGLE,
        { { 1.000000, 1.000000, 1.000000 }, { 400.000000, 400.000000, 400.000000 },
            { { -651.277222, 370.474304, -1217.432983 },
                { -691.848511, 370.474304, -1144.927002 },
                { -694.779419, 301.192505, -1234.338013 } } } },

    /** Patch 161 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -742.811951, 379.820679, -1186.801025 },
                { -764.567993, 345.179810, -1195.250000 },
                { -691.848511, 370.474304, -1144.927002 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -735.350037, 301.192505, -1161.832031 },
                { -691.848511, 370.474304, -1144.927002 },
                { -764.567993, 345.179810, -1195.250000 } } } },

    /** Patch 162 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -764.567993, 345.179810, -1195.250000 },
                { -744.281982, 345.179810, -1231.502930 },
                { -735.350037, 301.192505, -1161.832031 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -694.779419, 301.192505, -1234.338013 },
                { -735.350037, 301.192505, -1161.832031 },
                { -744.281982, 345.179810, -1231.502930 } } } },

    /** Patch 163 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -744.281982, 345.179810, -1231.502930 },
                { -722.525940, 379.820679, -1223.046997 },
                { -694.779419, 301.192505, -1234.338013 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -651.277222, 370.474304, -1217.432983 },
                { -694.779419, 301.192505, -1234.338013 },
                { -722.525940, 379.820679, -1223.046997 } } } },

    /** Patch 164 **/
    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -722.525940, 379.820679, -1223.046997 },
                { -742.811951, 379.820679, -1186.801025 },
                { -651.277222, 370.474304, -1217.432983 } } } },

    { MODEL_TRIANGLE,
        { { 0.373419, 0.373419, 0.373419 }, { 0.000000, 0.000000, 0.000000 },
            { { -691.848511, 370.474304, -1144.927002 },
                { -651.277222, 370.474304, -1217.432983 },
                { -742.811951, 379.820679, -1186.801025 } } } },

    /** Patch 165 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, 654.000183, -999.999023 },
                { 420.000031, 654.000183, -999.999023 },
                { -420.000031, 600.000061, -999.999023 } } } },

    /** Patch 166 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, 654.000183, -999.999023 },
                { 420.000031, 654.000183, -700.000000 },
                { 420.000031, 600.000061, -999.999023 } } } },

    /** Patch 167 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { 420.000031, 654.000183, -700.000000 },
                { -420.000031, 654.000183, -700.000000 },
                { 420.000031, 600.000061, -700.000000 } } } },

    /** Patch 168 **/
    { MODEL_RECTANGLE,
        { { 0.186700, 0.186700, 0.186700 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, 654.000183, -700.000000 },
                { -420.000031, 654.000183, -999.999023 },
                { -420.000031, 600.000061, -700.000000 } } } },

    /** Patch 169 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 120.000000, 120.000000, 120.000000 },
            { { -264.000092, 628.000122, -850.002991 },
                { -264.000092, 628.000122, -890.001038 },
                { -215.999710, 628.000122, -850.002991 } } } },

    /** Patch 170 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 60.000000, 60.000000, 60.000000 },
            { { -264.000092, 631.333496, -890.001038 },
                { -264.000092, 631.333496, -850.002991 },
                { -215.999710, 631.333496, -890.001038 } } } },

    /** Patch 171 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 120.000000, 120.000000, 120.000000 },
            { { -24.000200, 628.000122, -850.002991 },
                { -24.000200, 628.000122, -890.001038 },
                { 24.000200, 628.000122, -850.002991 } } } },

    /** Patch 172 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 60.000000, 60.000000, 60.000000 },
            { { -24.000200, 631.333496, -890.001038 },
                { -24.000200, 631.333496, -850.002991 },
                { 24.000200, 631.333496, -890.001038 } } } },

    /** Patch 173 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 120.000000, 120.000000, 120.000000 },
            { { 215.999710, 628.000122, -850.002991 },
                { 215.999710, 628.000122, -890.001038 },
                { 264.000092, 628.000122, -850.002991 } } } },

    /** Patch 174 **/
    { MODEL_RECTANGLE,
        { { 0.000000, 0.000000, 0.000000 }, { 60.000000, 60.000000, 60.000000 },
            { { 215.999710, 631.333496, -890.001038 },
                { 215.999710, 631.333496, -850.002991 },
                { 264.000092, 631.333496, -890.001038 } } } },

    /** Patch 0 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, 1701.000000 },
                { -847.000000, -800.002014, 1701.000000 },
                { 847.000000, 800.002014, 1701.000000 } } } },

    /** Patch 1 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, -800.002014, 1701.000000 },
                { -847.000000, -800.002014, 0.000000 },
                { -847.000000, 800.002014, 1701.000000 } } } },

    /** Patch 2 **/
    { MODEL_RECTANGLE,
        { { 0.615686, 0.737255, 0.870588 }, { 0.000000, 0.000000, 0.000000 },
            { { 847.000000, -800.002014, 0.000000 },
                { 847.000000, -800.002014, 1701.000000 },
                { 847.000000, 800.002014, 0.000000 } } } },

    /** Patch 3 **/
    { MODEL_RECTANGLE,
        { { 0.603921, 0.603921, 0.603921 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, 800.002014, 0.000000 },
                { 847.000000, 800.002014, 0.000000 },
                { -847.000000, 800.002014, 1701.000000 } } } },

    /** Patch 4 **/
    { MODEL_RECTANGLE,
        { { 0.501961, 0.501961, 0.501961 }, { 0.000000, 0.000000, 0.000000 },
            { { -847.000000, -800.002014, 1701.000000 },
                { 847.000000, -800.002014, 1701.000000 },
                { -847.000000, -800.002014, 0.000000 } } } },

    /** Patch 5 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 84.000000, -350.000000, 1699.999023 },
                { 840.000061, -350.000000, 859.998962 },
                { 84.000000, -299.999695, 1699.999023 } } } },

    /** Patch 6 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 840.000061, -350.000000, 859.998962 },
                { 84.000000, -350.000000, 1699.999023 },
                { 336.000000, -350.000000, 544.999695 } } } },

    /** Patch 7 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 336.000000, -299.999695, 544.999695 },
                { -420.000031, -299.999695, 1384.999023 },
                { 840.000061, -299.999695, 859.998962 } } } },

    /** Patch 8 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 336.000000, -350.000000, 544.999695 },
                { -420.000031, -350.000000, 1384.999023 },
                { 336.000000, -299.999695, 544.999695 } } } },

    /** Patch 9 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, -350.000000, 1384.999023 },
                { 84.000000, -350.000000, 1699.999023 },
                { -420.000031, -299.999695, 1384.999023 } } } },

    /** Patch 10 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 840.000061, -350.000000, 859.998962 },
                { 336.000000, -350.000000, 544.999695 },
                { 840.000061, -299.999695, 859.998962 } } } },

    /** Patch 11 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 306.000092, -800.002014, 619.999817 },
                { 786.001953, -800.002014, 920.003052 },
                { 306.000092, -350.000000, 619.999817 } } } },

    /** Patch 12 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 341.999695, -800.002014, 579.999695 },
                { 306.000092, -800.002014, 619.999817 },
                { 341.999695, -350.000000, 579.999695 } } } },

    /** Patch 13 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 786.001953, -800.002014, 920.003052 },
                { 822.002930, -800.002014, 879.998047 },
                { 786.001953, -350.000000, 920.003052 } } } },

    /** Patch 14 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 822.002930, -800.002014, 879.998047 },
                { 341.999695, -800.002014, 579.999695 },
                { 822.002930, -350.000000, 879.998047 } } } },

    /** Patch 15 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -113.999901, -533.332825, 1045.001953 },
                { -420.000031, -533.332825, 1384.999023 },
                { -113.999901, -500.000214, 1045.001953 } } } },

    /** Patch 16 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, -533.332825, 1384.999023 },
                { 84.000000, -533.332825, 1699.999023 },
                { -420.000031, -500.000214, 1384.999023 } } } },

    /** Patch 17 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -113.999901, -500.000214, 1045.001953 },
                { -420.000031, -500.000214, 1384.999023 },
                { 390.000092, -500.000214, 1360.001953 } } } },

    /** Patch 18 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 390.000092, -533.332825, 1360.001953 },
                { 84.000000, -533.332825, 1699.999023 },
                { -113.999901, -533.332825, 1045.001953 } } } },

    /** Patch 19 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 84.000000, -533.332825, 1699.999023 },
                { 390.000092, -533.332825, 1360.001953 },
                { 84.000000, -500.000214, 1699.999023 } } } },

    /** Patch 20 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 390.000092, -533.332825, 1360.001953 },
                { -113.999901, -533.332825, 1045.001953 },
                { 390.000092, -500.000214, 1360.001953 } } } },

    /** Patch 21 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -113.999901, -700.000000, 1045.001953 },
                { -420.000031, -700.000000, 1384.999023 },
                { -113.999901, -666.666687, 1045.001953 } } } },

    /** Patch 22 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -420.000031, -700.000000, 1384.999023 },
                { 84.000000, -700.000000, 1699.999023 },
                { -420.000031, -666.666687, 1384.999023 } } } },

    /** Patch 23 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { -113.999901, -666.666687, 1045.001953 },
                { -420.000031, -666.666687, 1384.999023 },
                { 390.000092, -666.666687, 1360.001953 } } } },

    /** Patch 24 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 390.000092, -700.000000, 1360.001953 },
                { 84.000000, -700.000000, 1699.999023 },
                { -113.999901, -700.000000, 1045.001953 } } } },

    /** Patch 25 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 84.000000, -700.000000, 1699.999023 },
                { 390.000092, -700.000000, 1360.001953 },
                { 84.000000, -666.666687, 1699.999023 } } } },

    /** Patch 26 **/
    { MODEL_RECTANGLE,
        { { 0.464516, 0.076674, 0.000000 }, { 0.000000, 0.000000, 0.000000 },
            { { 390.000092, -700.000000, 1360.001953 },
                { -113.999901, -700.000000, 1045.001953 },
                { 390.000092, -666.666687, 1360.001953 } } } },

    /** Patch 27 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 138.000107, -500.000214, 1640.001953 },
                { -341.999695, -500.000214, 1340.002930 },
                { 138.000107, -350.000000, 1640.001953 } } } },

    /** Patch 28 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -341.999695, -500.000214, 1340.002930 },
                { -378.000000, -500.000214, 1380.000977 },
                { -341.999695, -350.000000, 1340.002930 } } } },

    /** Patch 29 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -378.000000, -500.000214, 1380.000977 },
                { 101.999802, -500.000214, 1680.000122 },
                { -378.000000, -350.000000, 1380.000977 } } } },

    /** Patch 30 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 101.999802, -500.000214, 1680.000122 },
                { 138.000107, -500.000214, 1640.001953 },
                { 101.999802, -350.000000, 1680.000122 } } } },

    /** Patch 31 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 138.000107, -666.666687, 1640.001953 },
                { -341.999695, -666.666687, 1340.002930 },
                { 138.000107, -533.332825, 1640.001953 } } } },

    /** Patch 32 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -341.999695, -666.666687, 1340.002930 },
                { -378.000000, -666.666687, 1380.000977 },
                { -341.999695, -533.332825, 1340.002930 } } } },

    /** Patch 33 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -378.000000, -666.666687, 1380.000977 },
                { 101.999802, -666.666687, 1680.000122 },
                { -378.000000, -533.332825, 1380.000977 } } } },

    /** Patch 34 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 101.999802, -666.666687, 1680.000122 },
                { 138.000107, -666.666687, 1640.001953 },
                { 101.999802, -533.332825, 1680.000122 } } } },

    /** Patch 35 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 138.000107, -800.002014, 1640.001953 },
                { -341.999695, -800.002014, 1340.002930 },
                { 138.000107, -700.000000, 1640.001953 } } } },

    /** Patch 36 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -341.999695, -800.002014, 1340.002930 },
                { -378.000000, -800.002014, 1380.000977 },
                { -341.999695, -700.000000, 1340.002930 } } } },

    /** Patch 37 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { -378.000000, -800.002014, 1380.000977 },
                { 101.999802, -800.002014, 1680.000122 },
                { -378.000000, -700.000000, 1380.000977 } } } },

    /** Patch 38 **/
    { MODEL_RECTANGLE,
        { { 0.682353, 0.247059, 0.011765 }, { 0.000000, 0.000000, 0.000000 },
            { { 101.999802, -800.002014, 1680.000122 },
                { 138.000107, -800.002014, 1640.001953 },
                { 101.999802, -700.000000, 1680.000122 } } } },

    /** Patch 39 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -108.000198, -500.000214, 1080.002075 },
                { -120.000298, -500.000214, 1093.337036 },
                { -108.000198, -350.000000, 1080.002075 } } } },

    /** Patch 40 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -104.000397, -500.000214, 1103.333008 },
                { -91.999603, -500.000214, 1089.998047 },
                { -104.000397, -350.000000, 1103.333008 } } } },

    /** Patch 41 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -120.000298, -500.000214, 1093.337036 },
                { -104.000397, -500.000214, 1103.333008 },
                { -120.000298, -350.000000, 1093.337036 } } } },

    /** Patch 42 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -91.999603, -500.000214, 1089.998047 },
                { -108.000198, -500.000214, 1080.002075 },
                { -91.999603, -350.000000, 1089.998047 } } } },

    /** Patch 43 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -108.000198, -666.666687, 1080.002075 },
                { -120.000298, -666.666687, 1093.337036 },
                { -108.000198, -533.332825, 1080.002075 } } } },

    /** Patch 44 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -104.000397, -666.666687, 1103.333008 },
                { -91.999603, -666.666687, 1089.998047 },
                { -104.000397, -533.332825, 1103.333008 } } } },

    /** Patch 45 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -120.000298, -666.666687, 1093.337036 },
                { -104.000397, -666.666687, 1103.333008 },
                { -120.000298, -533.332825, 1093.337036 } } } },

    /** Patch 46 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { -91.999603, -666.666687, 1089.998047 },
                { -108.000198, -666.666687, 1080.002075 },
                { -91.999603, -533.332825, 1089.998047 } } } },

    /** Patch 47 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 308.000000, -500.000214, 1340.002930 },
                { 295.999207, -500.000214, 1353.331055 },
                { 308.000000, -350.000000, 1340.002930 } } } },

    /** Patch 48 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 295.999207, -500.000214, 1353.331055 },
                { 311.999786, -500.000214, 1363.333984 },
                { 295.999207, -350.000000, 1353.331055 } } } },

    /** Patch 49 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 311.999786, -500.000214, 1363.333984 },
                { 323.999908, -500.000214, 1349.999023 },
                { 311.999786, -350.000000, 1363.333984 } } } },

    /** Patch 50 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 323.999908, -500.000214, 1349.999023 },
                { 308.000000, -500.000214, 1340.002930 },
                { 323.999908, -350.000000, 1349.999023 } } } },

    /** Patch 51 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 308.000000, -666.666687, 1340.002930 },
                { 295.999207, -666.666687, 1353.331055 },
                { 308.000000, -533.332825, 1340.002930 } } } },

    /** Patch 52 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 295.999207, -666.666687, 1353.331055 },
                { 311.999786, -666.666687, 1363.333984 },
                { 295.999207, -533.332825, 1353.331055 } } } },

    /** Patch 53 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 311.999786, -666.666687, 1363.333984 },
                { 323.999908, -666.666687, 1349.999023 },
                { 311.999786, -533.332825, 1363.333984 } } } },

    /** Patch 54 **/
    { MODEL_RECTANGLE,
        { { 0.200000, 0.200000, 0.200000 }, { 0.000000, 0.000000, 0.000000 },
            { { 323.999908, -666.666687, 1349.999023 },
                { 308.000000, -666.666687, 1340.002930 },
                { 323.999908, -533.332825, 1349.999023 } } } },

    /** Patch 55 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -433.708832, -566.666809, 844.088013 },
                { -298.470917, -566.666809, 602.406677 },
                { -424.914001, -534.328918, 847.503967 } } } },

    /** Patch 56 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -298.470917, -566.666809, 602.406677 },
                { -509.488007, -518.146973, 520.407959 },
                { -289.676117, -534.328918, 605.824768 } } } },

    /** Patch 57 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -644.725891, -518.146973, 762.090027 },
                { -433.708832, -566.666809, 844.088013 },
                { -635.931091, -485.809113, 765.505981 } } } },

    /** Patch 58 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -433.708832, -566.666809, 844.088013 },
                { -644.725891, -518.146973, 762.090027 },
                { -298.470917, -566.666809, 602.406677 } } } },

    /** Patch 59 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -509.488007, -518.146973, 520.407959 },
                { -644.725891, -518.146973, 762.090027 },
                { -500.693207, -485.809113, 523.825378 } } } },

    /** Patch 60 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -289.676117, -534.328918, 605.824768 },
                { -500.693207, -485.809113, 523.825378 },
                { -424.914001, -534.328918, 847.503967 } } } },

    /** Patch 61 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -188.395203, -633.333435, 957.802979 },
                { -356.457520, -695.767822, 892.500000 },
                { -36.252300, -633.333435, 685.912537 } } } },

    /** Patch 62 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -36.252300, -633.333435, 685.912537 },
                { -204.315292, -695.767822, 620.605286 },
                { -49.833000, -602.427002, 680.635193 } } } },

    /** Patch 63 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -356.457520, -695.767822, 892.500000 },
                { -188.395203, -633.333435, 957.802979 },
                { -370.038208, -664.861389, 887.221985 } } } },

    /** Patch 64 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -204.315292, -695.767822, 620.605286 },
                { -356.457520, -695.767822, 892.500000 },
                { -217.895294, -664.861389, 615.328003 } } } },

    /** Patch 65 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -188.395203, -633.333435, 957.802979 },
                { -36.252300, -633.333435, 685.912537 },
                { -201.975204, -602.427002, 952.524963 } } } },

    /** Patch 66 **/
    { MODEL_RECTANGLE,
        { { 0.183806, 0.551419, 0.919032 }, { 0.000000, 0.000000, 0.000000 },
            { { -49.833000, -602.427002, 680.635193 },
                { -217.895294, -664.861389, 615.328003 },
                { -201.975204, -602.427002, 952.524963 } } } },

    /** Patch 67 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -650.722107, -800.002014, 689.105164 },
                { -580.434387, -800.002014, 563.493713 },
                { -533.348206, -541.608887, 785.043030 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -416.794708, -541.608887, 576.753113 },
                { -533.348206, -541.608887, 785.043030 },
                { -580.434387, -800.002014, 563.493713 } } } },

    /** Patch 68 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -650.722107, -800.002014, 689.105164 },
                { -533.348206, -541.608887, 785.043030 },
                { -689.917908, -800.002014, 670.970276 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -568.779419, -533.325073, 769.986023 },
                { -689.917908, -800.002014, 670.970276 },
                { -533.348206, -541.608887, 785.043030 } } } },

    /** Patch 69 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -416.794708, -541.608887, 576.753113 },
                { -580.434387, -800.002014, 563.493713 },
                { -453.411713, -533.325073, 563.813599 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -622.298584, -800.002014, 550.129272 },
                { -453.411713, -533.325073, 563.813599 },
                { -580.434387, -800.002014, 563.493713 } } } },

    /** Patch 70 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -622.298584, -800.002014, 550.129272 },
                { -689.917908, -800.002014, 670.970276 },
                { -453.411713, -533.325073, 563.813599 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -568.779419, -533.325073, 769.986023 },
                { -453.411713, -533.325073, 563.813599 },
                { -689.917908, -800.002014, 670.970276 } } } },

    /** Patch 71 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -67.053001, -800.002014, 725.025024 },
                { -172.239197, -800.002014, 913.003052 },
                { -147.604797, -670.852722, 665.175659 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -279.033295, -670.852722, 900.052979 },
                { -147.604797, -670.852722, 665.175659 },
                { -172.239197, -800.002014, 913.003052 } } } },

    /** Patch 72 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -279.033295, -670.852722, 900.052979 },
                { -172.239197, -800.002014, 913.003052 },
                { -244.124313, -658.100830, 912.344971 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -126.785400, -800.002014, 926.576050 },
                { -244.124313, -658.100830, 912.344971 },
                { -172.239197, -800.002014, 913.003052 } } } },

    /** Patch 73 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -67.053001, -800.002014, 725.025024 },
                { -147.604797, -670.852722, 665.175659 },
                { -25.356800, -800.002014, 745.317932 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -113.862701, -658.100830, 679.557190 },
                { -25.356800, -800.002014, 745.317932 },
                { -147.604797, -670.852722, 665.175659 } } } },

    /** Patch 74 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -126.785400, -800.002014, 926.576050 },
                { -25.356800, -800.002014, 745.317932 },
                { -244.124313, -658.100830, 912.344971 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -113.862701, -658.100830, 679.557190 },
                { -244.124313, -658.100830, 912.344971 },
                { -25.356800, -800.002014, 745.317932 } } } },

    /** Patch 75 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -185.036591, -766.667969, 883.266968 },
                { -102.610901, -766.667969, 735.966003 },
                { -623.555115, -766.667969, 679.991882 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -571.348389, -766.667969, 586.693787 },
                { -623.555115, -766.667969, 679.991882 },
                { -102.610901, -766.667969, 735.966003 } } } },

    /** Patch 76 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -641.077515, -800.002014, 671.869080 },
                { -161.664307, -800.002014, 894.102966 },
                { -623.555115, -766.667969, 679.991882 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -185.036591, -766.667969, 883.266968 },
                { -623.555115, -766.667969, 679.991882 },
                { -161.664307, -800.002014, 894.102966 } } } },

    /** Patch 77 **/
    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -77.628601, -800.002014, 743.924988 },
                { -590.078308, -800.002014, 580.729065 },
                { -102.610901, -766.667969, 735.966003 } } } },

    { MODEL_TRIANGLE,
        { { 0.506699, 0.690953, 0.552763 }, { 0.000000, 0.000000, 0.000000 },
            { { -571.348389, -766.667969, 586.693787 },
                { -102.610901, -766.667969, 735.966003 },
                { -590.078308, -800.002014, 580.729065 } } } },




    { MODEL_NULL } } ;

