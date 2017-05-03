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

/******************************************************* Common subroutines. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>

#include "common.h"

/********************************************************** Error reporting. */

void common_error(const char *fn, int line, const char *func, 
                  const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, 
            "error detected [file = %s, line = %d]\n"
            "%s: ",
            fn,
            line,
            func);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);    
}

void common_abort(const char *fn, int line, const char *func, 
                  const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, 
            "INTERNAL ERROR [file = %s, line = %d]\n"
            "%s: ",
            fn,
            line,
            func);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();    
}

/************************************************* fprintf with error guard. */

void common_fprintf(const char *fn, int line, const char *func, 
                    FILE *out,
                    const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if(vfprintf(out, format, args) < 0)
        common_error(fn, line, func, "file error writing output");
    va_end(args);
}

/******************************************************** Get the host name. */

#define MAX_HOSTNAME 256

const char *common_hostname(void)
{
    static char hn[MAX_HOSTNAME];

    struct utsname undata;
    uname(&undata);
    strcpy(hn, undata.nodename);
    return hn;
}

/******************************************************** Memory allocation. */

int common_malloc_balance = 0;

void *common_malloc_wrapper(size_t size)
{
    void *p = malloc(size);
    if(p == NULL)
        ABORT("malloc fails");
    common_malloc_balance++;
    return p;
}

void common_free_wrapper(void *p)
{
    free(p);
    common_malloc_balance--;
}

/****************************************************************** Timings. */

#define TIME_STACK_CAPACITY 256

clock_t time_stack[TIME_STACK_CAPACITY];
int     time_stack_top = -1;
int     do_time        = 0;

void enable_timing(void)
{
    do_time = 1;
}

void disable_timing(void)
{
    do_time = 0;
}

void push_time(void) 
{
    if(do_time) {
        if(time_stack_top + 1 > TIME_STACK_CAPACITY)
            ABORT("timing stack out of capacity");
        time_stack[++time_stack_top] = clock();
    }
}

double pop_time(void)
{
    if(do_time) {
        clock_t stop = clock();
        if(time_stack_top < 0)
            ABORT("pop on an empty timing stack");
        clock_t start = time_stack[time_stack_top--];
        return (double) (1000.0*((double) (stop-start))/CLOCKS_PER_SEC);    
    } else {
        return -1.0;
    }
}

void pop_print_time(const char *legend)
{
    if(do_time) {
        fprintf(stderr, " {%s: %.2fms}", legend, pop_time());
        fflush(stderr);
    }
}

void common_check_balance(void)
{
    if(common_malloc_balance != 0)
        ABORT("nonzero malloc balance");
    if(time_stack_top != -1)
        ABORT("nonempty timing stack", time_stack_top);
}

/***************************************************************** Printing. */

void print_int_array(FILE *out, int l, const int *a)
{   
    for(int cursor = 0; cursor < l; cursor++) {
        int lookahead = cursor + 1;
        for(; 
            a[lookahead-1]+1 == a[lookahead] && lookahead < l; 
            lookahead++)
            ;
        if(lookahead - cursor > 5) {
            fprintf(out, 
                    "%s%d %d ... %d",
                    cursor == 0 ? "" : " ",
                    a[cursor] + 1, a[cursor+1] + 1, a[lookahead-1] + 1);
            cursor = lookahead - 1;
        } else {
            fprintf(out,
                    "%s%d",
                    cursor == 0 ? "" : " ",
                    a[cursor] + 1);
        }
    }
}

/****************************************************************** Sorting. */

/******************************************* Shellsort for an integer array. */

void shellsort_int(int n, int *a)
{
    int h = 1;
    for(int i = n/3; h < i; h = 3*h+1)
        ;
    do {
        for(int i = h; i < n; i++) {
            int v = a[i];
            int j = i;
            do {
                int t = a[j-h];
                if(t <= v)
                    break;
                a[j] = t;
                j -= h;
            } while(j >= h);
            a[j] = v;
        }
        h /= 3;
    } while(h > 0);
}

/******************************************** Heapsort for an integer array. */

#define LEFT(x)      (x<<1)
#define RIGHT(x)     ((x<<1)+1)
#define PARENT(x)    (x>>1)

void heapsort_int(int n, int *a)
{
    int i;
    int x, y, z, t, s;
    
    /* Shift index origin from 0 to 1 */
    a--; 
    /* Build heap */
    for(i = 2; i <= n; i++) {
        x = i;
        while(x > 1) {
            y = PARENT(x);
            if(a[x] <= a[y]) {
                /* heap property ok */
                break;              
            }
            /* Exchange a[x] and a[y] to enforce heap property */
            t = a[x];
            a[x] = a[y];
            a[y] = t;
            x = y;
        }
    }

    /* Repeat delete max and insert */
    for(i = n; i > 1; i--) {
        t = a[i];
        /* Delete max */
        a[i] = a[1];
        /* Insert t */
        x = 1;
        while((y = LEFT(x)) < i) {
            z = RIGHT(x);
            if(z < i && a[y] < a[z]) {
                s = z;
                z = y;
                y = s;
            }
            /* Invariant: a[y] >= a[z] */
            if(t >= a[y]) {
                /* ok to insert here without violating heap property */
                break;
            }
            /* Move a[y] up the heap */
            a[x] = a[y];
            x = y;
        }
        /* Insert here */
        a[x] = t; 
    }
}

void heapsort_long(long n, long *a)
{
    long i;
    long x, y, z, t, s;
    
    /* Shift index origin from 0 to 1 */
    a--; 
    /* Build heap */
    for(i = 2; i <= n; i++) {
        x = i;
        while(x > 1) {
            y = PARENT(x);
            if(a[x] <= a[y]) {
                /* heap property ok */
                break;              
            }
            /* Exchange a[x] and a[y] to enforce heap property */
            t = a[x];
            a[x] = a[y];
            a[y] = t;
            x = y;
        }
    }

    /* Repeat delete max and insert */
    for(i = n; i > 1; i--) {
        t = a[i];
        /* Delete max */
        a[i] = a[1];
        /* Insert t */
        x = 1;
        while((y = LEFT(x)) < i) {
            z = RIGHT(x);
            if(z < i && a[y] < a[z]) {
                s = z;
                z = y;
                y = s;
            }
            /* Invariant: a[y] >= a[z] */
            if(t >= a[y]) {
                /* ok to insert here without violating heap property */
                break;
            }
            /* Move a[y] up the heap */
            a[x] = a[y];
            x = y;
        }
        /* Insert here */
        a[x] = t; 
    }
}

/* Transforms p so that a[p[i]] <= a[p[j]] iff i <= j. */

void heapsort_int_indirect(int n, const int *a, int *p)
{
    int i;
    int x, y, z, t, s;
    
    /* Shift index origin from 0 to 1 */
    p--; 
    /* Build heap */
    for(i = 2; i <= n; i++) {
        x = i;
        while(x > 1) {
            y = PARENT(x);
            if(a[p[x]] <= a[p[y]]) {
                /* heap property ok */
                break;              
            }
            /* Exchange p[x] and p[y] to enforce heap property */
            t = p[x];
            p[x] = p[y];
            p[y] = t;
            x = y;
        }
    }

    /* Repeat delete max and insert */
    for(i = n; i > 1; i--) {
        t = p[i];
        /* Delete max */
        p[i] = p[1];
        /* Insert t */
        x = 1;
        while((y = LEFT(x)) < i) {
            z = RIGHT(x);
            if(z < i && a[p[y]] < a[p[z]]) {
                s = z;
                z = y;
                y = s;
            }
            /* Invariant: a[p[y]] >= a[p[z]] */
            if(a[t] >= a[p[y]]) {
                /* ok to insert here without violating heap property */
                break;
            }
            /* Move p[y] up the heap */
            p[x] = p[y];
            x = y;
        }
        /* Insert here */
        p[x] = t; 
    }
}

/*****************************************************************************/
