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
 * POCKET+ C Implementation - Bit Buffer
 *
 * Authors:
 *   Georges Labrèche <georges@tanagraspace.com>
 *   Claude Code (claude-sonnet-4-5-20250929) <noreply@anthropic.com>
 *
 * Variable-length bit buffer for building compressed output.
 * Bits are appended sequentially from LSB to MSB within each byte.
 * ============================================================================
 */

#include "pocket_plus.h"
#include <string.h>

/* ========================================================================
 * Initialization
 * ======================================================================== */

void bitbuffer_init(bitbuffer_t *bb) {
    if (bb != NULL) {
        bb->num_bits = 0;
        memset(bb->data, 0, POCKET_MAX_OUTPUT_BYTES);
    }
}

void bitbuffer_clear(bitbuffer_t *bb) {
    bitbuffer_init(bb);
}

/* ========================================================================
 * Appending Bits
 * ======================================================================== */

int bitbuffer_append_bit(bitbuffer_t *bb, int bit) {
    if (bb == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Check for overflow */
    if (bb->num_bits >= POCKET_MAX_OUTPUT_BYTES * 8) {
        return POCKET_ERROR_OVERFLOW;
    }

    size_t byte_index = bb->num_bits / 8;
    size_t bit_index = bb->num_bits % 8;

    /* CCSDS uses MSB-first bit ordering: first bit goes to position 7 */
    if (bit) {
        bb->data[byte_index] |= (1 << (7 - bit_index));
    }
    /* No need to clear bit, buffer is zero-initialized */

    bb->num_bits++;

    return POCKET_OK;
}

int bitbuffer_append_bits(bitbuffer_t *bb, const uint8_t *data, size_t num_bits) {
    if (bb == NULL || data == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Check for overflow */
    if (bb->num_bits + num_bits > POCKET_MAX_OUTPUT_BYTES * 8) {
        return POCKET_ERROR_OVERFLOW;
    }

    /* Append each bit MSB-first */
    for (size_t i = 0; i < num_bits; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        /* Extract bits MSB-first (bit 7, 6, 5, ..., 0) */
        int bit = (data[byte_index] >> (7 - bit_index)) & 1;

        int result = bitbuffer_append_bit(bb, bit);
        if (result != POCKET_OK) {
            return result;
        }
    }

    return POCKET_OK;
}

int bitbuffer_append_bitvector(bitbuffer_t *bb, const bitvector_t *bv) {
    if (bb == NULL || bv == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Calculate number of bytes from bit length */
    size_t num_bytes = (bv->length + 7) / 8;

    /* CCSDS MSB-first: bytes in order, but bits within each byte from MSB to LSB */
    for (size_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        size_t bits_in_this_byte = 8;

        /* Last byte may have fewer than 8 bits */
        if (byte_idx == num_bytes - 1) {
            size_t remainder = bv->length % 8;
            if (remainder != 0) {
                bits_in_this_byte = remainder;
            }
        }

        /* Within this byte, append bits from MSB to LSB */
        /* Bit positions within byte: 7 (MSB) down to 0 (LSB) */
        size_t start_bit = byte_idx * 8;
        for (size_t bit_offset = bits_in_this_byte; bit_offset > 0; bit_offset--) {
            size_t pos = start_bit + (bit_offset - 1);
            int bit = bitvector_get_bit(bv, pos);

            int result = bitbuffer_append_bit(bb, bit);
            if (result != POCKET_OK) {
                return result;
            }
        }
    }

    return POCKET_OK;
}

/* ========================================================================
 * Query
 * ======================================================================== */

size_t bitbuffer_size(const bitbuffer_t *bb) {
    if (bb == NULL) {
        return 0;
    }

    return bb->num_bits;
}

/* ========================================================================
 * Byte Conversion
 * ======================================================================== */

size_t bitbuffer_to_bytes(const bitbuffer_t *bb, uint8_t *data, size_t max_bytes) {
    if (bb == NULL || data == NULL) {
        return 0;
    }

    /* Calculate number of bytes needed (ceiling division) */
    size_t num_bytes = (bb->num_bits + 7) / 8;

    if (num_bytes > max_bytes) {
        num_bytes = max_bytes;
    }

    memcpy(data, bb->data, num_bytes);

    return num_bytes;
}
