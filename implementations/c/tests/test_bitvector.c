/*
 * ============================================================================
 *  _____                                   ____
 * |_   _|_ _ _ __   __ _  __ _ _ __ __ _  / ___| _ __   __ _  ___ ___
 *   | |/ _` | '_ \ / _` |/ _` | '__/ _` | \___ \| '_ \ / _` |/ __/ _ \
 *   | | (_| | | | | (_| | (_| | | | (_| |  ___) | |_) | (_| | (_|  __/
 *   |_|\__,_|_| |_|\__,_|\__, |_|  \__,_| |____/| .__/ \__,_|\___\___|
 *                        |___/                  |_|
 * ============================================================================
 *
 * POCKET+ C Implementation - Bit Vector Tests
 * TDD: Write tests first, then implement
 * ============================================================================
 */

#include "pocket_plus.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void name(void); \
    static void run_##name(void) { \
        printf("  %s...", #name); \
        fflush(stdout); \
        name(); \
        tests_passed++; \
        printf(" ✓\n"); \
    } \
    static void name(void)

#define RUN_TEST(name) do { \
    tests_run++; \
    run_##name(); \
} while(0)

/* ========================================================================
 * Basic Initialization and Access Tests
 * ======================================================================== */

TEST(test_bitvector_init_valid) {
    bitvector_t bv;
    int result = bitvector_init(&bv, 8);

    assert(result == POCKET_OK);
    assert(bv.length == 8);
    assert(bv.num_words == 1);  /* Ceiling(Ceiling(8/8)/4) = Ceiling(1/4) = 1 */
}

TEST(test_bitvector_init_non_byte_aligned) {
    bitvector_t bv;
    int result = bitvector_init(&bv, 13);

    assert(result == POCKET_OK);
    assert(bv.length == 13);
    assert(bv.num_words == 1);  /* Ceiling(Ceiling(13/8)/4) = Ceiling(2/4) = 1 */
}

TEST(test_bitvector_init_max_size) {
    bitvector_t bv;
    int result = bitvector_init(&bv, POCKET_MAX_PACKET_LENGTH);

    assert(result == POCKET_OK);
    assert(bv.length == POCKET_MAX_PACKET_LENGTH);
}

TEST(test_bitvector_init_too_large) {
    bitvector_t bv;
    int result = bitvector_init(&bv, POCKET_MAX_PACKET_LENGTH + 1);

    assert(result == POCKET_ERROR_INVALID_ARG);
}

TEST(test_bitvector_init_zero) {
    bitvector_t bv;
    int result = bitvector_init(&bv, 0);

    assert(result == POCKET_ERROR_INVALID_ARG);
}

TEST(test_bitvector_zero) {
    bitvector_t bv;
    bitvector_init(&bv, 8);

    /* Set all bits to 1 (big-endian: byte 0 at bits 24-31) */
    bv.data[0] = 0xFF000000;

    /* Zero the vector */
    bitvector_zero(&bv);

    assert(bv.data[0] == 0x00);
}

TEST(test_bitvector_get_set_bit) {
    bitvector_t bv;
    bitvector_init(&bv, 8);
    bitvector_zero(&bv);

    /* Set bit 0 (LSB) */
    bitvector_set_bit(&bv, 0, 1);
    assert(bitvector_get_bit(&bv, 0) == 1);
    /* With little-endian word packing, byte 0 is at bits 0-7 of word 0 */
    assert(bv.data[0] == 0x00000001);

    /* Set bit 7 (MSB of first byte) */
    bitvector_set_bit(&bv, 7, 1);
    assert(bitvector_get_bit(&bv, 7) == 1);
    assert(bv.data[0] == 0x00000081);

    /* Clear bit 0 */
    bitvector_set_bit(&bv, 0, 0);
    assert(bitvector_get_bit(&bv, 0) == 0);
    assert(bv.data[0] == 0x00000080);
}

TEST(test_bitvector_copy) {
    bitvector_t src, dest;
    bitvector_init(&src, 8);
    bitvector_init(&dest, 8);

    src.data[0] = 0xAB;
    bitvector_copy(&dest, &src);

    assert(dest.data[0] == 0xAB);
    assert(dest.length == src.length);
}

/* ========================================================================
 * Bitwise Operation Tests
 * ======================================================================== */

TEST(test_bitvector_xor) {
    bitvector_t a, b, result;
    bitvector_init(&a, 8);
    bitvector_init(&b, 8);
    bitvector_init(&result, 8);

    a.data[0] = 0xB3000000;  /* 10110011 */
    b.data[0] = 0xCA000000;  /* 11001010 */

    bitvector_xor(&result, &a, &b);

    assert(result.data[0] == 0x00000079);  /* 01111001 */
}

TEST(test_bitvector_or) {
    bitvector_t a, b, result;
    bitvector_init(&a, 8);
    bitvector_init(&b, 8);
    bitvector_init(&result, 8);

    a.data[0] = 0xB3000000;  /* 10110011 */
    b.data[0] = 0xCA000000;  /* 11001010 */

    bitvector_or(&result, &a, &b);

    assert(result.data[0] == 0x000000FB);  /* 11111011 */
}

TEST(test_bitvector_and) {
    bitvector_t a, b, result;
    bitvector_init(&a, 8);
    bitvector_init(&b, 8);
    bitvector_init(&result, 8);

    a.data[0] = 0xB3000000;  /* 10110011 */
    b.data[0] = 0xCA000000;  /* 11001010 */

    bitvector_and(&result, &a, &b);

    assert(result.data[0] == 0x00000082);  /* 10000010 */
}

TEST(test_bitvector_not) {
    bitvector_t a, result;
    bitvector_init(&a, 8);
    bitvector_init(&result, 8);

    a.data[0] = 0xB3000000;  /* 10110011 in byte 0 */

    bitvector_not(&result, &a);

    assert(result.data[0] == 0x0000004C);  /* 01001100 in byte 0 */
}

/* ========================================================================
 * Shift and Reverse Tests
 * ======================================================================== */

TEST(test_bitvector_left_shift) {
    bitvector_t a, result;
    bitvector_init(&a, 8);
    bitvector_init(&result, 8);

    a.data[0] = 0xB3000000;  /* 10110011 */

    bitvector_left_shift(&result, &a);

    /* Shift left by 1, insert 0 at LSB: 01100110 */
    assert(result.data[0] == 0x00000066);
}

TEST(test_bitvector_reverse) {
    bitvector_t a, result;
    bitvector_init(&a, 8);
    bitvector_init(&result, 8);

    a.data[0] = 0xB3000000;  /* 10110011 */

    bitvector_reverse(&result, &a);

    /* Reversed: 11001101 */
    assert(result.data[0] == 0x000000CD);
}

/* ========================================================================
 * Utility Function Tests
 * ======================================================================== */

TEST(test_bitvector_hamming_weight) {
    bitvector_t bv;
    bitvector_init(&bv, 8);

    bv.data[0] = 0xB3000000;  /* 10110011 in byte 0 - Five '1' bits */

    size_t weight = bitvector_hamming_weight(&bv);
    assert(weight == 5);
}

TEST(test_bitvector_equals) {
    bitvector_t a, b;
    bitvector_init(&a, 8);
    bitvector_init(&b, 8);

    a.data[0] = 0x000000AB;
    b.data[0] = 0x000000AB;

    assert(bitvector_equals(&a, &b) == 1);

    b.data[0] = 0x000000CD;
    assert(bitvector_equals(&a, &b) == 0);
}

/* ========================================================================
 * Byte Conversion Tests
 * ======================================================================== */

TEST(test_bitvector_from_bytes) {
    bitvector_t bv;
    bitvector_init(&bv, 16);

    uint8_t bytes[] = {0xAB, 0xCD};
    int result = bitvector_from_bytes(&bv, bytes, 2);

    assert(result == POCKET_OK);
    /* Big-endian packing: word[0] = (byte[0] << 24) | (byte[1] << 16) */
    assert(bv.data[0] == 0x0000CDAB);
}

TEST(test_bitvector_to_bytes) {
    bitvector_t bv;
    bitvector_init(&bv, 16);

    /* Set word with big-endian packing: bytes {0xAB, 0xCD} */
    bv.data[0] = 0x0000CDAB;

    uint8_t bytes[2];
    int result = bitvector_to_bytes(&bv, bytes, 2);

    assert(result == POCKET_OK);
    assert(bytes[0] == 0xAB);
    assert(bytes[1] == 0xCD);
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main(void) {
    printf("\nBit Vector Tests\n");
    printf("================\n\n");

    /* Basic initialization */
    RUN_TEST(test_bitvector_init_valid);
    RUN_TEST(test_bitvector_init_non_byte_aligned);
    RUN_TEST(test_bitvector_init_max_size);
    RUN_TEST(test_bitvector_init_too_large);
    RUN_TEST(test_bitvector_init_zero);
    RUN_TEST(test_bitvector_zero);
    RUN_TEST(test_bitvector_get_set_bit);
    RUN_TEST(test_bitvector_copy);

    /* Bitwise operations */
    RUN_TEST(test_bitvector_xor);
    RUN_TEST(test_bitvector_or);
    RUN_TEST(test_bitvector_and);
    RUN_TEST(test_bitvector_not);

    /* Shift and reverse */
    RUN_TEST(test_bitvector_left_shift);
    RUN_TEST(test_bitvector_reverse);

    /* Utilities */
    RUN_TEST(test_bitvector_hamming_weight);
    RUN_TEST(test_bitvector_equals);

    /* Byte conversion */
    RUN_TEST(test_bitvector_from_bytes);
    RUN_TEST(test_bitvector_to_bytes);

    printf("\n%d/%d tests passed\n\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
