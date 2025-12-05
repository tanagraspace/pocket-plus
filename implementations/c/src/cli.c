/**
 * @file cli.c
 * @brief POCKET+ command line interface.
 *
 * @cond INTERNAL
 * ============================================================================
 *  _____                                   ____
 * |_   _|_ _ _ __   __ _  __ _ _ __ __ _  / ___| _ __   __ _  ___ ___
 *   | |/ _` | '_ \ / _` |/ _` | '__/ _` | \___ \| '_ \ / _` |/ __/ _ \
 *   | | (_| | | | | (_| | (_| | | | (_| |  ___) | |_) | (_| | (_|  __/
 *   |_|\__,_|_| |_|\__,_|\__, |_|  \__,_| |____/| .__/ \__,_|\___\___|
 *                        |___/                  |_|
 * ============================================================================
 * @endcond
 *
 * Provides a command-line interface for CCSDS 124.0-B-1 compression.
 *
 * @par Usage
 * @code
 * pocket_compress <input> <packet_size> <pt> <ft> <rt> <robustness>
 * @endcode
 *
 * @authors Georges Labrèche <georges@tanagraspace.com> — https://georges.fyi
 * @authors Claude Code (Anthropic) <noreply@anthropic.com>
 *
 * @see https://public.ccsds.org/Pubs/124x0b1.pdf CCSDS 124.0-B-1 Standard
 */

#include "pocket_plus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief ASCII art banner for help output. */
static const char *BANNER =
"                                              \n"
"  ____   ___   ____ _  _______ _____     _    \n"
" |  _ \\ / _ \\ / ___| |/ / ____|_   _|  _| |_  \n"
" | |_) | | | | |   | ' /|  _|   | |   |_   _| \n"
" |  __/| |_| | |___| . \\| |___  | |     |_|   \n"
" |_|    \\___/ \\____|_|\\_\\_____| |_|           \n"
"                                              \n"
"         by  T A N A G R A  S P A C E         \n";

/**
 * @brief Print help message with usage information.
 *
 * @param[in] prog_name Program name for usage example.
 */
static void print_help(const char *prog_name) {
    printf("\n%s\n", BANNER);
    printf("CCSDS 124.0-B-1 Lossless Compression\n");
    printf("=====================================\n\n");
    printf("References:\n");
    printf("  CCSDS 124.0-B-1: https://public.ccsds.org/Pubs/124x0b1.pdf\n");
    printf("  ESA POCKET+: https://opssat.esa.int/pocket-plus/\n\n");
    printf("Citation:\n");
    printf("  D. Evans, G. Labrèche, D. Marszk, S. Bammens, M. Hernandez-Cabronero,\n");
    printf("  V. Zelenevskiy, V. Shiradhonkar, M. Starcik, and M. Henkel. 2022.\n");
    printf("  \"Implementing the New CCSDS Housekeeping Data Compression Standard\n");
    printf("  124.0-B-1 (based on POCKET+) on OPS-SAT-1,\" Proceedings of the\n");
    printf("  Small Satellite Conference, Communications, SSC22-XII-03.\n");
    printf("  https://digitalcommons.usu.edu/smallsat/2022/all2022/133/\n\n");
    printf("Usage: %s <input> <packet_size> <pt> <ft> <rt> <robustness>\n\n", prog_name);
    printf("Arguments:\n");
    printf("  input        Input file to compress\n");
    printf("  packet_size  Packet size in bytes (e.g., 90)\n");
    printf("  pt           New mask period (e.g., 10, 20)\n");
    printf("  ft           Send mask period (e.g., 20, 50)\n");
    printf("  rt           Uncompressed period (e.g., 50, 100)\n");
    printf("  robustness   Robustness level 0-7 (e.g., 1, 2)\n\n");
    printf("Output:\n");
    printf("  Creates <input>.pkt compressed file\n\n");
    printf("Example:\n");
    printf("  %s data.bin 90 10 20 50 1\n\n", prog_name);
}

/**
 * @brief CLI entry point.
 *
 * Parses command-line arguments and performs compression.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument vector.
 *
 * @return 0 on success, 1 on error.
 */
int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return (argc < 2) ? 1 : 0;
    }

    if (argc != 7) {
        fprintf(stderr, "Error: Expected 6 arguments, got %d\n", argc - 1);
        fprintf(stderr, "Run '%s --help' for usage\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    int packet_size = atoi(argv[2]);
    int pt_period = atoi(argv[3]);
    int ft_period = atoi(argv[4]);
    int rt_period = atoi(argv[5]);
    int robustness = atoi(argv[6]);

    /* Validate parameters */
    if (packet_size <= 0 || packet_size > 8192) {
        fprintf(stderr, "Error: packet_size must be 1-8192 bytes\n");
        return 1;
    }
    if (robustness < 0 || robustness > 7) {
        fprintf(stderr, "Error: robustness must be 0-7\n");
        return 1;
    }
    if (pt_period <= 0 || ft_period <= 0 || rt_period <= 0) {
        fprintf(stderr, "Error: periods must be positive\n");
        return 1;
    }

    /* Read input file */
    FILE *fin = fopen(input_path, "rb");
    if (!fin) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_path);
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    size_t input_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    if (input_size % packet_size != 0) {
        fprintf(stderr, "Error: Input size (%zu) not divisible by packet size (%d)\n",
                input_size, packet_size);
        fclose(fin);
        return 1;
    }

    uint8_t *input_data = malloc(input_size);
    if (!input_data) {
        fprintf(stderr, "Error: Cannot allocate memory\n");
        fclose(fin);
        return 1;
    }

    if (fread(input_data, 1, input_size, fin) != input_size) {
        fprintf(stderr, "Error: Failed to read input file\n");
        fclose(fin);
        free(input_data);
        return 1;
    }
    fclose(fin);

    /* Create output filename */
    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%s.pkt", input_path);

    FILE *fout = fopen(output_path, "wb");
    if (!fout) {
        fprintf(stderr, "Error: Cannot create output file: %s\n", output_path);
        free(input_data);
        return 1;
    }

    /* Initialize compressor with automatic parameter management */
    int packet_length = packet_size * 8;
    pocket_compressor_t comp;
    pocket_compressor_init(&comp, packet_length, NULL, robustness,
                           pt_period, ft_period, rt_period);

    /* Allocate output buffer */
    size_t max_output = input_size * 2;  /* Conservative estimate */
    uint8_t *output_data = malloc(max_output);
    if (!output_data) {
        fprintf(stderr, "Error: Cannot allocate output buffer\n");
        fclose(fout);
        free(input_data);
        return 1;
    }

    /* Compress */
    size_t output_size = 0;
    int result = pocket_compress(&comp, input_data, input_size,
                                  output_data, max_output, &output_size);

    if (result != POCKET_OK) {
        fprintf(stderr, "Error: Compression failed with code %d\n", result);
        fclose(fout);
        free(input_data);
        free(output_data);
        return 1;
    }

    /* Write output */
    fwrite(output_data, 1, output_size, fout);
    fclose(fout);

    /* Print summary */
    int num_packets = input_size / packet_size;
    double ratio = (double)input_size / output_size;
    printf("Input:       %s (%zu bytes, %d packets)\n", input_path, input_size, num_packets);
    printf("Output:      %s (%zu bytes)\n", output_path, output_size);
    printf("Ratio:       %.2fx\n", ratio);
    printf("Parameters:  R=%d, pt=%d, ft=%d, rt=%d\n",
           robustness, pt_period, ft_period, rt_period);

    free(input_data);
    free(output_data);
    return 0;
}
