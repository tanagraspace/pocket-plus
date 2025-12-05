/**
 * @file test_decompress.c
 * @brief Unit tests for POCKET+ decompression functions.
 *
 * Tests follow TDD methodology - tests are written before implementation.
 * Code follows MISRA C:2012 guidelines.
 */

#include "pocketplus.h"
#include <stdio.h>
#include <string.h>

/* Test counters */
static int tests_passed = 0;
static int tests_total = 0;

/**
 * @brief Assert helper with descriptive output.
 */
#define TEST_ASSERT(cond, msg) do { \
    tests_total++; \
    if (cond) { \
        tests_passed++; \
        printf("  %s... \xe2\x9c\x93\n", msg); \
    } else { \
        printf("  %s... FAILED\n", msg); \
    } \
} while(0)

/* ============================================================================
 * COUNT Decoding Tests (inverse of COUNT encoding)
 *
 * CCSDS COUNT encoding:
 *   A = 1      → '0'
 *   2 ≤ A ≤ 33 → '110' ∥ BIT₅(A-2)
 *   A ≥ 34    → '111' ∥ BIT_E(A-2)
 *   Terminator → '10'
 * ============================================================================ */

/**
 * @brief Test COUNT decode of value 1 (encoded as '0').
 */
static void test_count_decode_1(void) {
    /* '0' decodes to 1 */
    uint8_t data[1] = {0x00};  /* 0b00000000 - first bit is 0 */
    bitreader_t reader;
    bitreader_init(&reader, data, 1U);

    uint32_t value = 0U;
    int result = pocket_count_decode(&reader, &value);

    TEST_ASSERT(result == POCKET_OK, "test_count_decode_1: returns OK");
    TEST_ASSERT(value == 1U, "test_count_decode_1: value is 1");
    TEST_ASSERT(bitreader_position(&reader) == 1U, "test_count_decode_1: consumed 1 bit");
}

/**
 * @brief Test COUNT decode of terminator (encoded as '10').
 */
static void test_count_decode_terminator(void) {
    /* '10' decodes to 0 (terminator) */
    uint8_t data[1] = {0x80};  /* 0b10000000 */
    bitreader_t reader;
    bitreader_init(&reader, data, 2U);

    uint32_t value = 0U;
    int result = pocket_count_decode(&reader, &value);

    TEST_ASSERT(result == POCKET_OK, "test_count_decode_terminator: returns OK");
    TEST_ASSERT(value == 0U, "test_count_decode_terminator: value is 0");
    TEST_ASSERT(bitreader_position(&reader) == 2U, "test_count_decode_terminator: consumed 2 bits");
}

/**
 * @brief Test COUNT decode of value 2 (encoded as '110' + 00000).
 */
static void test_count_decode_2(void) {
    /* '110' + BIT₅(0) = '11000000' = 0xC0 */
    uint8_t data[1] = {0xC0};  /* 0b11000000 */
    bitreader_t reader;
    bitreader_init(&reader, data, 8U);

    uint32_t value = 0U;
    int result = pocket_count_decode(&reader, &value);

    TEST_ASSERT(result == POCKET_OK, "test_count_decode_2: returns OK");
    TEST_ASSERT(value == 2U, "test_count_decode_2: value is 2");
    TEST_ASSERT(bitreader_position(&reader) == 8U, "test_count_decode_2: consumed 8 bits");
}

/**
 * @brief Test COUNT decode of value 33 (encoded as '110' + 11111).
 */
static void test_count_decode_33(void) {
    /* '110' + BIT₅(31) = '11011111' = 0xDF */
    uint8_t data[1] = {0xDF};  /* 0b11011111 */
    bitreader_t reader;
    bitreader_init(&reader, data, 8U);

    uint32_t value = 0U;
    int result = pocket_count_decode(&reader, &value);

    TEST_ASSERT(result == POCKET_OK, "test_count_decode_33: returns OK");
    TEST_ASSERT(value == 33U, "test_count_decode_33: value is 33");
}

/**
 * @brief Test COUNT decode of value 34 (first value using extended encoding).
 *
 * A=34: '111' + BIT_E(32) where E = 2*floor(log2(32)+1) - 6 = 2*6-6 = 6
 * BIT_6(32) = 100000
 * Full: '111100000' = 0xF0 0x00 (9 bits, but only need first byte + 1 bit)
 */
static void test_count_decode_34(void) {
    /* '111' + '100000' = 0b11110000 0... = 0xF0 */
    uint8_t data[2] = {0xF0, 0x00};
    bitreader_t reader;
    bitreader_init(&reader, data, 16U);

    uint32_t value = 0U;
    int result = pocket_count_decode(&reader, &value);

    TEST_ASSERT(result == POCKET_OK, "test_count_decode_34: returns OK");
    TEST_ASSERT(value == 34U, "test_count_decode_34: value is 34");
}

/**
 * @brief Test round-trip: encode then decode same value.
 */
static void test_count_roundtrip(void) {
    uint32_t test_values[] = {1U, 2U, 10U, 33U, 34U, 100U, 1000U};
    size_t num_values = sizeof(test_values) / sizeof(test_values[0]);
    int all_passed = 1;

    for (size_t i = 0U; i < num_values; i++) {
        /* Encode */
        bitbuffer_t encoded;
        bitbuffer_init(&encoded);
        (void)pocket_count_encode(&encoded, test_values[i]);

        /* Decode */
        uint8_t data[16];
        (void)bitbuffer_to_bytes(&encoded, data, sizeof(data));

        bitreader_t reader;
        bitreader_init(&reader, data, encoded.num_bits);

        uint32_t decoded = 0U;
        (void)pocket_count_decode(&reader, &decoded);

        if (decoded != test_values[i]) {
            all_passed = 0;
            printf("    Round-trip failed for %u: got %u\n", test_values[i], decoded);
        }
    }

    TEST_ASSERT(all_passed != 0, "test_count_roundtrip: all values round-trip correctly");
}

/* ============================================================================
 * RLE Decoding Tests (inverse of RLE encoding)
 * ============================================================================ */

/**
 * @brief Test RLE decode of all-zeros vector.
 *
 * All zeros: just terminator '10'
 */
static void test_rle_decode_all_zeros(void) {
    /* RLE of all zeros is just '10' (terminator) */
    uint8_t data[1] = {0x80};  /* 0b10000000 */
    bitreader_t reader;
    bitreader_init(&reader, data, 2U);

    bitvector_t result;
    bitvector_init(&result, 8U);

    int status = pocket_rle_decode(&reader, &result, 8U);

    TEST_ASSERT(status == POCKET_OK, "test_rle_decode_all_zeros: returns OK");
    TEST_ASSERT(bitvector_hamming_weight(&result) == 0U, "test_rle_decode_all_zeros: all bits zero");
}

/**
 * @brief Test RLE round-trip for various bit patterns.
 */
static void test_rle_roundtrip(void) {
    int all_passed = 1;

    /* Test patterns */
    uint8_t patterns[][2] = {
        {0x00, 0x00},  /* All zeros */
        {0x80, 0x00},  /* Single bit at MSB */
        {0x00, 0x01},  /* Single bit at LSB */
        {0xFF, 0xFF},  /* All ones */
        {0xAA, 0xAA},  /* Alternating */
        {0x55, 0x55},  /* Alternating inverse */
    };

    for (size_t p = 0U; p < 6U; p++) {
        /* Create input vector */
        bitvector_t input;
        bitvector_init(&input, 16U);
        bitvector_from_bytes(&input, patterns[p], 2U);

        /* Encode */
        bitbuffer_t encoded;
        bitbuffer_init(&encoded);
        (void)pocket_rle_encode(&encoded, &input);

        /* Get encoded bytes */
        uint8_t enc_data[16];
        (void)bitbuffer_to_bytes(&encoded, enc_data, sizeof(enc_data));

        /* Decode */
        bitreader_t reader;
        bitreader_init(&reader, enc_data, encoded.num_bits);

        bitvector_t decoded;
        bitvector_init(&decoded, 16U);
        (void)pocket_rle_decode(&reader, &decoded, 16U);

        /* Compare */
        if (bitvector_equals(&input, &decoded) == 0) {
            all_passed = 0;
            printf("    RLE round-trip failed for pattern %zu\n", p);
        }
    }

    TEST_ASSERT(all_passed != 0, "test_rle_roundtrip: all patterns round-trip correctly");
}

/* ============================================================================
 * Packet Decompression Tests
 * ============================================================================ */

/**
 * @brief Test decompression of a simple uncompressed packet.
 *
 * First packet with rt=1 contains full input data.
 */
static void test_decompress_uncompressed_packet(void) {
    /* Create a simple input packet */
    uint8_t input_data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t packet_bits = 32U;

    /* Compress it */
    pocket_compressor_t comp;
    pocket_compressor_init(&comp, packet_bits, NULL, 1U, 10, 20, 50);

    bitvector_t input;
    bitvector_init(&input, packet_bits);
    bitvector_from_bytes(&input, input_data, 4U);

    bitbuffer_t compressed;
    bitbuffer_init(&compressed);

    pocket_params_t params = {1U, 0, 1, 1};  /* First packet: ft=1, rt=1 */
    (void)pocket_compress_packet(&comp, &input, &compressed, &params);

    /* Now decompress */
    uint8_t comp_bytes[64];
    (void)bitbuffer_to_bytes(&compressed, comp_bytes, sizeof(comp_bytes));

    pocket_decompressor_t decomp;
    pocket_decompressor_init(&decomp, packet_bits, NULL, 1U);

    bitvector_t output;
    bitvector_init(&output, packet_bits);

    bitreader_t reader;
    bitreader_init(&reader, comp_bytes, compressed.num_bits);

    int result = pocket_decompress_packet(&decomp, &reader, &output);

    /* Verify */
    uint8_t output_data[4];
    bitvector_to_bytes(&output, output_data, 4U);

    int data_matches = (memcmp(input_data, output_data, 4U) == 0) ? 1 : 0;

    TEST_ASSERT(result == POCKET_OK, "test_decompress_uncompressed_packet: returns OK");
    TEST_ASSERT(data_matches != 0, "test_decompress_uncompressed_packet: data matches");
}

/**
 * @brief Test round-trip compression/decompression of multiple packets.
 */
static void test_decompress_roundtrip_multiple(void) {
    /* Create test data: 10 packets of 8 bytes each */
    uint8_t input_data[80];
    for (size_t i = 0U; i < 80U; i++) {
        input_data[i] = (uint8_t)(i * 3U);  /* Some pattern */
    }

    size_t packet_bits = 64U;

    /* Compress all packets */
    pocket_compressor_t comp;
    pocket_compressor_init(&comp, packet_bits, NULL, 1U, 10, 20, 50);

    uint8_t compressed_data[1024];
    size_t compressed_size = 0U;

    (void)pocket_compress(&comp, input_data, 80U, compressed_data, sizeof(compressed_data), &compressed_size);

    /* Decompress all packets */
    uint8_t output_data[80];
    size_t output_size = 0U;

    pocket_decompressor_t decomp;
    pocket_decompressor_init(&decomp, packet_bits, NULL, 1U);

    int result = pocket_decompress(&decomp, compressed_data, compressed_size,
                                    output_data, sizeof(output_data), &output_size);

    /* Verify */
    int data_matches = (memcmp(input_data, output_data, 80U) == 0) ? 1 : 0;

    TEST_ASSERT(result == POCKET_OK, "test_decompress_roundtrip_multiple: returns OK");
    TEST_ASSERT(output_size == 80U, "test_decompress_roundtrip_multiple: correct size");
    TEST_ASSERT(data_matches != 0, "test_decompress_roundtrip_multiple: data matches");
}

/* ============================================================================
 * Bit Reader Tests
 * ============================================================================ */

/**
 * @brief Test bit reader initialization.
 */
static void test_bitreader_init(void) {
    uint8_t data[4] = {0xAB, 0xCD, 0xEF, 0x12};
    bitreader_t reader;

    bitreader_init(&reader, data, 32U);

    TEST_ASSERT(bitreader_position(&reader) == 0U, "test_bitreader_init: position is 0");
    TEST_ASSERT(bitreader_remaining(&reader) == 32U, "test_bitreader_init: 32 bits remaining");
}

/**
 * @brief Test reading single bits.
 */
static void test_bitreader_read_bit(void) {
    uint8_t data[1] = {0xA5};  /* 0b10100101 */
    bitreader_t reader;
    bitreader_init(&reader, data, 8U);

    /* Read bits MSB first */
    int bit0 = bitreader_read_bit(&reader);  /* 1 */
    int bit1 = bitreader_read_bit(&reader);  /* 0 */
    int bit2 = bitreader_read_bit(&reader);  /* 1 */
    int bit3 = bitreader_read_bit(&reader);  /* 0 */

    TEST_ASSERT(bit0 == 1, "test_bitreader_read_bit: bit 0 is 1");
    TEST_ASSERT(bit1 == 0, "test_bitreader_read_bit: bit 1 is 0");
    TEST_ASSERT(bit2 == 1, "test_bitreader_read_bit: bit 2 is 1");
    TEST_ASSERT(bit3 == 0, "test_bitreader_read_bit: bit 3 is 0");
    TEST_ASSERT(bitreader_position(&reader) == 4U, "test_bitreader_read_bit: position is 4");
}

/**
 * @brief Test reading multiple bits as value.
 */
static void test_bitreader_read_bits(void) {
    uint8_t data[2] = {0xAB, 0xCD};  /* 0b10101011 11001101 */
    bitreader_t reader;
    bitreader_init(&reader, data, 16U);

    uint32_t val4 = bitreader_read_bits(&reader, 4U);   /* 0b1010 = 10 */
    uint32_t val8 = bitreader_read_bits(&reader, 8U);   /* 0b10111100 = 188 */

    TEST_ASSERT(val4 == 10U, "test_bitreader_read_bits: first 4 bits = 10");
    TEST_ASSERT(val8 == 188U, "test_bitreader_read_bits: next 8 bits = 188");
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("\nDecompression Tests\n");
    printf("===================\n\n");

    printf("Bit Reader Tests:\n");
    test_bitreader_init();
    test_bitreader_read_bit();
    test_bitreader_read_bits();

    printf("\nCOUNT Decoding Tests:\n");
    test_count_decode_1();
    test_count_decode_terminator();
    test_count_decode_2();
    test_count_decode_33();
    test_count_decode_34();
    test_count_roundtrip();

    printf("\nRLE Decoding Tests:\n");
    test_rle_decode_all_zeros();
    test_rle_roundtrip();

    printf("\nPacket Decompression Tests:\n");
    test_decompress_uncompressed_packet();
    test_decompress_roundtrip_multiple();

    printf("\n%d/%d tests passed\n\n", tests_passed, tests_total);

    return (tests_passed == tests_total) ? 0 : 1;
}
