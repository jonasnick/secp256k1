/**********************************************************************
 * Copyright (c) 2022 Jonas Nick, Sivaram Dhakshinamoorthy            *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_BATCH_MAIN_H
#define SECP256K1_MODULE_BATCH_MAIN_H

#include "include/secp256k1_batch.h"
#include "src/hash.h"
#include "src/scratch.h"

/* Assume two batch objects batch1 and batch2. If we call
 * batch_add_tweaks on batch1 and batch_add_schnorrsig on batch2.
 * In this case same randomizer will be created if the bytes added to
 * batch1->sha and batch2->sha are same. Including this tag during
 * randomizer generation prevents such mishaps. */
enum batch_add_type {schnorrsig = 1, tweak_check = 2};

/* Maximum number of terms (schnorrsig or tweak checks) for
 * which the strauss algorithm remains efficient */
#define STRAUSS_MAX_TERMS_PER_BATCH 80

/** Opaque data structure that holds information required for the batch verification.
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
 *             the batch object. Used for generating a random secp256k1_scalar
 *             for each term added by secp256k1_batch_add_*.
 *        len: number of points (or scalars) present on batch object's scratch space.
 *   capacity: max number of points (or scalars) that the batch object can hold.
 *     result: tells whether the given set of inputs (schnorrsigs/tweaks) is valid
 *             or invalid. 1 = valid and 0 = invalid. By default, this is set to 1
 *             during batch object's creation (i.e, `secp256k1_batch_create`).
 *
 *  The following struct name is typdef as secp256k1_batch (in include/secp256k1_batch.h).
 */
struct secp256k1_batch_struct{
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

/** Clears the scalar and points allocated on the batch object's scratch space */
static void secp256k1_batch_scratch_clear(secp256k1_batch* batch) {
    secp256k1_scalar_clear(&batch->sc_g);
    batch->len = 0;
}

/** Allocates space for `batch->capacity` amount of scalars and points on batch
 *  object's scratch space */
static int secp256k1_batch_scratch_alloc(const secp256k1_callback* error_callback, secp256k1_batch* batch) {
    size_t checkpoint = secp256k1_scratch_checkpoint(error_callback, batch->data);
    size_t count = batch->capacity;

    VERIFY_CHECK(count > 0);

    batch->scalars = (secp256k1_scalar*)secp256k1_scratch_alloc(error_callback, batch->data, count*sizeof(secp256k1_scalar));
    batch->points = (secp256k1_gej*)secp256k1_scratch_alloc(error_callback, batch->data, count*sizeof(secp256k1_gej));

    /* If scalar or point allocation fails, restore scratch space to previous state */
    if (batch->scalars == NULL || batch->points == NULL) {
        secp256k1_scratch_apply_checkpoint(error_callback, batch->data, checkpoint);
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

secp256k1_batch* secp256k1_batch_create(const secp256k1_context* ctx, size_t max_terms, const unsigned char *aux_rand16) {
    size_t batch_size;
    secp256k1_batch* batch;
    size_t batch_scratch_size;
    unsigned char zeros[16] = {0};
    /* max limit on scratch space size in a batch */
    if (max_terms > STRAUSS_MAX_TERMS_PER_BATCH) {
        max_terms = STRAUSS_MAX_TERMS_PER_BATCH;
    }

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(max_terms != 0);
    /* Check that `max_terms` is less than half of the maximum size_t value. This is necessary because
     * `batch_add_schnorrsig` and `batch_add_xonlypub_tweak_check` appends two (scalar, point) pairs
     * for each input (sig/tweak) */
    ARG_CHECK(max_terms <= SIZE_MAX / 2);
    /* Check that max_terms is less than 2^31 to ensure the same behavior of this function on 32-bit
     * and 64-bit platforms. */
    ARG_CHECK(max_terms < ((uint32_t)1 << 31));

    batch_size = sizeof(secp256k1_batch);
    batch = (secp256k1_batch *)checked_malloc(&ctx->error_callback, batch_size);
    batch_scratch_size = secp256k1_batch_scratch_size(2*max_terms);
    if (batch != NULL) {
        /* create scratch space inside batch object, if that fails return NULL*/
        batch->data = secp256k1_scratch_create(&ctx->error_callback, batch_scratch_size);
        if (batch->data == NULL) {
            return NULL;
        }
        /* allocate 2*max_terms scalars and points on scratch space */
        batch->capacity = 2*max_terms;
        if (!secp256k1_batch_scratch_alloc(&ctx->error_callback, batch)) {
        /* if scalar or point allocation fails, free all the previous the allocated memory
           and return NULL */
            secp256k1_scratch_destroy(&ctx->error_callback, batch->data);
            free(batch);
            return NULL;
        }

        /* set remaining data members */
        secp256k1_scalar_clear(&batch->sc_g);
        secp256k1_batch_sha256_tagged(&batch->sha256);
        if (aux_rand16 != NULL) {
            secp256k1_sha256_write(&batch->sha256, aux_rand16, 16);
        } else {
            /* use 16 bytes of 0x0000...000, if no fresh randomness provided */
            secp256k1_sha256_write(&batch->sha256, zeros, 16);
        }
        batch->len = 0;
        batch->result = 1;
    }

    return batch;
}

void secp256k1_batch_destroy(const secp256k1_context *ctx, secp256k1_batch *batch) {
    VERIFY_CHECK(ctx != NULL);

    if (batch != NULL) {
        if(batch->data != NULL) {
            secp256k1_scratch_apply_checkpoint(&ctx->error_callback, batch->data, 0);
            secp256k1_scratch_destroy(&ctx->error_callback, batch->data);
        }
        free(batch);
    }
}

int secp256k1_batch_usable(const secp256k1_context *ctx, const secp256k1_batch *batch) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(batch != NULL);

    return batch->result;
}

/** verifies the schnorrsig/tweaks present in the batch object.
 *
 * For computing the multi-scalar point multiplication, calls secp256k1_ecmult_strauss_batch
 * on a scratch space filled with 2n points and 2n scalars, where n = no of terms (user input
 * in secp256k1_batch_create)
 *
 * Fails if:
 * 0 != -(s1 + a2*s2 + ... + au*su)G
 *      + R1 + a2*R2 + ... + au*Ru + e1*P1 + (a2*e2)P2 + ... + (au*eu)Pu.
 */
int secp256k1_batch_verify(const secp256k1_context *ctx, secp256k1_batch *batch) {
    secp256k1_gej resj;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(batch != NULL);

    if(batch->result == 0) {
        return 0;
    }

    if (batch->len > 0) {
        int mid_res = secp256k1_ecmult_strauss_batch_internal(&ctx->error_callback, batch->data, &resj, batch->scalars, batch->points, &batch->sc_g, batch->len) && secp256k1_gej_is_infinity(&resj);
        batch->result = batch->result && mid_res;
        secp256k1_batch_scratch_clear(batch);
    }

    return batch->result;
}

#endif /* SECP256K1_MODULE_BATCH_MAIN_H */
