/**
 * @file encode.c
 * @brief POCKET+ encoding functions (COUNT, RLE, BE).
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
 * Implements CCSDS 124.0-B-1 Section 5.2 encoding schemes:
 * - Counter Encoding (COUNT) - Section 5.2.2, Table 5-1, Equation 9
 * - Run-Length Encoding (RLE) - Section 5.2.3, Equation 10
 * - Bit Extraction (BE) - Section 5.2.4, Equation 11
 *
 * @authors Georges Labrèche <georges@tanagraspace.com> — https://georges.fyi
 * @authors Claude Code (Anthropic) <noreply@anthropic.com>
 *
 * @see https://public.ccsds.org/Pubs/124x0b1.pdf CCSDS 124.0-B-1 Standard
 */

#include "pocketplus.h"
#include <math.h>

/**
 * @name Counter Encoding (COUNT)
 *
 * CCSDS Section 5.2.2, Table 5-1, Equation 9.
 * Encodes positive integers 1 ≤ A ≤ 2^16 - 1:
 * - A = 1 → '0'
 * - 2 ≤ A ≤ 33 → '110' ∥ BIT₅(A-2)
 * - A ≥ 34 → '111' ∥ BIT_E(A-2) where E = 2⌊log₂(A-2)+1⌋ - 6
 * @{
 */


int pocket_count_encode(bitbuffer_t *output, uint32_t A) {
    if (output == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    if (A == 0 || A > 65535) {
        return POCKET_ERROR_INVALID_ARG;
    }

    int result;

    if (A == 1) {
        /* Case 1: A = 1 → '0' */
        result = bitbuffer_append_bit(output, 0);
        return result;
    }

    if (A >= 2 && A <= 33) {
        /* Case 2: 2 ≤ A ≤ 33 → '110' ∥ BIT₅(A-2) */

        /* Append '110' MSB-first: 1, 1, 0 */
        result = bitbuffer_append_bit(output, 1);
        if (result != POCKET_OK) return result;

        result = bitbuffer_append_bit(output, 1);
        if (result != POCKET_OK) return result;

        result = bitbuffer_append_bit(output, 0);
        if (result != POCKET_OK) return result;

        /* Append BIT₅(A-2) MSB-first - 5 bits from bit 4 down to bit 0 */
        uint32_t value = A - 2;
        for (int i = 4; i >= 0; i--) {
            int bit = (value >> i) & 1;
            result = bitbuffer_append_bit(output, bit);
            if (result != POCKET_OK) return result;
        }

        return POCKET_OK;
    }

    /* Case 3: A ≥ 34 → '111' ∥ BIT_E(A-2) */

    /* Append '111' MSB-first: 1, 1, 1 (same in both orders) */
    result = bitbuffer_append_bit(output, 1);
    if (result != POCKET_OK) return result;

    result = bitbuffer_append_bit(output, 1);
    if (result != POCKET_OK) return result;

    result = bitbuffer_append_bit(output, 1);
    if (result != POCKET_OK) return result;

    /* Calculate E = 2⌊log₂(A-2)+1⌋ - 6 */
    uint32_t value = A - 2;
    double log_val = log2((double)value);
    int E = 2 * ((int)floor(log_val) + 1) - 6;

    /* Append BIT_E(A-2) MSB-first - E bits from bit E-1 down to bit 0 */
    for (int i = E - 1; i >= 0; i--) {
        int bit = (value >> i) & 1;
        result = bitbuffer_append_bit(output, bit);
        if (result != POCKET_OK) return result;
    }

    return POCKET_OK;
}

/** @} */ /* End of Counter Encoding */

/**
 * @name Run-Length Encoding (RLE)
 *
 * CCSDS Section 5.2.3, Equation 10.
 * RLE(a) = COUNT(C₀) ∥ COUNT(C₁) ∥ ... ∥ COUNT(C_{H(a)-1}) ∥ '10'
 *
 * where Cᵢ = 1 + (count of consecutive '0' bits before i-th '1' bit)
 * and H(a) = Hamming weight (number of '1' bits in a)
 *
 * @note Trailing zeros are not encoded (deducible from vector length)
 * @{
 */


int pocket_rle_encode(bitbuffer_t *output, const bitvector_t *input) {
    if (output == NULL || input == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* DeBruijn lookup table for fast LSB finding (matches reference implementation) */
    static const uint32_t debruijn_lookup[32] = {
        1, 2, 29, 3, 30, 15, 25, 4, 31, 23, 21, 16,
        26, 18, 5, 9, 32, 28, 14, 24, 22, 20, 17, 8,
        27, 13, 19, 7, 12, 6, 11, 10
    };

    /* Start from the end of the vector */
    int old_bit_position = input->length;

    /* Process words in reverse order (from high to low) */
    for (int word = (int)input->num_words - 1; word >= 0; word--) {
        uint32_t word_data = input->data[word];

        /* Process all set bits in this word */
        while (word_data != 0) {
            /* Isolate the LSB: x = change & -change */
            uint32_t lsb = word_data & (uint32_t)(-(int32_t)word_data);

            /* Find LSB position using DeBruijn sequence */
            uint32_t debruijn_index = (lsb * 0x077CB531U) >> 27;
            int bit_position_in_word = debruijn_lookup[debruijn_index];

            /* Count from the other side (reference line 754) */
            bit_position_in_word = 32 - bit_position_in_word;

            /* Calculate global bit position (reference line 756) */
            int new_bit_position = word * 32 + bit_position_in_word;

            /* Calculate delta (number of zeros + 1) */
            int delta = old_bit_position - new_bit_position;

            /* Encode the count */
            int result = pocket_count_encode(output, (uint32_t)delta);
            if (result != POCKET_OK) return result;

            /* Update old position for next iteration */
            old_bit_position = new_bit_position;

            /* Clear the processed bit */
            word_data ^= lsb;
        }
    }

    /* Append terminator '10' MSB-first (bits: 1, 0) */
    int result = bitbuffer_append_bit(output, 1);
    if (result != POCKET_OK) return result;

    result = bitbuffer_append_bit(output, 0);

    return result;
}

/** @} */ /* End of Run-Length Encoding */

/**
 * @name Bit Extraction (BE)
 *
 * CCSDS Section 5.2.4, Equation 11.
 * BE(a, b) = a_{g_{H(b)-1}} ∥ ... ∥ a_{g₁} ∥ a_{g₀}
 *
 * where gᵢ is the position of the i-th '1' bit in b (MSB to LSB order)
 *
 * Extracts bits from 'a' at positions where 'b' has '1' bits.
 * Output order: MSB to LSB (reverse order of finding '1' bits)
 *
 * @par Example
 * @code
 * BE('10110011', '01001010') = '001'
 * Positions with '1' in mask: 1, 3, 6
 * Extract from data: bit[1]=1, bit[3]=0, bit[6]=0
 * Output (MSB→LSB): 0, 0, 1
 * @endcode
 * @{
 */


int pocket_bit_extract(bitbuffer_t *output, const bitvector_t *data, const bitvector_t *mask) {
    if (output == NULL || data == NULL || mask == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    if (data->length != mask->length) {
        return POCKET_ERROR_INVALID_ARG;
    }

    size_t hamming_weight = bitvector_hamming_weight(mask);

    if (hamming_weight == 0) {
        /* No bits to extract */
        return POCKET_OK;
    }

    /* Collect positions of '1' bits in mask */
    size_t positions[POCKET_MAX_PACKET_LENGTH];
    size_t pos_count = 0;

    for (size_t i = 0; i < mask->length && pos_count < hamming_weight; i++) {
        if (bitvector_get_bit(mask, i)) {
            positions[pos_count++] = i;
        }
    }

    /* Extract bits in reverse order (highest position to lowest)
     * With MSB-first indexing, higher bit indices are closer to LSB
     * CCSDS BE(a,b) extracts from highest to lowest position */
    for (size_t i = pos_count; i > 0; i--) {
        size_t pos = positions[i - 1];
        int bit = bitvector_get_bit(data, pos);

        int result = bitbuffer_append_bit(output, bit);
        if (result != POCKET_OK) return result;
    }

    return POCKET_OK;
}


int pocket_bit_extract_forward(bitbuffer_t *output, const bitvector_t *data, const bitvector_t *mask) {
    if (output == NULL || data == NULL || mask == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    if (data->length != mask->length) {
        return POCKET_ERROR_INVALID_ARG;
    }

    size_t hamming_weight = bitvector_hamming_weight(mask);

    if (hamming_weight == 0) {
        /* No bits to extract */
        return POCKET_OK;
    }

    /* Collect positions of '1' bits in mask */
    size_t positions[POCKET_MAX_PACKET_LENGTH];
    size_t pos_count = 0;

    for (size_t i = 0; i < mask->length && pos_count < hamming_weight; i++) {
        if (bitvector_get_bit(mask, i)) {
            positions[pos_count++] = i;
        }
    }

    /* Extract bits in FORWARD order (lowest position to highest)
     * For kt component: processes mask values at changed positions
     * in order from lowest position index to highest */
    for (size_t i = 0; i < pos_count; i++) {
        size_t pos = positions[i];
        int bit = bitvector_get_bit(data, pos);

        int result = bitbuffer_append_bit(output, bit);
        if (result != POCKET_OK) return result;
    }

    return POCKET_OK;
}

/** @} */ /* End of Bit Extraction */
