/*************************************************************************
 * Copyright (c) 2020-2021 Elichai Turkel                                *
 * Distributed under the CC0 software license, see the accompanying file *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "random.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"

static void print_hex(unsigned char* data, size_t size) {
    size_t i;
    printf("0x");
    for (i = 0; i < size; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(void) {
    unsigned char seckey1[32];
    unsigned char seckey2[32];
    unsigned char compressed_pubkey1[33];
    unsigned char compressed_pubkey2[33];
    unsigned char shared_secret1[32];
    unsigned char shared_secret2[32];
    unsigned char randomize[32];
    size_t len;
    secp256k1_pubkey pubkey1;
    secp256k1_pubkey pubkey2;

    /* The docs in secp256k1.h above the `secp256k1_ec_pubkey_create` function say:
     * "pointer to a context object, initialized for signing"
     * Which is why we create a context for signing(SECP256K1_CONTEXT_SIGN).
     * (The docs for `secp256k1_ecdh` don't require any special context, just some initialized context) */
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return 1;
    }
    /* Randomizing the context is recommended to protect against side-channel leakage
     * See `secp256k1_context_randomize` in secp256k1.h for more information about it
     * Should never fail */
    assert(secp256k1_context_randomize(ctx, randomize));

    /*** Key Generation ***/

    /* If the secret key is zero or out of range (bigger than secp256k1's order), we try to sample a new key.
     * note that the probability of this happening is negligible */
    while (1) {
        if (!fill_random(seckey1, sizeof(seckey1)) || !fill_random(seckey2, sizeof(seckey2))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        if (secp256k1_ec_seckey_verify(ctx, seckey1) && secp256k1_ec_seckey_verify(ctx, seckey2)) {
            break;
        }
    }

    /* Public key creation using a valid context with a verified secret key should never fail */
    assert(secp256k1_ec_pubkey_create(ctx, &pubkey1, seckey1));
    assert(secp256k1_ec_pubkey_create(ctx, &pubkey2, seckey2));

    /* Serialize pubkey1 in a compressed form (33 bytes), should always return 1 */
    len = sizeof(compressed_pubkey1);
    assert(secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey1, &len, &pubkey1, SECP256K1_EC_COMPRESSED));
    /* Should be the same size as the size of the output, because we passed a 33 bytes array. */
    assert(len == sizeof(compressed_pubkey1));

    /* Serialize pubkey2 in a compressed form (33 bytes) */
    len = sizeof(compressed_pubkey2);
    secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey2, &len, &pubkey2, SECP256K1_EC_COMPRESSED);
    assert(len == sizeof(compressed_pubkey2));

    /*** Creating the shared secret ***/

    /* Perform ECDH with seckey1 and pubkey2, should never fail with a verified seckey and valid pubkey */
    assert(secp256k1_ecdh(ctx, shared_secret1, &pubkey2, seckey1, NULL, NULL));

    /* Perform ECDH with seckey2 and pubkey1, should never fail with a verified seckey and valid pubkey */
    assert(secp256k1_ecdh(ctx, shared_secret2, &pubkey1, seckey2, NULL, NULL));

    /* Both parties should end up with the same shared secret */
    assert(memcmp(shared_secret1, shared_secret2, sizeof(shared_secret1)) == 0);

    printf("Secret Key1: ");
    print_hex(seckey1, sizeof(seckey1));
    printf("Compressed Pubkey1: ");
    print_hex(compressed_pubkey1, sizeof(compressed_pubkey1));
    printf("\nSecret Key2: ");
    print_hex(seckey2, sizeof(seckey2));
    printf("Compressed Pubkey2: ");
    print_hex(compressed_pubkey2, sizeof(compressed_pubkey2));
    printf("\nShared Secret: ");
    print_hex(shared_secret1, sizeof(shared_secret1));

    /* This will clear everything from the context and free the memory */
    secp256k1_context_destroy(ctx);

    /* It's best practice to try and zero out secrets after using them.
     * This is done because some bugs can allow an attacker leak memory, for example out of bounds array access(see Heartbleed for example).
     * We want to prevent the secrets from living in memory after they are used so they won't be leaked,
     * for that we zero out the secret key buffer.
     *
     * TODO: Prevent these writes from being optimized out, as any good compiler will remove any writes that aren't used. */
    memset(seckey1, 0, sizeof(seckey1));
    memset(seckey2, 0, sizeof(seckey2));
    memset(shared_secret1, 0, sizeof(shared_secret1));
    memset(shared_secret2, 0, sizeof(shared_secret2));

    return 0;
}
