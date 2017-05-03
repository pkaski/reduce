/* 
 * This file is part of 'reduce', an experimental software implementation of
 * adaptive prefix-assignment symmetry reduction; cf.
 *
 * T. Junttila, M. Karppa, P. Kaski, J. Kohonen,
 * "An adaptive prefix-assignment technique for symmetry reduction".
 *
 * This experimental source code is supplied to accompany the 
 * aforementioned manuscript. 
 * 
 * The source code is subject to the following license.
 * 
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 T. Junttila, M. Karppa, P. Kaski, J. Kohonen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */

/********************************************************* Graph operations. */

#ifndef GRAPH_READ
#define GRAPH_READ

#include <stdio.h>
#include <stdlib.h>

/******************************************************* External interface. */

/* Graph data type. */

struct        graph_struct;
typedef       struct graph_struct  graph_t;

graph_t *       graph_alloc          (int order);
void            graph_free           (graph_t *g);
void            graph_empty          (graph_t *g);
void            graph_add_edge       (graph_t *g, int i, int j);
graph_t *       graph_dup            (graph_t *g);
graph_t *       graph_relabel        (graph_t *g, int *p);
graph_t *       graph_relabel_inv    (graph_t *g, int *p);
int             graph_order          (graph_t *g);
long            graph_num_edges      (graph_t *g);

int *           graph_lab            (graph_t *g);
int *           graph_ptn            (graph_t *g);

const int *     graph_can_lab        (graph_t *g);
graph_t *       graph_can_form       (graph_t *g);
const int *     graph_aut_idx        (graph_t *g);
const int *     graph_stab_seq       (graph_t *g);
const int *     graph_aut_gen        (graph_t *g);
const int *     graph_orbits         (graph_t *g);
int             graph_same_orbit     (graph_t *g, int i, int j);
const int *     graph_orbit_cells    (graph_t *g);

graph_t *       graph_parse          (FILE *in);
void            graph_print          (FILE *out, graph_t *g);
void            graph_print_orbits   (FILE *out, graph_t *g, int l, int *m);

/* Bag of graphs data type. */

struct        graphbag_struct;
typedef       struct graphbag_struct  graphbag_t;

graphbag_t *    graphbag_alloc       (void);
void            graphbag_free        (graphbag_t *b);
void            graphbag_empty       (graphbag_t *b);
int             graphbag_query       (graphbag_t *b, graph_t *g);
int             graphbag_insert      (graphbag_t *b, graph_t *g);

#endif
