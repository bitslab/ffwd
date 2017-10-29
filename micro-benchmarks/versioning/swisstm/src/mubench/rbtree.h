/* =============================================================================
 *
 * rbtree.h
 * -- Red-black balanced binary search tree
 *
 * =============================================================================
 *
 * Copyright (C) Sun Microsystems Inc., 2006.  All Rights Reserved.
 * Authors: Dave Dice, Nir Shavit, Ori Shalev.
 *
 * STM: Transactional Locking for Disjoint Access Parallelism
 *
 * Transactional Locking II,
 * Dave Dice, Ori Shalev, Nir Shavit
 * DISC 2006, Sept 2006, Stockholm, Sweden.
 *
 * =============================================================================
 *
 * Modified by Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef RBTREE_H
#define RBTREE_H 1


#include <inttypes.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct node {
    intptr_t k;
    intptr_t v;
    struct node* p;
    struct node* l;
    struct node* r;
    intptr_t c;
} node_t;


typedef struct rbtree {
    node_t* root;
} rbtree_t;


/* =============================================================================
 * rbtree_verify
 * =============================================================================
 */
int
rbtree_verify (rbtree_t* s, int verbose);


/* =============================================================================
 * rbtree_alloc
 * =============================================================================
 */
rbtree_t*
rbtree_alloc ();


/* =============================================================================
 * TMrbtree_alloc
 * =============================================================================
 */
rbtree_t*
TMrbtree_alloc (TM_ARGDECL_ALONE);


/* =============================================================================
 * rbtree_free
 * =============================================================================
 */
void
rbtree_free (rbtree_t* r);


/* =============================================================================
 * TMrbtree_free
 * =============================================================================
 */
void
TMrbtree_free (TM_ARGDECL  rbtree_t* r);


/* =============================================================================
 * rbtree_insert
 * =============================================================================
 */
int
rbtree_insert (rbtree_t* r, int key, int val);


/* =============================================================================
 * TMrbtree_insert
 * =============================================================================
 */
int
TMrbtree_insert (TM_ARGDECL  rbtree_t* r, int key, int val);


/* =============================================================================
 * rbtree_delete
 * =============================================================================
 */
int
rbtree_delete (rbtree_t* r, int key);


/* =============================================================================
 * TMrbtree_delete
 * =============================================================================
 */
int
TMrbtree_delete (TM_ARGDECL  rbtree_t* r, int key);


/* =============================================================================
 * rbtree_update
 * =============================================================================
 */
int
rbtree_update (rbtree_t* r, int key, int val);


/* =============================================================================
 * TMrbtree_update
 * =============================================================================
 */
int
TMrbtree_update (TM_ARGDECL  rbtree_t* r, int key, int val);


/* =============================================================================
 * rbtree_get
 * =============================================================================
 */
int
rbtree_get (rbtree_t* r, int key);


/* =============================================================================
 * TMrbtree_get
 * =============================================================================
 */
int
TMrbtree_get (TM_ARGDECL  rbtree_t* r, int key);


/* =============================================================================
 * rbtree_contains
 * =============================================================================
 */
int
rbtree_contains (rbtree_t* r, int key);


/* =============================================================================
 * TMrbtree_contains
 * =============================================================================
 */
int
TMrbtree_contains (TM_ARGDECL  rbtree_t* r, int key);


#define TMRBTREE_ALLOC()          TMrbtree_alloc(TM_ARG_ALONE)
#define TMRBTREE_FREE(r)          TMrbtree_free(TM_ARG  r)
#define TMRBTREE_INSERT(r, k, v)  TMrbtree_insert(TM_ARG  r, (int)(k), (int)(v))
#define TMRBTREE_DELETE(r, k)     TMrbtree_delete(TM_ARG  r, (int)(k))
#define TMRBTREE_UPDATE(r, k, v)  TMrbtree_update(TM_ARG  r, (int)(k), (int)(v))
#define TMRBTREE_GET(r, k)        TMrbtree_get(TM_ARG  r, (int)(k))
#define TMRBTREE_CONTAINS(r, k)   TMrbtree_contains(TM_ARG  r, (int)(k))


#ifdef __cplusplus
}
#endif



#endif /* RBTREE_H */



/* =============================================================================
 *
 * End of rbtree.h
 *
 * =============================================================================
 */
