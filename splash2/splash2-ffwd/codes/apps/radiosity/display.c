
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


static void add_radiosity_to_vertex(Edge *edge, long reverse, Element *elem, Vertex *p_c, long process_id);
static void _display_shaded_triangle(ElemVertex *ev1, ElemVertex *ev2, ElemVertex *ev3, Edge *e12, Edge *e23, Edge *e31, long process_id);
static void _disp_interactions(Element *elem, Interaction *inter, long mode, long process_id);
static void _ps_disp_interactions(Element *elem, Interaction *inter, long mode, long process_id);

struct context9 {
  Element *elem;
  float inv_weight;
};

void func9(struct context9 *ctx9);
void func9(struct context9 *ctx9)
{
  if( ctx9->elem->ev1->weight != 1.0 )
      {
          ctx9->inv_weight = (float)1.0 / ctx9->elem->ev1->weight  ;
          ctx9->elem->ev1->col.r *= ctx9->inv_weight ;
          ctx9->elem->ev1->col.g *= ctx9->inv_weight ;
          ctx9->elem->ev1->col.b *= ctx9->inv_weight ;
          ctx9->elem->ev1->weight = 1.0 ;
      }
}

struct context10 {
  Element *elem;
  float inv_weight;
};

void func10(struct context10 *ctx10);
void func10(struct context10 *ctx10)
{
  if( ctx10->elem->ev2->weight != 1.0 )
      {
          ctx10->inv_weight = (float)1.0 / ctx10->elem->ev2->weight  ;
          ctx10->elem->ev2->col.r *= ctx10->inv_weight ;
          ctx10->elem->ev2->col.g *= ctx10->inv_weight ;
          ctx10->elem->ev2->col.b *= ctx10->inv_weight ;
          ctx10->elem->ev2->weight = 1.0 ;
      }
}

struct context11 {
  Element *elem;
  float inv_weight;
};

void func11(struct context11 *ctx11);
void func11(struct context11 *ctx11)
{
  if( ctx11->elem->ev3->weight != 1.0 )
      {
          ctx11->inv_weight = (float)1.0 / ctx11->elem->ev3->weight  ;
          ctx11->elem->ev3->col.r *= ctx11->inv_weight ;
          ctx11->elem->ev3->col.g *= ctx11->inv_weight ;
          ctx11->elem->ev3->col.b *= ctx11->inv_weight ;
          ctx11->elem->ev3->weight = 1.0 ;
      }
}

/************************************************************************
 *
 *    radiosity_averaging
 *
 *************************************************************************/

void radiosity_averaging(Element *elem, long mode, long process_id)
{
    float inv_weight ;
    Vertex pc ;
    long reverse ;

    if( ! LEAF_ELEMENT(elem) )
        {
            create_radavg_task( elem->center, mode, process_id ) ;
            create_radavg_task( elem->top,    mode, process_id ) ;
            create_radavg_task( elem->right,  mode, process_id ) ;
            create_radavg_task( elem->left,   mode, process_id ) ;
            return ;
        }

    else if( mode == RAD_AVERAGING_MODE )
        {
            /* Compute center point */
            center_point( &elem->ev1->p, &elem->ev2->p, &elem->ev3->p, &pc ) ;

            reverse = EDGE_REVERSE( elem->e12, elem->ev1, elem->ev2 ) ;
            foreach_leaf_edge( elem->e12, reverse, add_radiosity_to_vertex, (long)elem, (long)&pc, process_id ) ;
            reverse = EDGE_REVERSE( elem->e23, elem->ev2, elem->ev3 ) ;
            foreach_leaf_edge( elem->e23, reverse, add_radiosity_to_vertex, (long)elem, (long)&pc, process_id ) ;
            reverse = EDGE_REVERSE( elem->e31, elem->ev3, elem->ev1 ) ;
            foreach_leaf_edge( elem->e31, reverse, add_radiosity_to_vertex, (long)elem, (long)&pc, process_id ) ;
        }
    else
        {
            /* Normalize it */
#ifdef FFWD
            GET_CONTEXT()
            uint64_t return_value = 0;

            struct context9 ctx9;
            ctx9.elem = elem;
            ctx9.inv_weight = inv_weight;
            FFWD_EXEC(0, &func9, return_value, 1, &ctx9)
            elem = ctx9.elem;
            inv_weight = ctx9.inv_weight;
#else
            {pthread_mutex_lock(&(elem->ev1->ev_lock->lock));};
            if( elem->ev1->weight != 1.0 )
                {
                    inv_weight = (float)1.0 / elem->ev1->weight  ;
                    elem->ev1->col.r *= inv_weight ;
                    elem->ev1->col.g *= inv_weight ;
                    elem->ev1->col.b *= inv_weight ;
                    elem->ev1->weight = 1.0 ;
                }
            {pthread_mutex_unlock(&(elem->ev1->ev_lock->lock));};
#endif

#ifdef FFWD
            //GET_CONTEXT()

            struct context10 ctx10;
            ctx10.elem = elem;
            ctx10.inv_weight = inv_weight;
            FFWD_EXEC(0, &func10, return_value, 1, &ctx10)
            elem = ctx10.elem;
            inv_weight = ctx10.inv_weight;
#else
            {pthread_mutex_lock(&(elem->ev2->ev_lock->lock));};
            if( elem->ev2->weight != 1.0 )
                {
                    inv_weight = (float)1.0 / elem->ev2->weight  ;
                    elem->ev2->col.r *= inv_weight ;
                    elem->ev2->col.g *= inv_weight ;
                    elem->ev2->col.b *= inv_weight ;
                    elem->ev2->weight = 1.0 ;
                }
            {pthread_mutex_unlock(&(elem->ev2->ev_lock->lock));};
#endif

#ifdef FFWD
            //GET_CONTEXT()

            struct context11 ctx11;
            ctx11.elem = elem;
            ctx11.inv_weight = inv_weight;
            FFWD_EXEC(0, &func11, return_value, 1, &ctx11)
            elem = ctx11.elem;
            inv_weight = ctx11.inv_weight;
#else
            {pthread_mutex_lock(&(elem->ev3->ev_lock->lock));};
            if( elem->ev3->weight != 1.0 )
                {
                    inv_weight = (float)1.0 / elem->ev3->weight  ;
                    elem->ev3->col.r *= inv_weight ;
                    elem->ev3->col.g *= inv_weight ;
                    elem->ev3->col.b *= inv_weight ;
                    elem->ev3->weight = 1.0 ;
                }
            {pthread_mutex_unlock(&(elem->ev3->ev_lock->lock));};
#endif
        }
}

struct context12 {
  Element *elem;
  ElemVertex *ev ;
  float weight ;
};

void func12(struct context12 *ctx12);
void func12(struct context12 *ctx12)
{
  ctx12->ev->col.r += (ctx12->elem->rad.r * ctx12->weight) ;
  ctx12->ev->col.g += (ctx12->elem->rad.g * ctx12->weight) ;
  ctx12->ev->col.b += (ctx12->elem->rad.b * ctx12->weight) ;
  ctx12->ev->weight += ctx12->weight ;
}

static void add_radiosity_to_vertex(Edge *edge, long reverse, Element *elem, Vertex *p_c, long process_id)
{
    ElemVertex *ev ;
    float weight ;

    if( reverse )
        ev = edge->pb ;
    else
        ev = edge->pa ;

    weight = (float)1.0 / distance( &ev->p, p_c ) ;
    weight = 1.0 ;
    weight = elem->area ;
#ifdef FFWD
    GET_CONTEXT()
    uint64_t return_value = 0;

    struct context12 ctx12;
    ctx12.elem = elem;
    ctx12.ev = ev;
    ctx12.weight = weight;
    FFWD_EXEC(0, &func12, return_value, 1, &ctx12)
    elem = ctx12.elem;
    ev = ctx12.ev;
    weight = ctx12.weight;
#else
    {pthread_mutex_lock(&(ev->ev_lock->lock));};
    ev->col.r += (elem->rad.r * weight) ;
    ev->col.g += (elem->rad.g * weight) ;
    ev->col.b += (elem->rad.b * weight) ;
    ev->weight += weight ;
    {pthread_mutex_unlock(&(ev->ev_lock->lock));};
#endif
}


/************************************************************************
 *
 *    setup_view()
 *
 *************************************************************************/

Vertex view_vec ;  /* Origin to the viewer */
static float view_rot_x, view_rot_y, view_dist, view_zoom ;


void setup_view(float rot_x, float rot_y, float dist, float zoom, long process_id)
{
    Vertex v1, v2 ;
    float cc, ss ;

    /* Save parameters */
    view_rot_x = rot_x ;
    view_rot_y = rot_y ;
    view_dist  = dist ;
    view_zoom  = zoom ;

    /* Compute view vector */
    v1.x = 0.0 ;
    v1.y = 0.0 ;
    v1.z = 1.0 ;

    /* Rotate view vector */
    cc = cos( -rot_x * (M_PI / 180.0) ) ;
    ss = sin( -rot_x * (M_PI / 180.0) ) ;
    v2.x = v1.x ;
    v2.y = cc * v1.y - ss * v1.z ;
    v2.z = ss * v1.y + cc * v1.z ;

    cc = cos( -rot_y * (M_PI / 180.0) ) ;
    ss = sin( -rot_y * (M_PI / 180.0) ) ;
    v1.z = cc * v2.z - ss * v2.x ;
    v1.x = ss * v2.z + cc * v2.x ;
    v1.y = v2.y ;

    /* Store view vector */
    view_vec = v1 ;
}


/************************************************************************
 *
 *    display_scene()
 *
 *************************************************************************/

void display_scene(long fill_sw, long patch_sw, long mesh_sw, long interaction_sw, long process_id)
{
    /* Clear the screen */
    g_clear() ;

    /* Set matrix */
    g_setup_view( view_rot_x, view_rot_y, view_dist, view_zoom ) ;

    if( fill_sw == 2 )
        {
            /* Fill surfaces */
            display_elements_in_bsp_tree( DISPLAY_SHADED, process_id ) ;
        }
    if( fill_sw == 1 )
        {
            /* Fill surfaces */
            display_elements_in_bsp_tree( DISPLAY_FILLED, process_id ) ;
        }
    if( mesh_sw )
        {
            /* Draw mesh */
            g_color( G_BLUE ) ;
            display_elements_in_bsp_tree( DISPLAY_EDGEONLY, process_id ) ;
        }
    if( patch_sw )
        {
            g_color( G_RED ) ;
            display_patches_in_bsp_tree( DISPLAY_EDGEONLY, process_id ) ;
        }
    if( interaction_sw )
        {
            g_color( G_GREEN ) ;
            display_interactions_in_bsp_tree(process_id) ;
        }

    /* Flush */
    g_flush() ;
}

/************************************************************************
 *
 *    display_patch()
 *
 *************************************************************************/

void display_patch(Patch *patch, long mode, long process_id)
{
    Vertex p_buf[4] ;
    Rgb   c_buf[4] ;

    if( mode == DISPLAY_SHADED )
        {
            if( inner_product( &patch->plane_equ.n, &view_vec ) < F_ZERO )
                return ;

            p_buf[0] = patch->p1 ;
            p_buf[1] = patch->p2 ;
            p_buf[2] = patch->p3 ;
            c_buf[0] = patch->color ;
            c_buf[1] = patch->color ;
            c_buf[2] = patch->color ;

            g_spolygon( 3, p_buf, c_buf ) ;
        }
    else if( mode == DISPLAY_FILLED )
        {
            if( inner_product( &patch->plane_equ.n, &view_vec ) < F_ZERO )
                return ;

            p_buf[0] = patch->p1 ;
            p_buf[1] = patch->p2 ;
            p_buf[2] = patch->p3 ;

            g_polygon( 3, p_buf ) ;
        }
    else
        {
            g_line( &patch->p1, &patch->p2 ) ;
            g_line( &patch->p2, &patch->p3 ) ;
            g_line( &patch->p3, &patch->p1 ) ;
        }
}


/************************************************************************
 *
 *    display_patches_in_bsp_tree()
 *
 *************************************************************************/

void display_patches_in_bsp_tree(long mode, long process_id)
{
    foreach_depth_sorted_patch( &view_vec, display_patch, (long)mode, process_id ) ;
}



/************************************************************************
 *
 *    display_element()
 *
 *************************************************************************/

void display_element(Element *element, long mode, long process_id)
{
    Vertex p_buf[4] ;

    if( inner_product( &element->patch->plane_equ.n, &view_vec ) < F_ZERO )
        return ;

    if( mode == DISPLAY_SHADED )
        {
            _display_shaded_triangle( element->ev1, element->ev2,
                                     element->ev3,
                                     element->e12, element->e23, element->e31, process_id ) ;
        }
    else if( mode == DISPLAY_FILLED )
        {
            g_rgb( element->rad ) ;
            p_buf[0] = element->ev1->p ;
            p_buf[1] = element->ev2->p ;
            p_buf[2] = element->ev3->p ;

            g_polygon( 3, p_buf ) ;
        }
    else
        {
            g_line( &element->ev1->p, &element->ev2->p ) ;
            g_line( &element->ev2->p, &element->ev3->p ) ;
            g_line( &element->ev3->p, &element->ev1->p ) ;
        }
}

static void _display_shaded_triangle(ElemVertex *ev1, ElemVertex *ev2, ElemVertex *ev3, Edge *e12, Edge *e23, Edge *e31, long process_id)
{
    Vertex p_buf[4] ;
    Rgb   c_buf[4] ;

    p_buf[0] = ev1->p ;
    p_buf[1] = ev2->p ;
    p_buf[2] = ev3->p ;
    c_buf[0] = ev1->col ;
    c_buf[1] = ev2->col ;
    c_buf[2] = ev3->col ;
    g_spolygon( 3, p_buf, c_buf ) ;
}


/************************************************************************
 *
 *    display_elements_in_patch()
 *
 *************************************************************************/

void display_elements_in_patch(Patch *patch, long mode, long process_id)
{
    foreach_leaf_element_in_patch( patch, display_element, mode, process_id ) ;
    g_flush() ;
}


/************************************************************************
 *
 *    display_elements_in_bsp_tree()
 *
 *************************************************************************/

void display_elements_in_bsp_tree(long mode, long process_id)
{
    foreach_depth_sorted_patch( &view_vec, display_elements_in_patch, mode, process_id );
}

/************************************************************************
 *
 *    display_interactions_in_element()
 *
 *************************************************************************/

void display_interactions_in_element(Element *elem, long mode, long process_id)
{

    foreach_interaction_in_element( elem, _disp_interactions, mode, process_id ) ;
    g_flush() ;
}


static void _disp_interactions(Element *elem, Interaction *inter, long mode, long process_id)
{
    Vertex pa, pb ;
    Element *edst ;


    /* Display interactions only with a particular patch */
    if(   (mode == DISPLAY_HALF_INTERACTIONS)
       && (inter->destination->patch->seq_no >= elem->patch->seq_no ) )
        return ;

    /* Compute mid point of the element */
    edst = inter->destination ;
    center_point( &elem->ev1->p, &elem->ev2->p, &elem->ev3->p, &pa ) ;
    center_point( &edst->ev1->p, &edst->ev2->p, &edst->ev3->p, &pb ) ;

    /* Draw a line */
    g_line( &pa, &pb ) ;
}



/************************************************************************
 *
 *    display_interactions_in_patch
 *
 *************************************************************************/

void display_interactions_in_patch(Patch *patch, long mode, long process_id)
{
    foreach_element_in_patch( patch, display_interactions_in_element, mode, process_id );
}

/************************************************************************
 *
 *    display_interactions_in_bsp_tree
 *
 *************************************************************************/

void display_interactions_in_bsp_tree(long process_id)
{
    foreach_patch_in_bsp( display_interactions_in_patch, DISPLAY_ALL_INTERACTIONS, process_id ) ;
}



/************************************************************************
 *************************************************************************
 *
 *  PostScript Version driver
 *
 *************************************************************************
 *************************************************************************/

/************************************************************************
 *
 *    ps_display_scene()
 *
 *************************************************************************/


void ps_display_scene(long fill_sw, long patch_sw, long mesh_sw, long interaction_sw, long process_id)
{
    if( fill_sw )
        {
            /* Fill surfaces */
            ps_display_elements_in_bsp_tree( DISPLAY_SHADED, process_id ) ;
        }
    if( mesh_sw )
        {
            /* Draw mesh */
            ps_linewidth( 0.5 ) ;
            ps_display_elements_in_bsp_tree( DISPLAY_EDGEONLY, process_id ) ;
        }
    if( patch_sw )
        {
            /* Draw patches */
            ps_linewidth( 1.2 ) ;
            ps_display_patches_in_bsp_tree( DISPLAY_EDGEONLY, process_id ) ;
        }
    if( interaction_sw )
        {
            /* Draw interactions */
            ps_linewidth( 0.2 ) ;
            ps_display_interactions_in_bsp_tree(process_id) ;
        }

}

/************************************************************************
 *
 *    ps_display_patch()
 *
 *************************************************************************/

void ps_display_patch(Patch *patch, long mode, long process_id)
{
    Vertex p_buf[4] ;
    Rgb   c_buf[4] ;

    if( mode == DISPLAY_SHADED )
        {
            if( inner_product( &patch->plane_equ.n, &view_vec ) < F_ZERO )
                return ;
            p_buf[0] = patch->p1 ;
            p_buf[1] = patch->p2 ;
            p_buf[2] = patch->p3 ;
            c_buf[0] = patch->color ;
            c_buf[1] = patch->color ;
            c_buf[2] = patch->color ;

            ps_spolygon( 3, p_buf, c_buf ) ;
        }
    else if( mode == DISPLAY_FILLED )
        {
            if( inner_product( &patch->plane_equ.n, &view_vec ) < F_ZERO )
                return ;
            p_buf[0] = patch->p1 ;
            p_buf[1] = patch->p2 ;
            p_buf[2] = patch->p3 ;

            ps_polygon( 3, p_buf ) ;
        }
    else
        {
            p_buf[0] = patch->p1 ;
            p_buf[1] = patch->p2 ;
            p_buf[2] = patch->p3 ;

            ps_polygonedge( 3, p_buf ) ;
        }
}


/************************************************************************
 *
 *    ps_display_patches_in_bsp_tree()
 *
 *************************************************************************/

void ps_display_patches_in_bsp_tree(long mode, long process_id)
{
    foreach_depth_sorted_patch( &view_vec, ps_display_patch, (long)mode, process_id ) ;
}



/************************************************************************
 *
 *    ps_display_element()
 *
 *************************************************************************/

void ps_display_element(Element *element, long mode, long process_id)
{
    Vertex p_buf[4] ;
    Rgb   c_buf[4] ;

    if( mode == DISPLAY_SHADED )
        {
            if( inner_product( &element->patch->plane_equ.n, &view_vec )
               < F_ZERO )
                return ;
            p_buf[0] = element->ev1->p ;
            p_buf[1] = element->ev2->p ;
            p_buf[2] = element->ev3->p ;
            c_buf[0] = element->rad ;
            c_buf[1] = element->rad ;
            c_buf[2] = element->rad ;

            ps_spolygon( 3, p_buf, c_buf ) ;
        }
    else if( mode == DISPLAY_FILLED )
        {
            if( inner_product( &element->patch->plane_equ.n, &view_vec )
               < F_ZERO )
                return ;
            p_buf[0] = element->ev1->p ;
            p_buf[1] = element->ev2->p ;
            p_buf[2] = element->ev3->p ;

            ps_polygon( 3, p_buf ) ;
        }
    else
        {
            p_buf[0] = element->ev1->p ;
            p_buf[1] = element->ev2->p ;
            p_buf[2] = element->ev3->p ;

            ps_polygonedge( 3, p_buf ) ;
        }
}


/************************************************************************
 *
 *    ps_display_elements_in_patch()
 *
 *************************************************************************/

void ps_display_elements_in_patch(Patch *patch, long mode, long process_id)
{
    foreach_leaf_element_in_patch( patch, ps_display_element, mode, process_id ) ;
}


/************************************************************************
 *
 *    ps_display_elements_in_bsp_tree()
 *
 *************************************************************************/

void ps_display_elements_in_bsp_tree(long mode, long process_id)
{
    foreach_depth_sorted_patch( &view_vec, ps_display_elements_in_patch, mode, process_id ) ;
}

/************************************************************************
 *
 *    ps_display_interactions_in_element()
 *
 *************************************************************************/

void ps_display_interactions_in_element(Element *elem, long mode, long process_id)
{
    foreach_interaction_in_element( elem, _ps_disp_interactions, mode, process_id ) ;
}


static void _ps_disp_interactions(Element *elem, Interaction *inter, long mode, long process_id)
{
    Vertex pa, pb ;
    Element *edst ;

    /* Display interactions only with a particular patch */
    if(   (mode == DISPLAY_HALF_INTERACTIONS)
       && (inter->destination->patch->seq_no >= elem->patch->seq_no ) )
        return ;

    /* Compute mid point of the element */
    edst = inter->destination ;
    center_point( &elem->ev1->p, &elem->ev2->p, &elem->ev3->p, &pa ) ;
    center_point( &edst->ev1->p, &edst->ev2->p, &edst->ev3->p, &pb ) ;

    /* Draw a line */
    ps_line( &pa, &pb ) ;
}



/************************************************************************
 *
 *    ps_display_interactions_in_patch
 *
 *************************************************************************/

void ps_display_interactions_in_patch(Patch *patch, long mode, long process_id)
{
    foreach_element_in_patch( patch, ps_display_interactions_in_element, mode, process_id );
}

/************************************************************************
 *
 *    ps_display_interactions_in_bsp_tree
 *
 *************************************************************************/

void ps_display_interactions_in_bsp_tree(long process_id)
{
    foreach_patch_in_bsp( ps_display_interactions_in_patch, DISPLAY_ALL_INTERACTIONS, process_id ) ;
}

