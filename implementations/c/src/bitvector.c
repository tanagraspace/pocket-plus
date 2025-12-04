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
 * POCKET+ C Implementation - Bit Vector (32-bit Word-Based)
 *
 * Authors:
 *   Georges Labrèche <georges@tanagraspace.com>
 *   Claude Code (claude-sonnet-4-5-20250929) <noreply@anthropic.com>
 *
 * Uses 32-bit words with big-endian byte packing to match ESA/ESOC reference.
 *
 * Bit numbering convention (CCSDS 124.0-B-1 Section 1.6.1):
 *   - Bit 0 = LSB (Least Significant Bit)
 *   - Bit N-1 = MSB (Most Significant Bit, transmitted first)
 *
 * Within each 32-bit word (big-endian byte packing):
 *   - Word[i] = (Byte[4i] << 24) | (Byte[4i+1] << 16) | (Byte[4i+2] << 8) | Byte[4i+3]
 *   - Bit 0 = LSB of word, Bit 31 = MSB of word
 * ============================================================================
 */

#include "pocket_plus.h"
#include <string.h>

/* ========================================================================
 * Initialization
 * ======================================================================== */

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

void bitvector_zero(bitvector_t *bv) {
    if (bv != NULL) {
        memset(bv->data, 0, bv->num_words * sizeof(uint32_t));
    }
}

void bitvector_copy(bitvector_t *dest, const bitvector_t *src) {
    if (dest != NULL && src != NULL) {
        dest->length = src->length;
        dest->num_words = src->num_words;
        memcpy(dest->data, src->data, src->num_words * sizeof(uint32_t));
    }
}

/* ========================================================================
 * Bit Access
 *
 * Big-endian word packing maps bytes to word bits as follows:
 *   Word[i] contains Bytes [4i, 4i+1, 4i+2, 4i+3]
 *   - Byte 4i   at bits 24-31 (most significant)
 *   - Byte 4i+1 at bits 16-23
 *   - Byte 4i+2 at bits 8-15
 *   - Byte 4i+3 at bits 0-7   (least significant)
 * ======================================================================== */

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

/* ========================================================================
 * Bitwise Operations (CCSDS Section 1.6.1)
 * ======================================================================== */

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
 * Left shift by 1 bit (inserts 0 at LSB): result = a << 1
 * With MSB-first indexing: shifts towards lower bit indices (towards MSB)
 * Example: 10110011 << 1 = 01100110
 * Bit 0 (MSB) receives bit from bit 1, LSB (bit N-1) becomes 0
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

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

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

int bitvector_equals(const bitvector_t *a, const bitvector_t *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }

    if (a->length != b->length) {
        return 0;
    }

    return memcmp(a->data, b->data, a->num_words * sizeof(uint32_t)) == 0;
}

/* ========================================================================
 * Byte Conversion
 * ======================================================================== */

/**
 * Load bytes into bitvector with big-endian word packing.
 * Matches ESA/ESOC reference implementation (pocket_compress.c lines 216-225).
 *
 * Each 32-bit word packs 4 bytes as: (B0<<24) | (B1<<16) | (B2<<8) | B3
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
 * Extract bytes from bitvector (reverse of big-endian word packing).
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

/* ========================================================================
 * Debug Output
 * ======================================================================== */

void bitvector_print(const bitvector_t *bv, FILE *fp) {
    if (bv == NULL || fp == NULL) {
        return;
    }

    /* Print from MSB to LSB */
    for (size_t i = bv->length; i > 0; i--) {
        fprintf(fp, "%d", bitvector_get_bit(bv, i - 1));

        /* Add space every 8 bits for readability */
        if ((i - 1) % 8 == 0 && i > 1) {
            fprintf(fp, " ");
        }
    }
}
