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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "graph.h"
#include "nausparse.h"

/********************************************************** Graph data type. */

struct graph_struct 
{
    int         order;
    int         num_edges;
    long *      edgebuf;
    long *      can_edgebuf;
    long        edgebuf_size;
    int         edgebuf_is_sorted;

    int *       orb;
    int *       lab;
    int *       ptn;
    int *       orb_cells;
    int         num_gen;
    int         idx_gen;
    int **      aut_gen;
    int *       aut_idx;
    int *       stab_seq;
    int         aut_idx_size;
    int         have_can;
};

/************************************* Initialization and release functions. */

static void graph_init(graph_t *g, int order, long edgebuf_size)
{
    if(order <= 0)
        ABORT("nonpositive order");
    g->order             = order;
    g->num_edges         = 0;
    g->edgebuf_size      = edgebuf_size;
    g->edgebuf           = (long *) MALLOC(sizeof(long)*g->edgebuf_size);
    g->can_edgebuf       = (long *) MALLOC(sizeof(long)*g->edgebuf_size);
    g->edgebuf_is_sorted = 1;
    
    g->lab          = (int *) MALLOC(sizeof(int)*order);
    g->ptn          = (int *) MALLOC(sizeof(int)*order);
    g->orb          = (int *) MALLOC(sizeof(int)*order);
    g->orb_cells    = NULL;
    g->aut_idx      = (int *) MALLOC(sizeof(int)*(order+1));
    g->stab_seq     = (int *) MALLOC(sizeof(int)*(order+1));
    g->aut_idx_size = 0;
    g->num_gen      = 0;
    g->idx_gen      = 0;
    g->aut_gen      = (int **) MALLOC(sizeof(int *)*order);

    for(int i = 0; i < order; i++)
        g->aut_gen[i] = NULL;

    for(int i = 0; i < order; i++) {
        g->lab[i] = i;
        g->ptn[i] = 1;
    }
    g->ptn[order-1] = 0;
    
    g->have_can = 0;
}

static void graph_release(graph_t *g)
{
    for(int i = 0; i < g->order; i++)
        if(g->aut_gen[i] != NULL)
            FREE(g->aut_gen[i]);
    FREE(g->aut_gen);
    FREE(g->stab_seq);
    FREE(g->aut_idx);
    if(g->orb_cells != NULL)
        FREE(g->orb_cells);
    FREE(g->orb);
    FREE(g->ptn);
    FREE(g->lab);
    FREE(g->can_edgebuf);
    FREE(g->edgebuf);
}

void graph_empty(graph_t *g)
{
    g->num_edges = 0;
    g->edgebuf_is_sorted = 1;
    g->have_can = 0;
}

static graph_t *graph_alloc_internal(int order, long edgebuf_size)
{
    graph_t *g = (graph_t *) MALLOC(sizeof(graph_t));
    graph_init(g, order, edgebuf_size);
    return g;
}

graph_t *graph_alloc(int order)
{
    graph_t *g = graph_alloc_internal(order, 3*order);
    return g;
}

void graph_free(graph_t *g)
{
    graph_release(g);
    FREE(g);
}

/*************************************** Subroutines for working with edges. */

static long edge_make(int i, int j) 
{
    if(i < j)
        return (((long) i)<<32) | j;
    else 
        return (((long) j)<<32) | i;
}

static int edge_i(long e)
{
    return e >> 32; 
}

static int edge_j(long e)
{
    return e & 0xFFFFFFFF;  
}

static long edge_relabel(int *p, long e)
{
    return edge_make(p[edge_i(e)], p[edge_j(e)]);
}

static void enlarge_edgebuf(graph_t *g, long size)
{
    long *buf = g->edgebuf;
    long s = g->edgebuf_size;
    if(size <= s)
        ABORT("too small size");
    FREE(g->can_edgebuf);
    g->edgebuf_size = size;
    g->edgebuf = MALLOC(sizeof(long)*size);
    g->can_edgebuf = MALLOC(sizeof(long)*size);
    for(long i = 0; i < g->num_edges; i++)
        g->edgebuf[i] = buf[i];
    FREE(buf);
    g->have_can = 0;
}

static void sort_edgebuf(graph_t *g, int in_parse)
{
    if(!g->edgebuf_is_sorted) {
        heapsort_long(g->num_edges, g->edgebuf);
        long *buf = g->edgebuf;
        for(long l = 1; l < g->num_edges; l++) {
            if(buf[l-1] >= buf[l]) {
                if(in_parse) {
                    ERROR("repeated edge (u = %d, v = %d)",
                          edge_i(buf[l]) + 1,
                          edge_j(buf[l]) + 1);
                } else {
                    ABORT("found repeated edge or bad edge sort");
                }
            }
        }
    }
    g->edgebuf_is_sorted = 1;   
}

/********************************************************** Relabel a graph. */

static void graph_permcheck(int n, int *p)
{
    int pc[n];
    for(int i = 0; i < n; i++)
        pc[i] = 0;
    for(int i = 0; i < n; i++) {
        if(p[i] < 0 || p[i] >= n || pc[p[i]] != 0)
            ABORT("invalid permutation");
        pc[p[i]] = 1;
    }
}

graph_t *graph_relabel(graph_t *g, int *p)
{
    graph_permcheck(g->order, p);
    graph_t *r = graph_alloc_internal(g->order, g->edgebuf_size);
    for(int i = 0; i < g->order; i++) {
        r->lab[i] = p[g->lab[i]];
        r->ptn[i] = g->ptn[i];
    }   
    r->num_edges = g->num_edges;
    long *t = r->edgebuf;
    long *s = g->edgebuf;
    for(long i = 0; i < g->num_edges; i++)
        t[i] = edge_relabel(p, s[i]);
    r->edgebuf_is_sorted = 0;
    return r;
}

graph_t *graph_relabel_inv(graph_t *g, int *p)
{
    graph_permcheck(g->order, p);
    int pinv[g->order];
    for(int i = 0; i < g->order; i++)
        pinv[p[i]] = i;
    return graph_relabel(g, pinv);
}

graph_t *graph_dup(graph_t *g)
{
    int pid[g->order];
    for(int i = 0; i < g->order; i++)
        pid[i] = i;
    return graph_relabel(g, pid);
}

/************************************************************** Add an edge. */

void graph_add_edge(graph_t *g, int i, int j)
{
    if(i < 0 || j < 0 || i >= g->order || j >= g->order || i == j)
        ABORT("bad edge (i = %d, j = %d)", i, j); 
    g->have_can = 0;
    g->edgebuf_is_sorted = 0;
    if(g->num_edges == g->edgebuf_size) 
        enlarge_edgebuf(g, 2 * g->edgebuf_size);
    g->edgebuf[g->num_edges++] = edge_make(i, j);
}

/********************************************* Returns the order of a graph. */

int graph_order(graph_t *g)
{
    return g->order;
}

/********************************************* Returns the order of a graph. */

long graph_num_edges(graph_t *g)
{
    return g->num_edges;
}

/****************************************************** Compares two graphs. */

static int graph_compare(graph_t *a, graph_t *b)
{
    int r = a->order - b->order;
    if(r != 0)
        return r;
    long rr = a->num_edges - b->num_edges;
    if(rr < 0)
        return -1;
    if(rr > 0)
        return 1;
    sort_edgebuf(a, 0);
    sort_edgebuf(b, 0);
    long *a_buf = a->edgebuf;
    long *b_buf = b->edgebuf;
    long l = a->num_edges;
    for(long i = 0; i < l; i++) {
        if(a_buf[i] < b_buf[i])
            return -1;
        if(a_buf[i] > b_buf[i])
            return 1;
    }
    int n = a->order;
    for(int i = 0; i < n-1; i++) {
        r = a->ptn[i] - b->ptn[i];
        if(r != 0)
            return r;
    }
    for(int i = 0; i < n; ) {
        int j = i;
        for(j = i; a->ptn[j] != 0 && j != n-1; j++)
            ;
        j++;
        heapsort_int(j - i, a->lab + i);
        heapsort_int(j - i, b->lab + i);
        for(; i < j; i++) {
            r = a->lab[i] - b->lab[i];
            if(r != 0)
                return r;
        }
    }
    return 0;
}

/*************************************** Computes the hash value of a graph. */

/*
 * Random bits generated by
 * http://www.fourmilab.ch/hotbits/
 */

setword graph_rnd_word[256] = {
0x83CF8896EA4E3EC3, 0xBBB17A9BA2A00D09, 0xF8465F601D83D9EF, 0x7DC2A75D944E58E7,
0x5D77A74C94ECCD38, 0x4D1D31AC0D59DA85, 0x8E2C06089CA0C029, 0x4FBD2851FB542AD4,
0xA14746663F0255AA, 0xABCDA389197AC320, 0x23B6398E0398CB81, 0xE2EF883656A22607,
0x77EC4A9593C08F81, 0x413D4357BF975B80, 0x739206246C8D84F0, 0x3F5C7A721E62AE20,
0x4D6A57B949FF0B44, 0x7293CE3D65CA6FFC, 0x66CB8EFF45276FB0, 0x4891D5F9CCEF1640,
0xA34B6E259628D97D, 0xC5BA75C31A3434E9, 0x9F65D872503ABF56, 0x291DEDB230C7E20B,
0xDE6D8B834C0D6409, 0x89F11EA94260E138, 0xF4F0858A2E62FB11, 0x6AA4C3179F8708A9,
0x7E42E5738895E348, 0xFE7C24AEA09A2007, 0xF89177E56584CADA, 0xBB2480ED0C8274DE,
0x39A8C4164B56B264, 0x7CC23E62657FC75F, 0xBBBC788C85D13342, 0x6B6212E7BF389275,
0x5F172B083CA5778C, 0x161D4E195CD18764, 0x9FF9752629F382D1, 0x308B2F62A3F0D348,
0xEAB4F1FA36DF4280, 0xFF1FBF269823BCDE, 0x743B812722117C92, 0xAB408DE10E57B0EC,
0xEF21CFBB5260F659, 0x294BAE102BD35B1D, 0xC671A420A1008343, 0x13D057FCE889875F,
0x792D6015772002FF, 0x30065DA1BA990D2F, 0xF1E3C5E6468E81F0, 0xAE1EDA7CF7838704,
0xE76E4E281C6DB047, 0xF6CA235E900111AA, 0xD8E56D637ECF6778, 0xA9A724A2FA54D5D6,
0xD2493A3B686AF8C2, 0xD9E661FF41AD6FA0, 0xCD4ACEE301163DB6, 0x6D595C1BC7D8134C,
0x8297349B280D9C6B, 0x12BC02F464990648, 0xE94EB44A5EF9546F, 0x3E242CCEE88AC748,
0xBAD0B59C632CDBBA, 0x1034A9EB9A68CF13, 0x029DF71F1DCC162A, 0x0AD553CC235E6D4D,
0x60A33EFFF870946F, 0x0AFDD96C266E76EC, 0x4C7163A091DB4C25, 0x0DD1DA887FF33A55,
0xBAC0272D6176EE97, 0x2C1711D5B785793D, 0x6F4F5FC04B808221, 0xDCC122206AEB230D,
0xA1CE30FEE4124CC9, 0xCEA3E0857E2024F8, 0xDF9D4B403BA1FC33, 0x329DD044BAC9A7F0,
0xAE03C251EE3C774B, 0x79912137A1A8EA42, 0x108EA56D44B0837A, 0x0058F9E5E04C653A,
0x715981667A5C6271, 0x5C3953FA79A49AC6, 0x0AFC95ABF6FAE6E5, 0x332C54DB2021CA30,
0x6088908D66C379C7, 0xCFADD91A36E2307F, 0x39727D0867DB3EBD, 0xBD3DA34AB35A2BFB,
0x1035AE3BDA2A5236, 0x29E375A3846AF982, 0x975AD44A26999944, 0x95ED7843CF32D117,
0xFF5EAA81000A2CC0, 0x857436BC72874BE2, 0x50BBCFDA2B5EDFA8, 0xCC33D3C4740BB47C,
0xB50F9DEAA2B2B088, 0x923F47741A4C2541, 0xFC3D8F8A12B11645, 0x6EDA3C8C433A6FBD,
0x12C4CB72885845A0, 0xB1BD80E99B494DA6, 0x5CCE5D02A9F2BFA5, 0xA789E7634134FB2F,
0xD5CD642F77B4C9C8, 0x50A33FA4F881432A, 0x386FC0853787AA49, 0x109F729F74327DBA,
0xB48A030209CB215C, 0xB7329DE83021ADB4, 0x44C52EC2462D9429, 0x2A38FAD7E26A5A6E,
0x6097F6A5C2C69B0A, 0xDB9AB05BCBBDE70F, 0x66ED03C363C423B7, 0x13434BC2CD2798CB,
0x370AF8CC77B0DC67, 0xB4D275B994700E38, 0xA495C1D54F658C6B, 0x6032ABF4DE746840,
0x2BEEF970D62ACEC8, 0xE85314293422CDA5, 0x6D6216AD0D28E33F, 0xDF7C10E72691614B,
0xD4B1FFEBC06B9B79, 0x30B0A09BCFAA0207, 0x6DA5E405274020B2, 0x61BF36DB94451EC1,
0xA185D35C1F75F879, 0x770978A885449637, 0x519F4F91EBD0337C, 0x862FA1D325C5FABB,
0xD4EF853599B2C266, 0xD0EA903495C03923, 0xA6A8BCED48F7C408, 0x6E2C262C32EDE0A8,
0xF04AC6580D990166, 0x2BBE87E81A138752, 0xB9F2DF291BB614C3, 0xF64EF4E8F8D0B10C,
0x175E361BA1A7FEFB, 0x5770F303F2D462B7, 0x7C02EE7A16EBF545, 0xA2C156942EFBE51E,
0x99FBCBC95BBD3439, 0x0A5B96EE76456B7C, 0x5FA7E69FFC0E3300, 0x8CFC34762269EE55,
0xB863974CD7866784, 0xD32B80D1564F2512, 0x0B497B2ECEC923B7, 0x3A9FF92895A7F31C,
0xCA81105A7BE78E10, 0xD505070480F73E51, 0x950195B000A4A188, 0xB6BAD44283B1A4F1,
0x48CE940FABB630D1, 0x69087DC37776D457, 0xEC957AEE05F99996, 0x8780B9AF7DC2DC5F,
0xBF5FFDAFAA2AE303, 0x7230CFE62D708CAA, 0xCBA7A501826DB72C, 0xF61625FACCB53679,
0x2DA092C391D3C3AE, 0xD34B1FEEDFF61864, 0x80E6884D5496C07B, 0x0875D1545ECDF1E2,
0xFE63C5972EF12D12, 0xB7D703A4BD9F5F4C, 0x94859BB2B6DA644D, 0xC14C7A6EFAD095FA,
0x37B8F10909250927, 0xB60D9483BCEE4B04, 0x35EF0CF63DCB695F, 0xBF5426F10C9AA5AF,
0xD2ED87FE63F5D64E, 0xE8E4918E3235B0CA, 0xC7B435AD77F61140, 0x176F0EF7BD4B2224,
0xEC850F4DE93CF4D3, 0xCF7FD438EF1B1D9F, 0x71D30D1192F29946, 0x96F52616713889BF,
0x786135C9A11268C8, 0x980D7E4521FEAE45, 0x6694A67A27B1103B, 0xF8363E6607B4BDAA,
0x214582A1128B35BA, 0xEA8E9CD0B622AD97, 0xAA17DC68FC946769, 0x7AF0F16C14952078,
0xB7F87881A35D69A2, 0x159C3A6C0BC4FCD1, 0x3E5C16D5399CD8B5, 0x25A975AEA7888D19,
0x0FE8525E33CCA9D2, 0xCD483AF63165B7DA, 0x11E37661A64A73C6, 0x7BB72E48AD5B8567,
0xB8320C8BAF5D6949, 0x5EB6896E1DB41DF2, 0xA39E95382A5A26CF, 0x460FFC688C9EE42B,
0x1877D32E0058ED54, 0x4AD2896A21BF0B83, 0x0374561815E56D9F, 0x3B192CFC4A64087B,
0xA170AC8A680C19A6, 0x1AD917B932560E3E, 0x83983FE605812AC5, 0xF4B9088ADFBA9FF1,
0x506DEB6B87BF6DB5, 0x110750A7AA2CA2EE, 0xE17415AF0744C8F7, 0x4C3939BC5280B92B,
0x8B4C7F3236564155, 0x709EBF805FB2EDC1, 0xC97085B7F40A29C5, 0xC95F471500F0B7DF,
0x9FEC3624CC8AFA09, 0xC27CF7A225306A4F, 0x22A9FEB0F2D3793E, 0x7BA6AC054EE8D3AF,
0x72A068CC15BD388C, 0x92756F48B02B556D, 0x35A7FEA606124BF7, 0x558FE7389FEC8278,
0x3F6FE896CEB064DA, 0x10244197BA2F90D6, 0xCA6B5A588118A230, 0x4BD532988DC1EFFE,
0x1429E904119B31B7, 0xCAF321F93587E1B7, 0x3BECB0B96FA7190E, 0x16735080E9E0D453,
0x9E1EC749490A50CA, 0xCDF74490CBA8D470, 0xEE15C663EA448F01, 0x4909FAB3A4591101,
0x7AB026547242CCAA, 0xE10A1591A4100E95, 0xB4401834E771F605, 0xA23715D46963156E,
0x8D6BF796DD83D07C, 0x3B2F61312E60F9A4, 0xAA4788B0AB1830D2, 0x75FE895C7C3A6F51,
0x6ED960ED0C0D8787, 0xB264012E0E167DCC, 0xDC93ED8636C5BD24, 0x22697BE3E473DCFD
};
#define ROT(z) (((z)<<1)|(((z)&0x8000000000000000) ? 1 : 0))

static setword graph_hash(graph_t *g)
{
    /* Somewhat of a dirty-and-all-too-periodic hack. */
    sort_edgebuf(g, 0);    
    unsigned char *b = (unsigned char *) g->edgebuf;
    setword h = graph_rnd_word[b[0]&0xFF];
    int l = g->num_edges*sizeof(long);
    for(long i = 1; i < l; i++)
        h = graph_rnd_word[b[i]&0xFF]^ROT(h);
    return h;
}

/********************************************** Returns the labeling vector. */

int *graph_lab(graph_t *g) 
{
    return g->lab;
}

/********************************************* Returns the partition vector. */

int *graph_ptn(graph_t *g) 
{
    return g->ptn;
}

/*********************** Computes canonical labeling for a graph (internal). */

graph_t *autom_g;

static void lvlproc(int *lab, int *ptn, int lvl, int *orb, statsblk *stats,
                    int tv, int idx, int tcellsize, int numcells,
                    int childcount, int n)
{
    autom_g->stab_seq[autom_g->aut_idx_size] = tv;
    autom_g->aut_idx[autom_g->aut_idx_size++] = idx;
}

static void automproc(int numgen, int *p, int *orb, 
                      int numorb, int stabv, int n)
{
    graph_t *g = autom_g;
    if(g->aut_gen[g->num_gen] == NULL)
        g->aut_gen[g->num_gen] = MALLOC(sizeof(int)*g->order);
    int *q = g->aut_gen[g->num_gen];
    for(int i = 0; i < g->order; i++)
        q[i] = p[i];
    g->num_gen++;
}

static void graph_getcan(graph_t *g)
{
    if(g->have_can)
        return;

    push_time();

    int  n = g->order;
    long m = g->num_edges;

    g->num_gen      = 0;
    g->idx_gen      = 0;
    g->aut_idx_size = 0;

    sparsegraph ng, ncg;
    SG_INIT(ng);
    SG_INIT(ncg);
    static DEFAULTOPTIONS_SPARSEGRAPH(options);
    statsblk stats;

    int mm = SETWORDSNEEDED(n);
    nauty_check(WORDSIZE, mm, n, NAUTYVERSIONID);

    size_t *v  = (size_t *) MALLOC(sizeof(size_t)*n); 
    int *d     = (int *) MALLOC(sizeof(int)*n);
    int *e     = (int *) MALLOC(sizeof(int)*m*2);
    size_t *cv = (size_t *) MALLOC(sizeof(size_t)*n); 
    int *cd    = (int *) MALLOC(sizeof(int)*n);
    int *ce    = (int *) MALLOC(sizeof(int)*m*2);

    ng.nv   = n;
    ng.nde  = m*2;
    ng.vlen = n;
    ng.dlen = n;
    ng.elen = m*2;
    ng.wlen = 0;
    ng.v    = v;
    ng.d    = d;
    ng.e    = e;
    ng.w    = NULL;

    ncg.nv   = n;
    ncg.nde  = m*2;
    ncg.vlen = n;
    ncg.dlen = n;
    ncg.elen = m*2;
    ncg.wlen = 0;
    ncg.v    = cv;
    ncg.d    = cd;
    ncg.e    = ce;
    ncg.w    = NULL;

    for(int i = 0; i < n; i++)
        d[i] = 0;
    long *buf = g->edgebuf;
    for(long l = 0; l < m; l++) {
        long b = buf[l];
        int i = edge_i(b);
        int j = edge_j(b);
        d[i]++;
        d[j]++;
    }
    v[0] = d[0];
    for(int i = 1; i < n; i++)
        v[i] = v[i-1] + d[i];
    if(v[n-1] != 2*m)
        ABORT("bad v array");
    for(long l = 0; l < m; l++) {
        long b = buf[l];
        int i = edge_i(b);
        int j = edge_j(b);
        e[--v[j]] = i;
        e[--v[i]] = j;
    }
    
    options.defaultptn    = 0;
    options.getcanon      = 1;
    options.userautomproc = &automproc;
    options.userlevelproc = &lvlproc;

    autom_g = g;
    g->num_gen = 0;

    sparsenauty(&ng, g->lab, g->ptn, g->orb, &options, &stats, &ncg);

    long l = 0;
    long *cbuf = g->can_edgebuf;

    for(int i = 0; i < n; i++) {
        heapsort_int(cd[i], ce + cv[i]);
        int *a = ce + cv[i];
        for(int k = 0; k < cd[i]; k++) {
            int j = a[k];
            if(i < j)
                cbuf[l++] = edge_make(i, j);
        }
    }
    if(l != m)
        ABORT("bad canonical form (l = %ld, m = %ld)", l, m);
    for(l = 1; l < m; l++)
        if(cbuf[l-1] >= cbuf[l])
            ABORT("bad sort in canonical form");

    FREE(ce);
    FREE(cd);
    FREE(cv);
    FREE(e);
    FREE(d);
    FREE(v);

    g->aut_idx[g->aut_idx_size] = 0;
    g->stab_seq[g->aut_idx_size] = -1;

    pop_print_time("nauty");

    g->have_can = 1;
}

/********************************** Computes canonical labeling for a graph. */

const int *graph_can_lab(graph_t *g)
{
    graph_getcan(g);
    return g->lab;
}

/*************************************** Computes a graph in canonical form. */

graph_t *graph_can_form(graph_t *g)
{
    graph_getcan(g);
    graph_t *cg = graph_alloc_internal(g->order, g->num_edges);

    long *s = g->can_edgebuf;
    long *t = cg->edgebuf;
    for(int l = 0; l < g->num_edges; l++)
        t[l] = s[l];
    cg->num_edges = g->num_edges;
    cg->edgebuf_is_sorted = 1;
    for(int i = 0; i < g->order; i++)
        cg->ptn[i] = g->ptn[i];
    return cg;
}

/****************************** Iterates over automorphism group generators. */

const int *graph_aut_gen(graph_t *g)
{
    graph_getcan(g);
    if(g->idx_gen >= g->num_gen) {
        g->idx_gen = 0;
        return NULL;
    } else {
        int *q = g->aut_gen[g->idx_gen];
        g->idx_gen++;
        return q;
    }
}

/********************* Returns an index sequence for the automorphism group. */

const int *graph_aut_idx(graph_t *g)
{
    graph_getcan(g);
    return g->aut_idx;
}

/***************** Returns a stabilizer sequence for the automorphism group. */

const int *graph_stab_seq(graph_t *g)
{
    graph_getcan(g);
    return g->stab_seq;
}

/********************************************** Returns automorphism orbits. */

const int *graph_orbits(graph_t *g)
{
    graph_getcan(g);
    return g->orb;
}

/************ Tests whether two vertices are in the same automorphism orbit. */

int graph_same_orbit(graph_t *g, int i, int j)
{
    if(i < 0 || j < 0 || i >= g->order || j >= g->order)
        ABORT("vertex number out of bounds (i = %d, j = %d)", i, j);
    if(i == j)
        return 1;
    graph_getcan(g);

    if(g->orb[i] == g->orb[j])
        return 1;
    else
        return 0;
}

/**************************************************** Build the orbit cells. */

const int *graph_orbit_cells(graph_t *g)
{
    graph_getcan(g);
    int n = g->order;
    int *p = g->orb_cells;
    if(p == NULL)
        p = (int *) MALLOC(sizeof(int)*n);
    g->orb_cells = p;
    for(int i = 0; i < n; i++)
        p[i] = i;
    heapsort_int_indirect(n, g->orb, p);
    int last = 0;
    while(last < n) {
        int i = last + 1;
        for(; i < n; i++)
            if(g->orb[p[i-1]] != g->orb[p[i]])
                break;
        heapsort_int(i - last, p + last);
        last = i;
    }
    return p;
}

/**************************************************** A simple graph parser. */

graph_t *graph_parse(FILE *in)
{
    int n, m;
    if(fscanf(in, "p edge %d %d\n", &n, &m) != 2)
        ERROR("parse error -- graph format line expected");
    if(n <= 1 || m < 0)
        ERROR("bad graph parameters n = %d, m = %d", n, m);
    graph_t *g = graph_alloc(n);
    int *colors = (int *) MALLOC(sizeof(int)*n);
    for(int i = 0; i < n; i++)
        colors[i] = -1;
    for(int i = 0; i < m; i++) {
        int u, v;
        if(fscanf(in, "e %d %d\n", &u, &v) != 2)
            ERROR("parse error -- edge line expected");
        if(u < 1 || v < 1 || u == v || u > n || v > n)
            ERROR("bad edge u = %d, v = %d", u, v);
        graph_add_edge(g, u-1, v-1);
    }
    for(int i = 0; i < n; i++) {
        int u, c;
        if(fscanf(in, "c %d %d\n", &u, &c) != 2)
            ERROR("parse error -- color line expected");
        if(u < 1 || c < 0 || u > n)
            ERROR("bad color u = %d, c = %d", u, c);
        colors[u-1] = c;
    }
    for(int i = 0; i < n; i++)
        if(colors[i] == -1)
            ERROR("vertex u = %d did not receive a color", i+1);
    heapsort_int_indirect(n, colors, g->lab);
    for(int i = 0; i < n; i++)
        if(i == n-1 || colors[g->lab[i]] != colors[g->lab[i+1]])
            g->ptn[i] = 0;
        else
            g->ptn[i] = 1;    
    FREE(colors);
    return g;
}

/*********************************************************** Graph printing. */

void graph_print(FILE *out, graph_t *g)
{
    int n = g->order;
    int m = g->num_edges;
    sort_edgebuf(g, 0);
    fprintf(out, "p edge %d %d\n", n, m);
    for(long l = 0; l < m; l++) {
        long e = g->edgebuf[l];
        int i = edge_i(e);
        int j = edge_j(e);
        fprintf(out, "e %d %d\n", i + 1, j + 1);
    }
    int c = 0;
    for(int i = 0; i < n; i++) {
        fprintf(stdout, "c %d %d\n", g->lab[i] + 1, c);
        if(g->ptn[i] == 0)
            c++;
    }
}

void graph_print_orbits(FILE *out, graph_t *g, int l, int *m)
{
    graph_getcan(g);
    int n = g->order;
    const int *p = graph_orbit_cells(g);
    const int *c = graph_orbits(g);
    int *q = (int *) MALLOC(sizeof(int)*n);
    for(int i = 0; i < n; i++)
        q[i] = 0;
    for(int i = 0; i < l; i++) {
        if(m[i] < 0 || m[i] >= n)
            ABORT("bad m array");
        q[m[i]] = 1;
    }
    int have_previous = 0;
    for(int s = 0; s < n; s++) {
        int u = s+1;
        for(; u < n && c[p[s]] == c[p[u]]; u++)
            ;
        for(int j = s+1; j < u; j++)
            if(q[p[s]] != q[p[j]])
                ABORT("bad m array -- not a union of orbits");
        if(q[p[s]]) {
            fprintf(out, "%s", have_previous ? " | " : "");
            print_int_array(out, u-s, p + s);
            have_previous = 1;
        }
        s = u-1;
    }
    FREE(q);
}

/*****************************************************************************/

/************************************************** Bag of graphs data type. */

/* Open-addressing hash table with linear probing. */

struct graphbag_struct
{
    setword      num_graphs;
    setword      table_size;
    graph_t **   table;
    setword      max_search;
};

/***************************************************** Internal subroutines. */

static setword get_idx(graphbag_t *b, graph_t *g)
{
    setword h   = graph_hash(g);
    setword s   = b->table_size;
    graph_t **t = b->table;
    setword l   = 0;
    setword i;
    for(i = h % s; t[i] != NULL; i = ((i+1 == s) ? 0 : i+1)) {
        l++;
        if(l > b->max_search)
            b->max_search = l;
        if(graph_compare(g, t[i]) == 0)
            return i;
    }
    return i;
}

static void enlarge(graphbag_t *b, setword size)
{
    graph_t **t = b->table;
    setword s = b->table_size;
    if(size <= s)
        ABORT("too small size");
    b->table_size = size;
    b->table = MALLOC(sizeof(graph_t *)*size);
    for(setword i = 0; i < b->table_size; i++)
        b->table[i] = NULL;
    for(setword i = 0; i < s; i++) {
        if(t[i] != NULL) {
            setword j = get_idx(b, t[i]);
            if(b->table[j] != NULL)
                ABORT("rehash error");
            b->table[j] = t[i];
        }
    }
    FREE(t);
}

/************************************************ Allocates a bag of graphs. */

#define BAG_START_SIZE 128

graphbag_t *graphbag_alloc(void)
{
    graphbag_t *b = (graphbag_t *) MALLOC(sizeof(graphbag_t));
    b->num_graphs = 0;
    b->table_size = BAG_START_SIZE;
    b->max_search = 0;
    b->table      = (graph_t **) MALLOC(sizeof(graph_t *)*b->table_size);
    for(setword i = 0; i < b->table_size; i++)
        b->table[i] = NULL;
    return b;
}

/**************************************************** Frees a bag of graphs. */

void graphbag_free(graphbag_t *b)
{
    for(setword i = 0; i < b->table_size; i++)
        if(b->table[i] != NULL)
            graph_free(b->table[i]);
    FREE(b->table);
    FREE(b);
}

/************************************************** Empties a bag of graphs. */

void graphbag_empty(graphbag_t *b)
{
    for(setword i = 0; i < b->table_size; i++) {
        if(b->table[i] != NULL) {           
            graph_free(b->table[i]);
            b->table[i] = NULL;
        }
    }
    b->num_graphs = 0;
}

/********************************************* Inserts a graph into the bag. */

int graphbag_insert(graphbag_t *b, graph_t *g)
{
    if(8*b->num_graphs >= b->table_size)
        enlarge(b, 2*b->table_size);
    setword i = get_idx(b, g);
    if(b->table[i] == NULL) {
        b->table[i] = g;
        b->num_graphs++;
        return 0;
    } else {
        return 1;
    }
}

/******************************** Queries whether a graph occurs in the bag. */

int graphbag_query(graphbag_t *b, graph_t *g)
{
    if(8*b->num_graphs >= b->table_size)
        enlarge(b, 2*b->table_size);
    setword i = get_idx(b, g);
    if(b->table[i] == NULL) {
        return 0;
    } else {
        return 1;
    }
}
