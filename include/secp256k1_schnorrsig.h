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

/** A pointer to a function to deterministically generate a nonce.
 *
 *  Same as secp256k1_nonce function with the exception of accepting an
 *  additional pubkey argument and not requiring an attempt argument. The pubkey
 *  argument can protect signature schemes with key-prefixed challenge hash
 *  inputs against reusing the nonce when signing with the wrong precomputed
 *  pubkey.
 *
 *  Returns: 1 if a nonce was successfully generated. 0 will cause signing to
 *           return an error.
 *  Out:     nonce32:   pointer to a 32-byte array to be filled by the function.
 *  In:        msg:     the message being verified (will not be NULL)
 *         msg_len:     the length of the message (will not be NULL)
 *           key32:     pointer to a 32-byte secret key (will not be NULL)
 *      xonly_pk32:     the 32-byte serialized xonly pubkey corresponding to key32
 *                      (will not be NULL)
 *           algo16:    pointer to a 16-byte array describing the signature
 *                      algorithm (will not be NULL).
 *           data:      Arbitrary data pointer that is passed through.
 *
 *  Except for test cases, this function should compute some cryptographic hash of
 *  the message, the key, the pubkey, the algorithm description, and data.
 */
typedef int (*secp256k1_nonce_function_hardened)(
    unsigned char *nonce32,
    const unsigned char *msg,
    size_t msg_len,
    const unsigned char *key32,
    const unsigned char *xonly_pk32,
    const unsigned char *algo16,
    void *data
);

/** An implementation of the nonce generation function as defined in Bitcoin
 *  Improvement Proposal 340 "Schnorr Signatures for secp256k1"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 *
 *  If a data pointer is passed, it is assumed to be a pointer to 32 bytes of
 *  auxiliary random data as defined in BIP-340. If the data pointer is NULL,
 *  schnorrsig_sign does not follow the BIP-340 nonce derivation procedure
 *  exactly. The algo16 argument must be non-NULL, otherwise the function will
 *  fail and return 0. The hash will be tagged with algo16 after removing all
 *  terminating null bytes. Therefore, to create BIP-340 compliant signatures,
 *  algo16 must be set to "BIP0340/nonce\0\0\0"
 */
SECP256K1_API extern const secp256k1_nonce_function_hardened secp256k1_nonce_function_bip340;

/** Data structure that holds additional arguments for schnorrsig signing.
 */
typedef struct {
    unsigned char magic[8];
    secp256k1_nonce_function_hardened noncefp;
    void* ndata;
} secp256k1_schnorrsig_config;

#define SECP256K1_SCHNORRSIG_CONFIG_INIT { "versio1", 0, 0 };

/** Create a Schnorr signature.
 *
 *  Does _not_ strictly follow BIP-340 because it does not verify the resulting
 *  signature. Instead, you can manually use secp256k1_schnorrsig_verify and
 *  abort if it fails.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:    ctx: pointer to a context object, initialized for signing (cannot be NULL)
 *  Out:   sig64: pointer to a 64-byte array to store the serialized signature (cannot be NULL)
 *  In:      msg: the message being signed (cannot be NULL)
 *       msg_len: length of the message
 *       keypair: pointer to an initialized keypair (cannot be NULL)
 *    aux_rand32: 32 bytes of fresh randomness. While recommended to provide
 *                this, it is only supplemental to security and can be NULL. See
 *                BIP-340 for a full explanation of this argument and for
 *                guidance if randomness is expensive.
 */
SECP256K1_API int secp256k1_schnorrsig_sign(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg,
    size_t msg_len,
    const secp256k1_keypair *keypair,
    unsigned char *aux_rand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

/** Create a Schnorr signature with a more flexible API.
 *
 *  Same arguments as secp256k1_schnorrsig_sign except that it accepts a pointer
 *  to a config object that allows customizing signing by passing additional
 *  arguments.
 *
 *  In: noncefp: pointer to a nonce generation function. If NULL,
 *               secp256k1_nonce_function_bip340 is used
 *        ndata: pointer to arbitrary data used by the nonce generation function
 *               (can be NULL). If it is non-NULL and
 *               secp256k1_nonce_function_bip340 is used, then ndata must be a
 *               pointer to 32-byte auxiliary randomness as per BIP-340.
 *       config: pointer to a config object.
 */
SECP256K1_API int secp256k1_schnorrsig_sign_custom(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg,
    size_t msg_len,
    const secp256k1_keypair *keypair,
    secp256k1_schnorrsig_config *config
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);


/** Verify a Schnorr signature.
 *
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *  In:    sig64: pointer to the 64-byte signature to verify (cannot be NULL)
 *           msg: the message being verified (cannot be NULL)
 *       msg_len: length of the message (cannot be NULL)
 *        pubkey: pointer to an x-only public key to verify with (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_verify(
    const secp256k1_context* ctx,
    const unsigned char *sig64,
    const unsigned char *msg,
    size_t msg_len,
    const secp256k1_xonly_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_H */
