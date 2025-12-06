/**
 * @file bitbuffer.c
 * @brief Variable-length bit buffer for building compressed output.
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
 * This module provides a dynamically-growing bit buffer for constructing
 * compressed output streams. Bits are appended sequentially using MSB-first
 * ordering as required by CCSDS 124.0-B-1.
 *
 * @par Bit Ordering
 * Bits are appended MSB-first within each byte:
 * - First bit appended goes to bit position 7
 * - Second bit goes to position 6, etc.
 *
 * @authors Georges Labrèche <georges@tanagraspace.com> — https://georges.fyi
 * @authors Claude Code (Anthropic) <noreply@anthropic.com>
 *
 * @see https://ccsds.org/Pubs/124x0b1.pdf CCSDS 124.0-B-1 Standard
 */

#include "pocketplus.h"
#include <string.h>

/**
 * @name Initialization Functions
 * @{
 */


void bitbuffer_init(bitbuffer_t *bb) {
    if (bb != NULL) {
        bb->num_bits = 0U;
        (void)memset(bb->data, 0, POCKET_MAX_OUTPUT_BYTES);
    }
}


void bitbuffer_clear(bitbuffer_t *bb) {
    bitbuffer_init(bb);
}

/** @} */ /* End of Initialization Functions */

/**
 * @name Bit Appending Functions
 * @{
 */


int bitbuffer_append_bit(bitbuffer_t *bb, int bit) {
    int result = POCKET_ERROR_INVALID_ARG;

    if (bb != NULL) {
        /* Check for overflow - POCKET_MAX_OUTPUT_BYTES * 8 bits */
        size_t max_bits = POCKET_MAX_OUTPUT_BYTES * 8U;
        if (bb->num_bits >= max_bits) {
            result = POCKET_ERROR_OVERFLOW;
        } else {
            size_t byte_index = bb->num_bits / 8U;
            size_t bit_index = bb->num_bits % 8U;

            /* CCSDS uses MSB-first bit ordering: first bit goes to position 7 */
            if (bit != 0) {
                bb->data[byte_index] |= (uint8_t)(1U << (7U - bit_index));
            }
            /* No need to clear bit, buffer is zero-initialized */

            bb->num_bits++;
            result = POCKET_OK;
        }
    }

    return result;
}


int bitbuffer_append_bits(bitbuffer_t *bb, const uint8_t *data, size_t num_bits) {
    int result = POCKET_ERROR_INVALID_ARG;

    if ((bb != NULL) && (data != NULL)) {
        /* Check for overflow - POCKET_MAX_OUTPUT_BYTES * 8 bits */
        size_t max_bits = POCKET_MAX_OUTPUT_BYTES * 8U;
        if ((bb->num_bits + num_bits) > max_bits) {
            result = POCKET_ERROR_OVERFLOW;
        } else {
            result = POCKET_OK;

            /* Append each bit MSB-first */
            for (size_t i = 0U; (i < num_bits) && (result == POCKET_OK); i++) {
                size_t byte_index = i / 8U;
                size_t bit_index = i % 8U;

                /* Extract bits MSB-first (bit 7, 6, 5, ..., 0) */
                uint32_t shift_amount = 7U - (uint32_t)bit_index;
                uint32_t shifted = (uint32_t)data[byte_index] >> shift_amount;
                uint32_t masked = shifted & 1U;
                int bit = (int)masked;

                result = bitbuffer_append_bit(bb, bit);
            }
        }
    }

    return result;
}


int bitbuffer_append_bitvector(bitbuffer_t *bb, const bitvector_t *bv) {
    int result = POCKET_ERROR_INVALID_ARG;

    if ((bb != NULL) && (bv != NULL)) {
        result = POCKET_OK;

        /* Calculate number of bytes from bit length */
        size_t num_bytes = (bv->length + 7U) / 8U;

        /* CCSDS MSB-first: bytes in order, but bits within each byte from MSB to LSB */
        for (size_t byte_idx = 0U; (byte_idx < num_bytes) && (result == POCKET_OK); byte_idx++) {
            size_t bits_in_this_byte = 8U;

            /* Last byte may have fewer than 8 bits */
            if (byte_idx == (num_bytes - 1U)) {
                size_t remainder = bv->length % 8U;
                if (remainder != 0U) {
                    bits_in_this_byte = remainder;
                }
            }

            /* With MSB-first bitvector indexing: bit 0 is MSB, bit 7 is LSB
             * We want to append bits in order: MSB first, LSB last
             * So we iterate through positions 0, 1, 2, ..., bits_in_this_byte-1 */
            size_t start_bit = byte_idx * 8U;
            for (size_t bit_offset = 0U; (bit_offset < bits_in_this_byte) && (result == POCKET_OK); bit_offset++) {
                size_t pos = start_bit + bit_offset;
                int bit = bitvector_get_bit(bv, pos);

                result = bitbuffer_append_bit(bb, bit);
            }
        }
    }

    return result;
}

/** @} */ /* End of Bit Appending Functions */

/**
 * @name Query Functions
 * @{
 */


size_t bitbuffer_size(const bitbuffer_t *bb) {
    size_t size = 0U;

    if (bb != NULL) {
        size = bb->num_bits;
    }

    return size;
}

/** @} */ /* End of Query Functions */

/**
 * @name Byte Conversion Functions
 * @{
 */


size_t bitbuffer_to_bytes(const bitbuffer_t *bb, uint8_t *data, size_t max_bytes) {
    size_t num_bytes = 0U;

    if ((bb != NULL) && (data != NULL)) {
        /* Calculate number of bytes needed (ceiling division) */
        num_bytes = (bb->num_bits + 7U) / 8U;

        if (num_bytes > max_bytes) {
            num_bytes = max_bytes;
        }

        (void)memcpy(data, bb->data, num_bytes);
    }

    return num_bytes;
}

/** @} */ /* End of Byte Conversion Functions */
