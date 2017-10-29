
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
 *	Utility package
 *
 ***************************************************************/

#include <stdio.h>


#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
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


static void clear_element_radiosity(Element *elem, long dummy, long process_id);

/***************************************
 *
 *    Global variables
 *
 ****************************************/

#define MAX_INTERACTION_PER_ELEMENT (100)

long total_patches ;
long total_elements ;
long total_equiv_elements ;
long total_interactions ;
long total_comp_visible_interactions ;
long total_invisible_interactions ;
long total_match3, total_match2, total_match1, total_match0 ;


typedef struct
{
    long count ;
    float area ;
} Elem_Interaction ;

Elem_Interaction elem_interaction[MAX_INTERACTION_PER_ELEMENT+1] ;
Elem_Interaction many_interaction ;


/***************************************
 *
 *    Prinit statistics
 *
 ****************************************/

void print_statistics(FILE *fd, long process_id)
{
    long i ;

    /* Initialize information */
    total_patches = 0 ;
    total_elements = 0 ;
    total_equiv_elements = 0 ;
    total_interactions = 0 ;
    total_comp_visible_interactions = 0 ;
    total_invisible_interactions = 0 ;
    total_match3 = 0 ;
    total_match2 = 0 ;
    total_match1 = 0 ;
    total_match0 = 0 ;

    for( i = 0 ; i < MAX_INTERACTION_PER_ELEMENT ; i++ )
        {
            elem_interaction[i].count = 0 ;
            elem_interaction[i].area = 0 ;
        }
    many_interaction.count = 0 ;
    many_interaction.area  = 0 ;

    foreach_patch_in_bsp( get_patch_stat,  0, 0 ) ;

    fprintf( fd, "Rasiosity Statistics\n\n" ) ;

    fprintf( fd, "    Histogram of interactions/elem\n" ) ;
    fprintf( fd, "\t Interactions  Occurrence\n" ) ;
    fprintf( fd, "\t -------------------------------\n" ) ;
    if( many_interaction.count > 0 )
        {
            fprintf( fd, "\t (Over %d)      %ld (%f)\n",
                    MAX_INTERACTION_PER_ELEMENT,
                    many_interaction.count,
                    many_interaction.area / many_interaction.count ) ;
        }
    for( i = MAX_INTERACTION_PER_ELEMENT ;
        elem_interaction[i].count == 0 ; i-- ) ;
    for( ; i >= 0 ; i-- )
        {
            if( elem_interaction[i].count == 0 )
                continue ;

            if( elem_interaction[i].count == 0 )
                fprintf( fd, "\t    %ld          %ld (---)\n",
                        i, elem_interaction[i].count ) ;

            else
                fprintf( fd, "\t    %ld          %ld (%f)\n",
                        i, elem_interaction[i].count,
                        elem_interaction[i].area / elem_interaction[i].count);
        }

    fprintf( fd, "    Configurations\n" ) ;
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    fprintf( fd, "\tPatch assignment: Costbased\n" ) ;
    fprintf( fd, "\tUsing non-greedy cost-based algorithm\n") ;
#endif
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_STATIC
    fprintf( fd, "\tPatch assignment: Static equal number\n" ) ;
#endif

    fprintf( fd, "\tAlways inserting at top of list for visibility testing (not sorted)\n" ) ;
    fprintf( fd, "\tRecursive pruning enabled for BSP tree traversal\n" ) ;
    fprintf( fd, "\tPatch cache:      Enabled\n" ) ;
    fprintf( fd, "\tAlways check all other queues when task stealing (not neighbor scheme)\n" ) ;


    fprintf( fd, "    Parameters\n" ) ;
    fprintf( fd, "\tNumber of processors:    %ld\n", n_processors ) ;
    fprintf( fd, "\tNumber of task queues:   %ld\n", n_taskqueues ) ;
    fprintf( fd, "\tNumber of tasks / queue: %ld\n", n_tasks_per_queue ) ;
    fprintf( fd, "\tArea epsilon:            %f\n", Area_epsilon ) ;
    fprintf( fd, "\t#inter parallel refine:  %ld\n",
            N_inter_parallel_bf_refine);
    fprintf( fd, "\t#visibility comp / task: %ld\n", N_visibility_per_task ) ;
    fprintf( fd, "\tBF epsilon:              %f\n", BFepsilon ) ;
    fprintf( fd, "\tEnergy convergence:      %f\n", Energy_epsilon ) ;

    fprintf( fd, "    Iterations to converge:   %ld times\n",
            global->iteration_count ) ;

    fprintf( fd, "    Resource Usage\n" ) ;
    fprintf( fd, "\tNumber of patches:            %ld\n", total_patches ) ;
    fprintf( fd, "\tTotal number of elements:     %ld\n", total_elements ) ;
    fprintf( fd, "\tTotal number of interactions: %ld\n", total_interactions);
    fprintf( fd, "\t          completely visible: %ld\n",
            total_comp_visible_interactions ) ;
    fprintf( fd, "\t        completely invisible: %ld\n",
            total_invisible_interactions ) ;
    fprintf( fd, "\t           partially visible: %ld\n",
            total_interactions - total_comp_visible_interactions
            - total_invisible_interactions ) ;
    fprintf( fd, "\tInteraction coherence (root interaction not counted)\n");
    fprintf( fd, "\t       Common for 4 siblings: %ld\n", total_match3 ) ;
    fprintf( fd, "\t       Common for 3 siblings: %ld\n", total_match2 ) ;
    fprintf( fd, "\t       Common for 2 siblings: %ld\n", total_match1 ) ;
    fprintf( fd, "\t       Common for no sibling: %ld\n", total_match0 ) ;
    fprintf( fd, "\tAvg. elements per patch:      %.1f\n",
            (float)total_elements / (float)total_patches ) ;
    fprintf( fd, "\tAvg. interactions per patch:  %.1f\n",
            (float)total_interactions / (float)total_patches ) ;
    fprintf( fd, "\tAvg. interactions per element:%.1f\n",
            (float)total_interactions / (float)total_elements ) ;
    fprintf( fd, "\tNumber of elements in equivalent uniform mesh: %ld\n",
            total_equiv_elements ) ;
    fprintf( fd, "\tElem(hierarchical)/Elem(uniform): %.2f%%\n",
            (float)total_elements / (float)total_equiv_elements * 100.0 ) ;
}




/**********************************************************
 *
 *    print_per_process_info()
 *
 ***********************************************************/

void print_per_process_info(FILE *fd, long process)
{
    long cache_line ;
    long iteration ;
    StatisticalInfo *ps ;
    Element *e ;

    ps = &global->stat_info[process] ;

    fprintf( fd, "\t\tModeling tasks:            %ld\n",
    ps->total_modeling_tasks ) ;
    fprintf( fd, "\t\tDefine patch tasks:        %ld\n",
    ps->total_def_patch_tasks ) ;
    fprintf( fd, "\t\tFF refinement tasks:       %ld\n",
    ps->total_ff_ref_tasks ) ;
    fprintf( fd, "\t\tRay processing tasks:      %ld\n",
    ps->total_ray_tasks ) ;
    fprintf( fd, "\t\tRadiosity Avg/Norm tasks:  %ld\n",
    ps->total_radavg_tasks ) ;
    fprintf( fd, "\t\tInteraction computations:  %ld\n",
    ps->total_interaction_comp ) ;
    fprintf( fd, "\t\tVisibility computations:   %ld\n",
    ps->total_visibility_comp ) ;
    fprintf( fd, "\t\t   (%ld of %ld were partially visible)\n",
    ps->partially_visible,
    ps->total_visibility_comp ) ;
    fprintf( fd, "\t\tRay intersection tests:    %ld\n",
    ps->total_ray_intersect_test ) ;
    fprintf( fd, "\t\tPatch cache hit ratio:     %.2f%%\n",
    ps->total_patch_cache_hit * 100 /
    (ps->total_patch_cache_check + 0.01) ) ;
    for( cache_line = 0 ; cache_line < PATCH_CACHE_SIZE ; cache_line++ )
    fprintf( fd, "\t\t    (level %ld):             %.2f%%\n",
    cache_line,
    ps->patch_cache_hit[cache_line] * 100 /
    (ps->total_patch_cache_check + 0.01));

    /* Per iteration info */
    fprintf( fd, "\t\tPer iteration info.\n" ) ;
    for( iteration = 0 ; iteration < global->iteration_count ; iteration++ )
    {
        fprintf( fd, "\t\t     [%ld]  Interaction comp:   %ld\n",
        iteration, ps->per_iteration[iteration].visibility_comp ) ;
        fprintf( fd, "\t\t          Ray Intersection:   %ld\n",
        ps->per_iteration[iteration].ray_intersect_test ) ;
        fprintf( fd, "\t\t          Tasks from my Q:    %ld\n",
        ps->per_iteration[iteration].tasks_from_myq ) ;
        fprintf( fd, "\t\t          Tasks from other Q: %ld\n",
        ps->per_iteration[iteration].tasks_from_otherq ) ;
        fprintf( fd, "\t\t     Process_task wait count: %ld\n",
        ps->per_iteration[iteration].process_tasks_wait ) ;
        e = ps->per_iteration[iteration].last_pr_task ;
        if( e == 0 )
        continue ;
        if( e->parent == 0 )
        {
            fprintf( fd, "\t\t          Last task: Patch level\n" ) ;
            fprintf( fd, "\t\t           (%ld root inter)\n",
            e->n_interactions ) ;
        }
        else
        {
            fprintf( fd, "\t\t          Last task: Elem level\n" ) ;
            fprintf( fd, "\t\t           (%ld inter, %.3f Elem/Patch)\n",
            e->n_interactions, e->area / e->patch->area ) ;
        }
    }
}



/**********************************************************
*
*    get_patch_stat()
*
***********************************************************/

long   n_elements_in_patch ;
long   n_equiv_elem_in_patch ;
float min_elem_area ;
long   n_interactions_in_patch ;
long   n_comp_visible_interactions ;
long   n_invisible_interactions ;


void get_patch_stat(Patch *patch, long dummy, long process_id)
{
    /* Initialize stat info for element */
    n_elements_in_patch = 0 ;
    n_equiv_elem_in_patch = 1 ;
    min_elem_area = patch->area ;
    n_interactions_in_patch = 0 ;
    n_comp_visible_interactions = 0 ;
    n_invisible_interactions = 0 ;

    /* Traverse the quad tree */
    foreach_element_in_patch( patch, get_elem_stat, 0, process_id ) ;

    /* Update global stat variables */
    total_patches++ ;
    total_elements += n_elements_in_patch ;
    total_equiv_elements += n_equiv_elem_in_patch ;
    total_interactions += n_interactions_in_patch ;
    total_comp_visible_interactions += n_comp_visible_interactions ;
    total_invisible_interactions += n_invisible_interactions ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    if( n_interactions_in_patch
    != global->patch_cost[patch->seq_no].n_total_inter )
    {
        printf( "Error: patch(%d) Inter counted: %d (n_total_inter %d)\n",
        patch->seq_no,
        n_interactions_in_patch,
        global->patch_cost[patch->seq_no].n_total_inter ) ;
    }
#endif
}

void get_elem_stat(Element *elem, long dummy, long process_id)
{
    Interaction *pi ;
    long p_visible = 0 ;
    long c_visible = 0 ;
    long i_visible = 0 ;
    long match0, match1, match2, match3 ;

    n_elements_in_patch++ ;

    while( elem->area < min_elem_area )
    {
        min_elem_area *= 0.25 ;
        n_equiv_elem_in_patch *= 4 ;
    }

    /* Classify visibility */
    n_interactions_in_patch += elem->n_interactions ;
    for( pi = elem->interactions ; pi ; pi = pi->next )
    {
        if( pi->visibility == 0.0 )
        i_visible++ ;
        else if( pi->visibility == 1.0 )
        c_visible++ ;
        else
        p_visible++ ;
    }
    if( i_visible + c_visible + p_visible != elem->n_interactions )
    printf( "Fatal: Interactions count miss match\n" ) ;
    if( elem->n_vis_undef_inter != 0 )
    printf( "Fatal: Visibility undef list count non zero(%ld)\n",
    elem->n_vis_undef_inter ) ;
    if( elem->vis_undef_inter != 0 )
    printf( "Fatal: Visibility undef list not empty\n" ) ;

    n_comp_visible_interactions += c_visible ;
    n_invisible_interactions    += i_visible ;


    /* Count interactions / element */
    if( elem->n_interactions > MAX_INTERACTION_PER_ELEMENT )
    {
        many_interaction.count++ ;
        many_interaction.area += elem->area ;
    }
    else
    {
        elem_interaction[ elem->n_interactions ].count++ ;
        elem_interaction[ elem->n_interactions ].area += elem->area ;
    }

    /* Analyze object coherence */
    if( ! LEAF_ELEMENT( elem ) )
    {
        match0 = match1 = match2 = match3 = 0 ;

        count_interaction(elem->center, elem->top, elem->right, elem->left,
        &match3, &match2, &match1, &match0, process_id ) ;
        count_interaction(elem->top, elem->right, elem->left, elem->center,
        &match3, &match2, &match1, &match0, process_id ) ;
        count_interaction(elem->right, elem->left, elem->center, elem->top,
        &match3, &match2, &match1, &match0, process_id ) ;
        count_interaction(elem->left, elem->center, elem->top, elem->right,
        &match3, &match2, &match1, &match0, process_id ) ;

        total_match3 += match3 ;
        total_match2 += match2 ;
        total_match1 += match1 ;
        total_match0 += match0 ;
    }
}



void count_interaction(Element *es, Element *e1, Element *e2, Element *e3, long *c3, long *c2, long *c1, long *c0, long process_id)
{
    Interaction *pi ;
    long occurrence ;

    for( pi = es->interactions ; pi ; pi = pi->next )
    {
        occurrence  = search_intearction( e1->interactions, pi, process_id ) ;
        occurrence += search_intearction( e2->interactions, pi, process_id ) ;
        occurrence += search_intearction( e3->interactions, pi, process_id ) ;
        switch( occurrence )
        {
            case 0:  (*c0)++ ; break ;
            case 1:  (*c1)++ ; break ;
            case 2:  (*c2)++ ; break ;
            case 3:  (*c3)++ ; break ;
        }
    }
}

long search_intearction(Interaction *int_list, Interaction *inter, long process_id)
{
    while( int_list )
    {
        if( int_list->destination == inter->destination )
        return( 1 ) ;

        int_list = int_list->next ;
    }

    return( 0 ) ;
}

/***************************************
*
*    Prinit running time
*
****************************************/

void print_running_time(long process_id)
{
    long time_diff, time_diff1 ;

    time_diff = time_rad_end - time_rad_start ;
    time_diff1 = time_rad_end - timing[0]->rad_start;
    if( time_diff < 0 )
    time_diff += CLOCK_MAX_VAL ;
    if( time_diff1 < 0 )
    time_diff1 += CLOCK_MAX_VAL ;

    printf( "\tOverall start time\t%20lu\n", time_rad_start);
    printf( "\tOverall end time\t%20lu\n", time_rad_end);
    printf( "\tTotal time with initialization\t%20lu\n", time_diff);
    printf( "\tTotal time without initialization\t%20lu\n", time_diff1);
}


/***************************************
*
*    Print process creation overhead
*
****************************************/

void print_fork_time(long process_id)
{
    long pid ;

    if( n_processors <= 1 )
    return ;

    printf( "\tProcess fork overhead\n" ) ;
    for( pid = 0 ; pid < n_processors-1 ; pid++ )
    {
        printf( "\t Process %ld  %.2f mS\n",
        pid,
        (timing[pid]->rad_start - time_rad_start) / 1000.0 ) ;
    }

    printf( "\t (total)    %.2f mS\n",
    (time_process_start[n_processors-2] - time_rad_start) / 1000.0 ) ;
}


/***************************************
*
*    Initialize statistical info
*
****************************************/

void init_stat_info(long process_id)
{
    long pid ;
    long i ;
    StatisticalInfo *ps ;

    for( pid = 0 ; pid < MAX_PROCESSORS ; pid++ )
    {
        ps = &global->stat_info[ pid ] ;
        ps->total_modeling_tasks    = 0 ;
        ps->total_def_patch_tasks   = 0 ;
        ps->total_ff_ref_tasks      = 0 ;
        ps->total_ray_tasks         = 0 ;
        ps->total_radavg_tasks      = 0 ;
        ps->total_interaction_comp  = 0 ;
        ps->total_visibility_comp   = 0 ;
        ps->partially_visible       = 0 ;
        ps->total_ray_intersect_test= 0 ;
        ps->total_patch_cache_check = 0 ;
        ps->total_patch_cache_hit   = 0 ;

        for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        ps->patch_cache_hit[i]   = 0 ;

        for( i = 0 ; i < MAX_ITERATION_INFO ; i++ )
        {
            ps->per_iteration[ i ].visibility_comp    = 0 ;
            ps->per_iteration[ i ].ray_intersect_test = 0 ;
            ps->per_iteration[ i ].tasks_from_myq     = 0 ;
            ps->per_iteration[ i ].tasks_from_otherq  = 0 ;
            ps->per_iteration[ i ].process_tasks_wait = 0 ;
            ps->per_iteration[ i ].last_pr_task       = 0 ;
        }
    }
}


/**********************************************************
*
*    clear_radiosity()
*
***********************************************************/


void clear_radiosity(long process_id)
{
    foreach_patch_in_bsp( clear_patch_radiosity,  0, 0 ) ;
}


void clear_patch_radiosity(Patch *patch, long dummy, long process_id)
{
    foreach_element_in_patch( patch, clear_element_radiosity, 0, process_id ) ;
}


static void clear_element_radiosity(Element *elem, long dummy, long process_id)
{
    elem->rad.r = 0 ;
    elem->rad.g = 0 ;
    elem->rad.b = 0 ;

    elem->rad_subtree.r = 0 ;
    elem->rad_subtree.g = 0 ;
    elem->rad_subtree.b = 0 ;

    global->prev_total_energy = global->total_energy ;
    global->total_energy.r = 0 ;
    global->total_energy.g = 0 ;
    global->total_energy.b = 0 ;
}
