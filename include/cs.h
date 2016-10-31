/* NB: this is a subset of the routines in the CSPARSE package by
 Tim Davis et. al., for the full package please visit
 http://www.cise.ufl.edu/research/sparse/CSparse/ */

#ifndef CS_H
#define CS_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "constants.h"
#include "glob_opts.h"



typedef struct    /* matrix in compressed-column or triplet form */
{
    c_int nzmax ;	        /* maximum number of entries */
    c_int m ;	            /* number of rows */
    c_int n ;	            /* number of columns */
    c_int *p ;	            /* column pointers (size n+1) or col indices (size nzmax) */
    c_int *i ;	            /* row indices, size nzmax */
    c_float *x ;	            /* numerical values, size nzmax */
    c_int nnz ;	            /* # of entries in triplet matrix, -1 for compressed-col */
} csc ;


/* Create Compressed-Column-Sparse matrix from existing arrays
(no MALLOC to create inner arrays x, i, p)
*/
csc* csc_matrix(c_int m, c_int n, c_int nnz, c_float* x, c_int* i, c_int* p);

/* Create uninitialized Compressed-Column-Sparse matrix
(uses MALLOC to create inner arrays x, i, p)
*/
csc* new_csc_matrix(c_int m, c_int n, c_int nnz);

/* Free sparse matrix
(uses FREE to free inner arrays x, i, p)
 */
void free_csc_matrix(csc * M);

/* Convert sparse to dense */
c_float * csc_to_dns(csc * M);


#endif
