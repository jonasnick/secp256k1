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

/** Same as secp256k1_nonce function with the exception of accepting an
 *  additional pubkey argument. This can protect signature schemes with
 *  key-prefixed challenge hash inputs against reusing the nonce when signing
 *  with the wrong precomputed pubkey.
 *
 *  In:  xonly_pk32: the 32-byte serialized xonly pubkey corresponding to key32 (will not be NULL)
 */
typedef int (*secp256k1_nonce_function_hardened)(
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
 *  "BIP340/nonce" (and the midstate is precomputed). This is used to create BIP
 *  340 compliant signatures.
 */
SECP256K1_API extern const secp256k1_nonce_function_hardened secp256k1_nonce_function_bip340;

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_H */
