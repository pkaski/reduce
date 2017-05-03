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

/******************************************************* Symmetry reduction. */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "graph.h"
#include "gmp.h"

/******************************* A rudimentary command-line argument parser. */

#define ARG_NO_PARAM         0
#define ARG_STRING_PARAM     1
#define ARG_LONG_PARAM       2
#define ARG_INT_ARRAY_PARAM  3

struct argdef_struct 
{
    char     short_desc;
    char *   long_desc;
    int      type;
};

typedef struct argdef_struct argdef_t;

int argdefs_count = 0; // automatically initialized based on sentinel

argdef_t argdefs[] = {
    { 'h', "help",          ARG_NO_PARAM },
    { 'u', "usage",         ARG_NO_PARAM },
    { 'v', "verbose",       ARG_NO_PARAM },
    { 'g', "graph",         ARG_NO_PARAM },
    { 'n', "no-cnf",        ARG_NO_PARAM },
    { 's', "symmetry-only", ARG_NO_PARAM },
    { 'i', "incremental",   ARG_NO_PARAM },
    { 't', "threshold",     ARG_LONG_PARAM },
    { 'l', "length",        ARG_LONG_PARAM },
    { 'p', "prefix",        ARG_INT_ARRAY_PARAM },
    { 'f', "file",          ARG_STRING_PARAM },
    { 'o', "output",        ARG_STRING_PARAM },
    { 'Z', "ZZZZZZ",        ARG_NO_PARAM } }; // sentinel last argument

struct argparse_struct
{
    int      argc;
    char **  argv;
    int      num_parsed_args;
    char **  parsed_args;
    void **  parsed_params; 
};

typedef struct argparse_struct argparse_t;

int arg_get_long(argparse_t *p, int c, int i)
{
    long l;
    if(i+1 >= p->argc)
        ERROR("expected an integer parameter to '%s' but ran out of arguments",
              p->parsed_args[c]);
    if(sscanf(p->argv[i+1], "%ld", &l) != 1)
        ERROR("parse error in parameter '%s' to '%s' at the command line",
              p->argv[i+1],
              p->parsed_args[c]);
    long *ll = (long *) MALLOC(sizeof(long));
    *ll = l;
    p->parsed_params[c] = (void *) ll;
    return i+1;
}

int arg_get_string(argparse_t *p, int c, int i)
{
    if(i+1 >= p->argc)
        ERROR("expected a string parameter to '%s' but ran out of arguments",
              p->parsed_args[c]);
    if(p->argv[i+1][0] == '-')
        ERROR("expected a string parameter to '%s' but got the argument '%s'",
              p->parsed_args[c], p->argv[i+1]);
    char *t = (char *) MALLOC(sizeof(char)*(strlen(p->argv[i+1])+1));
    strcpy(t, p->argv[i+1]);
    p->parsed_params[c] = (void *) t;
    return i+1;
}

int arg_get_int_array(argparse_t *p, int c, int i)
{
    int j = i+1;
    for(; j < p->argc && p->argv[j][0] != '-'; j++)
        ;
    int length = j - (i+1);
    int *a = (int *) MALLOC(sizeof(int)*(length + 1));
    a[0] = length;
    p->parsed_params[c] = (void *) a;
    int *b = a+1;
    for(int k = 0; k < length; k++) {
        int v;
        if(sscanf(p->argv[i+1+k], "%d", &v) != 1)
            ERROR("parse error in parameter '%s' to '%s' at the command line",
                  p->argv[i+1+k],
                  p->parsed_args[c]);
        b[k] = v-1;
    }
    return j-1;
}

argparse_t *arg_parse(int argc, char **argv)
{
    argparse_t *p = (argparse_t *) MALLOC(sizeof(argparse_t));
    p->argc = argc;
    p->argv = argv;

    for(argdefs_count = 0; 
        argdefs[argdefs_count].short_desc != 'Z'; 
        argdefs_count++)
        ;

    /* Check and count the arguments before parsing them. */
    int c = 0;
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-' && argv[i][1] != '-') {
            /* Short-form argument. */
            for(int j = 1; argv[i][j] != 0; j++) {
                char d = argv[i][j];
                int k = 0;
                for(; k < argdefs_count; k++) {
                    if(argdefs[k].short_desc == d) {
                        c++;
                        switch(argdefs[k].type) {
                        case ARG_NO_PARAM:
                            break;
                        case ARG_LONG_PARAM:
                        case ARG_INT_ARRAY_PARAM:
                        case ARG_STRING_PARAM:
                            if(argv[i][j+1] != 0)
                                ERROR("argument '-%c' should be immediately "
                                      "followed by a parameter", d);
                            break;
                        }
                        break;
                    }
                }
                if(k == argdefs_count)
                    ERROR("unrecognized argument '-%c' at command line", d);
            }           
        }
        if(argv[i][0] == '-' && argv[i][1] == '-') {
            /* Long-form argument. */
            char *d = argv[i] + 2;
            int k = 0;
            for(; k < argdefs_count; k++) {
                if(!strcmp(argdefs[k].long_desc, d)) {
                    c++;
                    break;
                }
            }           
            if(k == argdefs_count)
                ERROR("unrecognized argument '--%s' at command line", d);
        }
    }

    /* Allocate parameter arrays. */

    p->num_parsed_args = c;
    p->parsed_args     = (char **) MALLOC(sizeof(char *)*c);
    p->parsed_params   = (void **) MALLOC(sizeof(void *)*c);
    
    /* Now parse. */

    c = 0;
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-' && argv[i][1] != '-') {
            /* Short-form argument. */
            int stop = 0;
            for(int j = 1; !stop && argv[i][j] != 0; j++) {
                char d = argv[i][j];
                int k = 0;
                for(; k < argdefs_count; k++) {
                    if(argdefs[k].short_desc == d) {
                        p->parsed_args[c] = argdefs[k].long_desc;
                        for(int u = 0; u < c; u++)
                            if(!strcmp(p->parsed_args[c],
                                       p->parsed_args[u]))
                                ERROR("duplicate argument '%s' at command line",
                                      p->parsed_args[c]);
                        switch(argdefs[k].type) {
                        case ARG_NO_PARAM:
                            p->parsed_params[c] = NULL;
                            break;
                        case ARG_LONG_PARAM:
                            i = arg_get_long(p, c, i);
                            stop = 1;
                            break;                          
                        case ARG_INT_ARRAY_PARAM:
                            i = arg_get_int_array(p, c, i);
                            stop = 1;
                            break;
                        case ARG_STRING_PARAM:
                            i = arg_get_string(p, c, i);
                            stop = 1;
                            break;
                        default:
                            ABORT("bad internal argument definitions");
                            break;
                        }
                        c++;
                        break;
                    }
                }
                if(k == argdefs_count)
                    ERROR("unrecognized argument '-%c' at command line", d);
            }           
        } else {
            if(argv[i][0] == '-' && argv[i][1] == '-') {
                /* Long-form argument. */
                char *d = argv[i] + 2;
                int k = 0;
                for(; k < argdefs_count; k++) {
                    if(!strcmp(argdefs[k].long_desc, d)) {
                        p->parsed_args[c] = argdefs[k].long_desc;
                        for(int u = 0; u < c; u++)
                            if(!strcmp(p->parsed_args[c],
                                       p->parsed_args[u]))
                                ERROR("duplicate argument '%s' at command line",
                                      p->parsed_args[c]);
                        switch(argdefs[k].type) {
                        case ARG_NO_PARAM:
                            p->parsed_params[c] = NULL;
                            break;
                        case ARG_LONG_PARAM:
                            i = arg_get_long(p, c, i);
                            break;                          
                        case ARG_INT_ARRAY_PARAM:
                            i = arg_get_long(p, c, i);
                            break;
                        case ARG_STRING_PARAM:
                            i = arg_get_string(p, c, i);
                            break;
                        default:
                            ABORT("bad internal argument definitions");
                            break;
                        }
                        c++;
                        break;
                    }
                }           
                if(k == argdefs_count)
                    ERROR("unrecognized argument '--%s' at command line", d);
            } else {
                ERROR("bad argument '%s' at command line", argv[i]);
            }
        }
    }

    return p;
}

void arg_free(argparse_t *p)
{
    for(int i = 0; i < p->num_parsed_args; i++)
        if(p->parsed_params[i] != NULL)
            FREE(p->parsed_params[i]);
    FREE(p->parsed_params);
    FREE(p->parsed_args);
    FREE(p);    
}

int arg_have(argparse_t *p, const char *s) 
{
    for(int i = 0; i < p->num_parsed_args; i++)
        if(!strcmp(s, p->parsed_args[i]))
            return 1;
    return 0;       
}

char *arg_string(argparse_t *p, const char *s)
{
    for(int i = 0; i < p->num_parsed_args; i++) {
        if(!strcmp(s, p->parsed_args[i]))
            return (char *) p->parsed_params[i];
    }
    ABORT("argument %s not found", s);
    return (char *) 0;
}

long arg_long(argparse_t *p, const char *s)
{
    for(int i = 0; i < p->num_parsed_args; i++) {
        if(!strcmp(s, p->parsed_args[i]))
            return *((long *) p->parsed_params[i]);
    }
    ABORT("argument %s not found", s);
    return 0;
}

int *arg_int_array(argparse_t *p, const char *s)
{
    for(int i = 0; i < p->num_parsed_args; i++) {
        if(!strcmp(s, p->parsed_args[i]))
            return (int *) p->parsed_params[i];
    }
    ABORT("argument %s not found", s);
    return (int *) 0;
}

void arg_print(FILE *out, argparse_t *p)
{
    for(int i = 0; i < p->num_parsed_args; i++) {
        fprintf(out, "[%d] %s:", i, p->parsed_args[i]);
        int k = 0;
        for(; k < argdefs_count; k++) {
            if(!strcmp(argdefs[k].long_desc, p->parsed_args[i])) {
                switch(argdefs[k].type) {
                case ARG_NO_PARAM:
                    fprintf(out, " [no parameters]\n");
                    break;
                case ARG_LONG_PARAM:
                    fprintf(out, " %ld\n", arg_long(p, p->parsed_args[i]));
                    break;                          
                case ARG_STRING_PARAM:
                    fprintf(out, " %s\n", arg_string(p, p->parsed_args[i]));
                    break;
                case ARG_INT_ARRAY_PARAM:
                    {                       
                    int *a = arg_int_array(p, p->parsed_args[i]);
                    int l = a[0];
                    int *b = a + 1;
                    for(int j = 0; j < l; j++)
                        fprintf(out, " %d", b[j] + 1);
                    fprintf(out, "\n");
                    }
                    break;
                }
            }
        }
    }   
}

/*************************************************** Reducer data structure. */

struct reducer_struct
{
    int         nv;              /* Number of variables in the CNF instance. */
    long        nc;              /* Number of clauses in the CNF instance. */
    int         *clauses;        /* The clauses. */
    int         have_cnf;        /* Have CNF? */

    int         n;               /* Number of vertices in base graph. */
    graph_t     *base;           /* The base graph. */
    int         v;               /* Number of variables. */
    int         *var;            /* Variable vertices in base graph. */
    char        **var_legend;    /* String identifiers for the variables. */
    int         *var_trans;      /* Translation from graph to CNF variables. */ 
    int         r;               /* Number of values. */
    int         *val;            /* Value vertices in base graph. */
    char        **val_legend;    /* String identifiers for the values. */

    long        t;               /* Threshold size for automorphism group. */

    int         prefix_capacity; /* Prefix capacity. */   
    int         target_length;   /* Prefix target length. */
    int         k;               /* Length of prefix sequence. */
    int         *prefix;         /* The prefix sequence. */   
    int         a;               /* Number of assignments in prefix sequence. */
    int         *asgn;           /* The value assignments in the sequence. */

    int         initialized;     /* Initialized? */

    int         **orbits;        /* Indicators for prefix element orbits.*/
    int         *trav_sizes;     /* Traversal sizes. */
    int         **trav_ind;      /* Traversal indicators. */
    int         ***traversals;   /* The traversal permutations. */
    graph_t     *last_prefix_g;  /* Last graph in the prefix sequence. */
    
    int         *work;           /* The work stack. */
    int         **seed_min;      /* Indicators for seed-orbit minima. */
    int         *scratch;        /* Scratch. */
    int         stack_top;       /* Position of the stack top. */ 

    long        *stat_gen;       /* Generated assignments. */
    long        *stat_can;       /* Canonical assignments. */
    long        *stat_out;       /* Assignments output. */

    int         verbose;         /* Verbose output? */
};

typedef struct reducer_struct reducer_t;

/***************************************** Subroutines for orbit traversals. */

int traversal_prepare(int ***trav, int root, graph_t *g)
{
    int n = graph_order(g);
    if(root < 0 || root >= n)
        ABORT("bad root");
    int len = 0;
    int *ind = (int *) MALLOC(sizeof(int)*n);
    int *list = (int *) MALLOC(sizeof(int)*n);
    int rootpos = -1;
    for(int i = 0; i < n; i++) {
        if(graph_same_orbit(g, i, root)) {
            ind[i] = len;
            list[len] = i;
            if(i == root)
                rootpos = len;
            len++;
        } else {
            ind[i] = -(n+1);
        }
    }
    int **t = (int **) MALLOC(sizeof(int *)*len);
    for(int i = 0; i < len; i++)
        t[i] = (int *) MALLOC(sizeof(int)*n);
    ind[root] = -1;
    for(int i = 0; i < n; i++)
        t[rootpos][i] = i;
    while(1) {
        int done = 1;
        for(int j = 0; j < len; j++) {
            if(ind[list[j]] >= 0) {
                done = 0;
                break;
            }
        }
        if(done)
            break;
        const int *p = NULL;
        while((p = graph_aut_gen(g)) != NULL) {
            for(int j = 0; j < len; j++) {
                int u = list[j];
                int v = p[u];
                int q = ind[v];
                if(q >= 0 && ind[u] < 0) {
                    for(int i = 0; i < n; i++)
                        t[q][i] = p[t[j][i]];
                    ind[v] = -1;
                }
            }
        }
    }
    *trav = t;
    for(int j = 0; j < len; j++)
        if(t[j][root] != list[j])
            ABORT("bad traversal");

    FREE(list);
    FREE(ind);
    return len;
}

void traversal_release(int length, int **trav)
{
    for(int i = 0; i < length; i++)
        FREE(trav[i]);
    FREE(trav);
}

/****************************************************** Parse reducer input. */

static void enlarge_int_array(int **a, int capacity, int new_capacity)
{
    int *t = (int *) MALLOC(sizeof(int)*new_capacity);
    if(capacity >= new_capacity)
        ABORT("new capacity is no larger than old");    
    int *s = *a;
    for(int i = 0; i < capacity; i++)
        t[i] = s[i];
    FREE(s);
    *a = t;
}

static void enlarge_long_array(long **a, int capacity, int new_capacity)
{
    long *t = (long *) MALLOC(sizeof(long)*new_capacity);
    if(capacity >= new_capacity)
        ABORT("new capacity is no larger than old");    
    long *s = *a;
    for(int i = 0; i < capacity; i++)
        t[i] = s[i];
    FREE(s);
    *a = t;
}

static void enlarge_p_array(void ***a, int capacity, int new_capacity) 
{
    void **t = (void **) MALLOC(sizeof(void *)*new_capacity);
    if(capacity >= new_capacity)
        ABORT("new capacity is no larger than old");    
    void **s = *a;
    for(int i = 0; i < capacity; i++)
        t[i] = s[i];
    FREE(s);
    *a = t;
}

void reducer_enlarge_prefix(reducer_t *r, int capacity)
{
    int c = r->prefix_capacity;
    
    enlarge_int_array(&r->prefix, c, capacity);    
    enlarge_int_array(&r->asgn, c, capacity);

    if(r->initialized) {
        enlarge_p_array((void ***) &r->orbits, c, capacity);
        enlarge_p_array((void ***) &r->seed_min, c, capacity);
        enlarge_int_array(&r->trav_sizes, c, capacity);
        enlarge_p_array((void ***) &r->trav_ind, c, capacity);
        enlarge_p_array((void ***) &r->traversals, c, capacity);
        enlarge_int_array(&r->work, 
                          (2*c+1)*(c+1), 
                          (2*capacity+1)*(capacity+1));
        enlarge_int_array(&r->scratch,
                          2*c+2,
                          2*capacity+2);
        enlarge_long_array(&r->stat_gen, c, capacity);
        enlarge_long_array(&r->stat_can, c, capacity);
        enlarge_long_array(&r->stat_out, c, capacity);
    }
    r->prefix_capacity = capacity;
}

void eat_comment_lines(FILE *in)
{
    int c;
    while((c = getc(in)) == 'c')
        while((c = getc(in)) != '\n' && c != EOF)
            ;
    if(c != 'c' && c != EOF)
        ungetc(c, in);
}

reducer_t *reducer_parse(FILE *in, argparse_t *p)
{
    reducer_t *r = (reducer_t *) MALLOC(sizeof(reducer_t));

    r->verbose = 0;
    if(arg_have(p, "verbose"))
        r->verbose = 1;

    if(!arg_have(p, "no-cnf")) {
        /* Parse CNF from input. */
        int nv;
        long nc;
        eat_comment_lines(in);
        if(fscanf(in, "p cnf %d %ld\n", &nv, &nc) != 2)
            ERROR("parse error -- CNF format line expected");
        if(nv < 1)
            ERROR("bad number-of-variables parameter (n = %d) in CNF", nv);
        if(nc < 0)
            ERROR("bad number-of-clauses parameter (c = %ld) in CNF", nc);
        long cursor = 0;
        long buffer_capacity = 128;
        int *buf = (int *) MALLOC(sizeof(int)*buffer_capacity);
        for(long c = 0; c < nc; c++) {
            while(1) {
                int l;
                if(cursor == 0)
                    eat_comment_lines(in);
                if(fscanf(in, "%d ", &l) != 1)
                    ERROR("parse error -- CNF literal expected");
                if(abs(l) > nv)
                    ERROR("bad literal %d in CNF input (n = %d)", l, nv);
                if(cursor == buffer_capacity) {
                    int *temp = buf;
                    int new_capacity = 2*buffer_capacity;
                    buf = (int *) MALLOC(sizeof(int)*2*new_capacity);
                    for(long u = 0; u < buffer_capacity; u++)
                        buf[u] = temp[u];
                    buffer_capacity = new_capacity;
                    FREE(temp);
                }
                buf[cursor++] = l;
                if(l == 0)
                    break;
            }
        }
        eat_comment_lines(in);
        r->nv = nv;
        r->nc = nc;
        r->clauses = buf;
        r->have_cnf = 1;
        fscanf(in, "\n");
    } else {
        r->have_cnf = 0;
    }
        
    if(arg_have(p, "graph")) {
        /* Parse the graph of symmetries from input. */

        r->base = graph_parse(in);
        int n = graph_order(r->base);
        r->n = n;

        int v;
        if(fscanf(in, "p variable %d\n", &v) != 1)
            ERROR("parse error -- variable format line expected");
        if(v < 1)
            ERROR("bad variable parameter v = %d", v);
        r->v = v;
        r->var = (int *) MALLOC(sizeof(int)*v);
        r->var_legend = (char **) MALLOC(sizeof(char *)*v);
        char temp[100];
        for(int i = 0; i < v; i++) {
            int u;
            if(fscanf(in, "v %d %50s\n", &u, temp) != 2)
                ERROR("parse error -- variable line expected");
            if(u < 1 || u > n)
                ERROR("bad variable identifier u = %d", u);
            r->var[i] = u-1;
            r->var_legend[i] = (char *) MALLOC(sizeof(char)*(strlen(temp)+1));
            strcpy(r->var_legend[i], temp);
        }
        
        int d;
        if(fscanf(in, "p value %d\n", &d) != 1)
            ERROR("parse error -- value format line expected");
        if(d < 1)
            ERROR("bad value parameter r = %d", d);
        r->r = d;
        r->val = (int *) MALLOC(sizeof(int)*d);
        r->val_legend = (char **) MALLOC(sizeof(char *)*d);
        
        for(int i = 0; i < d; i++) {
            int u;
            if(fscanf(in, "r %d %50s\n", &u, temp) != 2)
                ERROR("parse error -- value line expected");
            if(u < 1 || u > n)
                ERROR("bad value identifier u = %d", u);
            r->val[i] = u-1;
            r->val_legend[i] = (char *) MALLOC(sizeof(char)*(strlen(temp)+1));
            strcpy(r->val_legend[i], temp);
        }
    } else {
        /* Build the graph of symmetries from CNF. */

        if(!r->have_cnf)
            ERROR("cannot build the symmetry graph since no CNF was given");
        int  nv = r->nv;
        long nc = r->nc;
        int   n = 3*nv + 2 + nc;

        graph_t *g = graph_alloc(n);

        for(int i = 0; i < nv; i++) {
            graph_add_edge(g, i, nv + i);
            graph_add_edge(g, i, 2*nv + i);
        }

        long cursor = 0;
        int *buf = r->clauses;
        for(long c = 0; c < nc; c++) {
            while(1) {
                int l = buf[cursor++];
                if(l == 0) {
                    break;
                } else {
                    if(l < 0) {
                        l = (-l)-1;
                        if(l >= nv)
                            ERROR("negative literal (%d) out of range", 
                                  -(l + 1));
                        graph_add_edge(g, nv + l, 3*nv + 2 + c);
                    } else {
                        l = l-1;
                        if(l >= nv)
                            ERROR("positive literal (%d) out of range", 
                                  l + 1);
                        graph_add_edge(g, 2*nv + l, 3*nv + 2 + c);
                    }
                }
            }
        }

        int *colors = (int *) MALLOC(sizeof(int)*n);
        for(int i = 0; i < n; i++)
            colors[i] = -1;
        for(int i = 0; i < nv; i++)
            colors[i] = 0;
        for(int i = 0; i < nv; i++)
            colors[nv + i] = 1;
        for(int i = 0; i < nv; i++)
            colors[2*nv + i] = 2;
        colors[3*nv + 0] = 3;
        colors[3*nv + 1] = 4;
        for(long i = 0; i < nc; i++)
            colors[3*nv + 2 + i] = 5;        
        for(int u = 0; u < n; u++)
            if(colors[u] == -1)
                ABORT("vertex u = %d did not receive a color", u);
        int *lab = graph_lab(g);
        int *ptn = graph_ptn(g);
        heapsort_int_indirect(n, colors, lab);
        for(int i = 0; i < n; i++)
            if(i == n-1 || colors[lab[i]] != colors[lab[i+1]])
                ptn[i] = 0;
            else
                ptn[i] = 1;    
        FREE(colors);

        r->n = n;
        r->base = g;

        r->v = nv;
        r->var = (int *) MALLOC(sizeof(int)*nv);
        r->var_legend = (char **) MALLOC(sizeof(char *)*nv);
        char temp[100];
        for(int i = 0; i < nv; i++) {
            sprintf(temp, "%d", i + 1);
            r->var[i] = i;
            r->var_legend[i] = (char *) MALLOC(sizeof(char)*(strlen(temp)+1));
            strcpy(r->var_legend[i], temp);
        }
        
        r->r = 2;
        r->val = (int *) MALLOC(sizeof(int)*2);
        r->val_legend = (char **) MALLOC(sizeof(char *)*2);

        r->val[0] = 3*nv + 0;
        {
            const char *s = "false";
            r->val_legend[0] = (char *) MALLOC(sizeof(char)*(strlen(s)+1));
            strcpy(r->val_legend[0], s);
        }       
        r->val[1] = 3*nv + 1;
        {
            const char *s = "true";
            r->val_legend[1] = (char *) MALLOC(sizeof(char)*(strlen(s)+1));
            strcpy(r->val_legend[1], s);
        }       
    }
    if(!arg_have(p, "prefix") && !arg_have(p, "length")) {
        /* Read the prefix from input. */
        
        int n = r->n;
        int k;
        int a;
        long t;
        if(fscanf(in, "p prefix %d %d %ld\n", &k, &a, &t) != 3)
            ERROR("parse error -- prefix format line expected");
        if(k < 0 || a < 0 || a > k || t < 0)
            ERROR("bad prefix parameters k = %d, a = %d, t = %ld", k, a, t);
        r->prefix_capacity = k;
        r->k = k;
        r->a = a;
        r->t = t;
        r->prefix = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
        r->asgn = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
        
        /* Caveat: remove assignment feature 
         *         -- or repair bad assignment check */
        for(int i = 0; i < a; i++) {
            int u, w;
            if(fscanf(in, "a %d %d\n", &u, &w) != 2)
                ERROR("parse error -- assignment line expected");
            if(u < 1 || u > n || w < 1 || w > n)
                ERROR("bad assignment u = %d, w = %d", u, w);
            r->prefix[i] = u-1;
            r->asgn[i] = w-1;
        }
        
        for(int i = a; i < k; i++) {
            int u;
            if(fscanf(in, "f %d\n", &u) != 1)
                ERROR("parse error -- prefix line expected");
            if(u < 1 || u > n)
                ERROR("bad assignment u = %d", u);
            r->prefix[i] = u-1;
        }
    } else {
        if(arg_have(p, "prefix")) {
            /* Build the prefix from the command line argument. */
            
            int *q = arg_int_array(p, "prefix");
            int k = q[0];
            q++;
            int a = 0;
            long t = 0;
            if(k < 0 || a < 0 || a > k || t < 0)
                ERROR("bad prefix parameters k = %d, a = %d, t = %ld", k, a, t);
            r->prefix_capacity = k;
            r->k = k;
            r->a = a;
            r->t = t;
            r->prefix = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
            r->asgn = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
            for(int i = 0; i < k; i++) {
                if(q[i] < 0 || q[i] >= r->n)
                    ERROR("prefix element (%d) out of bounds", q[i]+1);
                r->prefix[i] = q[i];
            }
        } else {
            r->k = 0;
            r->a = 0;
            r->t = 0;
            r->prefix_capacity = 1;
            r->prefix = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
            r->asgn = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
        }
    }
    if(arg_have(p, "length"))
        r->target_length = arg_long(p, "length");
    else
        r->target_length = r->k;
    if(r->target_length <= 0 &&
       r->k == 0)
        ERROR("no prefix given and nonpositive target length for prefix "
              "-- nothing to do");
    if(r->k > r->target_length)
        ERROR("length of given prefix exceeds given target length for prefix");

    r->last_prefix_g = (graph_t *) 0;

    {
        /* Test variables for repeated elements. */
        int *q = (int *) MALLOC(sizeof(int)*r->v);
        for(int i = 0; i < r->v; i++)
            q[i] = r->var[i];
        heapsort_int(r->v, q);
        for(int i = 1; i < r->v; i++)
            if(q[i-1] == q[i])
                ERROR("variable list repeats an element (%d)", q[i] + 1);
        FREE(q);
    }
    {
        /* Test values for repeated elements. */
        int *q = (int *) MALLOC(sizeof(int)*r->r);
        for(int i = 0; i < r->r; i++)
            q[i] = r->val[i];
        heapsort_int(r->r, q);
        for(int i = 1; i < r->r; i++)
            if(q[i-1] == q[i])
                ERROR("value list repeats an element (%d)", q[i] + 1);
        FREE(q);
    }
    {
        /* Test prefix for repeated elements. */
        int *q = (int *) MALLOC(sizeof(int)*r->k);
        for(int i = 0; i < r->k; i++)
            q[i] = r->prefix[i];
        heapsort_int(r->k, q);
        for(int i = 1; i < r->k; i++)
            if(q[i-1] == q[i])
                ERROR("prefix repeats an element (%d)", q[i] + 1);
        FREE(q);
    }

    r->var_trans = (int *) MALLOC(sizeof(int)*r->n);
    if(r->have_cnf) {
        /* Build the translation array from graph variable vertices
         * to selected CNF variables. */

        int *q = (int *) MALLOC(sizeof(int)*r->v);
        for(int i = 0; i < r->n; i++)
            r->var_trans[i] = -1;
        for(int i = 0; i < r->v; i++) {
            int u;
            if(sscanf(r->var_legend[i], "%d", &u) != 1)
                ERROR("parse error in variable legend '%s'", r->var_legend[i]);
            u = u-1;
            if(u < 0 || u >= r->nv)
                ERROR("parsed CNF variable in legend (%d) is out of range",
                      u + 1);
            r->var_trans[r->var[i]] = u;
            q[i] = u;
        }
        heapsort_int(r->v, q);
        for(int i = 1; i < r->v; i++)
            if(q[i-1] == q[i])
                ERROR("repeated CNF variable (%d) in legend", q[i]+1);
        FREE(q);
                
        /* Build the translation array from graph false/true vertices
         * to CNF values, i.e. make sure false and true are present
         * and in this order. */

        if(r->r != 2)
            ERROR("value range does not consist of 'false' and 'true'");
        if(!strcmp(r->val_legend[0], "true") &&
           !strcmp(r->val_legend[1], "false")) {
            int t = r->val[0];
            r->val[0] = r->val[1];
            r->val[1] = t;
            char *s = r->val_legend[0];
            r->val_legend[0] = r->val_legend[1];
            r->val_legend[1] = s;
        } else {
            if(!(!strcmp(r->val_legend[1], "true") &&
                 !strcmp(r->val_legend[0], "false")))
                ERROR("value range does not consist of 'false' and 'true'");
        }                               
    } else {
        for(int i = 0; i < r->v; i++)
            r->var_trans[i] = i;
    }
    for(int i = 0; i < r->k; i++)
        if(r->var_trans[r->prefix[i]] == -1)
            ERROR("prefix element (%d) is not a declared variable vertex", 
                  r->prefix[i]+1);

    r->initialized = 0;
    
    return r;
}

/****************************************** Initialize a configured reducer. */


static void orbit_min_ind(graph_t *g, int *relabel, int *ind)
{
    int n = graph_order(g);
    const int *p = graph_orbit_cells(g);
    const int *c = graph_orbits(g);
    for(int i = 0; i < n; i++)
        ind[i] = 0;
    for(int i = 0; i < n; i++) {
        if(relabel != NULL)
            ind[relabel[p[i]]] = 1;
        else
            ind[p[i]] = 1;
        int j = i+1;
        for(; j < n && c[p[i]] == c[p[j]]; j++)
            ;
        i = j-1;
    }    
}

static void print_aut_order(FILE *out, graph_t *g)
{
    mpz_t aut_order;
    mpz_init(aut_order);
    mpz_set_si(aut_order, 1L);
    const int *ai = graph_aut_idx(g);
    while(*ai != 0) {
        mpz_mul_si(aut_order, aut_order, (long) *ai);
        ai++;
    }
    fprintf(out, "\n   |Aut| = ");
    mpz_out_str(out, 10, aut_order);
    mpz_clear(aut_order);
}

static void print_orbit_perms(FILE *out, graph_t *g, int l, int *m)
{
    int n = graph_order(g);
    const int *p = graph_orbit_cells(g);
    const int *c = graph_orbits(g);
    int *q = (int *) MALLOC(sizeof(int)*n);
    for(int u = 0; u < n; u++)
        q[u] = 0;
    for(int u = 0; u < l; u++)
        q[m[u]] = 1;

    for(int i = 0; i < n; i++) {
        int j = i+1;
        for(; j < n && c[p[i]] == c[p[j]]; j++)
            ;
        if(q[p[i]] > 0) {
            fprintf(out, "orbit: ");
            print_int_array(out, j - i, p + i);
            fprintf(out, "\n");
            const int *a = NULL;
            while((a = graph_aut_gen(g)) != NULL) {
                fprintf(out, "       ");
                for(int u = 0; u < j - i; u++)
                    q[p[u + i]] = 2;
                int u = 0;
                int num_fixed = 0;
                int num_moved = 0;
                while(u < l) {
                    int z = m[u];
                    int first = 1;
                    int len = 0;
                    if(q[z] == 2) {
                        int w = z;
                        do {                        
                            q[w] = 1;
                            fprintf(out, 
                                    "%s%d",
                                    first == 1 ? "(" : " ",
                                    w + 1);
                            first = 0;
                            w = a[w];
                            fprintf(out, "%s",
                                    w == z ? ")" : "");
                            len++;
                        } while(z != w);
                        if(len == 1)
                            num_fixed += len;
                        if(len >= 2)
                            num_moved += len ;                     
                    }
                    u++;
                }
                fprintf(out, " -- fix = %d, move = %d\n", num_fixed, num_moved);
            }
        }
        i = j-1;
    }

    FREE(q);
}

static int orbit_select(graph_t *g, 
                        int l, int *m, 
                        int k, int *f,
                        int *t)
{
    int n = graph_order(g);
    const int *p = graph_orbit_cells(g);
    const int *c = graph_orbits(g);
    int *q = (int *) MALLOC(sizeof(int)*n);
    for(int u = 0; u < n; u++)
        q[u] = 0;
    for(int u = 0; u < l; u++)
        q[m[u]] = 1; // include var vertices
    for(int u = 0; u < k; u++)
        q[f[u]] = 0; // omit existing prefix

    /* By default select the first point from the previous orbit, if any. */
    if(t != NULL) {
        for(int i = 0; i < n; i++) {
            if(q[i] > 0 && t[i] > 0) {
                FREE(q);
                return i;
            }
        }
    }

    /* Otherwise choose an orbit of the maximum length that 
     * has an associated automorphism with both fixed and moved points
     * in the orbit. */
    int max_length = -1;
    int max_p = 0;
    int have_good = 0;
    int have_first_eligible = 0;
    int first_eligible = -1;
    for(int i = 0; i < n; i++) {
        int j = i+1;
        for(; j < n && c[p[i]] == c[p[j]]; j++)
            ;
        if(q[p[i]] > 0) {
            if(!have_first_eligible) {
                have_first_eligible = 1;
                first_eligible = p[i];
            }
            const int *a = NULL;
            while((a = graph_aut_gen(g)) != NULL) {
                for(int u = 0; u < j - i; u++)
                    q[p[u + i]] = 2;
                int u = 0;
                int num_fixed = 0;
                int num_moved = 0;
                while(u < l) {
                    int z = m[u];
                    int first = 1;
                    int len = 0;
                    if(q[z] == 2) {
                        int w = z;
                        do {                        
                            q[w] = 1;
                            first = 0;
                            w = a[w];
                            len++;
                        } while(z != w);
                        if(len == 1)
                            num_fixed += len;
                        if(len >= 2)
                            num_moved += len ;                     
                    }
                    u++;
                }
                if(!have_good && j-i >= max_length) {
                    max_length = j-i;
                    max_p = p[i];
                }
                if(num_fixed > 0 && num_moved > 0) {
                    if(!have_good || j-i > max_length) {
                        max_length = j-i;
                        max_p = p[i];
                        have_good = 1;
                    }
                }
            }
        }
        i = j-1;
    }
    
    FREE(q);
    if(!have_first_eligible)
        ABORT("no eligible orbit");
    if(max_length >= 2)
        return max_p;
    else
        return first_eligible;
}


graph_t *reducer_expand_prefix(reducer_t *r, int k, int p, graph_t *prev)
{
    if(!r->initialized)
        ABORT("cannot expand an uninitialized prefix");
    if(k + 1 >= r->prefix_capacity)
        reducer_enlarge_prefix(r, 2*r->prefix_capacity+1);

    push_time();
    r->prefix[k] = p;
    if(k > r->k)
        ABORT("unsupported expansion");
    if(r->k == k) {
        r->stat_gen[k] = 0;
        r->stat_can[k] = 0;
        r->stat_out[k] = 0;
        r->k = k+1;
    }

    graph_t *g = prev;
    if(g == NULL) {
        g = graph_dup(r->base);
        for(int j = 0; j < k; j++)
            graph_add_edge(g, r->prefix[j], r->val[0]);
    }
    fprintf(stderr, "graph [%d]:", k);
    print_aut_order(stderr, g);
    fprintf(stderr, "\n");

    if(k == 0) {
        /* Check the base graph against the variable and value lists. */

        const int *p = graph_orbit_cells(g);
        const int *c = graph_orbits(g);
        int *q = (int *) MALLOC(sizeof(int)*r->n);
        for(int j = 0; j < r->n; j++)
            q[j] = 0;
        for(int j = 0; j < r->v; j++)
            q[r->var[j]] = 1;
        for(int s = 0; s < r->n; s++) {
            int u = s+1;
            for(; c[p[s]] == c[p[u]] && u < r->n; u++)
                ;
            for(int j = s+1; j < u; j++)
                if(q[p[j]] != q[p[s]])
                    ERROR("variable list is not a union of "
                          "orbits of base graph "
                          "(%d and %d have different orbits)",
                          p[j] + 1, p[s] + 1);
            s = u-1;
        }
        FREE(q);
        for(int j = 0; j < r->n; j++)
            for(int s = 0; s < r->r; s++)
                if(p[j] == r->val[s] &&
                   ((j > 0 && c[p[j-1]] == c[p[j]])||
                    (j < r->n - 1 && c[p[j]] == c[p[j+1]])))
                    ERROR("value vertex (%d) is not fixed by the automorphism "
                          "group of the base graph", r->val[s] + 1);
    }

    fprintf(stderr, "   orbits = [");
    graph_print_orbits(stderr, g, r->v, r->var);
    fprintf(stderr, "]\n");

    if(r->verbose) {
        print_orbit_perms(stderr, g, r->v, r->var);
        fprintf(stderr, "select = %d\n", orbit_select(g, r->v, r->var,
                                                      k, r->prefix,
                                                      k > 0 ?
                                                      r->trav_ind[k-1] : 
                                                      NULL));
    }

    fprintf(stderr, "prefix[%d] = %d:", k + 1, r->prefix[k] + 1);
    r->orbits[k] = (int *) MALLOC(sizeof(int)*r->n);
    r->trav_ind[k] = (int *) MALLOC(sizeof(int)*r->n);
    r->seed_min[k] = (int *) MALLOC(sizeof(int)*r->n);

    push_time();
    r->trav_sizes[k] = traversal_prepare(r->traversals + k,
                                         r->prefix[k],
                                         g);
    pop_print_time("traversal");
    graph_free(g);

    int *a = (int *) MALLOC(sizeof(int)*r->trav_sizes[k]);
    for(int j = 0; j < r->trav_sizes[k]; j++)
        a[j] = r->traversals[k][j][r->prefix[k]];
    for(int i = 0; i < r->n; i++)
        r->trav_ind[k][i] = 0;
    for(int j = 0; j < r->trav_sizes[k]; j++)
        r->trav_ind[k][a[j]] = 1;
    fprintf(stderr, "\n   traversal: ");
    print_int_array(stderr, r->trav_sizes[k], a);
    fprintf(stderr, " [length = %d]\n", r->trav_sizes[k]);
    FREE(a);

    g = graph_dup(r->base);
    for(int j = 0; j <= k; j++)
        graph_add_edge(g, r->prefix[j], r->val[0]);
    for(int j = 0; j < r->n; j++)
        r->orbits[k][j] = graph_same_orbit(g, r->prefix[k], j);

    pop_print_time("prefix_total");
    fprintf(stderr, "\n");

    return g;
}

void reducer_initialize(reducer_t *r)
{
    if(r->initialized)
        return;

    int k = r->k;
    if(k > r->prefix_capacity)
        ABORT("prefix overrun at init");

    push_time();

    r->orbits = (int **) MALLOC(sizeof(int *)*r->prefix_capacity);
    r->trav_sizes = (int *) MALLOC(sizeof(int)*r->prefix_capacity);
    r->trav_ind = (int **) MALLOC(sizeof(int *)*r->prefix_capacity);
    r->traversals = (int ***) MALLOC(sizeof(int **)*r->prefix_capacity);

    r->work = (int *) MALLOC(sizeof(int)*(2*r->prefix_capacity+1)
                                        *(r->prefix_capacity+1));
    r->seed_min = (int **) MALLOC(sizeof(int *)*r->prefix_capacity);
    r->scratch = (int *) MALLOC(sizeof(int)*(2*r->prefix_capacity+2));
    r->stack_top = 0; /* The stack is empty. */


    r->stat_gen = (long *) MALLOC(sizeof(long)*r->prefix_capacity);
    r->stat_can = (long *) MALLOC(sizeof(long)*r->prefix_capacity);
    r->stat_out = (long *) MALLOC(sizeof(long)*r->prefix_capacity);

    r->initialized = 1;

    graph_t *g = NULL;
    int i = 0;
    for(; i < k; i++)
        g = reducer_expand_prefix(r, i, r->prefix[i], g);

    if(g != NULL) {
        fprintf(stderr, "graph [%d]:", i);
        print_aut_order(stderr, g);
        fprintf(stderr, "\n");
        fprintf(stderr, "   orbits = [");
        graph_print_orbits(stderr, g, r->v, r->var);
        fprintf(stderr, "]\n");

        if(r->verbose) {
            print_orbit_perms(stderr, g, r->v, r->var);
            fprintf(stderr, "select = %d\n", orbit_select(g, r->v, r->var,
                                                          k, r->prefix,
                                                          k > 0 ?
                                                          r->trav_ind[k-1] : 
                                                          NULL));
        }
        r->last_prefix_g = g;
    }

    fprintf(stderr, "init:");
    pop_print_time("reducer_initialize");
    fprintf(stderr, "\n");       
}


/******************************************************** Release a reducer. */

void reducer_free(reducer_t *r)
{
    if(r->initialized) {
        FREE(r->stat_out);
        FREE(r->stat_can);
        FREE(r->stat_gen);
        int k = r->k;       
        FREE(r->scratch);
        FREE(r->work);
        for(int i = 0; i < k; i++) {
            traversal_release(r->trav_sizes[i], r->traversals[i]);
            FREE(r->trav_ind[i]);
            FREE(r->orbits[i]);
            FREE(r->seed_min[i]);
        }
        FREE(r->seed_min);
        FREE(r->traversals);
        FREE(r->trav_ind);
        FREE(r->trav_sizes);
        FREE(r->orbits);
    }
    if(r->last_prefix_g != NULL)
        graph_free(r->last_prefix_g);

    if(r->have_cnf)
        FREE(r->clauses);

    FREE(r->var_trans);

    FREE(r->prefix);
    FREE(r->asgn);
    for(int i = 0; i < r->r; i++)
        FREE(r->val_legend[i]);
    FREE(r->val_legend);
    FREE(r->val);
    for(int i = 0; i < r->v; i++)
        FREE(r->var_legend[i]);
    FREE(r->var_legend);
    FREE(r->var);
    graph_free(r->base);
    FREE(r);
}

/************************************* Print the configuration of a reducer. */

void reducer_print(FILE *out, reducer_t *r)
{
    graph_print(out, r->base);
    FPRINTF(out, "p variable %d\n", r->v);
    for(int i = 0; i < r->v; i++)
        FPRINTF(out, "v %d %s\n", r->var[i] + 1, r->var_legend[i]);
    FPRINTF(out, "p value %d\n", r->r);
    for(int i = 0; i < r->r; i++)
        FPRINTF(out, "r %d %s\n", r->val[i] + 1, r->val_legend[i]);
    FPRINTF(out, "p prefix %d %d %ld\n", r->k, r->a, r->t);
    for(int i = 0; i < r->a; i++)
        FPRINTF(out, "a %d %d\n", r->prefix[i] + 1, r->asgn[i] + 1);
    for(int i = r->a; i < r->k; i++)
        FPRINTF(out, "f %d\n", r->prefix[i] + 1);
}

void reducer_print_cnf(FILE *out, 
                       const char *fmt, 
                       int header_var_adjust, 
                       int header_clause_adjust,
                       reducer_t *r)
{
    if(!r->have_cnf)
        ABORT("do not have CNF to print");

    int  nv = r->nv;
    long nc = r->nc;

    int do_header_counts = 1;
    if(header_var_adjust < 0 ||
       header_clause_adjust < 0)
        do_header_counts = 0;

    if(do_header_counts) {
        FPRINTF(out, 
                "p %s %d %ld\n", 
                fmt, 
                nv + header_var_adjust, 
                nc + header_clause_adjust);
    } else {
        FPRINTF(out, "p %s\n", fmt);
    }
    
    long cursor = 0;
    int *buf = r->clauses;
    for(long c = 0; c < nc; c++) {
        int first = 1;
        while(1) {
            int l = buf[cursor++];
            FPRINTF(out, "%s%d", first ? "" : " ", l);
            first = 0;
            if(l == 0) {
                FPRINTF(out, "\n");
                first = 1;
                break;
            }
        }
    }
}

/********************************* Get a prefix assignment from the reducer. */

static int aut_order_trunc(graph_t *g)
{
    mpz_t aut_order;
    mpz_init(aut_order);
    mpz_set_si(aut_order, 1L);
    const int *ai = graph_aut_idx(g);
    while(*ai != 0) {
        mpz_mul_si(aut_order, aut_order, (long) *ai);
        ai++;
    }
    int aut_trunc = 999999999;
    if(mpz_cmp_si(aut_order, aut_trunc) < 0)
        aut_trunc = (int) mpz_get_si(aut_order);
    mpz_clear(aut_order);
    return aut_trunc;
}

const int *reducer_get_prefix_assignment(reducer_t *r)
{
    int n = r->n;
    int k = r->k;
    int d = r->r;

    if(r->target_length == 0)
        return NULL;
        
    if(r->stack_top == 0) {
        if(k == 0) {
            /* Initialize the prefix. */            
            int p = orbit_select(r->base, r->v, r->var,
                                 k, r->prefix,
                                 k > 0 ?
                                 r->trav_ind[k-1] : 
                                 NULL);
            r->last_prefix_g = reducer_expand_prefix(r, k, p, NULL);
            k++;
        }

        /* Initialize minimum indicators for orbits of the base graph. */
        orbit_min_ind(r->base, NULL, r->seed_min[0]);

        /* Initialize the iterator work stack. */
        /* Caveat: should initialize the r->a assigned variables here. */
        /* First variable is minimum in its base-automorphism orbit. */

        int p = 0;
        for(; p < r->trav_sizes[0]; p++) {
            if(r->seed_min[0][r->traversals[0][p][r->prefix[0]]]) {
                r->work[0] = r->traversals[0][p][r->prefix[0]];
                break;
            }
        }
        if(p == r->trav_sizes[0])
            ABORT("no minimum found for base orbit");
        r->work[1] = 0;
        r->work[2] = 1;
        r->stack_top = 3;

        for(int i = 0; i < k; i++) {
            r->stat_out[i] = 0;
            r->stat_gen[i] = 0;
            r->stat_can[i] = 0;
        }
    }
    while(r->stack_top > 0) {
        /* Pop the stack top. */
        int size = r->work[r->stack_top - 1];
        int *vars = r->work + r->stack_top - 1 - 2*size;
        int *vals = r->work + r->stack_top - 1 - size;
        r->stack_top = r->stack_top - (2*size+1);

        /* Find the current variable. */
        int lvl = size - 1;
        int current = -1;
        int current_idx = -1;
        for(int j = 0; j < r->trav_sizes[lvl]; j++) {
            for(int i = 0; i < size; i++) {             
                if(vars[i] == r->traversals[lvl][j][r->prefix[lvl]]) {
                    current = j;
                    current_idx = i;
                }
            }
        }
        if(current == -1)
            ABORT("no current variable");

        if(size >= r->prefix_capacity)
            ABORT("prefix overrun");

        int current_val = vals[current_idx];
        if(current_val < d) {
            r->stat_gen[lvl]++;

            /* Save next value, relying on existing stack contents. */
            vals[current_idx]++;
            r->stack_top = r->stack_top + (2*size+1);
            
            /* Process stack top. */
            int *nu = (int *) MALLOC(sizeof(int)*n);
            for(int i = 0; i < n; i++)
                nu[r->traversals[lvl][current][i]] = i;
            if(nu[vars[current_idx]] != r->prefix[lvl])
                ABORT("bad nu");
            graph_t *g = graph_dup(r->base);
            for(int i = 0; i < size; i++) {
                if(i != current_idx) {
                    graph_add_edge(g, vars[i], r->val[vals[i]]);
                } else {
                    graph_add_edge(g, vars[i], r->val[current_val]);
                }
            }
            const int *lab = graph_can_lab(g);
            int qlab = -1;
            int t = 0;          
            for(; t < n; t++) {
                qlab = lab[t];
                if(r->orbits[lvl][nu[qlab]])
                    break;
            }
            if(t == n)
                ABORT("bad qlab");
            if(graph_same_orbit(g, qlab, vars[current_idx])) {
                /* Top was accepted by isomorph rejection. */
                r->stat_can[lvl]++;
                
                /* Normalize top to scratch. */
                r->scratch[0] = size;
                int *norm_vars = r->scratch + 1;
                int *norm_vals = r->scratch + 1 + size;
                for(int i = 0; i < size; i++) {
                    norm_vars[i] = nu[vars[i]];
                    if(i != current_idx) {
                        norm_vals[i] = vals[i];
                    } else {
                        norm_vals[i] = current_val;
                    }
                }
                int aut = aut_order_trunc(g);
                r->scratch[2*size+1] = aut;
                if(size == r->target_length || aut <= r->t) {
                    for(int i = 0; i < size; i++)
                        norm_vals[i] = r->val[norm_vals[i]];
                    FREE(nu);
                    graph_free(g);
                    /* Report to caller. */
                    r->stat_out[lvl]++;
                    return r->scratch;
                } else {
                    /* Expand. */
                    if(size + 1 > k) {
                        /* Expand prefix. */
                        graph_t *lpg = r->last_prefix_g;
                        int p = orbit_select(lpg, r->v, r->var,
                                             k, r->prefix,
                                             k > 0 ?
                                             r->trav_ind[k-1] : 
                                             NULL);
                        lpg = reducer_expand_prefix(r, k, p, lpg);
                        r->last_prefix_g = lpg;
                        k++;
                        /* Reset pointers to potentially new scratch. */
                        norm_vars = r->scratch + 1;
                        norm_vals = r->scratch + 1 + size;
                    }
                    int *exp_vars = r->work + r->stack_top;
                    int *exp_vals = r->work + r->stack_top + (size + 1);
                    r->work[r->stack_top + 2*(size+1)] = size + 1;
                    r->stack_top = r->stack_top + 2*(size+1) + 1;
                    
                    for(int i = 0; i < size; i++) {
                        exp_vars[i] = norm_vars[i];
                        exp_vals[i] = norm_vals[i];
                    }

                    /* Save minima of (normalised) automorphism orbits. */
                    orbit_min_ind(g, nu, r->seed_min[lvl+1]);

                    /* First var is minimum in its seed-automorphism orbit. */
                    int s = 0;
                    for(; s < r->trav_sizes[lvl+1]; s++) {
                        if(r->seed_min[lvl+1][r->traversals[lvl+1][s][r->prefix[lvl+1]]]) {                         
                            exp_vars[size] = 
                                r->traversals[lvl+1][s][r->prefix[lvl+1]];
                            break;
                        }
                    }
                    if(s == r->trav_sizes[lvl+1])
                        ABORT("no minimum found in extending orbit");
                    exp_vals[size] = 0;
                }
            }
            FREE(nu);
            graph_free(g);
        } else {
            /* Proceed to the next variable, if any. */
            /* Next variable must be minimum in its seed-automorphism orbit. */
            /* Again rely on existing stack contents. */          
            for(; current + 1 < r->trav_sizes[lvl]; current++) {
                if(r->seed_min[lvl][r->traversals[lvl][current+1][r->prefix[lvl]]]) {
                    vars[current_idx] = 
                        r->traversals[lvl][current+1][r->prefix[lvl]];
                    vals[current_idx] = 0;
                    r->stack_top = r->stack_top + 2*size + 1;
                    break;
                }               
            }
        }
    }
    if(r->stack_top < 0)
        ABORT("work stack out of balance");
    /* Work done. */

    return NULL;
}

/********************** Print a prefix assignment obtained from the reducer. */

void reducer_print_assignment(FILE *out, reducer_t *r, const int *a)
{
    int size = a[0];
    const int *vars = a + 1;
    const int *vals = a + 1 + size;
    for(int i = 0; i < size; i++) {
        int j, jj;
        for(j = 0; j < r->v; j++) {
            if(vars[i] == r->var[j])
                break;          
        }
        for(jj = 0; jj < r->r; jj++) {
            if(vals[i] == r->val[jj])
                break;          
        }
        if(j == r->v || jj == r->r)
            ABORT("no data for assignment");
        FPRINTF(out, "%s -> %s%s", 
                r->var_legend[j], 
                r->val_legend[jj],
                i == size-1 ? "\n" : ", ");
    }
}

/****************************************************** Program entry point. */

const char *usage_str = 
"\n"
"usage: %s [arguments]\n"
"\n"
"Arguments:\n"
"short   long                function\n"    
"   -h   --help              print this help text to stdout and exit\n"
"   -u   --usage             print this help text to stdout and exit\n"
"   -f   --file <IN>         read input from file <IN>\n"
"   -o   --output <OUT>      write output to file <OUT>\n"
"   -n   --no-cnf            do not expect CNF in input\n"
"   -g   --graph             separate symmetry graph supplied in input\n"
"   -p   --prefix <SEQ>      use the prefix <SEQ> of variable vertices\n"
"   -l   --length <K>        set target length for prefix to <K>\n"
"   -t   --threshold <N>     output partial assignment when |Aut| <= <N>\n"
"   -s   --symmetry-only     print symmetry information only\n"
"   -i   --incremental       give output in icnf format\n"
"   -v   --verbose           verbose output\n"
"\n";

int main(int argc, char **argv)
{
    argparse_t *p = arg_parse(argc, argv);
    arg_print(stderr, p);

    if(arg_have(p, "help") || arg_have(p, "usage")) {
        FPRINTF(stdout, usage_str, argv[0]);
        arg_free(p);
        common_check_balance();
        return 0;
    }

    FILE *in = stdin;
    if(arg_have(p, "file")) {
        if((in = fopen(arg_string(p, "file"), "r")) == NULL)
            ERROR("error opening \"%s\" for input", arg_string(p, "file"));
    }

    FILE *out = stdout;
    if(arg_have(p, "output")) {
        if((out = fopen(arg_string(p, "output"), "w")) == NULL)
            ERROR("error opening \"%s\" for output", arg_string(p, "output"));
    }

    enable_timing(); // enable timings

    push_time();
    push_time();
    reducer_t *r = reducer_parse(in, p);
    if(arg_have(p, "threshold"))
        r->t = arg_long(p, "threshold");
    fprintf(stderr, 
            "input: n = %d, m = %ld, v = %d, r = %d, k = %d, t = %ld",
            r->n, graph_num_edges(r->base), r->v, r->r, r->k, r->t);
    pop_print_time("reducer_parse");
    fprintf(stderr, "\n");

    reducer_initialize(r);

    disable_timing(); // time only the init phase

    if(!arg_have(p, "symmetry-only")) {
        if(!arg_have(p, "incremental")) {
            if(!r->have_cnf) {
                int count = 0;
                const int *a = NULL;
                while((a = reducer_get_prefix_assignment(r)) != NULL) {
                    count++;
                    FPRINTF(out, "%d: [%d] ", count, a[2*a[0]+1]);
                    reducer_print_assignment(out, r, a);
                }
            } else {
                /* Store conjuncts in a buffer. */
                int conjbuf_cap = 128;
                int *conjbuf = (int *) MALLOC(sizeof(int)*conjbuf_cap);
                int cursor = 0;
                int count = 0;
                const int *a = NULL;
                while((a = reducer_get_prefix_assignment(r)) != NULL) {
                    count++;
                    int len = a[0];
                    if(cursor + len + 1 >= conjbuf_cap) {
                        int new_cap = 2*conjbuf_cap + 1 + len;
                        enlarge_int_array(&conjbuf, conjbuf_cap, new_cap);
                        conjbuf_cap = new_cap;
                    }
                    fprintf(stderr, "c branch %d %d\n", count, a[2*len+1]);
                    for(int i = 0; i < len; i++)
                        conjbuf[cursor++] = (a[1+i+len] == r->val[0]) ?
                            -(1+r->var_trans[a[1+i]]) :
                            1+r->var_trans[a[1+i]];
                    conjbuf[cursor++] = 0;
                }
                /* Print CNF with adjust for conjunct-clauses. */
                reducer_print_cnf(out, 
                                  "cnf", 
                                  count, 
                                  cursor - count + 1, 
                                  r);
                /* Print the conjunct-clauses. */
                int nv_base = r->nv;
                int u = 0;
                int end = cursor;
                cursor = 0;
                while(cursor < end) {
                    if(conjbuf[cursor] == 0)
                        u++;
                    else
                        FPRINTF(out, 
                                "%d %d 0\n", 
                                conjbuf[cursor],
                                -(1 + nv_base + u));
                    cursor++;
                }
                if(u != count)
                    ABORT("bad conjunct buffer");
                /* Print the final clause of conjunct-variables. */
                for(int i = 0; i < count; i++)
                    FPRINTF(out, 
                            "%d%s",
                            1 + nv_base + i,
                            i == count - 1 ? " 0\n" : " ");
                FREE(conjbuf);
            }
        } else {
            reducer_print_cnf(out, "inccnf", -1, -1, r);
            int count = 0;
            const int *a = NULL;
            while((a = reducer_get_prefix_assignment(r)) != NULL) {
                count++;
                fprintf(stderr, "c branch %d %d\n", count, a[2*a[0]+1]);
                for(int i = 0; i < a[0]; i++)
                    FPRINTF(out, 
                            "%s%d",
                            i == 0 ? "a " : " ",
                            a[1+i+a[0]] == r->val[0] ?
                            -(1+r->var_trans[a[1+i]]) :
                            1+r->var_trans[a[1+i]]);
                FPRINTF(out, " 0\n");
            }
        }
        fprintf(stderr, 
                "c %7s %14s %14s %14s\n",
                "Size",
                "Generated",
                "Canonical",
                "Output");
        for(int l = 0; l < r->k; l++)
            fprintf(stderr, 
                    "c %7d %14ld %14ld %14ld\n", 
                    l+1, 
                    r->stat_gen[l], 
                    r->stat_can[l],
                    r->stat_out[l]);
    }
    reducer_free(r);

    if(fclose(out) != 0)
        ERROR("error closing output");
    if(fclose(in) != 0)
        ERROR("error closing input");

    enable_timing(); // enable timings
    fprintf(stderr, "host: %s", common_hostname());
    pop_print_time("total");
    fprintf(stderr, "\n");

    fprintf(stderr, "build: %s\n", COMMITID);

    arg_free(p);

    common_check_balance(); /* Check malloc balance to catch a memory leak. */
    return 0;
}
