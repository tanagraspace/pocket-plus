/*
 * Simple compression test - compress input and save output for decompression testing
 */

#include "pocket_plus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <output_file> <packet_size_bytes>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];
    int packet_size = atoi(argv[3]);
    int packet_length = packet_size * 8; // bits

    /* Read input file */
    FILE *fin = fopen(input_path, "rb");
    if (!fin) {
        fprintf(stderr, "Failed to open input file: %s\n", input_path);
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    size_t input_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    uint8_t *input_data = malloc(input_size);
    if (fread(input_data, 1, input_size, fin) != input_size) {
        fprintf(stderr, "Failed to read input file\n");
        fclose(fin);
        free(input_data);
        return 1;
    }
    fclose(fin);

    /* Open output file */
    FILE *fout = fopen(output_path, "wb");
    if (!fout) {
        fprintf(stderr, "Failed to open output file: %s\n", output_path);
        free(input_data);
        return 1;
    }

    /* Initialize compressor */
    pocket_compressor_t comp;
    pocket_compressor_init(&comp, packet_length, NULL, 1);  // robustness=1

    int num_packets = input_size / packet_size;
    fprintf(stderr, "Compressing %d packets of %d bytes each...\n", num_packets, packet_size);

    /* Compress all packets */
    for (int i = 0; i < num_packets; i++) {
        bitvector_t input;
        bitbuffer_t output;

        bitvector_init(&input, packet_length);
        bitbuffer_init(&output);

        /* Load packet data */
        bitvector_from_bytes(&input, &input_data[i * packet_size], packet_size);

        /* Set parameters according to test vector config */
        pocket_params_t params;
        params.min_robustness = 1;
        params.new_mask_flag = ((i + 1) % 10 == 0) ? 1 : 0;   // pt=10
        params.send_mask_flag = ((i + 1) % 20 == 0) ? 1 : 0;  // ft=20
        params.uncompressed_flag = ((i + 1) % 50 == 0) ? 1 : 0; // rt=50

        /* CCSDS requirement: Force ft=1, rt=1, pt=0 for first Rt+1 packets */
        if (i < 2) {  // robustness=1, so first 2 packets
            params.send_mask_flag = 1;
            params.uncompressed_flag = 1;
            params.new_mask_flag = 0;
        }

        /* Compress packet */
        pocket_compress_packet(&comp, &input, &output, &params);

        /* Write to output file */
        uint8_t packet_output[1000];
        size_t packet_bytes = bitbuffer_to_bytes(&output, packet_output, sizeof(packet_output));
        fwrite(packet_output, 1, packet_bytes, fout);
    }

    fclose(fout);
    free(input_data);

    fprintf(stderr, "Compression complete. Output saved to %s\n", output_path);
    return 0;
}
