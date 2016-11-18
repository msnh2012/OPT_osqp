#include "polish.h"



/// Solution polishing: Solve equality constrained QP with assumed active constr.
c_int polish(Work *work){
    c_int j, ptr, mred=0, Ared_nnz=0;
    Priv *plsh; // = c_calloc(1, sizeof(Priv));

    #if PROFILING > 0
    tic(work->timer); // Start timer
    #endif

    // Initialize counters for active/inactive constraints
    work->pol->n_lAct = 0;
    work->pol->n_uAct = 0;
    work->pol->n_free = 0;

    /* Guess which linear constraints are lower-active, upper-active and free
     *    A2Ared[j] = -1    (if j-th row of A is not inserted in Ared)
     *    A2Ared[j] =  i    (if j-th row of A is inserted at i-th row of Ared)
     */
    for (j = 0; j < work->data->m; j++) {
        if ( work->z[work->data->n + j] - work->data->lA[j] <
             -work->settings->rho * work->u[j] ) {
                work->pol->ind_lAct[work->pol->n_lAct++] = j;     // lower-active
                work->pol->A2Ared[j] = mred++;
        }
        else if ( work->data->uA[j] - work->z[work->data->n + j] <
                  work->settings->rho * work->u[j] ) {
                    work->pol->ind_uAct[work->pol->n_uAct++] = j; // upper-active
                    work->pol->A2Ared[j] = mred++;
        }
        else {
            work->pol->ind_free[work->pol->n_free++] = j;        // free
            work->pol->A2Ared[j] = -1;
        }
    }
    work->pol->lambda_red = c_malloc(mred * sizeof(c_float));

    // Count number of elements in Ared
    for (j = 0; j < work->data->A->nzmax; j++) {
        if (work->pol->A2Ared[work->data->A->i[j]] != -1)
            Ared_nnz++;
    }
    // Form Ared
    work->pol->Ared = csc_spalloc(mred, work->data->n, Ared_nnz, 1, 0);
    Ared_nnz = 0;
    for (j = 0; j < work->data->n; j++) {  // Cycle over columns of A
        work->pol->Ared->p[j] = Ared_nnz;
        for (ptr = work->data->A->p[j]; ptr < work->data->A->p[j + 1]; ptr++) {
            if (work->pol->A2Ared[work->data->A->i[ptr]] != -1) {
                // if row of A should be added to Ared
                work->pol->Ared->i[Ared_nnz] = work->pol->A2Ared[work->data->A->i[ptr]];
                work->pol->Ared->x[Ared_nnz++] = work->data->A->x[ptr];
            }
        }
    }
    work->pol->Ared->p[work->data->n] = Ared_nnz;

    // Form and factorize reduced KKT
    plsh = init_priv(work->data->P, work->pol->Ared, work->settings, 1);

    // Form the rhs of the reduced KKT linear system
    c_float *rhs = c_malloc(sizeof(c_float) * (work->data->n + mred));
    for (j = 0; j < work->data->n; j++) {
        rhs[j] = -work->data->q[j];
    }
    for (j = 0; j < work->pol->n_lAct; j++) {
        rhs[work->data->n + j] = work->data->lA[work->pol->ind_lAct[j]];
    }
    for (j = 0; j < work->pol->n_uAct; j++) {
        rhs[work->data->n + work->pol->n_lAct + j] =
            work->data->uA[work->pol->ind_uAct[j]];
    }

    // Solve the reduced KKT system
    solve_lin_sys(work->settings, plsh, rhs);
    prea_vec_copy(rhs, work->pol->x, work->data->n);
    prea_vec_copy(rhs + work->data->n, work->pol->lambda_red, mred);
    mat_vec(work->data->A, work->pol->x, work->pol->Ax, 0);

    // Compute primal and dual residuals at the polished solution
    update_info(work, 0, 1);

    // Check if polishing was successful
    if (work->pol->pri_res < work->info->pri_res &&
        work->pol->dua_res < work->info->dua_res) {
            // Update solver information
            work->info->obj_val = work->pol->obj_val;
            work->info->pri_res = work->pol->pri_res;
            work->info->dua_res = work->pol->dua_res;
            work->info->status_polish = 1;
            // Update primal and dual variables
            prea_vec_copy(work->pol->x, work->solution->x, work->data->n);
            for (j = 0; j < work->data->m; j++) {
                if (work->pol->A2Ared[j] != -1){
                    work->solution->lambda[j] = work->pol->lambda_red[work->pol->A2Ared[j]];
                } else {
                    work->solution->lambda[j] = 0.0;
                }
            }
            // Print summary
            #if PRINTLEVEL > 1
            if (work->settings->verbose)
                print_polishing(work->info);
            #endif
    } else {
        // Polishing failed
        work->info->status_polish = 0;
        // TODO: Try to find a better solution on the line connecting ADMM
        //       and polished solution
    }

    /* Update timing */
    #if PROFILING > 0
    work->info->polish_time = toc(work->timer);
    #endif

    // Memory clean-up
    free_priv(plsh);
    c_free(rhs);

    return 0;
}