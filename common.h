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

#ifndef COMMON_READ
#define COMMON_READ

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define MALLOC(x)  common_malloc_wrapper(x)
#define FREE(x)    common_free_wrapper(x)
#define ERROR(...) common_error(__FILE__,__LINE__,__func__,__VA_ARGS__);
#define ABORT(...) common_abort(__FILE__,__LINE__,__func__,__VA_ARGS__);
#define FPRINTF(...) common_fprintf(__FILE__,__LINE__,__func__,__VA_ARGS__);
    
void          common_error            (const char *fn, 
                                       int line, 
                                       const char *func, 
                                       const char *format, ...);
void          common_abort            (const char *fn, 
                                       int line, 
                                       const char *func, 
                                       const char *format, ...);

void          common_fprintf          (const char *fn, 
                                       int line, 
                                       const char *func, 
                                       FILE *out,
                                       const char *format, ...);

const char *  common_hostname         (void);

void *        common_malloc_wrapper   (size_t size);
void          common_free_wrapper     (void *p);
void          common_check_balance    (void);
extern int    common_malloc_balance;

void          enable_timing           (void);
void          disable_timing          (void);
void          push_time               (void);
double        pop_time                (void);
void          pop_print_time          (const char *legend);

void          print_int_array         (FILE *out, int l, const int *a);

void          shellsort_int           (int n, int *a);
void          heapsort_int            (int n, int *a);
void          heapsort_long           (long n, long *a);
void          heapsort_int_indirect   (int n, const int *a, int *p);

#endif
