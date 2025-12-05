/**
 * @file bitvector.c
 * @brief Fixed-length bit vector implementation using 32-bit words.
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
 * This module provides fixed-length bit vector operations optimized for
 * POCKET+ compression. Uses 32-bit words with big-endian byte packing to
 * match ESA/ESOC reference implementation.
 *
 * @par Bit Numbering Convention (CCSDS 124.0-B-1 Section 1.6.1)
 * - Bit 0 = LSB (Least Significant Bit)
 * - Bit N-1 = MSB (Most Significant Bit, transmitted first)
 *
 * @par Word Packing (Big-Endian)
 * Within each 32-bit word:
 * - Word[i] = (Byte[4i] << 24) | (Byte[4i+1] << 16) | (Byte[4i+2] << 8) | Byte[4i+3]
 * - Bit 0 = LSB of word, Bit 31 = MSB of word
 *
 * @authors Georges Labrèche <georges@tanagraspace.com> — https://georges.fyi
 * @authors Claude Code (Anthropic) <noreply@anthropic.com>
 *
 * @see https://public.ccsds.org/Pubs/124x0b1.pdf CCSDS 124.0-B-1 Standard
 */

#include "pocket_plus.h"
#include <string.h>

/**
 * @name Initialization Functions
 * @{
 */

/**
 * @brief Initialize a bit vector with specified length.
 *
 * Allocates internal storage for the specified number of bits and zeros
 * all words. Uses 32-bit word storage with big-endian byte packing.
 *
 * @param[out] bv       Pointer to bit vector structure to initialize.
 * @param[in]  num_bits Number of bits in the vector (1 to POCKET_MAX_PACKET_LENGTH).
 *
 * @return POCKET_OK on success.
 * @return POCKET_ERROR_INVALID_ARG if bv is NULL or num_bits is invalid.
 */
int bitvector_init(bitvector_t *bv, size_t num_bits) {
    if (bv == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    if (num_bits == 0 || num_bits > POCKET_MAX_PACKET_LENGTH) {
        return POCKET_ERROR_INVALID_ARG;
    }

    bv->length = num_bits;
    /* Calculate number of 32-bit words needed */
    size_t num_bytes = (num_bits + 7) / 8;
    bv->num_words = (num_bytes + 3) / 4;  /* Ceiling division */

    /* Zero all words */
    memset(bv->data, 0, bv->num_words * sizeof(uint32_t));

    return POCKET_OK;
}

/**
 * @brief Zero all bits in a bit vector.
 *
 * Sets all words in the bit vector to zero without changing the length.
 *
 * @param[in,out] bv Pointer to bit vector to zero. No-op if NULL.
 */
void bitvector_zero(bitvector_t *bv) {
    if (bv != NULL) {
        memset(bv->data, 0, bv->num_words * sizeof(uint32_t));
    }
}

/**
 * @brief Copy a bit vector to another.
 *
 * Performs a deep copy of all data, length, and word count from source
 * to destination bit vector.
 *
 * @param[out] dest Pointer to destination bit vector.
 * @param[in]  src  Pointer to source bit vector.
 *
 * @note No-op if either pointer is NULL.
 */
void bitvector_copy(bitvector_t *dest, const bitvector_t *src) {
    if (dest != NULL && src != NULL) {
        dest->length = src->length;
        dest->num_words = src->num_words;
        memcpy(dest->data, src->data, src->num_words * sizeof(uint32_t));
    }
}

/** @} */ /* End of Initialization Functions */

/**
 * @name Bit Access Functions
 *
 * Big-endian word packing maps bytes to word bits as follows:
 * - Word[i] contains Bytes [4i, 4i+1, 4i+2, 4i+3]
 * - Byte 4i at bits 24-31 (most significant)
 * - Byte 4i+1 at bits 16-23
 * - Byte 4i+2 at bits 8-15
 * - Byte 4i+3 at bits 0-7 (least significant)
 * @{
 */

/**
 * @brief Get the value of a single bit.
 *
 * Retrieves the bit at the specified position using MSB-first indexing
 * with big-endian word packing.
 *
 * @param[in] bv  Pointer to bit vector.
 * @param[in] pos Bit position (0 = MSB, length-1 = LSB).
 *
 * @return 1 if bit is set, 0 if bit is clear or on error.
 */
int bitvector_get_bit(const bitvector_t *bv, size_t pos) {
    if (bv == NULL || pos >= bv->length) {
        return 0;
    }

    /* Calculate byte and bit within byte */
    size_t byte_index = pos / 8;
    size_t bit_in_byte = pos % 8;

    /* Calculate word index and byte position within word */
    size_t word_index = byte_index / 4;
    size_t byte_in_word = byte_index % 4;

    /* Big-endian: byte 0 is at bits 24-31, byte 1 at 16-23, etc.
     * MSB-first indexing: bit 0 is the MSB (leftmost) within each byte
     * This matches the reference implementation */
    size_t bit_in_word = (3 - byte_in_word) * 8 + (7 - bit_in_byte);

    return (bv->data[word_index] >> bit_in_word) & 1;
}

/**
 * @brief Set or clear a single bit.
 *
 * Sets or clears the bit at the specified position using MSB-first indexing
 * with big-endian word packing.
 *
 * @param[in,out] bv    Pointer to bit vector.
 * @param[in]     pos   Bit position (0 = MSB, length-1 = LSB).
 * @param[in]     value Non-zero to set bit, zero to clear.
 *
 * @note No-op if bv is NULL or pos is out of range.
 */
void bitvector_set_bit(bitvector_t *bv, size_t pos, int value) {
    if (bv == NULL || pos >= bv->length) {
        return;
    }

    /* Calculate byte and bit within byte */
    size_t byte_index = pos / 8;
    size_t bit_in_byte = pos % 8;

    /* Calculate word index and byte position within word */
    size_t word_index = byte_index / 4;
    size_t byte_in_word = byte_index % 4;

    /* Big-endian: byte 0 is at bits 24-31, byte 1 at 16-23, etc.
     * MSB-first indexing: bit 0 is the MSB (leftmost) within each byte
     * This matches the reference implementation */
    size_t bit_in_word = (3 - byte_in_word) * 8 + (7 - bit_in_byte);

    if (value) {
        /* Set bit to 1 */
        bv->data[word_index] |= (1U << bit_in_word);
    } else {
        /* Clear bit to 0 */
        bv->data[word_index] &= ~(1U << bit_in_word);
    }
}

/** @} */ /* End of Bit Access Functions */

/**
 * @name Bitwise Operations
 *
 * Implements bitwise operations as defined in CCSDS 124.0-B-1 Section 1.6.1.
 * All operations work word-by-word for efficiency.
 * @{
 */

/**
 * @brief Compute bitwise XOR of two bit vectors.
 *
 * Computes result = a XOR b element-wise.
 *
 * @param[out] result Pointer to result bit vector.
 * @param[in]  a      Pointer to first operand.
 * @param[in]  b      Pointer to second operand.
 *
 * @note Result inherits length from operand a. No-op if any pointer is NULL.
 */
void bitvector_xor(bitvector_t *result, const bitvector_t *a, const bitvector_t *b) {
    if (result == NULL || a == NULL || b == NULL) {
        return;
    }

    size_t num_words = a->num_words;
    if (b->num_words < num_words) {
        num_words = b->num_words;
    }

    for (size_t i = 0; i < num_words; i++) {
        result->data[i] = a->data[i] ^ b->data[i];
    }

    result->length = a->length;
    result->num_words = a->num_words;
}

/**
 * @brief Compute bitwise OR of two bit vectors.
 *
 * Computes result = a OR b element-wise.
 *
 * @param[out] result Pointer to result bit vector.
 * @param[in]  a      Pointer to first operand.
 * @param[in]  b      Pointer to second operand.
 *
 * @note Result inherits length from operand a. No-op if any pointer is NULL.
 */
void bitvector_or(bitvector_t *result, const bitvector_t *a, const bitvector_t *b) {
    if (result == NULL || a == NULL || b == NULL) {
        return;
    }

    size_t num_words = a->num_words;
    if (b->num_words < num_words) {
        num_words = b->num_words;
    }

    for (size_t i = 0; i < num_words; i++) {
        result->data[i] = a->data[i] | b->data[i];
    }

    result->length = a->length;
    result->num_words = a->num_words;
}

/**
 * @brief Compute bitwise AND of two bit vectors.
 *
 * Computes result = a AND b element-wise.
 *
 * @param[out] result Pointer to result bit vector.
 * @param[in]  a      Pointer to first operand.
 * @param[in]  b      Pointer to second operand.
 *
 * @note Result inherits length from operand a. No-op if any pointer is NULL.
 */
void bitvector_and(bitvector_t *result, const bitvector_t *a, const bitvector_t *b) {
    if (result == NULL || a == NULL || b == NULL) {
        return;
    }

    size_t num_words = a->num_words;
    if (b->num_words < num_words) {
        num_words = b->num_words;
    }

    for (size_t i = 0; i < num_words; i++) {
        result->data[i] = a->data[i] & b->data[i];
    }

    result->length = a->length;
    result->num_words = a->num_words;
}

/**
 * @brief Compute bitwise NOT (complement) of a bit vector.
 *
 * Computes result = NOT a, inverting all bits. Unused bits in the
 * last word are masked off to maintain valid vector state.
 *
 * @param[out] result Pointer to result bit vector.
 * @param[in]  a      Pointer to operand.
 *
 * @note Result inherits length from operand a. No-op if any pointer is NULL.
 */
void bitvector_not(bitvector_t *result, const bitvector_t *a) {
    if (result == NULL || a == NULL) {
        return;
    }

    for (size_t i = 0; i < a->num_words; i++) {
        result->data[i] = ~a->data[i];
    }

    /* Mask off unused bits in last word with big-endian packing */
    if (a->num_words > 0) {
        size_t num_bytes = (a->length + 7) / 8;
        size_t bytes_in_last_word = ((num_bytes - 1) % 4) + 1;
        size_t bits_in_last_byte = a->length - ((num_bytes - 1) * 8);

        /* Create mask for valid bits in big-endian word */
        uint32_t mask = 0;
        for (size_t byte = 0; byte < bytes_in_last_word; byte++) {
            uint8_t byte_mask = (byte == bytes_in_last_word - 1) ?
                               ((1U << bits_in_last_byte) - 1) : 0xFF;
            mask |= ((uint32_t)byte_mask) << ((3 - byte) * 8);
        }
        result->data[a->num_words - 1] &= mask;
    }

    result->length = a->length;
    result->num_words = a->num_words;
}

/**
 * @brief Left shift bit vector by 1 bit.
 *
 * Performs result = a << 1, inserting 0 at LSB.
 * With MSB-first indexing, this shifts bits towards lower indices (towards MSB).
 *
 * @par Example
 * @code
 * 10110011 << 1 = 01100110
 * @endcode
 *
 * Bit 0 (MSB) receives bit from bit 1, LSB (bit N-1) becomes 0.
 *
 * @param[out] result Pointer to result bit vector.
 * @param[in]  a      Pointer to operand.
 *
 * @note No-op if any pointer is NULL.
 */
void bitvector_left_shift(bitvector_t *result, const bitvector_t *a) {
    if (result == NULL || a == NULL) {
        return;
    }

    bitvector_zero(result);
    result->length = a->length;
    result->num_words = a->num_words;

    /* MSB-first: left shift means shift towards MSB (lower indices)
     * Bit 0 → MSB, Bit N-1 → LSB
     * Left shift: move all bits one position towards MSB, insert 0 at LSB */
    for (size_t i = 0; i < a->length - 1; i++) {
        int bit = bitvector_get_bit(a, i + 1);
        bitvector_set_bit(result, i, bit);
    }
    bitvector_set_bit(result, a->length - 1, 0);  /* Clear LSB */
}

/**
 * @brief Reverse the bit order in a bit vector.
 *
 * Reverses all bits so that bit 0 becomes bit N-1 and vice versa.
 *
 * @param[out] result Pointer to result bit vector.
 * @param[in]  a      Pointer to operand.
 *
 * @note No-op if any pointer is NULL.
 */
void bitvector_reverse(bitvector_t *result, const bitvector_t *a) {
    if (result == NULL || a == NULL) {
        return;
    }

    bitvector_zero(result);
    result->length = a->length;
    result->num_words = a->num_words;

    /* Reverse bit order */
    for (size_t i = 0; i < a->length; i++) {
        int bit = bitvector_get_bit(a, i);
        bitvector_set_bit(result, a->length - 1 - i, bit);
    }
}

/** @} */ /* End of Bitwise Operations */

/**
 * @name Utility Functions
 * @{
 */

/**
 * @brief Count the number of set bits (Hamming weight).
 *
 * Counts the number of '1' bits in the vector using Brian Kernighan's
 * algorithm for efficient bit counting.
 *
 * @param[in] bv Pointer to bit vector.
 *
 * @return Number of bits set to 1, or 0 if bv is NULL.
 */
size_t bitvector_hamming_weight(const bitvector_t *bv) {
    if (bv == NULL) {
        return 0;
    }

    size_t count = 0;

    /* Count '1' bits in each word */
    for (size_t i = 0; i < bv->num_words; i++) {
        uint32_t word = bv->data[i];

        /* Brian Kernighan's algorithm */
        while (word) {
            word &= (word - 1);
            count++;
        }
    }

    /* Adjust for any extra bits in last word */
    size_t num_bytes = (bv->length + 7) / 8;
    size_t extra_bits = (num_bytes * 8) - bv->length;
    if (extra_bits > 0 && bv->num_words > 0) {
        /* Count bits in the unused portion of the last word and subtract */
        uint32_t last_word = bv->data[bv->num_words - 1];
        uint32_t mask = ((1U << extra_bits) - 1);  /* Mask for the unused LSBs */
        uint32_t extra_word = last_word & mask;

        /* Subtract any '1' bits in the extra portion */
        while (extra_word) {
            extra_word &= (extra_word - 1);
            count--;
        }
    }

    return count;
}

/**
 * @brief Compare two bit vectors for equality.
 *
 * Compares length and data of two bit vectors.
 *
 * @param[in] a Pointer to first bit vector.
 * @param[in] b Pointer to second bit vector.
 *
 * @return 1 if vectors are equal, 0 if different or either is NULL.
 */
int bitvector_equals(const bitvector_t *a, const bitvector_t *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }

    if (a->length != b->length) {
        return 0;
    }

    return memcmp(a->data, b->data, a->num_words * sizeof(uint32_t)) == 0;
}

/** @} */ /* End of Utility Functions */

/**
 * @name Byte Conversion Functions
 * @{
 */

/**
 * @brief Load bytes into bit vector with big-endian word packing.
 *
 * Packs input bytes into 32-bit words matching ESA/ESOC reference
 * implementation (pocket_compress.c lines 216-225).
 *
 * Each 32-bit word packs 4 bytes as: (B0<<24) | (B1<<16) | (B2<<8) | B3
 *
 * @param[out] bv        Pointer to initialized bit vector.
 * @param[in]  data      Pointer to source byte array.
 * @param[in]  num_bytes Number of bytes to load.
 *
 * @return POCKET_OK on success.
 * @return POCKET_ERROR_INVALID_ARG if bv or data is NULL.
 * @return POCKET_ERROR_OVERFLOW if num_bytes exceeds vector capacity.
 */
int bitvector_from_bytes(bitvector_t *bv, const uint8_t *data, size_t num_bytes) {
    if (bv == NULL || data == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    size_t expected_bytes = (bv->length + 7) / 8;
    if (num_bytes > expected_bytes) {
        return POCKET_ERROR_OVERFLOW;
    }

    /* Zero the array first */
    memset(bv->data, 0, bv->num_words * sizeof(uint32_t));

    /* Pack bytes into 32-bit words (big-endian) */
    int j = 4;  /* Counter for bytes within word (4, 3, 2, 1) */
    uint32_t bytes_to_int = 0;
    size_t current_word = 0;

    for (size_t current_byte = 0; current_byte < num_bytes; current_byte++) {
        j--;
        bytes_to_int |= ((uint32_t)data[current_byte]) << (j * 8);

        if (j == 0) {
            /* Word complete - store it */
            bv->data[current_word] = bytes_to_int;
            current_word++;
            bytes_to_int = 0;
            j = 4;
        }
    }

    /* Handle incomplete final word */
    if (j < 4) {
        bv->data[current_word] = bytes_to_int;
    }

    return POCKET_OK;
}

/**
 * @brief Extract bytes from bit vector (reverse of big-endian word packing).
 *
 * Unpacks 32-bit words into individual bytes.
 *
 * @param[in]  bv        Pointer to source bit vector.
 * @param[out] data      Pointer to destination byte array.
 * @param[in]  num_bytes Size of destination buffer.
 *
 * @return POCKET_OK on success.
 * @return POCKET_ERROR_INVALID_ARG if bv or data is NULL.
 * @return POCKET_ERROR_UNDERFLOW if buffer is too small.
 */
int bitvector_to_bytes(const bitvector_t *bv, uint8_t *data, size_t num_bytes) {
    if (bv == NULL || data == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    size_t expected_bytes = (bv->length + 7) / 8;
    if (num_bytes < expected_bytes) {
        return POCKET_ERROR_UNDERFLOW;
    }

    /* Extract bytes from 32-bit words (big-endian) */
    size_t byte_index = 0;
    for (size_t word_index = 0; word_index < bv->num_words && byte_index < num_bytes; word_index++) {
        uint32_t word = bv->data[word_index];

        /* Extract up to 4 bytes from this word */
        for (int j = 3; j >= 0 && byte_index < expected_bytes; j--) {
            data[byte_index++] = (word >> (j * 8)) & 0xFF;
        }
    }

    return POCKET_OK;
}

/** @} */ /* End of Byte Conversion Functions */
