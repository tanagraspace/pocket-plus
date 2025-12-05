/**
 * @file compress.c
 * @brief POCKET+ compression algorithm implementation.
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
 * Implements CCSDS 124.0-B-1 Section 5.3 (Encoding Step):
 * - Compressor initialization and state management
 * - Main compression algorithm
 * - Output packet encoding: oₜ = hₜ ∥ qₜ ∥ uₜ
 *
 * @authors Georges Labrèche <georges@tanagraspace.com> — https://georges.fyi
 * @authors Claude Code (Anthropic) <noreply@anthropic.com>
 *
 * @see https://public.ccsds.org/Pubs/124x0b1.pdf CCSDS 124.0-B-1 Standard
 */

#include "pocketplus.h"
#include <string.h>

/**
 * @name Compressor Initialization
 * @{
 */


int pocket_compressor_init(
    pocket_compressor_t *comp,
    size_t F,
    const bitvector_t *initial_mask,
    uint8_t robustness,
    int pt_limit,
    int ft_limit,
    int rt_limit
) {
    if (comp == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    if (F == 0 || F > POCKET_MAX_PACKET_LENGTH) {
        return POCKET_ERROR_INVALID_ARG;
    }

    if (robustness > POCKET_MAX_ROBUSTNESS) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Store configuration */
    comp->F = F;
    comp->robustness = robustness;
    comp->pt_limit = pt_limit;
    comp->ft_limit = ft_limit;
    comp->rt_limit = rt_limit;

    /* Initialize all bit vectors */
    bitvector_init(&comp->mask, F);
    bitvector_init(&comp->prev_mask, F);
    bitvector_init(&comp->build, F);
    bitvector_init(&comp->prev_input, F);
    bitvector_init(&comp->initial_mask, F);

    /* Initialize change history */
    for (int i = 0; i < POCKET_MAX_HISTORY; i++) {
        bitvector_init(&comp->change_history[i], F);
        bitvector_zero(&comp->change_history[i]);
    }

    /* Initialize flag history for cₜ calculation */
    for (int i = 0; i < POCKET_MAX_VT_HISTORY; i++) {
        comp->new_mask_flag_history[i] = 0;
    }
    comp->flag_history_index = 0;

    /* Set initial mask */
    if (initial_mask != NULL) {
        bitvector_copy(&comp->initial_mask, initial_mask);
        bitvector_copy(&comp->mask, initial_mask);
    } else {
        bitvector_zero(&comp->initial_mask);
        bitvector_zero(&comp->mask);
    }

    /* Reset state */
    pocket_compressor_reset(comp);

    return POCKET_OK;
}


void pocket_compressor_reset(pocket_compressor_t *comp) {
    /* Reset time index */
    comp->t = 0;
    comp->history_index = 0;

    /* Reset mask to initial */
    bitvector_copy(&comp->mask, &comp->initial_mask);
    bitvector_zero(&comp->prev_mask);

    /* Clear build and prev_input */
    bitvector_zero(&comp->build);
    bitvector_zero(&comp->prev_input);

    /* Clear change history */
    for (int i = 0; i < POCKET_MAX_HISTORY; i++) {
        bitvector_zero(&comp->change_history[i]);
    }

    /* Reset countdown counters (match reference implementation behavior) */
    comp->pt_counter = comp->pt_limit;
    comp->ft_counter = comp->ft_limit;
    comp->rt_counter = comp->rt_limit;
}

/** @} */ /* End of Compressor Initialization */

/**
 * @name Internal Helper Functions
 * @{
 */

/**
 * @brief Get default compression parameters.
 *
 * Initializes parameters with robustness from compressor and all flags cleared.
 *
 * @param[out] params Pointer to parameters structure.
 * @param[in]  comp   Pointer to compressor for robustness level.
 *
 * @note Caller should handle CCSDS initialization requirements
 *       (ft=1, rt=1 for first Rt+1 packets).
 */
static void get_default_params(
    pocket_params_t *params,
    const pocket_compressor_t *comp
) {
    /* Default parameters */
    params->min_robustness = comp->robustness;
    params->new_mask_flag = 0;
    params->send_mask_flag = 0;
    params->uncompressed_flag = 0;

    /* Note: Caller should handle CCSDS initialization requirements
     * (ft=1, rt=1 for first Rt+1 packets) */
}

/** @} */ /* End of Internal Helper Functions */

/**
 * @name CCSDS Helper Functions
 * @{
 */


void pocket_compute_robustness_window(
    bitvector_t *Xt,
    const pocket_compressor_t *comp,
    const bitvector_t *current_change
) {
    /* Xₜ = <(Dₜ₋ᴿₜ OR Dₜ₋ᴿₜ₊₁ OR ... OR Dₜ)>
     * Where <a> means reverse the bit order */

    bitvector_init(Xt, comp->F);

    if (comp->robustness == 0 || comp->t == 0) {
        /* Xₜ = Dₜ (no reversal - RLE processes LSB to MSB directly) */
        bitvector_copy(Xt, current_change);
        return;
    }

    /* OR together changes from t-Rt to t */
    bitvector_t combined;
    bitvector_init(&combined, comp->F);
    bitvector_copy(&combined, current_change);

    /* Determine how many historical changes to include */
    size_t num_changes = (comp->t < comp->robustness) ? comp->t : comp->robustness;

    /* OR with historical changes (going backwards from current) */
    for (size_t i = 1; i <= num_changes; i++) {
        /* Calculate index of change from i iterations ago */
        size_t hist_idx = (comp->history_index - i + POCKET_MAX_HISTORY) % POCKET_MAX_HISTORY;

        bitvector_t temp;
        bitvector_init(&temp, comp->F);
        bitvector_or(&temp, &combined, &comp->change_history[hist_idx]);
        bitvector_copy(&combined, &temp);
    }

    /* Don't reverse - RLE will process from LSB to MSB directly */
    bitvector_copy(Xt, &combined);
}


uint8_t pocket_compute_effective_robustness(
    const pocket_compressor_t *comp,
    const bitvector_t *current_change
) {
    /* Vₜ = Rₜ + Cₜ (CCSDS Section 5.3.2.2)
     * where Cₜ = number of consecutive iterations with no mask changes
     *
     * For t ≤ Rt: Vt = Rt
     * For t > Rt: Vt = Rt + Ct */

    (void)current_change;  /* Unused - Ct is computed from history */

    uint8_t Rt = comp->robustness;
    uint8_t Vt = Rt;

    /* For t ≤ Rt, return Rt directly */
    if (comp->t <= Rt) {
        return Vt;
    }

    /* For t > Rt, compute Ct by counting consecutive iterations with no changes
     * Reference implementation starts at position Rt+1 earlier (not t-1 or t-2)
     * and counts consecutive "no change" entries */
    uint8_t Ct = 0;

    /* Count backwards through history starting from Rt+1 positions back
     * (matching reference: start = pt_history_index + Rt + 1) */
    for (size_t i = Rt + 1; i <= 15 && i <= comp->t; i++) {
        size_t hist_idx = (comp->history_index - i + POCKET_MAX_HISTORY) % POCKET_MAX_HISTORY;
        if (bitvector_hamming_weight(&comp->change_history[hist_idx]) > 0) {
            break;  /* Found a change, stop counting */
        }
        Ct++;
        if (Ct >= 15 - Rt) {
            break;  /* Cap at maximum Ct value */
        }
    }

    Vt = Rt + Ct;
    return (Vt > 15) ? 15 : Vt;  /* Cap at 15 (4 bits) */
}


int pocket_has_positive_updates(
    const bitvector_t *Xt,
    const bitvector_t *mask
) {
    /* eₜ = 1 if any changed bits (in Xₜ) are predictable (mask bit = 0)
     * Xₜ is not reversed, so check directly */

    /* Check each changed bit */
    for (size_t i = 0; i < Xt->length; i++) {
        int bit_changed = bitvector_get_bit(Xt, i);
        int bit_predictable = !bitvector_get_bit(mask, i);  /* mask=0 means predictable */

        if (bit_changed && bit_predictable) {
            return 1;  /* Found a positive update (unpredictable → predictable) */
        }
    }

    return 0;  /* No positive updates */
}


int pocket_compute_ct_flag(
    const pocket_compressor_t *comp,
    uint8_t Vt,
    int current_new_mask_flag
) {
    /* cₜ = 1 if new_mask_flag was set 2+ times in last Vₜ+1 iterations
     * (including current packet) - matches reference implementation */

    if (Vt == 0) {
        return 0;
    }

    /* Count how many times new_mask_flag was set.
     * Reference checks from current (i=pt_history_index) to current+Vt,
     * which is Vt+1 entries including current packet */
    int count = 0;

    /* Include current packet's flag */
    if (current_new_mask_flag) {
        count++;
    }

    /* Check history for Vt previous entries */
    size_t iterations_to_check = (Vt <= comp->t) ? Vt : comp->t;

    for (size_t i = 0; i < iterations_to_check; i++) {
        /* Calculate history index going backwards from previous */
        size_t hist_idx = (comp->flag_history_index - 1 - i + POCKET_MAX_VT_HISTORY) % POCKET_MAX_VT_HISTORY;
        if (comp->new_mask_flag_history[hist_idx]) {
            count++;
        }
    }

    return (count >= 2) ? 1 : 0;
}

/** @} */ /* End of CCSDS Helper Functions */

/**
 * @name Main Compression Functions
 * @{
 */


int pocket_compress_packet(
    pocket_compressor_t *comp,
    const bitvector_t *input,
    bitbuffer_t *output,
    const pocket_params_t *params
) {
    if (comp == NULL || input == NULL || output == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Validate input length */
    if (input->length != comp->F) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Get parameters (use defaults if NULL) */
    pocket_params_t local_params;
    if (params == NULL) {
        get_default_params(&local_params, comp);
        params = &local_params;
    }

    /* Clear output buffer */
    bitbuffer_clear(output);

    /* ====================================================================
     * STEP 1: Update Mask and Build Vectors (CCSDS Section 4)
     * ==================================================================== */

    bitvector_t prev_mask;
    bitvector_init(&prev_mask, comp->F);
    bitvector_copy(&prev_mask, &comp->mask);

    bitvector_t prev_build;
    bitvector_init(&prev_build, comp->F);
    bitvector_copy(&prev_build, &comp->build);

    /* Update build vector (Equation 6) */
    if (comp->t > 0) {
        pocket_update_build(&comp->build, input, &comp->prev_input,
                           params->new_mask_flag, comp->t);
    }

    /* Update mask vector (Equation 7) */
    if (comp->t > 0) {
        pocket_update_mask(&comp->mask, input, &comp->prev_input,
                          &prev_build, params->new_mask_flag);
    }

    /* Compute change vector (Equation 8) */
    bitvector_t change;
    bitvector_init(&change, comp->F);

    pocket_compute_change(&change, &comp->mask, &prev_mask, comp->t);

    /* Store change in history (circular buffer) */
    bitvector_copy(&comp->change_history[comp->history_index], &change);

    /* ====================================================================
     * STEP 2: Encode Output Packet (CCSDS Section 5.3)
     * oₜ = hₜ ∥ qₜ ∥ uₜ
     * Full CCSDS encoding per ALGORITHM.md
     * ==================================================================== */

    /* Calculate Xₜ (robustness window) */
    bitvector_t Xt;
    pocket_compute_robustness_window(&Xt, comp, &change);

    /* Calculate Vₜ (effective robustness) */
    uint8_t Vt = pocket_compute_effective_robustness(comp, &change);

    /* Calculate ḋₜ flag */
    uint8_t dt = (params->send_mask_flag == 0 && params->uncompressed_flag == 0) ? 1 : 0;

    /* ====================================================================
     * Component hₜ: Mask change information
     * hₜ = RLE(Xₜ) ∥ BIT₄(Vₜ) ∥ eₜ ∥ kₜ ∥ cₜ ∥ ḋₜ
     * ==================================================================== */

    /* 1. RLE(Xₜ) - Run-length encode the robustness window */
    pocket_rle_encode(output, &Xt);

    /* 2. BIT₄(Vₜ) - 4-bit effective robustness level
     * CCSDS encodes Vt directly (reference implementation confirmed) */
    for (int i = 3; i >= 0; i--) {
        bitbuffer_append_bit(output, (Vt >> i) & 1);
    }

    /* 3. eₜ, kₜ, cₜ - Only if Vₜ > 0 and there are mask changes */
    if (Vt > 0 && bitvector_hamming_weight(&Xt) > 0) {
        /* Calculate eₜ */
        int et = pocket_has_positive_updates(&Xt, &comp->mask);

        bitbuffer_append_bit(output, et);

        if (et == 1) {
            /* kₜ - Output '1' for positive updates (mask=0), '0' for negative updates (mask=1)
             * Reference implementation shows kt outputs 1 when mask bit is 0 at changed positions */

            /* Extract INVERTED mask values (1 where mask=0, 0 where mask=1)
             * Use forward order (lowest position to highest) for kt */
            bitvector_t inverted_mask;
            bitvector_init(&inverted_mask, comp->F);
            for (size_t i = 0; i < comp->mask.length; i++) {
                bitvector_set_bit(&inverted_mask, i, !bitvector_get_bit(&comp->mask, i));
            }

            pocket_bit_extract_forward(output, &inverted_mask, &Xt);

            /* Calculate and encode cₜ */
            int ct = pocket_compute_ct_flag(comp, Vt, params->new_mask_flag);

            bitbuffer_append_bit(output, ct);
        }
    }

    /* 4. ḋₜ - Flag indicating if both ḟₜ and ṙₜ are zero */
    bitbuffer_append_bit(output, dt);

    /* ====================================================================
     * Component qₜ: Optional full mask
     * qₜ = ∅ if ḋₜ=1, '1' ∥ RLE(<(Mₜ XOR (Mₜ≪))>) if ḟₜ=1, '0' otherwise
     * ==================================================================== */

    if (dt == 0) {  /* Only if ḋₜ = 0 */
        if (params->send_mask_flag) {
            bitbuffer_append_bit(output, 1);  /* Flag: mask follows */

            /* Encode mask as RLE(M XOR (M<<)) - no reversal needed */
            bitvector_t mask_shifted, mask_diff;
            bitvector_init(&mask_shifted, comp->F);
            bitvector_init(&mask_diff, comp->F);

            bitvector_left_shift(&mask_shifted, &comp->mask);
            bitvector_xor(&mask_diff, &comp->mask, &mask_shifted);

            pocket_rle_encode(output, &mask_diff);
        } else {
            bitbuffer_append_bit(output, 0);  /* Flag: no mask */
        }
    }

    /* ====================================================================
     * Component uₜ: Unpredictable bits or full input
     * Per ALGORITHM.md, depends on ḋₜ, ṙₜ, cₜ
     * ==================================================================== */

    if (params->uncompressed_flag) {
        /* '1' ∥ COUNT(F) ∥ Iₜ */
        bitbuffer_append_bit(output, 1);  /* Flag: full input follows */

        pocket_count_encode(output, comp->F);

        bitbuffer_append_bitvector(output, input);
    } else {

        if (dt == 0) {
            /* '0' ∥ BE(...) */
            bitbuffer_append_bit(output, 0);  /* Flag: compressed */
        }

        /* Determine extraction mask based on cₜ */
        int ct = pocket_compute_ct_flag(comp, Vt, params->new_mask_flag);

        if (ct == 1 && Vt > 0) {
            /* BE(Iₜ, (Xₜ OR Mₜ)) - extract bits where mask OR changes are set */
            bitvector_t extraction_mask;
            bitvector_init(&extraction_mask, comp->F);

            bitvector_or(&extraction_mask, &comp->mask, &Xt);  /* Mₜ OR Xₜ */

            pocket_bit_extract(output, input, &extraction_mask);
        } else {
            /* BE(Iₜ, Mₜ) - extract only unpredictable bits */
            pocket_bit_extract(output, input, &comp->mask);
        }
    }

    /* ====================================================================
     * STEP 3: Update State for Next Cycle
     * ==================================================================== */

    /* Save current input and mask as previous for next iteration */
    bitvector_copy(&comp->prev_input, input);
    bitvector_copy(&comp->prev_mask, &comp->mask);

    /* Track new_mask_flag for cₜ calculation */
    comp->new_mask_flag_history[comp->flag_history_index] = params->new_mask_flag;
    comp->flag_history_index = (comp->flag_history_index + 1) % POCKET_MAX_VT_HISTORY;

    /* Advance time */
    comp->t++;

    /* Advance history index (circular buffer) */
    comp->history_index = (comp->history_index + 1) % POCKET_MAX_HISTORY;

    return POCKET_OK;
}


int pocket_compress(
    pocket_compressor_t *comp,
    const uint8_t *input_data,
    size_t input_size,
    uint8_t *output_buffer,
    size_t output_buffer_size,
    size_t *output_size
) {
    if (comp == NULL || input_data == NULL || output_buffer == NULL || output_size == NULL) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Calculate packet size in bytes */
    size_t packet_size_bytes = (comp->F + 7) / 8;

    /* Verify input size is a multiple of packet size */
    if (input_size % packet_size_bytes != 0) {
        return POCKET_ERROR_INVALID_ARG;
    }

    /* Calculate number of packets */
    int num_packets = input_size / packet_size_bytes;

    /* Reset compressor state */
    pocket_compressor_reset(comp);

    /* Output accumulation */
    size_t total_output_bytes = 0;

    /* Process each packet */
    for (int i = 0; i < num_packets; i++) {
        bitvector_t input;
        bitbuffer_t packet_output;

        bitvector_init(&input, comp->F);
        bitbuffer_init(&packet_output);

        /* Load packet data */
        bitvector_from_bytes(&input, &input_data[i * packet_size_bytes], packet_size_bytes);

        /* Compute parameters */
        pocket_params_t params;
        params.min_robustness = comp->robustness;

        /* If limits are set, manage parameters automatically using countdown counters
         * (matches reference implementation exactly) */
        if (comp->pt_limit > 0 && comp->ft_limit > 0 && comp->rt_limit > 0) {
            /* Reference implementation: packet 1 handled separately, loop starts at packet 2 (1-indexed).
             * In 0-indexed: packet 0 = handled separately, counters start at packet 1. */
            if (i == 0) {
                /* First packet: fixed init values, counters not checked */
                params.send_mask_flag = 1;
                params.uncompressed_flag = 1;
                params.new_mask_flag = 0;
            } else {
                /* Packets 1+: check and update countdown counters */
                /* ft counter */
                if (comp->ft_counter == 1) {
                    params.send_mask_flag = 1;
                    comp->ft_counter = comp->ft_limit;
                } else {
                    comp->ft_counter--;
                    params.send_mask_flag = 0;
                }

                /* pt counter */
                if (comp->pt_counter == 1) {
                    params.new_mask_flag = 1;
                    comp->pt_counter = comp->pt_limit;
                } else {
                    comp->pt_counter--;
                    params.new_mask_flag = 0;
                }

                /* rt counter */
                if (comp->rt_counter == 1) {
                    params.uncompressed_flag = 1;
                    comp->rt_counter = comp->rt_limit;
                } else {
                    comp->rt_counter--;
                    params.uncompressed_flag = 0;
                }

                /* Override for remaining init packets: CCSDS requires first Rt+1 packets
                 * to have ft=1, rt=1, pt=0. Reference checks: if (i < Rt+2) in 1-indexed.
                 * In 0-indexed: if (i <= Rt). Counters are still decremented per reference. */
                if (i <= (int)comp->robustness) {
                    params.send_mask_flag = 1;
                    params.uncompressed_flag = 1;
                    params.new_mask_flag = 0;
                }
            }
        } else {
            /* Manual control: use defaults (ft=0, rt=0, pt=0 for normal operation) */
            params.send_mask_flag = 0;
            params.uncompressed_flag = 0;
            params.new_mask_flag = 0;
        }

        /* Compress packet */
        int result = pocket_compress_packet(comp, &input, &packet_output, &params);
        if (result != POCKET_OK) {
            return result;
        }

        /* Convert packet to bytes with byte-boundary padding */
        uint8_t packet_bytes[POCKET_MAX_OUTPUT_BYTES];
        size_t packet_size = bitbuffer_to_bytes(&packet_output, packet_bytes, sizeof(packet_bytes));

        /* Check if output buffer has space */
        if (total_output_bytes + packet_size > output_buffer_size) {
            return POCKET_ERROR_OVERFLOW;
        }

        /* Append to output buffer */
        memcpy(output_buffer + total_output_bytes, packet_bytes, packet_size);
        total_output_bytes += packet_size;
    }

    /* Return total output size */
    *output_size = total_output_bytes;
    return POCKET_OK;
}

/** @} */ /* End of Main Compression Functions */
