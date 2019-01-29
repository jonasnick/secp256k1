#ifndef SECP256K1_SCHNORRSIG_H
#define SECP256K1_SCHNORRSIG_H

/** This module implements a variant of Schnorr signatures compliant with
 * BIP-schnorr
 * (https://github.com/sipa/bips/blob/bip-schnorr/bip-schnorr.mediawiki).
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

/** Serialize a Schnorr signature.
 *
 *  Returns: 1
 *  Args:    ctx: a secp256k1 context object
 *  Out:   out64: pointer to a 64-byte array to store the serialized signature
 *  In:      sig: pointer to the signature
 *
 *  See secp256k1_schnorrsig_parse for details about the encoding.
 */
SECP256K1_API int secp256k1_schnorrsig_serialize(
    const secp256k1_context* ctx,
    unsigned char *out64,
    const secp256k1_schnorrsig* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse a Schnorr signature.
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args:    ctx: a secp256k1 context object
 *  Out:     sig: pointer to a signature object
 *  In:     in64: pointer to the 64-byte signature to be parsed
 *
 * The signature is serialized in the form R||s, where R is a 32-byte public
 * key (x-coordinate only; the y-coordinate is considered to be the unique
 * y-coordinate satisfying the curve equation that is a quadratic residue)
 * and s is a 32-byte big-endian scalar.
 *
 * After the call, sig will always be initialized. If parsing failed or the
 * encoded numbers are out of range, signature validation with it is
 * guaranteed to fail for every message and public key.
 */
SECP256K1_API int secp256k1_schnorrsig_parse(
    const secp256k1_context* ctx,
    secp256k1_schnorrsig* sig,
    const unsigned char *in64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Anti Nonce Sidechannel Protocol
 *
 *  The next functions can be used to prevent a signing device from exfiltrating the secret signing
 *  keys through biased signature nonces. The general idea is that a host provides additional
 *  randomness to the signing device client and the client commits to the randomness in the nonce
 *  using sign-to-contract.
 *  In order to make the randomness unpredictable, the host and client must engage in a
 *  commit-reveal protocol as follows:
 *  1. The host draws the randomness, commits to it with the anti_nonce_sidechan_host_commit
 *     function and sends the commitment to the client.
 *  2. The client commits to its sign-to-contract original nonce (which is the nonce without the
 *     sign-to-contract tweak) using the hosts commitment by calling the
 *     secp256k1_schnorrsig_anti_nonce_sidechan_client_commit function. The client gets the original
 *     nonce of the sign-to-contract commitment using secp256k1_s2c_commit_get_original_nonce and
 *     sends it to the host.
 *  3. The host replies with the randomness generated in step 1.
 *  4. The client uses anti_nonce_sidechan_client_setrand to check that the hosts commitment opens
 *     to the provided randomness. If not, it waits until the host sends the correct randomness or
 *     the protocol restarts. If the randomness matches the commitment, the client signs with the
 *     nonce_function_bipschnorr using the s2c context as nonce data and sends the signature and
 *     negated nonce flag to the host.
 *  5. The host checks that the signature contains an sign-to-contract commitment to the randomness
 *     by calling verify_s2c_commit with the original nonce received in step 2 and the signature and
 *     negated nonce flag received in step 4. If verification does not succeed, it waits until the
 *     client sends a signature with a correct commitment or the protocol is restarted.
 */

/** Create a randomness commitment on the host as part of the Anti Nonce Sidechannel Protocol.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:              ctx: pointer to a context object (cannot be NULL)
 *  Out: rand_commitment32: pointer to 32-byte array to store the returned commitment (cannot be NULL)
 *  In:             rand32: the 32-byte randomness to commit to (cannot be NULL)
 */
SECP256K1_API int secp256k1_schnorrsig_anti_nonce_sidechan_host_commit(
    secp256k1_context *ctx,
    unsigned char *rand_commitment32,
    const unsigned char *rand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Compute commitment on the client as part of the Anti Nonce Sidechannel Protocol.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:           ctx: pointer to a context object (cannot be NULL)
 *  Out:        s2c_ctx: pointer to an s2c context where the opening will be placed (cannot be NULL)
 *  In:           msg32: the 32-byte message hash to be signed (cannot be NULL)
 *             seckey32: the 32-byte secret key used for signing (cannot be NULL)
 *    rand_commitment32: the 32-byte randomness commitment from the host (cannot be NULL)
 */
SECP256K1_API int secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(
    secp256k1_context *ctx,
    secp256k1_s2c_commit_context *s2c_ctx,
    const unsigned char *msg32,
    const unsigned char *seckey32,
    const unsigned char *rand_commitment32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Set host randomness on the client as part of the Anti Nonce Sidechannel Protocol.
 *
 *  Returns:    1: given randomness matches randomness commitment stored in s2c_ctx
 *              0: failure
 *  Args:     ctx: pointer to a context object (cannot be NULL)
 *  Out:  s2c_ctx: pointer to an s2c context where the randomness will be stored (cannot be NULL)
 *  In:    rand32: 32-byte randomness matching the previously received commitment (cannot be NULL)
 */
SECP256K1_API int secp256k1_schnorrsig_anti_nonce_sidechan_client_setrand(
    secp256k1_context *ctx,
    secp256k1_s2c_commit_context *s2c_ctx,
    const unsigned char *rand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Create a Schnorr signature.
 *
 * Returns 1 on success, 0 on failure.
 *  Args:    ctx: pointer to a context object, initialized for signing (cannot be NULL)
 *  Out:     sig: pointer to the returned signature (cannot be NULL)
 *       nonce_is_negated: a pointer to an integer indicates if signing algorithm negated the
 *                nonce (can be NULL)
 *  In:    msg32: the 32-byte message hash being signed (cannot be NULL)
 *        seckey: pointer to a 32-byte secret key (cannot be NULL)
 *       noncefp: pointer to a nonce generation function. If NULL,
 *                secp256k1_nonce_function_bipschnorr is used
 *         ndata: pointer to arbitrary data used by the nonce generation function. If non-NULL must
 *                be a pointer to an s2c_context object when using the default nonce function
 *                secp256k1_nonce_function_bipschnorr. s2c_context must be initialized with
 *                secp256k1_s2c_commit_context_create. (can be NULL)
 */
SECP256K1_API int secp256k1_schnorrsig_sign(
    const secp256k1_context* ctx,
    secp256k1_schnorrsig *sig,
    int *nonce_is_negated,
    const unsigned char *msg32,
    const unsigned char *seckey,
    secp256k1_nonce_function noncefp,
    void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Verify a Schnorr signature.
 *
 *  Returns: 1: correct signature
 *           0: incorrect or unparseable signature
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *  In:      sig: the signature being verified (cannot be NULL)
 *         msg32: the 32-byte message hash being verified (cannot be NULL)
 *        pubkey: pointer to a public key to verify with (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_verify(
    const secp256k1_context* ctx,
    const secp256k1_schnorrsig *sig,
    const unsigned char *msg32,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verifies a set of Schnorr signatures.
 *
 * Returns 1 if all succeeded, 0 otherwise. In particular, returns 1 if n_sigs is 0.
 *
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *       scratch: scratch space used for the multiexponentiation
 *  In:      sig: array of signatures, or NULL if there are no signatures
 *         msg32: array of messages, or NULL if there are no signatures
 *            pk: array of public keys, or NULL if there are no signatures
 *        n_sigs: number of signatures in above arrays. Must be smaller than
 *                2^31 and smaller than half the maximum size_t value. Must be 0
 *                if above arrays are NULL.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_verify_batch(
    const secp256k1_context* ctx,
    secp256k1_scratch_space *scratch,
    const secp256k1_schnorrsig *const *sig,
    const unsigned char *const *msg32,
    const secp256k1_pubkey *const *pk,
    size_t n_sigs
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Verify a sign-to-contract commitment.
 *
 *  Returns: 1: the signature contains a commitment to data32
 *           0: incorrect opening
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *  In:      sig: the signature containing the sign-to-contract commitment (cannot be NULL)
 *        data32: the 32-byte data that was committed to (cannot be NULL)
 *       original_nonce: pointer to the original_nonce created when signing (cannot be NULL)
 * negated_nonce: integer indicating if signing algorithm negated the nonce
 */
SECP256K1_API int secp256k1_schnorrsig_verify_s2c_commit(
    const secp256k1_context* ctx,
    const secp256k1_schnorrsig *sig,
    const unsigned char *data32,
    const secp256k1_pubkey *original_nonce,
    int negated_nonce
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

#endif
