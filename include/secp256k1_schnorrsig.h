#ifndef SECP256K1_SCHNORRSIG_H
#define SECP256K1_SCHNORRSIG_H

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This module implements a variant of Schnorr signatures compliant with
 *  Bitcoin Improvement Proposal 340 "Schnorr Signatures for secp256k1"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 */

/** Opaque data structure that holds a parsed Schnorr signature.
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage, transmission, or
 *  comparison, use the `secp256k1_schnorrsig_serialize` and
 *  `secp256k1_schnorrsig_parse` functions.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_schnorrsig;

/** Same as secp256k1_nonce function with the exception of accepting an
 *  additional pubkey argument. This can protect signature schemes with
 *  key-prefixed challenge hash inputs against reusing the nonce when signing
 *  with the wrong precomputed pubkey.
 *
 *  In:  xonly_pk32: the 32-byte serialized xonly pubkey corresponding to key32 (will not be NULL)
 */
typedef int (*secp256k1_nonce_function_extended)(
    unsigned char *nonce32,
    const unsigned char *msg32,
    const unsigned char *key32,
    const unsigned char *xonly_pk32,
    const unsigned char *algo16,
    void *data,
    unsigned int attempt
);

/** An implementation of the nonce generation function as defined in Bitcoin
 *  Improvement Proposal 340 "Schnorr Signatures for secp256k1"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 *
 *  If a data pointer is passed, it is assumed to be a pointer to 32 bytes of
 *  auxiliary random data as defined in BIP-340. If the data pointer is NULL,
 *  schnorrsig_sign does not produce BIP-340 compliant signatures. The algo16
 *  argument must be non-NULL and the attempt argument must be 0, otherwise the
 *  function will fail and return 0. The hash will be tagged with the algo16
 * 
 *  "BIP340/nonce" (and the midstate is precomputed). This is used to create BIP
 *  340 compliant signatures.
 */
SECP256K1_API extern const secp256k1_nonce_function_extended secp256k1_nonce_function_bip340;

/** Create a Schnorr signature.
 *
 *  Does _not_ strictly follow BIP-340 because it does not verify the resulting
 *  signature. Instead, you can manually use secp256k1_schnorrsig_verify and
 *  abort if it fails.
 *
 *  Otherwise BIP-340 compliant if the noncefp argument is NULL or
 *  secp256k1_nonce_function_bip340 and the ndata argument is 32-byte auxiliary
 *  randomness.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:    ctx: pointer to a context object, initialized for signing (cannot be NULL)
 *  Out:   sig64: pointer to a 64-byte array to store the serialized signature (cannot be NULL)
 *  In:    msg32: the 32-byte message being signed (cannot be NULL)
 *       keypair: pointer to an initialized keypair (cannot be NULL)
 *       noncefp: pointer to a nonce generation function. If NULL, secp256k1_nonce_function_bip340 is used
 *         ndata: pointer to arbitrary data used by the nonce generation
 *                function (can be NULL). If it is non-NULL and
 *                secp256k1_nonce_function_bip340 is used, then ndata must be a
 *                pointer to 32-byte auxiliary randomness as per BIP-340.
 */
SECP256K1_API int secp256k1_schnorrsig_sign(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_keypair *keypair,
    secp256k1_nonce_function_extended noncefp,
    void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verify a Schnorr signature.
 *
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *  In:    sig64: pointer to the 64-byte signatur to verify (cannot be NULL)
 *         msg32: the 32-byte message being verified (cannot be NULL)
 *        pubkey: pointer to an x-only public key to verify with (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_verify(
    const secp256k1_context* ctx,
    const unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_xonly_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verifies a set of Schnorr signatures.
 *
 * Returns 1 if all succeeded, 0 otherwise. In particular, returns 1 if n_sigs is 0.
 *
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *       scratch: scratch space used for the multiexponentiation
 *  In:      sig: array of pointers to signatures, or NULL if there are no signatures
 *         msg32: array of pointers to messages, or NULL if there are no signatures
 *            pk: array of pointers to x-only public keys, or NULL if there are no signatures
 *        n_sigs: number of signatures in above arrays. Must be below the
 *                minimum of 2^31 and SIZE_MAX/2. Must be 0 if above arrays are NULL.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_verify_batch(
    const secp256k1_context* ctx,
    secp256k1_scratch_space *scratch,
    const unsigned char *const *sig,
    const unsigned char *const *msg32,
    const secp256k1_xonly_pubkey *const *pk,
    size_t n_sigs
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_H */
