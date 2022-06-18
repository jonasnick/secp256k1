#ifndef SECP256K1_BATCH_IMPL_H
#define SECP256K1_BATCH_IMPL_H

#include "../include/secp256k1.h"
#include "util.h"

/** Opaque data structure that holds context information for schnorr batch verification.
 *
 *  Members:
 *       data: scratch space object that contains points (gej) and their
 *             respective scalars. To be used in Multi-Scalar Multiplication
 *             algorithms such as Strauss and Pippenger.
 *    scalars: pointer to scalars allocated on the scratch space.
 *     points: pointer to points allocated on the scratch space.
 *       sc_g: scalar corresponding to the generator point in Multi-Scalar
 *             Multiplication equation.
 *     sha256: contains hash of all the inputs (schnorrsig/tweaks) present in
 *             the batch context. Used for generating a random secp256k1_scalar
 *             for each term added by secp256k1_batch_context_add_*.
 *        len: number of points (or scalars) present on batch context's scratch space.
 *   capacity: max number of points (or scalars) that the batch object can hold.
 *     result: tells whether the given set of inputs (schnorrsigs/tweaks) is valid
 *             or invalid. 1 = valid and 0 = invalid. By default, this is set to 1
 *             during batch context creation (i.e, `secp256k1_batch_create`).
 *
 *  The following struct name is typdef as secp256k1_batch_context (in include/secp256k1.h).
 */

struct secp256k1_batch_context_struct{
    secp256k1_scratch *data;
    secp256k1_scalar *scalars;
    secp256k1_gej *points;
    secp256k1_scalar sc_g;
    secp256k1_sha256 sha256;
    size_t len;
    size_t capacity;
    int result;
};

static size_t secp256k1_batch_scratch_size(int max_terms) {
    size_t ret = secp256k1_strauss_scratch_size(max_terms) + STRAUSS_SCRATCH_OBJECTS*16;
    /* Return value of 0 is reserved for error */
    VERIFY_CHECK(ret != 0);

    return ret;
}

/** Clears the scalar and points allocated on the batch context's scratch space */
static void secp256k1_batch_scratch_clear(secp256k1_batch_context* batch_ctx) {
    secp256k1_scalar_clear(&batch_ctx->sc_g);
    batch_ctx->len = 0;
}

/** Allocates space for `batch_ctx->capacity` amount of scalars and points on batch
 *  context's scratch space */
static int secp256k1_batch_scratch_alloc(const secp256k1_callback* error_callback, secp256k1_batch_context* batch_ctx) {
    size_t checkpoint = secp256k1_scratch_checkpoint(error_callback, batch_ctx->data);
    size_t count = batch_ctx->capacity;

    VERIFY_CHECK(count > 0);

    batch_ctx->scalars = (secp256k1_scalar*)secp256k1_scratch_alloc(error_callback, batch_ctx->data, count*sizeof(secp256k1_scalar));
    batch_ctx->points = (secp256k1_gej*)secp256k1_scratch_alloc(error_callback, batch_ctx->data, count*sizeof(secp256k1_gej));

    /* If scalar or point allocation fails, restore scratch space to previous state */
    if (batch_ctx->scalars == NULL || batch_ctx->points == NULL) {
        secp256k1_scratch_apply_checkpoint(error_callback, batch_ctx->data, checkpoint);
        return 0;
    }

    return 1;
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("BIP0340/batch")||SHA256("BIP0340/batch"). */
static void secp256k1_batch_sha256_tagged(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0x79e3e0d2ul;
    sha->s[1] = 0x12284f32ul;
    sha->s[2] = 0xd7d89e1cul;
    sha->s[3] = 0x6491ea9aul;
    sha->s[4] = 0xad823b2ful;
    sha->s[5] = 0xfacfe0b6ul;
    sha->s[6] = 0x342b78baul;
    sha->s[7] = 0x12ece87cul;

    sha->bytes = 64;
}

static secp256k1_batch_context* secp256k1_batch_create(const secp256k1_callback* error_callback, size_t max_terms) {
    size_t batch_size = sizeof(secp256k1_batch_context);
    size_t batch_scratch_size = secp256k1_batch_scratch_size(2*max_terms);
    secp256k1_batch_context* batch_ctx = (secp256k1_batch_context*)checked_malloc(&default_error_callback, batch_size);

    if (batch_ctx != NULL) {
        /* create scratch space inside batch context */
        batch_ctx->data = secp256k1_scratch_create(error_callback, batch_scratch_size);
        /* allocate 2*max_terms scalars and points on scratch space */
        batch_ctx->capacity = 2*max_terms;
        if (!secp256k1_batch_scratch_alloc(error_callback, batch_ctx)) {
        /* if scalar or point allocation fails, free all the previous the allocated memory
           and return NULL */
            secp256k1_scratch_destroy(error_callback, batch_ctx->data);
            free(batch_ctx);
            return NULL;
        }

        /* set remaining data members */
        secp256k1_scalar_clear(&batch_ctx->sc_g);
        secp256k1_batch_sha256_tagged(&batch_ctx->sha256);
        batch_ctx->len = 0;
        batch_ctx->result = 1;
    }

    return batch_ctx;
}

static void secp256k1_batch_destroy(const secp256k1_callback* error_callback, secp256k1_batch_context* batch_ctx) {
    if (batch_ctx != NULL) {
        if(batch_ctx->data != NULL) {
            secp256k1_scratch_apply_checkpoint(error_callback, batch_ctx->data, 0);
            secp256k1_scratch_destroy(error_callback, batch_ctx->data);
        }
        free(batch_ctx);
    }
}

/** Batch verifies the schnorrsig/tweaks present in the batch context object.
 *
 * For computing the multi-scalar point multiplication, calls secp256k1_ecmult_strauss_batch
 * on a scratch space filled with 2n points and 2n scalars, where n = no of terms (user input
 * in secp256k1_batch_context_create)
 *
 * Fails if:
 * 0 != -(s1 + a2*s2 + ... + au*su)G
 *      + R1 + a2*R2 + ... + au*Ru + e1*P1 + (a2*e2)P2 + ... + (au*eu)Pu.
 */
static int secp256k1_batch_verify(const secp256k1_callback* error_callback, secp256k1_batch_context* batch_ctx) {
    secp256k1_gej resj;
    int mid_res;

    if (batch_ctx->len > 0) {
        if(batch_ctx->scalars != NULL && batch_ctx->points != NULL) {
            mid_res = secp256k1_ecmult_strauss_batch(error_callback, batch_ctx->data, &resj, batch_ctx->scalars, batch_ctx->points, &batch_ctx->sc_g, NULL, NULL, batch_ctx->len, 0) && secp256k1_gej_is_infinity(&resj);
            batch_ctx->result = batch_ctx->result && mid_res;
        }
    }

    return batch_ctx->result;
}

#endif
