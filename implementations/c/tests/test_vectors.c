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
 * POCKET+ C Implementation - Reference Test Vectors
 *
 * Tests compression against reference test vectors to ensure correctness
 * and interoperability with other implementations.
 *
 * Authors:
 *   Georges Labrèche <georges@tanagraspace.com>
 *   Claude Code (claude-sonnet-4-5-20250929) <noreply@anthropic.com>
 * ============================================================================
 */

#include "pocket_plus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test vector paths relative to c/ directory */
#define TEST_VECTORS_DIR "../../test-vectors"

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

/* Helper: Read file into buffer */
static size_t read_file(const char *path, uint8_t *buffer, size_t max_size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "\n  ERROR: Could not open file: %s\n", path);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > (long)max_size) {
        fprintf(stderr, "\n  ERROR: File too large: %ld bytes (max %zu)\n",
                file_size, max_size);
        fclose(fp);
        return 0;
    }

    size_t bytes_read = fread(buffer, 1, file_size, fp);
    fclose(fp);

    return bytes_read;
}

/* Helper: Compare our output with expected output */
static int compare_output(const uint8_t *actual, size_t actual_size,
                         const uint8_t *expected, size_t expected_size,
                         const char *test_name) {
    if (actual_size != expected_size) {
        fprintf(stderr, "\n  FAIL: Size mismatch in %s\n", test_name);
        fprintf(stderr, "    Expected: %zu bytes\n", expected_size);
        fprintf(stderr, "    Got:      %zu bytes\n", actual_size);
        return 0;
    }

    for (size_t i = 0; i < actual_size; i++) {
        if (actual[i] != expected[i]) {
            fprintf(stderr, "\n  FAIL: Byte mismatch in %s at offset %zu\n",
                    test_name, i);
            fprintf(stderr, "    Expected: 0x%02X\n", expected[i]);
            fprintf(stderr, "    Got:      0x%02X\n", actual[i]);

            /* Show some context */
            fprintf(stderr, "    Context (offset %zu):\n", i);
            size_t start = (i > 4) ? i - 4 : 0;
            size_t end = (i + 5 < actual_size) ? i + 5 : actual_size;

            fprintf(stderr, "      Expected: ");
            for (size_t j = start; j < end; j++) {
                fprintf(stderr, "%02X ", expected[j]);
                if (j == i) fprintf(stderr, "[");
            }
            fprintf(stderr, "\n");

            fprintf(stderr, "      Got:      ");
            for (size_t j = start; j < end; j++) {
                fprintf(stderr, "%02X ", actual[j]);
                if (j == i) fprintf(stderr, "[");
            }
            fprintf(stderr, "\n");

            return 0;
        }
    }

    return 1;
}

/* Helper: Compress entire file and compare with expected output */
static int compress_and_verify(
    const char *input_path,
    const char *expected_path,
    size_t packet_length_bits,
    int pt_limit,
    int ft_limit,
    int rt_limit,
    uint8_t robustness,
    const char *test_name
) {
    /* Read input file */
    uint8_t input_data[10000];
    size_t input_size = read_file(input_path, input_data, sizeof(input_data));
    if (input_size == 0) {
        fprintf(stderr, "\n  FAIL: Could not read input file: %s\n", input_path);
        return 0;
    }

    /* Read expected output */
    uint8_t expected_output[10000];
    size_t expected_size = read_file(expected_path, expected_output, sizeof(expected_output));
    if (expected_size == 0) {
        fprintf(stderr, "\n  FAIL: Could not read expected output: %s\n", expected_path);
        return 0;
    }

    printf("\n    Input: %zu bytes, Expected output: %zu bytes\n",
           input_size, expected_size);

    /* Initialize compressor */
    size_t packet_size_bytes = packet_length_bits / 8;
    int num_packets = input_size / packet_size_bytes;

    printf("    Processing %d packets of %zu bits each\n", num_packets, packet_length_bits);

    pocket_compressor_t comp;
    int result = pocket_compressor_init(&comp, packet_length_bits, NULL, robustness);
    if (result != POCKET_OK) {
        fprintf(stderr, "\n  FAIL: Compressor init failed\n");
        return 0;
    }

    /* Compress all packets */
    uint8_t actual_output[10000];
    size_t actual_size = 0;

    for (int i = 0; i < num_packets; i++) {
        bitvector_t input;
        bitbuffer_t packet_output;

        bitvector_init(&input, packet_length_bits);
        bitbuffer_init(&packet_output);

        /* Load packet data */
        bitvector_from_bytes(&input, &input_data[i * packet_size_bytes], packet_size_bytes);

        /* Calculate parameters according to test vector periods */
        pocket_params_t params;
        params.min_robustness = robustness;

        int packet_num = i + 1;
        const int pt_first_trigger = pt_limit + robustness;
        const int ft_first_trigger = ft_limit + robustness;
        const int rt_first_trigger = rt_limit + robustness;

        params.new_mask_flag = (packet_num >= pt_first_trigger && packet_num % pt_limit == (pt_first_trigger % pt_limit)) ? 1 : 0;
        params.send_mask_flag = (packet_num >= ft_first_trigger && packet_num % ft_limit == (ft_first_trigger % ft_limit)) ? 1 : 0;
        params.uncompressed_flag = (packet_num >= rt_first_trigger && packet_num % rt_limit == (rt_first_trigger % rt_limit)) ? 1 : 0;

        /* CCSDS requirement: Force ft=1, rt=1, pt=0 for first Rt+1 packets */
        if (i <= robustness) {
            params.send_mask_flag = 1;
            params.uncompressed_flag = 1;
            params.new_mask_flag = 0;
        }

        /* Compress packet */
        result = pocket_compress_packet(&comp, &input, &packet_output, &params);
        if (result != POCKET_OK) {
            fprintf(stderr, "\n  FAIL: Compression failed at packet %d\n", i);
            return 0;
        }

        /* Accumulate output (byte-boundary padding per packet) */
        uint8_t packet_bytes[2000];
        size_t packet_size = bitbuffer_to_bytes(&packet_output, packet_bytes, sizeof(packet_bytes));

        memcpy(actual_output + actual_size, packet_bytes, packet_size);
        actual_size += packet_size;
    }

    printf("    Compressed: %zu bytes (ratio: %.2fx)\n",
           actual_size, (float)input_size / actual_size);

    /* Compare with expected output */
    return compare_output(actual_output, actual_size,
                         expected_output, expected_size, test_name);
}

/* ========================================================================
 * Test Vector: Simple
 *
 * Parameters from metadata.json:
 *   packet_length: 90 bytes (720 bits)
 *   pt: 10 (new mask every 10 packets)
 *   ft: 20 (send mask every 20 packets)
 *   rt: 50 (uncompressed every 50 packets)
 *   robustness: 1
 * ======================================================================== */

TEST(test_vector_simple) {
    char input_path[256];
    char expected_path[256];
    snprintf(input_path, sizeof(input_path), "%s/input/simple.bin", TEST_VECTORS_DIR);
    snprintf(expected_path, sizeof(expected_path), "%s/expected-output/simple.bin.pkt", TEST_VECTORS_DIR);

    if (!compress_and_verify(input_path, expected_path, 720, 10, 20, 50, 1, "simple")) {
        tests_failed++;
    }
}

/* ========================================================================
 * Test Vector: Housekeeping
 * ======================================================================== */

TEST(test_vector_housekeeping) {
    printf("\n    Skipping (not yet implemented)\n");
}

/* ========================================================================
 * Test Vector: Edge Cases
 * ======================================================================== */

TEST(test_vector_edge_cases) {
    printf("\n    Skipping (not yet implemented)\n");
}

/* ========================================================================
 * Test Vector: Venus Express
 * ======================================================================== */

TEST(test_vector_venus_express) {
    printf("\n    Skipping (not yet implemented)\n");
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main(void) {
    printf("\nReference Test Vectors\n");
    printf("======================\n\n");

    printf("Testing against reference implementations...\n");
    printf("Test vectors location: %s\n\n", TEST_VECTORS_DIR);

    RUN_TEST(test_vector_simple);
    RUN_TEST(test_vector_housekeeping);
    RUN_TEST(test_vector_edge_cases);
    RUN_TEST(test_vector_venus_express);

    printf("\n");
    printf("Results: %d/%d tests passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %d failed", tests_failed);
    }
    printf("\n\n");

    if (tests_failed > 0) {
        printf("FAIL: Some tests failed unexpectedly.\n\n");
        return 1;
    }

    printf("All tests passed!\n\n");
    return 0;
}
