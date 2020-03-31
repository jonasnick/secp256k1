/**********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "include/secp256k1.h"
#include "include/secp256k1_preallocated.h"

#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "group_impl.h"
#include "ecmult_impl.h"
#include "ecmult_const_impl.h"
#include "ecmult_gen_impl.h"
#include "ecdsa_impl.h"
#include "eckey_impl.h"
#include "hash_impl.h"
#include "scratch_impl.h"

#if defined(VALGRIND)
# include <valgrind/memcheck.h>
#endif

#define ARG_CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        secp256k1_callback_call(&ctx->illegal_callback, #cond); \
        return 0; \
    } \
} while(0)

#define ARG_CHECK_NO_RETURN(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        secp256k1_callback_call(&ctx->illegal_callback, #cond); \
    } \
} while(0)

#ifndef USE_EXTERNAL_DEFAULT_CALLBACKS
#include <stdlib.h>
#include <stdio.h>
static void secp256k1_default_illegal_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libsecp256k1] illegal argument: %s\n", str);
    abort();
}
static void secp256k1_default_error_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libsecp256k1] internal consistency check failed: %s\n", str);
    abort();
}
#else
void secp256k1_default_illegal_callback_fn(const char* str, void* data);
void secp256k1_default_error_callback_fn(const char* str, void* data);
#endif

static const secp256k1_callback default_illegal_callback = {
    secp256k1_default_illegal_callback_fn,
    NULL
};

static const secp256k1_callback default_error_callback = {
    secp256k1_default_error_callback_fn,
    NULL
};

struct secp256k1_context_struct {
    secp256k1_ecmult_context ecmult_ctx;
    secp256k1_ecmult_gen_context ecmult_gen_ctx;
    secp256k1_callback illegal_callback;
    secp256k1_callback error_callback;
    int declassify;
};

static const secp256k1_context secp256k1_context_no_precomp_ = {
    { 0 },
    { 0 },
    { secp256k1_default_illegal_callback_fn, 0 },
    { secp256k1_default_error_callback_fn, 0 },
    0
};
const secp256k1_context *secp256k1_context_no_precomp = &secp256k1_context_no_precomp_;

size_t secp256k1_context_preallocated_size(unsigned int flags) {
    size_t ret = ROUND_TO_ALIGN(sizeof(secp256k1_context));

    if (EXPECT((flags & SECP256K1_FLAGS_TYPE_MASK) != SECP256K1_FLAGS_TYPE_CONTEXT, 0)) {
            secp256k1_callback_call(&default_illegal_callback,
                                    "Invalid flags");
            return 0;
    }

    if (flags & SECP256K1_FLAGS_BIT_CONTEXT_SIGN) {
        ret += SECP256K1_ECMULT_GEN_CONTEXT_PREALLOCATED_SIZE;
    }
    if (flags & SECP256K1_FLAGS_BIT_CONTEXT_VERIFY) {
        ret += SECP256K1_ECMULT_CONTEXT_PREALLOCATED_SIZE;
    }
    return ret;
}

size_t secp256k1_context_preallocated_clone_size(const secp256k1_context* ctx) {
    size_t ret = ROUND_TO_ALIGN(sizeof(secp256k1_context));
    VERIFY_CHECK(ctx != NULL);
    if (secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx)) {
        ret += SECP256K1_ECMULT_GEN_CONTEXT_PREALLOCATED_SIZE;
    }
    if (secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx)) {
        ret += SECP256K1_ECMULT_CONTEXT_PREALLOCATED_SIZE;
    }
    return ret;
}

secp256k1_context* secp256k1_context_preallocated_create(void* prealloc, unsigned int flags) {
    void* const base = prealloc;
    size_t prealloc_size;
    secp256k1_context* ret;

    VERIFY_CHECK(prealloc != NULL);
    prealloc_size = secp256k1_context_preallocated_size(flags);
    ret = (secp256k1_context*)manual_alloc(&prealloc, sizeof(secp256k1_context), base, prealloc_size);
    ret->illegal_callback = default_illegal_callback;
    ret->error_callback = default_error_callback;

    if (EXPECT((flags & SECP256K1_FLAGS_TYPE_MASK) != SECP256K1_FLAGS_TYPE_CONTEXT, 0)) {
            secp256k1_callback_call(&ret->illegal_callback,
                                    "Invalid flags");
            return NULL;
    }

    secp256k1_ecmult_context_init(&ret->ecmult_ctx);
    secp256k1_ecmult_gen_context_init(&ret->ecmult_gen_ctx);

    if (flags & SECP256K1_FLAGS_BIT_CONTEXT_SIGN) {
        secp256k1_ecmult_gen_context_build(&ret->ecmult_gen_ctx, &prealloc);
    }
    if (flags & SECP256K1_FLAGS_BIT_CONTEXT_VERIFY) {
        secp256k1_ecmult_context_build(&ret->ecmult_ctx, &prealloc);
    }
    ret->declassify = !!(flags & SECP256K1_FLAGS_BIT_CONTEXT_DECLASSIFY);

    return (secp256k1_context*) ret;
}

secp256k1_context* secp256k1_context_create(unsigned int flags) {
    size_t const prealloc_size = secp256k1_context_preallocated_size(flags);
    secp256k1_context* ctx = (secp256k1_context*)checked_malloc(&default_error_callback, prealloc_size);
    if (EXPECT(secp256k1_context_preallocated_create(ctx, flags) == NULL, 0)) {
        free(ctx);
        return NULL;
    }

    return ctx;
}

secp256k1_context* secp256k1_context_preallocated_clone(const secp256k1_context* ctx, void* prealloc) {
    size_t prealloc_size;
    secp256k1_context* ret;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(prealloc != NULL);

    prealloc_size = secp256k1_context_preallocated_clone_size(ctx);
    ret = (secp256k1_context*)prealloc;
    memcpy(ret, ctx, prealloc_size);
    secp256k1_ecmult_gen_context_finalize_memcpy(&ret->ecmult_gen_ctx, &ctx->ecmult_gen_ctx);
    secp256k1_ecmult_context_finalize_memcpy(&ret->ecmult_ctx, &ctx->ecmult_ctx);
    return ret;
}

secp256k1_context* secp256k1_context_clone(const secp256k1_context* ctx) {
    secp256k1_context* ret;
    size_t prealloc_size;

    VERIFY_CHECK(ctx != NULL);
    prealloc_size = secp256k1_context_preallocated_clone_size(ctx);
    ret = (secp256k1_context*)checked_malloc(&ctx->error_callback, prealloc_size);
    ret = secp256k1_context_preallocated_clone(ctx, ret);
    return ret;
}

void secp256k1_context_preallocated_destroy(secp256k1_context* ctx) {
    ARG_CHECK_NO_RETURN(ctx != secp256k1_context_no_precomp);
    if (ctx != NULL) {
        secp256k1_ecmult_context_clear(&ctx->ecmult_ctx);
        secp256k1_ecmult_gen_context_clear(&ctx->ecmult_gen_ctx);
    }
}

void secp256k1_context_destroy(secp256k1_context* ctx) {
    if (ctx != NULL) {
        secp256k1_context_preallocated_destroy(ctx);
        free(ctx);
    }
}

void secp256k1_context_set_illegal_callback(secp256k1_context* ctx, void (*fun)(const char* message, void* data), const void* data) {
    ARG_CHECK_NO_RETURN(ctx != secp256k1_context_no_precomp);
    if (fun == NULL) {
        fun = secp256k1_default_illegal_callback_fn;
    }
    ctx->illegal_callback.fn = fun;
    ctx->illegal_callback.data = data;
}

void secp256k1_context_set_error_callback(secp256k1_context* ctx, void (*fun)(const char* message, void* data), const void* data) {
    ARG_CHECK_NO_RETURN(ctx != secp256k1_context_no_precomp);
    if (fun == NULL) {
        fun = secp256k1_default_error_callback_fn;
    }
    ctx->error_callback.fn = fun;
    ctx->error_callback.data = data;
}

secp256k1_scratch_space* secp256k1_scratch_space_create(const secp256k1_context* ctx, size_t max_size) {
    VERIFY_CHECK(ctx != NULL);
    return secp256k1_scratch_create(&ctx->error_callback, max_size);
}

void secp256k1_scratch_space_destroy(const secp256k1_context *ctx, secp256k1_scratch_space* scratch) {
    VERIFY_CHECK(ctx != NULL);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

/* Mark memory as no-longer-secret for the purpose of analysing constant-time behaviour
 *  of the software. This is setup for use with valgrind but could be substituted with
 *  the appropriate instrumentation for other analysis tools.
 */
static SECP256K1_INLINE void secp256k1_declassify(const secp256k1_context* ctx, const void *p, size_t len) {
#if defined(VALGRIND)
    if (EXPECT(ctx->declassify,0)) VALGRIND_MAKE_MEM_DEFINED(p, len);
#else
    (void)ctx;
    (void)p;
    (void)len;
#endif
}

static int secp256k1_pubkey_load(const secp256k1_context* ctx, secp256k1_ge* ge, const secp256k1_pubkey* pubkey) {
    if (sizeof(secp256k1_ge_storage) == 64) {
        /* When the secp256k1_ge_storage type is exactly 64 byte, use its
         * representation inside secp256k1_pubkey, as conversion is very fast.
         * Note that secp256k1_pubkey_save must use the same representation. */
        secp256k1_ge_storage s;
        memcpy(&s, &pubkey->data[0], sizeof(s));
        secp256k1_ge_from_storage(ge, &s);
    } else {
        /* Otherwise, fall back to 32-byte big endian for X and Y. */
        secp256k1_fe x, y;
        secp256k1_fe_set_b32(&x, pubkey->data);
        secp256k1_fe_set_b32(&y, pubkey->data + 32);
        secp256k1_ge_set_xy(ge, &x, &y);
    }
    ARG_CHECK(!secp256k1_fe_is_zero(&ge->x));
    return 1;
}

static void secp256k1_pubkey_save(secp256k1_pubkey* pubkey, secp256k1_ge* ge) {
    if (sizeof(secp256k1_ge_storage) == 64) {
        secp256k1_ge_storage s;
        secp256k1_ge_to_storage(&s, ge);
        memcpy(&pubkey->data[0], &s, sizeof(s));
    } else {
        VERIFY_CHECK(!secp256k1_ge_is_infinity(ge));
        secp256k1_fe_normalize_var(&ge->x);
        secp256k1_fe_normalize_var(&ge->y);
        secp256k1_fe_get_b32(pubkey->data, &ge->x);
        secp256k1_fe_get_b32(pubkey->data + 32, &ge->y);
    }
}

int secp256k1_ec_pubkey_parse(const secp256k1_context* ctx, secp256k1_pubkey* pubkey, const unsigned char *input, size_t inputlen) {
    secp256k1_ge Q;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(input != NULL);
    if (!secp256k1_eckey_pubkey_parse(&Q, input, inputlen)) {
        return 0;
    }
    secp256k1_pubkey_save(pubkey, &Q);
    secp256k1_ge_clear(&Q);
    return 1;
}

int secp256k1_ec_pubkey_serialize(const secp256k1_context* ctx, unsigned char *output, size_t *outputlen, const secp256k1_pubkey* pubkey, unsigned int flags) {
    secp256k1_ge Q;
    size_t len;
    int ret = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(outputlen != NULL);
    ARG_CHECK(*outputlen >= ((flags & SECP256K1_FLAGS_BIT_COMPRESSION) ? 33 : 65));
    len = *outputlen;
    *outputlen = 0;
    ARG_CHECK(output != NULL);
    memset(output, 0, len);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK((flags & SECP256K1_FLAGS_TYPE_MASK) == SECP256K1_FLAGS_TYPE_COMPRESSION);
    if (secp256k1_pubkey_load(ctx, &Q, pubkey)) {
        ret = secp256k1_eckey_pubkey_serialize(&Q, output, &len, flags & SECP256K1_FLAGS_BIT_COMPRESSION);
        if (ret) {
            *outputlen = len;
        }
    }
    return ret;
}

static void secp256k1_ecdsa_signature_load(const secp256k1_context* ctx, secp256k1_scalar* r, secp256k1_scalar* s, const secp256k1_ecdsa_signature* sig) {
    (void)ctx;
    if (sizeof(secp256k1_scalar) == 32) {
        /* When the secp256k1_scalar type is exactly 32 byte, use its
         * representation inside secp256k1_ecdsa_signature, as conversion is very fast.
         * Note that secp256k1_ecdsa_signature_save must use the same representation. */
        memcpy(r, &sig->data[0], 32);
        memcpy(s, &sig->data[32], 32);
    } else {
        secp256k1_scalar_set_b32(r, &sig->data[0], NULL);
        secp256k1_scalar_set_b32(s, &sig->data[32], NULL);
    }
}

static void secp256k1_ecdsa_signature_save(secp256k1_ecdsa_signature* sig, const secp256k1_scalar* r, const secp256k1_scalar* s) {
    if (sizeof(secp256k1_scalar) == 32) {
        memcpy(&sig->data[0], r, 32);
        memcpy(&sig->data[32], s, 32);
    } else {
        secp256k1_scalar_get_b32(&sig->data[0], r);
        secp256k1_scalar_get_b32(&sig->data[32], s);
    }
}

int secp256k1_ecdsa_signature_parse_der(const secp256k1_context* ctx, secp256k1_ecdsa_signature* sig, const unsigned char *input, size_t inputlen) {
    secp256k1_scalar r, s;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(input != NULL);

    if (secp256k1_ecdsa_sig_parse(&r, &s, input, inputlen)) {
        secp256k1_ecdsa_signature_save(sig, &r, &s);
        return 1;
    } else {
        memset(sig, 0, sizeof(*sig));
        return 0;
    }
}

int secp256k1_ecdsa_signature_parse_compact(const secp256k1_context* ctx, secp256k1_ecdsa_signature* sig, const unsigned char *input64) {
    secp256k1_scalar r, s;
    int ret = 1;
    int overflow = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(input64 != NULL);

    secp256k1_scalar_set_b32(&r, &input64[0], &overflow);
    ret &= !overflow;
    secp256k1_scalar_set_b32(&s, &input64[32], &overflow);
    ret &= !overflow;
    if (ret) {
        secp256k1_ecdsa_signature_save(sig, &r, &s);
    } else {
        memset(sig, 0, sizeof(*sig));
    }
    return ret;
}

int secp256k1_ecdsa_signature_serialize_der(const secp256k1_context* ctx, unsigned char *output, size_t *outputlen, const secp256k1_ecdsa_signature* sig) {
    secp256k1_scalar r, s;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(outputlen != NULL);
    ARG_CHECK(sig != NULL);

    secp256k1_ecdsa_signature_load(ctx, &r, &s, sig);
    return secp256k1_ecdsa_sig_serialize(output, outputlen, &r, &s);
}

int secp256k1_ecdsa_signature_serialize_compact(const secp256k1_context* ctx, unsigned char *output64, const secp256k1_ecdsa_signature* sig) {
    secp256k1_scalar r, s;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output64 != NULL);
    ARG_CHECK(sig != NULL);

    secp256k1_ecdsa_signature_load(ctx, &r, &s, sig);
    secp256k1_scalar_get_b32(&output64[0], &r);
    secp256k1_scalar_get_b32(&output64[32], &s);
    return 1;
}

int secp256k1_ecdsa_signature_normalize(const secp256k1_context* ctx, secp256k1_ecdsa_signature *sigout, const secp256k1_ecdsa_signature *sigin) {
    secp256k1_scalar r, s;
    int ret = 0;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sigin != NULL);

    secp256k1_ecdsa_signature_load(ctx, &r, &s, sigin);
    ret = secp256k1_scalar_is_high(&s);
    if (sigout != NULL) {
        if (ret) {
            secp256k1_scalar_negate(&s, &s);
        }
        secp256k1_ecdsa_signature_save(sigout, &r, &s);
    }

    return ret;
}

int secp256k1_ecdsa_verify(const secp256k1_context* ctx, const secp256k1_ecdsa_signature *sig, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ge q;
    secp256k1_scalar r, s;
    secp256k1_scalar m;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(pubkey != NULL);

    secp256k1_scalar_set_b32(&m, msg32, NULL);
    secp256k1_ecdsa_signature_load(ctx, &r, &s, sig);
    return (!secp256k1_scalar_is_high(&s) &&
            secp256k1_pubkey_load(ctx, &q, pubkey) &&
            secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &r, &s, &q, &m));
}

static SECP256K1_INLINE void buffer_append(unsigned char *buf, unsigned int *offset, const void *data, unsigned int len) {
    memcpy(buf + *offset, data, len);
    *offset += len;
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("BIP340/nonce")||SHA256("BIP340/nonce"). */
static void secp256k1_nonce_function_bip340_sha256_tagged(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0xa96e75cbul;
    sha->s[1] = 0x74f9f0acul;
    sha->s[2] = 0xc49e3c98ul;
    sha->s[3] = 0x202f99baul;
    sha->s[4] = 0x8946a616ul;
    sha->s[5] = 0x4accf415ul;
    sha->s[6] = 0x86e335c3ul;
    sha->s[7] = 0x48d0a072ul;

    sha->bytes = 64;
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("BIP340/aux")||SHA256("BIP340/aux"). */
static void secp256k1_nonce_function_bip340_sha256_tagged_aux(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0x5d74a872ul;
    sha->s[1] = 0xd57064d4ul;
    sha->s[2] = 0x89495becul;
    sha->s[3] = 0x910f46f5ul;
    sha->s[4] = 0xcbc6fd3eul;
    sha->s[5] = 0xaf05d9d0ul;
    sha->s[6] = 0xcb781ce6ul;
    sha->s[7] = 0x062930acul;

    sha->bytes = 64;
}

static int nonce_function_bip340(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo16, void *data, unsigned int counter) {
    secp256k1_sha256 sha;
    unsigned char masked_key[32];
    int i;

    if (counter != 0) {
        return 0;
    }
    if (algo16 == NULL) {
        return 0;
    }

    if (data != NULL) {
        secp256k1_nonce_function_bip340_sha256_tagged_aux(&sha);
        secp256k1_sha256_write(&sha, data, 32);
        secp256k1_sha256_finalize(&sha, masked_key);
        for (i = 0; i < 32; i++) {
            masked_key[i] ^= key32[i];
        }
    }

    /* Tag the hash with algo16 which is important to avoid nonce reuse across
     * algorithms. If this nonce function is used in BIP-340 signing as defined
     * in the spec, an optimized tagging implementation is used. */
    if (memcmp(algo16, "BIP340/nonce0000", 16) == 0) {
        secp256k1_nonce_function_bip340_sha256_tagged(&sha);
    } else {
        secp256k1_sha256_initialize_tagged(&sha, algo16, 16);
    }

    /* Hash (masked-)key||pk||msg using the tagged hash as per the spec */
    if (data != NULL) {
        secp256k1_sha256_write(&sha, masked_key, 32);
    } else {
        secp256k1_sha256_write(&sha, key32, 32);
    }
    secp256k1_sha256_write(&sha, xonly_pk32, 32);
    secp256k1_sha256_write(&sha, msg32, 32);
    secp256k1_sha256_finalize(&sha, nonce32);
    return 1;
}

static int nonce_function_rfc6979(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
   unsigned char keydata[112];
   unsigned int offset = 0;
   secp256k1_rfc6979_hmac_sha256 rng;
   unsigned int i;
   /* We feed a byte array to the PRNG as input, consisting of:
    * - the private key (32 bytes) and message (32 bytes), see RFC 6979 3.2d.
    * - optionally 32 extra bytes of data, see RFC 6979 3.6 Additional Data.
    * - optionally 16 extra bytes with the algorithm name.
    * Because the arguments have distinct fixed lengths it is not possible for
    *  different argument mixtures to emulate each other and result in the same
    *  nonces.
    */
   buffer_append(keydata, &offset, key32, 32);
   buffer_append(keydata, &offset, msg32, 32);
   if (data != NULL) {
       buffer_append(keydata, &offset, data, 32);
   }
   if (algo16 != NULL) {
       buffer_append(keydata, &offset, algo16, 16);
   }
   secp256k1_rfc6979_hmac_sha256_initialize(&rng, keydata, offset);
   memset(keydata, 0, sizeof(keydata));
   for (i = 0; i <= counter; i++) {
       secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
   }
   secp256k1_rfc6979_hmac_sha256_finalize(&rng);
   return 1;
}

const secp256k1_nonce_function_extended secp256k1_nonce_function_bip340 = nonce_function_bip340;
const secp256k1_nonce_function secp256k1_nonce_function_rfc6979 = nonce_function_rfc6979;
const secp256k1_nonce_function secp256k1_nonce_function_default = nonce_function_rfc6979;

int secp256k1_ecdsa_sign(const secp256k1_context* ctx, secp256k1_ecdsa_signature *signature, const unsigned char *msg32, const unsigned char *seckey, secp256k1_nonce_function noncefp, const void* noncedata) {
    secp256k1_scalar r, s;
    secp256k1_scalar sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned char nonce32[32];
    unsigned int count = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(signature != NULL);
    ARG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    /* Fail if the secret key is invalid. */
    overflow |= secp256k1_scalar_is_zero(&sec);
    secp256k1_scalar_cmov(&sec, &secp256k1_scalar_one, overflow);
    secp256k1_scalar_set_b32(&msg, msg32, NULL);
    while (1) {
        int koverflow;
        ret = noncefp(nonce32, msg32, seckey, NULL, (void*)noncedata, count);
        if (!ret) {
            break;
        }
        secp256k1_scalar_set_b32(&non, nonce32, &koverflow);
        koverflow |= secp256k1_scalar_is_zero(&non);
        /* The nonce is still secret here, but it overflowing or being zero is is less likely than 1:2^255. */
        secp256k1_declassify(ctx, &koverflow, sizeof(koverflow));
        if (!koverflow) {
            ret = secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &r, &s, &sec, &msg, &non, NULL);
            /* The final signature is no longer a secret, nor is the fact that we were successful or not. */
            secp256k1_declassify(ctx, &ret, sizeof(ret));
            if (ret) {
                break;
            }
        }
        count++;
    }
    memset(nonce32, 0, 32);
    secp256k1_scalar_clear(&msg);
    secp256k1_scalar_clear(&non);
    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_cmov(&r, &secp256k1_scalar_zero, (!ret) | overflow);
    secp256k1_scalar_cmov(&s, &secp256k1_scalar_zero, (!ret) | overflow);
    secp256k1_ecdsa_signature_save(signature, &r, &s);
    return !!ret & !overflow;
}

int secp256k1_ec_seckey_verify(const secp256k1_context* ctx, const unsigned char *seckey) {
    secp256k1_scalar sec;
    int ret;
    int overflow;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = !overflow & !secp256k1_scalar_is_zero(&sec);
    secp256k1_scalar_clear(&sec);
    return ret;
}


static int secp256k1_ec_pubkey_create_helper(const secp256k1_ecmult_gen_context *ecmult_gen_ctx, secp256k1_scalar *seckey_scalar, secp256k1_ge *p, const unsigned char *seckey) {
    secp256k1_gej pj;
    int ret;
    int overflow;
    secp256k1_scalar_set_b32(seckey_scalar, seckey, &overflow);
    ret = !overflow & !secp256k1_scalar_is_zero(seckey_scalar);
    secp256k1_scalar_cmov(seckey_scalar, &secp256k1_scalar_one, !ret);

    secp256k1_ecmult_gen(ecmult_gen_ctx, &pj, seckey_scalar);
    secp256k1_ge_set_gej(p, &pj);
    return ret;
}

int secp256k1_ec_pubkey_create(const secp256k1_context* ctx, secp256k1_pubkey *pubkey, const unsigned char *seckey) {
    secp256k1_ge p;
    secp256k1_scalar seckey_scalar;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(seckey != NULL);

    ret = secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &seckey_scalar, &p, seckey);
    secp256k1_pubkey_save(pubkey, &p);
    memczero(pubkey, sizeof(*pubkey), !ret);

    secp256k1_scalar_clear(&seckey_scalar);
    return ret;
}

int secp256k1_ec_privkey_negate(const secp256k1_context* ctx, unsigned char *seckey) {
    secp256k1_scalar sec;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    secp256k1_scalar_negate(&sec, &sec);
    secp256k1_scalar_get_b32(seckey, &sec);

    secp256k1_scalar_clear(&sec);
    return 1;
}

int secp256k1_ec_pubkey_negate(const secp256k1_context* ctx, secp256k1_pubkey *pubkey) {
    int ret = 0;
    secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);

    ret = secp256k1_pubkey_load(ctx, &p, pubkey);
    memset(pubkey, 0, sizeof(*pubkey));
    if (ret) {
        secp256k1_ge_neg(&p, &p);
        secp256k1_pubkey_save(pubkey, &p);
    }
    return ret;
}

int secp256k1_ec_privkey_tweak_add_helper(secp256k1_scalar *sec, const unsigned char *tweak) {
    secp256k1_scalar term;
    int overflow = 0;
    int ret = 0;

    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    ret = (!overflow) & secp256k1_eckey_privkey_tweak_add(sec, &term);
    secp256k1_scalar_cmov(sec, &secp256k1_scalar_zero, !ret);
    secp256k1_scalar_clear(&term);
    return ret;
}

int secp256k1_ec_privkey_tweak_add(const secp256k1_context* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar sec;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);
    ARG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    ret = secp256k1_ec_privkey_tweak_add_helper(&sec, tweak);
    secp256k1_scalar_get_b32(seckey, &sec);

    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ec_pubkey_tweak_add_helper(const secp256k1_ecmult_context* ecmult_ctx, secp256k1_ge *p, const unsigned char *tweak) {
    secp256k1_scalar term;
    int overflow = 0;
    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    return !overflow && secp256k1_eckey_pubkey_tweak_add(ecmult_ctx, p, &term);
}

int secp256k1_ec_pubkey_tweak_add(const secp256k1_context* ctx, secp256k1_pubkey *pubkey, const unsigned char *tweak) {
    secp256k1_ge p;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(tweak != NULL);

    ret = secp256k1_pubkey_load(ctx, &p, pubkey);
    memset(pubkey, 0, sizeof(*pubkey));
    ret = ret && secp256k1_ec_pubkey_tweak_add_helper(&ctx->ecmult_ctx, &p, tweak);
    if (ret) {
        secp256k1_pubkey_save(pubkey, &p);
    }

    return ret;
}

int secp256k1_ec_privkey_tweak_mul(const secp256k1_context* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar factor;
    secp256k1_scalar sec;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);
    ARG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    ret = (!overflow) & secp256k1_eckey_privkey_tweak_mul(&sec, &factor);
    secp256k1_scalar_cmov(&sec, &secp256k1_scalar_zero, !ret);
    secp256k1_scalar_get_b32(seckey, &sec);

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&factor);
    return ret;
}

int secp256k1_ec_pubkey_tweak_mul(const secp256k1_context* ctx, secp256k1_pubkey *pubkey, const unsigned char *tweak) {
    secp256k1_ge p;
    secp256k1_scalar factor;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    ret = !overflow && secp256k1_pubkey_load(ctx, &p, pubkey);
    memset(pubkey, 0, sizeof(*pubkey));
    if (ret) {
        if (secp256k1_eckey_pubkey_tweak_mul(&ctx->ecmult_ctx, &p, &factor)) {
            secp256k1_pubkey_save(pubkey, &p);
        } else {
            ret = 0;
        }
    }

    return ret;
}

int secp256k1_context_randomize(secp256k1_context* ctx, const unsigned char *seed32) {
    VERIFY_CHECK(ctx != NULL);
    if (secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx)) {
        secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, seed32);
    }
    return 1;
}

int secp256k1_ec_pubkey_combine(const secp256k1_context* ctx, secp256k1_pubkey *pubnonce, const secp256k1_pubkey * const *pubnonces, size_t n) {
    size_t i;
    secp256k1_gej Qj;
    secp256k1_ge Q;

    ARG_CHECK(pubnonce != NULL);
    memset(pubnonce, 0, sizeof(*pubnonce));
    ARG_CHECK(n >= 1);
    ARG_CHECK(pubnonces != NULL);

    secp256k1_gej_set_infinity(&Qj);

    for (i = 0; i < n; i++) {
        secp256k1_pubkey_load(ctx, &Q, pubnonces[i]);
        secp256k1_gej_add_ge(&Qj, &Qj, &Q);
    }
    if (secp256k1_gej_is_infinity(&Qj)) {
        return 0;
    }
    secp256k1_ge_set_gej(&Q, &Qj);
    secp256k1_pubkey_save(pubnonce, &Q);
    return 1;
}

static SECP256K1_INLINE int secp256k1_xonly_pubkey_load(const secp256k1_context* ctx, secp256k1_ge *ge, const secp256k1_xonly_pubkey *pubkey) {
    return secp256k1_pubkey_load(ctx, ge, (const secp256k1_pubkey *) pubkey);
}

static SECP256K1_INLINE void secp256k1_xonly_pubkey_save(secp256k1_xonly_pubkey *pubkey, secp256k1_ge *ge) {
    secp256k1_pubkey_save((secp256k1_pubkey *) pubkey, ge);
}

int secp256k1_xonly_pubkey_parse(const secp256k1_context* ctx, secp256k1_xonly_pubkey *pubkey, const unsigned char *input32) {
    secp256k1_ge pk;
    secp256k1_fe x;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(input32 != NULL);

    if (!secp256k1_fe_set_b32(&x, input32)) {
        return 0;
    }
    if (!secp256k1_ge_set_xo_var(&pk, &x, 0)) {
        return 0;
    }
    secp256k1_xonly_pubkey_save(pubkey, &pk);
    secp256k1_ge_clear(&pk);
    return 1;
}

int secp256k1_xonly_pubkey_serialize(const secp256k1_context* ctx, unsigned char *output32, const secp256k1_xonly_pubkey *pubkey) {
    secp256k1_ge pk;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output32 != NULL);
    memset(output32, 0, 32);
    ARG_CHECK(pubkey != NULL);

    if (!secp256k1_xonly_pubkey_load(ctx, &pk, pubkey)) {
        return 0;
    }
    secp256k1_fe_get_b32(output32, &pk.x);
    return 1;
}

int secp256k1_xonly_pubkey_from_pubkey(const secp256k1_context* ctx, secp256k1_xonly_pubkey *xonly_pubkey, int *y_parity, const secp256k1_pubkey *pubkey) {
    secp256k1_ge pk;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(xonly_pubkey != NULL);
    ARG_CHECK(pubkey != NULL);

    secp256k1_pubkey_load(ctx, &pk, pubkey);
    secp256k1_ge_even_y(&pk, y_parity);
    secp256k1_xonly_pubkey_save(xonly_pubkey, &pk);
    return 1;
}

int secp256k1_xonly_pubkey_tweak_add(const secp256k1_context* ctx, secp256k1_pubkey *output_pubkey, const secp256k1_xonly_pubkey *internal_pubkey, const unsigned char *tweak32) {
    secp256k1_ge pk;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(output_pubkey != NULL);
    ARG_CHECK(internal_pubkey != NULL);
    ARG_CHECK(tweak32 != NULL);

    memset(output_pubkey, 0, sizeof(*output_pubkey));
    if (!secp256k1_xonly_pubkey_load(ctx, &pk, internal_pubkey)
        || !secp256k1_ec_pubkey_tweak_add_helper(&ctx->ecmult_ctx, &pk, tweak32)) {
        return 0;
    }
    secp256k1_pubkey_save(output_pubkey, &pk);
    return 1;
}

int secp256k1_xonly_pubkey_tweak_test(const secp256k1_context* ctx, const unsigned char *output_pubkey32, int output_pubkey_parity, const secp256k1_xonly_pubkey *internal_pubkey, const unsigned char *tweak32) {
    secp256k1_ge pk;
    unsigned char pk_expected32[32];

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(internal_pubkey != NULL);
    ARG_CHECK(output_pubkey32 != NULL);
    ARG_CHECK(tweak32 != NULL);

    if (!secp256k1_xonly_pubkey_load(ctx, &pk, internal_pubkey)
        || !secp256k1_ec_pubkey_tweak_add_helper(&ctx->ecmult_ctx, &pk, tweak32)) {
        return 0;
    }
    secp256k1_fe_normalize_var(&pk.x);
    secp256k1_fe_normalize_var(&pk.y);
    secp256k1_fe_get_b32(pk_expected32, &pk.x);

    return memcmp(&pk_expected32, output_pubkey32, 32) == 0
            && secp256k1_fe_is_odd(&pk.y) == output_pubkey_parity;
}

static void secp256k1_keypair_save(secp256k1_keypair *keypair, const secp256k1_scalar *sk, secp256k1_ge *pk) {
    secp256k1_scalar_get_b32(&keypair->data[0], sk);
    secp256k1_pubkey_save((secp256k1_pubkey *)&keypair->data[32], pk);
}

/* Always initializes seckey and ge with some values, even if this function
 * returns 0. */
static int secp256k1_keypair_load(const secp256k1_context* ctx, secp256k1_scalar *sk, secp256k1_ge *pk, const secp256k1_keypair *keypair) {
    int ret = 1;
    if (sk != NULL) {
        secp256k1_scalar_set_b32(sk, &keypair->data[0], NULL);
        ret &= !secp256k1_scalar_is_zero(sk);
        secp256k1_scalar_cmov(sk, &secp256k1_scalar_one, !ret);
    }
    if (pk != NULL) {
        const secp256k1_pubkey *pubkey = (const secp256k1_pubkey *)&keypair->data[32];
        /* Need to declassify the pubkey because pubkey_load ARG_CHECK's it. */
        secp256k1_declassify(ctx, pubkey, sizeof(*pubkey));
        if (!secp256k1_pubkey_load(ctx, pk, pubkey)) {
            *pk = secp256k1_ge_const_g;
            ret = 0;
        }
    }
    return ret;
}

int secp256k1_keypair_create(const secp256k1_context* ctx, secp256k1_keypair *keypair, const unsigned char *seckey32) {
    secp256k1_scalar sk;
    secp256k1_ge pk;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(keypair != NULL);
    memset(keypair, 0, sizeof(*keypair));
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(seckey32 != NULL);

    ret = secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &sk, &pk, seckey32);
    secp256k1_keypair_save(keypair, &sk, &pk);
    memczero(keypair, sizeof(*keypair), !ret);

    secp256k1_scalar_clear(&sk);
    return ret;
}

int secp256k1_keypair_pub(const secp256k1_context* ctx, secp256k1_pubkey *pubkey, const secp256k1_keypair *keypair) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(keypair != NULL);
    memcpy(pubkey->data, &keypair->data[32], sizeof(*pubkey));
    return 1;
}

int secp256k1_keypair_pub_xonly(const secp256k1_context* ctx, secp256k1_xonly_pubkey *pubkey, int *pubkey_parity, const secp256k1_keypair *keypair) {
    secp256k1_ge pk;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(keypair != NULL);

    if (!secp256k1_keypair_load(ctx, NULL, &pk, keypair)) {
        return 0;
    }
    secp256k1_ge_even_y(&pk, pubkey_parity);
    secp256k1_xonly_pubkey_save(pubkey, &pk);

    return 1;
}

int secp256k1_keypair_xonly_tweak_add(const secp256k1_context* ctx, secp256k1_keypair *keypair, const unsigned char *tweak32) {
    secp256k1_ge pk;
    secp256k1_scalar sk;
    int ret;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(keypair != NULL);
    ARG_CHECK(tweak32 != NULL);

    ret = secp256k1_keypair_load(ctx, &sk, &pk, keypair);
    memset(keypair, 0, sizeof(*keypair));

    if (secp256k1_fe_is_odd(&pk.y)) {
        secp256k1_scalar_negate(&sk, &sk);
        secp256k1_ge_neg(&pk, &pk);
    }

    ret &= secp256k1_ec_privkey_tweak_add_helper(&sk, tweak32);
    ret &= secp256k1_ec_pubkey_tweak_add_helper(&ctx->ecmult_ctx, &pk, tweak32);

    secp256k1_declassify(ctx, &ret, sizeof(ret));
    if (ret) {
        secp256k1_keypair_save(keypair, &sk, &pk);
    }

    secp256k1_scalar_clear(&sk);
    return ret;
}

#ifdef ENABLE_MODULE_ECDH
# include "modules/ecdh/main_impl.h"
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
# include "modules/schnorrsig/main_impl.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
# include "modules/recovery/main_impl.h"
#endif
