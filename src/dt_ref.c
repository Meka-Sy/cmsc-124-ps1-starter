/*
 * dt_ref.c: Owned references for Unit 5, Section J.
 *
 * This module detects three ownership failures.
 *
 * A dangling reference retains an address after release.
 * Reading that address has undefined behavior. It can return old data or terminate.
 *
 * A double release gives the same allocation to free twice.
 * This operation has undefined behavior. The visible failure can occur later.
 *
 * An unreleased allocation still has an owner at the final check. The driver
 * reports DT_ERR_LEAK before cleanup.
 *
 * A released flag lets this small interface report the first two mistakes as
 * DT_ERR_RELEASED. The driver's final check reports the third as DT_ERR_LEAK.
 * These checks model this assignment's ownership contract only.
 *
 * Ownership stops at the cell. dt_ref_new copies the value into an owned cell.
 * The environment still owns a string that the copied value references.
 */

#include "dt.h"

#include <stdlib.h>

struct dt_ref {
    dt_value *cell;
    bool      released;
};

/*
 * dt_ref_new builds a reference to a copy of v.
 * The reference owns this cell. It returns NULL after an allocation failure.
 */
dt_ref *dt_ref_new(dt_value v)
{
    /* TODO: Allocate the handle and cell. Copy v into the cell.
       Set the initial release state to false.
       dt_ref_new(dt_value_int(42))  -> a reference that prints as ref(42)
       an allocation failure          -> NULL
       cases/ownership/ref_released.case */

    // Allocate memory for the reference handle
    dt_ref *p = malloc(sizeof(dt_ref));

    // Return null if the handle allocation failed
    if (p == NULL){
        return NULL;
    }

    // Allocate the memory for the cell owned by the reference
    p -> cell = malloc(sizeof(dt_value));

    // If the cell allocation fails, free the handle before returning
    if (p->cell ==NULL){
        free(p);
        return NULL;
    }

    // Copy the value into the newly allocated cell
    *(p->cell) = v;

    // Mark the reference as not released when it is first created
    p->released = false;

    return p;
}

/*
 * dt_ref_borrow writes a copy of the cell value to *out.
 * It returns DT_ERR_RELEASED and does not change *out after release.
 * Check the release flag before you access the cell pointer.
 */
dt_status dt_ref_borrow(const dt_ref *p, dt_value *out)
{
    /* TODO: Return DT_ERR_RELEASED after release.
       Otherwise, copy the cell value to *out.
       Check the flag before you access the cell pointer.
       a live reference to 42:  dt_ref_borrow(p, &out) -> DT_OK, *out is 42
       after dt_ref_release(p): dt_ref_borrow(p, &out) -> DT_ERR_RELEASED,
                                                          *out untouched
       cases/ownership/ref_released.case,
       cases/post-release/borrow_after_release.case */
    
    // Check if the reference has already been released
    if (p->released){
        return DT_ERR_RELEASED;
    }

    // Copy the value from the owned cell to *out
    *out = *(p->cell);
    return DT_OK;
}

/*
 * dt_ref_release releases the cell and sets the release state.
 * It returns DT_ERR_RELEASED and changes nothing after an earlier release.
 */
dt_status dt_ref_release(dt_ref *p)
{
    /* TODO: Return DT_ERR_RELEASED after an earlier release.
       Otherwise, release the cell. Set the pointer to NULL. Set the release flag.
       The NULL assignment removes the stale cell address.
       The release flag must prevent each later access.
       first call on a live reference   -> DT_OK and releases the cell
       second call on the same one      -> DT_ERR_RELEASED and releases nothing
       a reference holding a string     -> releases the cell and preserves the string
       cases/ownership/ref_double_release.case,
       cases/ownership/ref_aliases_string.case */

    // Check if the reference has already been released
    if (p->released){
        return DT_ERR_RELEASED;
    }

    // Free the memory owned by the reference's cell
    free(p->cell);

    // Remove the cell address after freeing the cell
    p->cell = NULL;

    // Mark the reference as released to prevent later access
    p->released = true;

    return DT_OK;
}

/*
 * dt_ref_is_released reports the release state.
 * The driver uses this state to identify leaked cells.
 */
bool dt_ref_is_released(const dt_ref *p)
{
    /* TODO: Return the flag that dt_ref_release sets.
       The driver checks this flag for each reference at exit.
       A constant true result hides leaks. A constant false result reports false leaks.
       a live reference        -> false, so the driver reports DT_ERR_LEAK
       after dt_ref_release(p) -> true, so the driver reports no leak
       cases/ownership/ref_never_released.case, cases/ownership/ref_released.case */
    
    // Return the current release state of the reference
    return p->released;
}

/*
 * dt_ref_destroy releases a remaining cell and then releases the handle.
 * The driver reports leaks before it calls this function.
 * This function accepts NULL and does not report leaks.
 */
void dt_ref_destroy(dt_ref *p)
{
    /* TODO: Release a remaining cell. Then release the handle.
       Do not report leaks here. The driver already completed that check.
       a released reference  -> only the handle is left to free
       a live reference      -> the cell and the handle both go, quietly
       dt_ref_destroy(NULL)  -> returns, having done nothing */
    
    // Check if there is no reference to destroy
    if (p==NULL){
        return;
    }

    // Release the cell only if it has not already been released
    if (!p->released){
        free(p->cell);
        p->cell=NULL;
        p->released = true;
    }

    // Free the reference handle itself
    free(p);
}
