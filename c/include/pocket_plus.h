#ifndef POCKET_PLUS_H
#define POCKET_PLUS_H

#include <stdint.h>
#include <stddef.h>

#define POCKET_PLUS_VERSION "0.1.0"

/**
 * POCKET+ Compression Algorithm
 *
 * Implementation of CCSDS 124.0-B-1 lossless compression algorithm
 * designed for spacecraft housekeeping data.
 */

/**
 * Compress data using POCKET+ algorithm
 *
 * @param input Input data buffer
 * @param input_size Size of input data in bytes
 * @param output Output buffer for compressed data
 * @param output_size Size of output buffer in bytes
 * @return Number of bytes written to output, or -1 on error
 */
int pocket_plus_compress(const uint8_t *input, size_t input_size,
                        uint8_t *output, size_t output_size);

/**
 * Decompress data using POCKET+ algorithm
 *
 * @param input Compressed input data buffer
 * @param input_size Size of compressed data in bytes
 * @param output Output buffer for decompressed data
 * @param output_size Size of output buffer in bytes
 * @return Number of bytes written to output, or -1 on error
 */
int pocket_plus_decompress(const uint8_t *input, size_t input_size,
                          uint8_t *output, size_t output_size);

#endif /* POCKET_PLUS_H */
