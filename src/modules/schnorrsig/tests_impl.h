/**********************************************************************
 * Copyright (c) 2018-2020 Andrew Poelstra, Jonas Nick                *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_MODULE_SCHNORRSIG_TESTS_
#define _SECP256K1_MODULE_SCHNORRSIG_TESTS_


#include "secp256k1_schnorrsig.h"

/* Checks that a bit flip in the n_flip-th argument (that has n_bytes many
 * bytes) changes the hash function
 */
void nonce_function_bip340_bitflip(unsigned char **args, size_t n_flip, size_t n_bytes) {
    unsigned char nonces[2][32];
    /* TODO: use random msg_len <= 32 */
    CHECK(nonce_function_bip340(nonces[0], args[0], 32, args[1], args[2], args[3], args[4]) == 1);
    secp256k1_rand_flip(args[n_flip], n_bytes);
    CHECK(nonce_function_bip340(nonces[1], args[0], 32, args[1], args[2], args[3], args[4]) == 1);
    CHECK(memcmp(nonces[0], nonces[1], 32) != 0);
}

void run_nonce_function_bip340_tests(void) {
    unsigned char tag[12] = "BIP340/nonce";
    unsigned char aux_tag[10] = "BIP340/aux";
    unsigned char algo16[16] = "BIP340/nonce\0\0\0\0";
    secp256k1_sha256 sha;
    secp256k1_sha256 sha_optimized;
    unsigned char nonce[32];
    unsigned char msg[32];
    unsigned char key[32];
    unsigned char pk[32];
    unsigned char aux_rand[32];
    unsigned char *args[5];
    int i;

    /* Check that hash initialized by
     * secp256k1_nonce_function_bip340_sha256_tagged has the expected
     * state. */
    secp256k1_sha256_initialize_tagged(&sha, tag, sizeof(tag));
    secp256k1_nonce_function_bip340_sha256_tagged(&sha_optimized);
    test_sha256_eq(&sha, &sha_optimized);

   /* Check that hash initialized by
    * secp256k1_nonce_function_bip340_sha256_tagged_aux has the expected
    * state. */
    secp256k1_sha256_initialize_tagged(&sha, aux_tag, sizeof(aux_tag));
    secp256k1_nonce_function_bip340_sha256_tagged_aux(&sha_optimized);
    test_sha256_eq(&sha, &sha_optimized);

    secp256k1_rand256(msg);
    secp256k1_rand256(key);
    secp256k1_rand256(pk);
    secp256k1_rand256(aux_rand);

    /* Check that a bitflip in an argument results in different nonces. */
    args[0] = msg;
    args[1] = key;
    args[2] = pk;
    args[3] = algo16;
    args[4] = aux_rand;
    for (i = 0; i < count; i++) {
        nonce_function_bip340_bitflip(args, 0, 32);
        nonce_function_bip340_bitflip(args, 1, 32);
        nonce_function_bip340_bitflip(args, 2, 32);
        /* Flip algo16 special case "BIP340/nonce" */
        nonce_function_bip340_bitflip(args, 3, 16);
        /* Flip algo16 again */
        nonce_function_bip340_bitflip(args, 3, 16);
        nonce_function_bip340_bitflip(args, 4, 32);
    }

    /* NULL algo16 is disallowed */
    CHECK(nonce_function_bip340(nonce, msg, sizeof(msg), key, pk, NULL, NULL) == 0);
    /* Empty algo16 is fine */
    memset(algo16, 0x00, 16);
    CHECK(nonce_function_bip340(nonce, msg, sizeof(msg), key, pk, algo16, NULL) == 1);
    /* algo16 with terminating null bytes is fine */
    algo16[1] = 65;
    CHECK(nonce_function_bip340(nonce, msg, sizeof(msg), key, pk, algo16, NULL) == 1);
    /* Other algo16 is fine */
    memset(algo16, 0xFF, 16);
    CHECK(nonce_function_bip340(nonce, msg, sizeof(msg), key, pk, algo16, NULL) == 1);

    /* NULL aux_rand argument is allowed. */
    CHECK(nonce_function_bip340(nonce, msg, sizeof(msg), key, pk, algo16, NULL) == 1);
}

void test_schnorrsig_api(void) {
    unsigned char sk1[32];
    unsigned char sk2[32];
    unsigned char sk3[32];
    unsigned char msg[32];
    secp256k1_keypair keypairs[3];
    secp256k1_xonly_pubkey pk[3];
    secp256k1_xonly_pubkey zero_pk;
    unsigned char sig[64];

    /** setup **/
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_context *vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_context *both = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    int ecount;

    secp256k1_context_set_error_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(both, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(both, counting_illegal_callback_fn, &ecount);

    secp256k1_rand256(sk1);
    secp256k1_rand256(sk2);
    secp256k1_rand256(sk3);
    secp256k1_rand256(msg);
    CHECK(secp256k1_keypair_create(ctx, &keypairs[0], sk1) == 1);
    CHECK(secp256k1_keypair_create(ctx, &keypairs[1], sk2) == 1);
    CHECK(secp256k1_keypair_create(ctx, &keypairs[2], sk3) == 1);
    CHECK(secp256k1_keypair_xonly_pub(ctx, &pk[0], NULL, &keypairs[0]) == 1);
    CHECK(secp256k1_keypair_xonly_pub(ctx, &pk[1], NULL, &keypairs[1]) == 1);
    CHECK(secp256k1_keypair_xonly_pub(ctx, &pk[2], NULL, &keypairs[2]) == 1);
    memset(&zero_pk, 0, sizeof(zero_pk));

    /** main test body **/
    ecount = 0;
    CHECK(secp256k1_schnorrsig_sign(none, sig, msg, sizeof(msg), &keypairs[0], NULL, NULL) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_sign(vrfy, sig, msg, sizeof(msg), &keypairs[0], NULL, NULL) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_sign(sign, sig, msg, sizeof(msg), &keypairs[0], NULL, NULL) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_sign(sign, NULL, msg, sizeof(msg), &keypairs[0], NULL, NULL) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_sign(sign, sig, NULL, sizeof(msg), &keypairs[0], NULL, NULL) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_sign(sign, sig, msg, sizeof(msg), NULL, NULL, NULL) == 0);
    CHECK(ecount == 5);

    ecount = 0;
    CHECK(secp256k1_schnorrsig_verify(none, sig, msg, sizeof(msg), &pk[0]) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_verify(sign, sig, msg, sizeof(msg), &pk[0]) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify(vrfy, sig, msg, sizeof(msg), &pk[0]) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify(vrfy, NULL, msg, sizeof(msg), &pk[0]) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_verify(vrfy, sig, NULL, sizeof(msg), &pk[0]) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_verify(vrfy, sig, msg, sizeof(msg), NULL) == 0);
    CHECK(ecount == 5);
    CHECK(secp256k1_schnorrsig_verify(vrfy, sig, msg, sizeof(msg), &zero_pk) == 0);
    CHECK(ecount == 6);

    secp256k1_context_destroy(none);
    secp256k1_context_destroy(sign);
    secp256k1_context_destroy(vrfy);
    secp256k1_context_destroy(both);
}

/* Checks that hash initialized by secp256k1_schnorrsig_sha256_tagged has the
 * expected state. */
void test_schnorrsig_sha256_tagged(void) {
    char tag[16] = "BIP340/challenge";
    secp256k1_sha256 sha;
    secp256k1_sha256 sha_optimized;

    secp256k1_sha256_initialize_tagged(&sha, (unsigned char *) tag, sizeof(tag));
    secp256k1_schnorrsig_sha256_tagged(&sha_optimized);
    test_sha256_eq(&sha, &sha_optimized);
}

/* Helper function for schnorrsig_bip_vectors
 * Signs the message and checks that it's the same as expected_sig. */
void test_schnorrsig_bip_vectors_check_signing(const unsigned char *sk, const unsigned char *pk_serialized, unsigned char *aux_rand, const unsigned char *msg, const unsigned char *expected_sig) {
    unsigned char sig[64];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pk, pk_expected;

    CHECK(secp256k1_keypair_create(ctx, &keypair, sk));
    CHECK(secp256k1_schnorrsig_sign(ctx, sig, msg, 32, &keypair, NULL, aux_rand));
    CHECK(memcmp(sig, expected_sig, 64) == 0);

    CHECK(secp256k1_xonly_pubkey_parse(ctx, &pk_expected, pk_serialized));
    CHECK(secp256k1_keypair_xonly_pub(ctx, &pk, NULL, &keypair));
    CHECK(memcmp(&pk, &pk_expected, sizeof(pk)) == 0);
    CHECK(secp256k1_schnorrsig_verify(ctx, sig, msg, 32, &pk));
}

/* Helper function for schnorrsig_bip_vectors
 * Checks that both verify and verify_batch (TODO) return the same value as expected. */
void test_schnorrsig_bip_vectors_check_verify(const unsigned char *pk_serialized, const unsigned char *msg32, const unsigned char *sig, int expected) {
    secp256k1_xonly_pubkey pk;

    CHECK(secp256k1_xonly_pubkey_parse(ctx, &pk, pk_serialized));
    CHECK(expected == secp256k1_schnorrsig_verify(ctx, sig, msg32, 32, &pk));
}

/* Test vectors according to BIP-340 ("Schnorr Signatures for secp256k1"). See
 * https://github.com/bitcoin/bips/blob/master/bip-0340/test-vectors.csv. */
void test_schnorrsig_bip_vectors(void) {
    {
        /* Test vector 0 */
        const unsigned char sk[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03
        };
        const unsigned char pk[32] = {
            0xF9, 0x30, 0x8A, 0x01, 0x92, 0x58, 0xC3, 0x10,
            0x49, 0x34, 0x4F, 0x85, 0xF8, 0x9D, 0x52, 0x29,
            0xB5, 0x31, 0xC8, 0x45, 0x83, 0x6F, 0x99, 0xB0,
            0x86, 0x01, 0xF1, 0x13, 0xBC, 0xE0, 0x36, 0xF9
        };
        unsigned char aux_rand[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        const unsigned char msg[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        const unsigned char sig[64] = {
            0x06, 0x7E, 0x33, 0x7A, 0xD5, 0x51, 0xB2, 0x27,
            0x6E, 0xC7, 0x05, 0xE4, 0x3F, 0x09, 0x20, 0x92,
            0x6A, 0x9C, 0xE0, 0x8A, 0xC6, 0x81, 0x59, 0xF9,
            0xD2, 0x58, 0xC9, 0xBB, 0xA4, 0x12, 0x78, 0x1C,
            0x9F, 0x05, 0x9F, 0xCD, 0xF4, 0x82, 0x4F, 0x13,
            0xB3, 0xD7, 0xC1, 0x30, 0x53, 0x16, 0xF9, 0x56,
            0x70, 0x4B, 0xB3, 0xFE, 0xA2, 0xC2, 0x61, 0x42,
            0xE1, 0x8A, 0xCD, 0x90, 0xA9, 0x0C, 0x94, 0x7E
        };
        test_schnorrsig_bip_vectors_check_signing(sk, pk, aux_rand, msg, sig);
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 1);
    }
    {
        /* Test vector 1 */
        const unsigned char sk[32] = {
            0xB7, 0xE1, 0x51, 0x62, 0x8A, 0xED, 0x2A, 0x6A,
            0xBF, 0x71, 0x58, 0x80, 0x9C, 0xF4, 0xF3, 0xC7,
            0x62, 0xE7, 0x16, 0x0F, 0x38, 0xB4, 0xDA, 0x56,
            0xA7, 0x84, 0xD9, 0x04, 0x51, 0x90, 0xCF, 0xEF
        };
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        unsigned char aux_rand[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0x0E, 0x12, 0xB8, 0xC5, 0x20, 0x94, 0x8A, 0x77,
            0x67, 0x53, 0xA9, 0x6F, 0x21, 0xAB, 0xD7, 0xFD,
            0xC2, 0xD7, 0xD0, 0xC0, 0xDD, 0xC9, 0x08, 0x51,
            0xBE, 0x17, 0xB0, 0x4E, 0x75, 0xEF, 0x86, 0xA4,
            0x7E, 0xF0, 0xDA, 0x46, 0xC4, 0xDC, 0x4D, 0x0D,
            0x1B, 0xCB, 0x86, 0x68, 0xC2, 0xCE, 0x16, 0xC5,
            0x4C, 0x7C, 0x23, 0xA6, 0x71, 0x6E, 0xDE, 0x30,
            0x3A, 0xF8, 0x67, 0x74, 0x91, 0x7C, 0xF9, 0x28
        };
        test_schnorrsig_bip_vectors_check_signing(sk, pk, aux_rand, msg, sig);
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 1);
    }
    {
        /* Test vector 2 */
        const unsigned char sk[32] = {
            0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
            0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
            0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
            0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x14, 0xE5, 0xC9
        };
        const unsigned char pk[32] = {
            0xDD, 0x30, 0x8A, 0xFE, 0xC5, 0x77, 0x7E, 0x13,
            0x12, 0x1F, 0xA7, 0x2B, 0x9C, 0xC1, 0xB7, 0xCC,
            0x01, 0x39, 0x71, 0x53, 0x09, 0xB0, 0x86, 0xC9,
            0x60, 0xE1, 0x8F, 0xD9, 0x69, 0x77, 0x4E, 0xB8
        };
        unsigned char aux_rand[32] = {
            0xC8, 0x7A, 0xA5, 0x38, 0x24, 0xB4, 0xD7, 0xAE,
            0x2E, 0xB0, 0x35, 0xA2, 0xB5, 0xBB, 0xBC, 0xCC,
            0x08, 0x0E, 0x76, 0xCD, 0xC6, 0xD1, 0x69, 0x2C,
            0x4B, 0x0B, 0x62, 0xD7, 0x98, 0xE6, 0xD9, 0x06
        };
        const unsigned char msg[32] = {
            0x7E, 0x2D, 0x58, 0xD8, 0xB3, 0xBC, 0xDF, 0x1A,
            0xBA, 0xDE, 0xC7, 0x82, 0x90, 0x54, 0xF9, 0x0D,
            0xDA, 0x98, 0x05, 0xAA, 0xB5, 0x6C, 0x77, 0x33,
            0x30, 0x24, 0xB9, 0xD0, 0xA5, 0x08, 0xB7, 0x5C
        };
        const unsigned char sig[64] = {
            0xFC, 0x01, 0x2F, 0x9F, 0xB8, 0xFE, 0x00, 0xA3,
            0x58, 0xF5, 0x1E, 0xF9, 0x3D, 0xCE, 0x0D, 0xC0,
            0xC8, 0x95, 0xF6, 0xE9, 0xA8, 0x7C, 0x6C, 0x49,
            0x05, 0xBC, 0x82, 0x0B, 0x0C, 0x36, 0x77, 0x61,
            0x6B, 0x87, 0x37, 0xD1, 0x4E, 0x70, 0x3A, 0xF8,
            0xE1, 0x6E, 0x22, 0xE5, 0xB8, 0xF2, 0x62, 0x27,
            0xD4, 0x1E, 0x51, 0x28, 0xF8, 0x2D, 0x86, 0xF7,
            0x47, 0x24, 0x4C, 0xC2, 0x89, 0xC7, 0x4D, 0x1D
        };
        test_schnorrsig_bip_vectors_check_signing(sk, pk, aux_rand, msg, sig);
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 1);
    }
    {
        /* Test vector 3 */
        const unsigned char sk[32] = {
            0x0B, 0x43, 0x2B, 0x26, 0x77, 0x93, 0x73, 0x81,
            0xAE, 0xF0, 0x5B, 0xB0, 0x2A, 0x66, 0xEC, 0xD0,
            0x12, 0x77, 0x30, 0x62, 0xCF, 0x3F, 0xA2, 0x54,
            0x9E, 0x44, 0xF5, 0x8E, 0xD2, 0x40, 0x17, 0x10
        };
        const unsigned char pk[32] = {
            0x25, 0xD1, 0xDF, 0xF9, 0x51, 0x05, 0xF5, 0x25,
            0x3C, 0x40, 0x22, 0xF6, 0x28, 0xA9, 0x96, 0xAD,
            0x3A, 0x0D, 0x95, 0xFB, 0xF2, 0x1D, 0x46, 0x8A,
            0x1B, 0x33, 0xF8, 0xC1, 0x60, 0xD8, 0xF5, 0x17
        };
        unsigned char aux_rand[32] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };
        const unsigned char msg[32] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };
        const unsigned char sig[64] = {
            0xFC, 0x13, 0x2D, 0x4E, 0x42, 0x6D, 0xFF, 0x53,
            0x5A, 0xEC, 0x0F, 0xA7, 0x08, 0x3A, 0xC5, 0x11,
            0x8B, 0xC1, 0xD5, 0xFF, 0xFD, 0x84, 0x8A, 0xBD,
            0x82, 0x90, 0xC2, 0x3F, 0x27, 0x1C, 0xA0, 0xDD,
            0x11, 0xAE, 0xDC, 0xEA, 0x3F, 0x55, 0xDA, 0x9B,
            0xD6, 0x77, 0xFE, 0x29, 0xC9, 0xDD, 0xA0, 0xCF,
            0x87, 0x8B, 0xCE, 0x43, 0xFD, 0xE0, 0xE3, 0x13,
            0xD6, 0x9D, 0x1A, 0xF7, 0xA5, 0xAE, 0x83, 0x69
        };
        test_schnorrsig_bip_vectors_check_signing(sk, pk, aux_rand, msg, sig);
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 1);
    }
    {
        /* Test vector 4 */
        const unsigned char pk[32] = {
            0xD6, 0x9C, 0x35, 0x09, 0xBB, 0x99, 0xE4, 0x12,
            0xE6, 0x8B, 0x0F, 0xE8, 0x54, 0x4E, 0x72, 0x83,
            0x7D, 0xFA, 0x30, 0x74, 0x6D, 0x8B, 0xE2, 0xAA,
            0x65, 0x97, 0x5F, 0x29, 0xD2, 0x2D, 0xC7, 0xB9
        };
        const unsigned char msg[32] = {
            0x4D, 0xF3, 0xC3, 0xF6, 0x8F, 0xCC, 0x83, 0xB2,
            0x7E, 0x9D, 0x42, 0xC9, 0x04, 0x31, 0xA7, 0x24,
            0x99, 0xF1, 0x78, 0x75, 0xC8, 0x1A, 0x59, 0x9B,
            0x56, 0x6C, 0x98, 0x89, 0xB9, 0x69, 0x67, 0x03
        };
        const unsigned char sig[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x3B, 0x78, 0xCE, 0x56, 0x3F,
            0x89, 0xA0, 0xED, 0x94, 0x14, 0xF5, 0xAA, 0x28,
            0xAD, 0x0D, 0x96, 0xD6, 0x79, 0x5F, 0x9C, 0x63,
            0x0E, 0xC5, 0x0E, 0x53, 0x63, 0xE2, 0x27, 0xAC,
            0xAC, 0x6F, 0x54, 0x2C, 0xE1, 0xC0, 0xB1, 0x86,
            0x65, 0x7E, 0x0E, 0x0D, 0x1A, 0x6F, 0xFE, 0x28,
            0x3A, 0x33, 0x43, 0x8D, 0xE4, 0x73, 0x84, 0x19
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 1);
    }
    {
        /* Test vector 5 */
        const unsigned char pk[32] = {
            0xEE, 0xFD, 0xEA, 0x4C, 0xDB, 0x67, 0x77, 0x50,
            0xA4, 0x20, 0xFE, 0xE8, 0x07, 0xEA, 0xCF, 0x21,
            0xEB, 0x98, 0x98, 0xAE, 0x79, 0xB9, 0x76, 0x87,
            0x66, 0xE4, 0xFA, 0xA0, 0x4A, 0x2D, 0x4A, 0x34
        };
        secp256k1_xonly_pubkey pk_parsed;
        /* No need to check the signature of the test vector as parsing the pubkey already fails */
        CHECK(!secp256k1_xonly_pubkey_parse(ctx, &pk_parsed, pk));
    }
    {
        /* Test vector 6 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0xF9, 0x30, 0x8A, 0x01, 0x92, 0x58, 0xC3, 0x10,
            0x49, 0x34, 0x4F, 0x85, 0xF8, 0x9D, 0x52, 0x29,
            0xB5, 0x31, 0xC8, 0x45, 0x83, 0x6F, 0x99, 0xB0,
            0x86, 0x01, 0xF1, 0x13, 0xBC, 0xE0, 0x36, 0xF9,
            0x95, 0xA5, 0x79, 0xDA, 0x95, 0x9F, 0xA7, 0x39,
            0xFC, 0xE3, 0x9E, 0x8B, 0xD1, 0x6F, 0xEC, 0xB5,
            0xCD, 0xCF, 0x97, 0x06, 0x0B, 0x2C, 0x73, 0xCD,
            0xE6, 0x0E, 0x87, 0xAB, 0xCA, 0x1A, 0xA5, 0xD9
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 7 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0xF8, 0x70, 0x46, 0x54, 0xF4, 0x68, 0x7B, 0x73,
            0x65, 0xED, 0x32, 0xE7, 0x96, 0xDE, 0x92, 0x76,
            0x13, 0x90, 0xA3, 0xBC, 0xC4, 0x95, 0x17, 0x9B,
            0xFE, 0x07, 0x38, 0x17, 0xB7, 0xED, 0x32, 0x82,
            0x4E, 0x76, 0xB9, 0x87, 0xF7, 0xC1, 0xF9, 0xA7,
            0x51, 0xEF, 0x5C, 0x34, 0x3F, 0x76, 0x45, 0xD3,
            0xCF, 0xFC, 0x7D, 0x57, 0x0B, 0x9A, 0x71, 0x92,
            0xEB, 0xF1, 0x89, 0x8E, 0x13, 0x44, 0xE3, 0xBF
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 8 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0x70, 0x36, 0xD6, 0xBF, 0xE1, 0x83, 0x7A, 0xE9,
            0x19, 0x63, 0x10, 0x39, 0xA2, 0xCF, 0x65, 0x2A,
            0x29, 0x5D, 0xFA, 0xC9, 0xA8, 0xBB, 0xB0, 0x80,
            0x60, 0x14, 0xB2, 0xF4, 0x8D, 0xD7, 0xC8, 0x07,
            0x6B, 0xE9, 0xF8, 0x4A, 0x9C, 0x54, 0x45, 0xBE,
            0xBD, 0x78, 0x0C, 0x8B, 0x5C, 0xCD, 0x45, 0xC8,
            0x83, 0xD0, 0xDC, 0x47, 0xCD, 0x59, 0x4B, 0x21,
            0xA8, 0x58, 0xF3, 0x1A, 0x19, 0xAA, 0xB7, 0x1D
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 9 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x99, 0x15, 0xEE, 0x59, 0xF0, 0x7F, 0x9D, 0xBB,
            0xAE, 0xDC, 0x31, 0xBF, 0xCC, 0x9B, 0x34, 0xAD,
            0x49, 0xDE, 0x66, 0x9C, 0xD2, 0x47, 0x73, 0xBC,
            0xED, 0x77, 0xDD, 0xA3, 0x6D, 0x07, 0x3E, 0xC8
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 10 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0xC7, 0xEC, 0x91, 0x8B, 0x2B, 0x9C, 0xF3, 0x40,
            0x71, 0xBB, 0x54, 0xBE, 0xD7, 0xEB, 0x4B, 0xB6,
            0xBA, 0xB1, 0x48, 0xE9, 0xA7, 0xE3, 0x6E, 0x6B,
            0x22, 0x8F, 0x95, 0xDF, 0xA0, 0x8B, 0x43, 0xEC
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 11 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0x4A, 0x29, 0x8D, 0xAC, 0xAE, 0x57, 0x39, 0x5A,
            0x15, 0xD0, 0x79, 0x5D, 0xDB, 0xFD, 0x1D, 0xCB,
            0x56, 0x4D, 0xA8, 0x2B, 0x0F, 0x26, 0x9B, 0xC7,
            0x0A, 0x74, 0xF8, 0x22, 0x04, 0x29, 0xBA, 0x1D,
            0x94, 0x16, 0x07, 0xB5, 0x63, 0xAB, 0xBA, 0x41,
            0x42, 0x87, 0xF3, 0x74, 0xA3, 0x32, 0xBA, 0x36,
            0x36, 0xDE, 0x00, 0x9E, 0xE1, 0xEF, 0x55, 0x1A,
            0x17, 0x79, 0x6B, 0x72, 0xB6, 0x8B, 0x8A, 0x24
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 12 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F,
            0x94, 0x16, 0x07, 0xB5, 0x63, 0xAB, 0xBA, 0x41,
            0x42, 0x87, 0xF3, 0x74, 0xA3, 0x32, 0xBA, 0x36,
            0x36, 0xDE, 0x00, 0x9E, 0xE1, 0xEF, 0x55, 0x1A,
            0x17, 0x79, 0x6B, 0x72, 0xB6, 0x8B, 0x8A, 0x24
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 13 */
        const unsigned char pk[32] = {
            0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C, 0x5F,
            0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41, 0xBE,
            0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE, 0xD8,
            0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6, 0x59
        };
        const unsigned char msg[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig[64] = {
            0x70, 0x36, 0xD6, 0xBF, 0xE1, 0x83, 0x7A, 0xE9,
            0x19, 0x63, 0x10, 0x39, 0xA2, 0xCF, 0x65, 0x2A,
            0x29, 0x5D, 0xFA, 0xC9, 0xA8, 0xBB, 0xB0, 0x80,
            0x60, 0x14, 0xB2, 0xF4, 0x8D, 0xD7, 0xC8, 0x07,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
            0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
            0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
        };
        test_schnorrsig_bip_vectors_check_verify(pk, msg, sig, 0);
    }
    {
        /* Test vector 14 */
        const unsigned char pk[32] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
        };
        secp256k1_xonly_pubkey pk_parsed;
        /* No need to check the signature of the test vector as parsing the pubkey already fails */
        CHECK(!secp256k1_xonly_pubkey_parse(ctx, &pk_parsed, pk));
    }
}

/* Nonce function that returns constant 0 */
static int nonce_function_failing(unsigned char *nonce32, const unsigned char *msg, size_t msg_len, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo16, void *data) {
    (void) msg;
    (void) msg_len;
    (void) key32;
    (void) xonly_pk32;
    (void) algo16;
    (void) data;
    (void) nonce32;
    return 0;
}

/* Nonce function that sets nonce to 0 */
static int nonce_function_0(unsigned char *nonce32, const unsigned char *msg, size_t msg_len, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo16, void *data) {
    (void) msg;
    (void) msg_len;
    (void) key32;
    (void) xonly_pk32;
    (void) algo16;
    (void) data;

    memset(nonce32, 0, 32);
    return 1;
}

/* Nonce function that sets nonce to 0xFF...0xFF */
static int nonce_function_overflowing(unsigned char *nonce32, const unsigned char *msg, size_t msg_len, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo16, void *data) {
    (void) msg;
    (void) msg_len;
    (void) key32;
    (void) xonly_pk32;
    (void) algo16;
    (void) data;

    memset(nonce32, 0xFF, 32);
    return 1;
}

void test_schnorrsig_sign(void) {
    unsigned char sk[32];
    secp256k1_keypair keypair;
    const unsigned char msg[32] = "this is a msg for a schnorrsig..";
    unsigned char sig[64];
    unsigned char zeros64[64] = { 0 };

    memset(sk, 23, sizeof(sk));
    CHECK(secp256k1_keypair_create(ctx, &keypair, sk));
    CHECK(secp256k1_schnorrsig_sign(ctx, sig, msg, sizeof(msg), &keypair, NULL, NULL) == 1);

    /* Test different nonce functions */
    memset(sig, 1, sizeof(sig));
    CHECK(secp256k1_schnorrsig_sign(ctx, sig, msg, sizeof(msg), &keypair, nonce_function_failing, NULL) == 0);
    CHECK(memcmp(sig, zeros64, sizeof(sig)) == 0);
    memset(&sig, 1, sizeof(sig));
    CHECK(secp256k1_schnorrsig_sign(ctx, sig, msg, sizeof(msg), &keypair, nonce_function_0, NULL) == 0);
    CHECK(memcmp(sig, zeros64, sizeof(sig)) == 0);
    CHECK(secp256k1_schnorrsig_sign(ctx, sig, msg, sizeof(msg), &keypair, nonce_function_overflowing, NULL) == 1);
    CHECK(memcmp(sig, zeros64, sizeof(sig)) != 0);
}

#define N_SIGS  200
/* Creates N_SIGS valid signatures and verifies them with verify and
 * verify_batch (TODO). Then flips some bits and checks that verification now
 * fails. */
void test_schnorrsig_sign_verify(void) {
    const unsigned char sk[32] = "shhhhhhhh! this key is a secret.";
    unsigned char msg[N_SIGS][32];
    unsigned char sig[N_SIGS][64];
    size_t i;
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pk;

    CHECK(secp256k1_keypair_create(ctx, &keypair, sk));
    CHECK(secp256k1_keypair_xonly_pub(ctx, &pk, NULL, &keypair));

    for (i = 0; i < N_SIGS; i++) {
        secp256k1_rand256(msg[i]);
        CHECK(secp256k1_schnorrsig_sign(ctx, sig[i], msg[i], sizeof(msg[i]), &keypair, NULL, NULL));
        CHECK(secp256k1_schnorrsig_verify(ctx, sig[i], msg[i], sizeof(msg[i]), &pk));
        /* Wrong msg_len */
        CHECK(!secp256k1_schnorrsig_verify(ctx, sig[i], msg[i], sizeof(msg[i]) - 1, &pk));
    }

    {
        /* Flip a few bits in the signature and in the message and check that
         * verify and verify_batch (TODO) fail */
        size_t sig_idx = secp256k1_rand_int(4);
        size_t byte_idx = secp256k1_rand_int(32);
        unsigned char xorbyte = secp256k1_rand_int(254)+1;
        sig[sig_idx][byte_idx] ^= xorbyte;
        CHECK(!secp256k1_schnorrsig_verify(ctx, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk));
        sig[sig_idx][byte_idx] ^= xorbyte;

        byte_idx = secp256k1_rand_int(32);
        sig[sig_idx][32+byte_idx] ^= xorbyte;
        CHECK(!secp256k1_schnorrsig_verify(ctx, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk));
        sig[sig_idx][32+byte_idx] ^= xorbyte;

        byte_idx = secp256k1_rand_int(32);
        msg[sig_idx][byte_idx] ^= xorbyte;
        CHECK(!secp256k1_schnorrsig_verify(ctx, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk));
        msg[sig_idx][byte_idx] ^= xorbyte;

        /* Check that above bitflips have been reversed correctly */
        CHECK(secp256k1_schnorrsig_verify(ctx, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk));
    }
}
#undef N_SIGS

void test_schnorrsig_taproot(void) {
    unsigned char sk[32];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey internal_pk;
    unsigned char internal_pk_bytes[32];
    secp256k1_xonly_pubkey output_pk;
    unsigned char output_pk_bytes[32];
    unsigned char tweak[32];
    int pk_parity;
    unsigned char msg[32];
    unsigned char sig[64];

    /* Create output key */
    secp256k1_rand256(sk);
    CHECK(secp256k1_keypair_create(ctx, &keypair, sk) == 1);
    CHECK(secp256k1_keypair_xonly_pub(ctx, &internal_pk, NULL, &keypair) == 1);
    /* In actual taproot the tweak would be hash of internal_pk */
    CHECK(secp256k1_xonly_pubkey_serialize(ctx, tweak, &internal_pk) == 1);
    CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &keypair, tweak) == 1);
    CHECK(secp256k1_keypair_xonly_pub(ctx, &output_pk, &pk_parity, &keypair) == 1);
    CHECK(secp256k1_xonly_pubkey_serialize(ctx, output_pk_bytes, &output_pk) == 1);

    /* Key spend */
    secp256k1_rand256(msg);
    CHECK(secp256k1_schnorrsig_sign(ctx, sig, msg, sizeof(msg), &keypair, NULL, NULL) == 1);
    /* Verify key spend */
    CHECK(secp256k1_xonly_pubkey_parse(ctx, &output_pk, output_pk_bytes) == 1);
    CHECK(secp256k1_schnorrsig_verify(ctx, sig, msg, sizeof(msg), &output_pk) == 1);

    /* Script spend */
    CHECK(secp256k1_xonly_pubkey_serialize(ctx, internal_pk_bytes, &internal_pk) == 1);
    /* Verify script spend */
    CHECK(secp256k1_xonly_pubkey_parse(ctx, &internal_pk, internal_pk_bytes) == 1);
    CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, output_pk_bytes, pk_parity, &internal_pk, tweak) == 1);
}

void run_schnorrsig_tests(void) {
    run_nonce_function_bip340_tests();

    test_schnorrsig_api();
    test_schnorrsig_sha256_tagged();
    test_schnorrsig_bip_vectors();
    test_schnorrsig_sign();
    test_schnorrsig_sign_verify();
    test_schnorrsig_taproot();
    /* TODO: test more different msg lens */
}

#endif
