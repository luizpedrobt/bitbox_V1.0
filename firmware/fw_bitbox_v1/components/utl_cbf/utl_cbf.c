/*
 * utl_cbf.c
 *
 *  Created on: Oct 5, 2025
 *      Author: pedro-bittencourt
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "utl_cbf.h"

#define CBF_INC(v,mv)   ((((v) + 1) >= (mv)) ? 0 : (v) + 1)

utl_cbf_status_t utl_cbf_init(utl_cbf_t *cb, uint8_t *area, uint32_t size)
{
    cb->buffer = area;
    cb->size = size;
    cb->prod = cb->cons = 0;

    return UTL_CBF_OK;
}

uint32_t utl_cbf_bytes_available(utl_cbf_t *cb)
{
    if(cb->prod >= cb->cons)
        return cb->prod - cb->cons;
    else
        return cb->prod + (cb->size - cb->cons);
}

utl_cbf_status_t utl_cbf_flush(utl_cbf_t *cb)
{
    cb->prod = cb->cons = 0;

    return UTL_CBF_OK;
}

utl_cbf_status_t utl_cbf_get(utl_cbf_t *cb, uint8_t *c)
{
    if(cb->cons == cb->prod)
        return UTL_CBF_EMPTY;

    *c = cb->buffer[cb->cons];
    cb->cons = CBF_INC(cb->cons,cb->size);

    return UTL_CBF_OK;
}

utl_cbf_status_t utl_cbf_put(utl_cbf_t *cb, uint8_t c)
{
    size_t next_prod = CBF_INC(cb->prod,cb->size);

    if(next_prod == cb->cons)
        return UTL_CBF_FULL;

    cb->buffer[cb->prod] = c;
    cb->prod = next_prod;

    return UTL_CBF_OK;
}
