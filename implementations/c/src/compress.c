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
 * POCKET+ C Implementation - Compression
 *
 * Authors:
 *   Georges Labrèche <georges@tanagraspace.com>
 *   Claude Code (claude-sonnet-4-5-20250929) <noreply@anthropic.com>
 *
 * Implements CCSDS 124.0-B-1 Section 5.3 (Encoding Step):
 * - Compressor initialization and state management
 * - Main compression algorithm
 * - Output packet encoding: oₜ = hₜ ∥ qₜ ∥ uₜ
 * ============================================================================
 */

#include "pocket_plus.h"
#include <string.h>

/* Debug: Global packet counter for debugging */
size_t g_debug_packet_num = 0;

/* ========================================================================
 * Compressor Initialization
 * ======================================================================== */

int pocket_compressor_init(
    pocket_compressor_t *comp,
    size_t F,
    const bitvector_t *initial_mask,
    uint8_t robustness
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

    /* Debug: Print init parameters */
    fprintf(stderr, "DEBUG: pocket_compressor_init: F=%zu, robustness=%u\n", F, robustness);

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

/* ========================================================================
 * Reset Compressor
 * ======================================================================== */

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
}

/* ========================================================================
 * Helper: Get default parameters based on robustness requirements
 * ======================================================================== */

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

/* ========================================================================
 * CCSDS Helper Functions
 * ======================================================================== */

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

    /* Debug for packet 1 and 2 */
    if (comp->t == 1 || comp->t == 2) {
        fprintf(stderr, "\n=== Xt computation for packet %zu ===\n", comp->t);
        fprintf(stderr, "Robustness=%u, t=%zu, num_changes=%zu, history_index=%zu\n",
                comp->robustness, comp->t, num_changes, comp->history_index);

        /* Print current change (D_t) before ORing */
        fprintf(stderr, "D_t (current change) first 64 bits: ");
        for (size_t j = 0; j < 64 && j < current_change->length; j++) {
            fprintf(stderr, "%d", bitvector_get_bit(current_change, j));
            if ((j+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\nD_t Hamming weight: %zu\n", bitvector_hamming_weight(current_change));

        if (comp->t == 2) {
            fprintf(stderr, "[OUR-PKT2] D_t words:\n");
            for (size_t w = 0; w < current_change->num_words && w < 3; w++) {
                fprintf(stderr, "[OUR-PKT2]   D_t[%zu]=0x%08X\n", w, current_change->data[w]);
            }
        }
    }

    /* OR with historical changes (going backwards from current) */
    for (size_t i = 1; i <= num_changes; i++) {
        /* Calculate index of change from i iterations ago */
        size_t hist_idx = (comp->history_index - i + POCKET_MAX_HISTORY) % POCKET_MAX_HISTORY;

        /* Debug for packet 1 and 2 */
        if (comp->t == 1 || comp->t == 2) {
            fprintf(stderr, "Including D(t-%zu) from history[%zu], first 64 bits: ", i, hist_idx);
            for (size_t j = 0; j < 64 && j < comp->change_history[hist_idx].length; j++) {
                fprintf(stderr, "%d", bitvector_get_bit(&comp->change_history[hist_idx], j));
                if ((j+1) % 8 == 0) fprintf(stderr, " ");
            }
            fprintf(stderr, "\nHamming weight: %zu\n", bitvector_hamming_weight(&comp->change_history[hist_idx]));
        }

        bitvector_t temp;
        bitvector_init(&temp, comp->F);
        bitvector_or(&temp, &combined, &comp->change_history[hist_idx]);
        bitvector_copy(&combined, &temp);
    }

    /* Debug: Print combined vector before copying to Xt */
    if (comp->t == 2) {
        fprintf(stderr, "[OUR-PKT2] Combined vector (Num_of_words=%zu):\n", combined.num_words);
        for (size_t w = 0; w < combined.num_words; w++) {
            fprintf(stderr, "[OUR-PKT2]   combined.data[%zu]=0x%08X\n", w, combined.data[w]);
        }
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
     * Per CCSDS 5.3.2.2: Ct is largest value for which D_{t-i} = ∅ for all 1 < i ≤ Ct + Rt
     * This means we start from i=2 (checking D_{t-2}), NOT i=1 (D_{t-1}) */
    uint8_t Ct = 0;

    /* Count backwards through history starting from t-2 (skip t-1 per CCSDS spec) */
    for (size_t i = 2; i <= 15 && i <= comp->t; i++) {
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

/* ========================================================================
 * eₜ, kₜ, cₜ Helper Functions
 * ======================================================================== */

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
    uint8_t Vt
) {
    /* cₜ = 1 if new_mask_flag was set 2+ times in last Vₜ iterations */

    if (comp->t == 0 || Vt == 0) {
        return 0;
    }

    /* Count how many times new_mask_flag was set in last Vₜ iterations */
    int count = 0;
    size_t iterations_to_check = (Vt < comp->t) ? Vt : comp->t;

    for (size_t i = 0; i < iterations_to_check; i++) {
        /* Calculate history index going backwards from current */
        size_t hist_idx = (comp->flag_history_index - 1 - i + POCKET_MAX_VT_HISTORY) % POCKET_MAX_VT_HISTORY;
        if (comp->new_mask_flag_history[hist_idx]) {
            count++;
        }
    }

    return (count >= 2) ? 1 : 0;
}

/* ========================================================================
 * Main Compression Function
 *
 * Implements CCSDS 124.0-B-1 Section 5.3
 * Output: oₜ = hₜ ∥ qₜ ∥ uₜ
 * ======================================================================== */

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

    /* Debug: Set global packet number */
    g_debug_packet_num = comp->t;

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

    /* Debug for packet 2: print masks and inputs before computing change */
    if (comp->t == 2) {
        fprintf(stderr, "\n[OUR-PKT2] Before change computation:\n");
        fprintf(stderr, "[OUR-PKT2] I_t (current input) word[1]=0x%08X\n", input->data[1]);
        fprintf(stderr, "[OUR-PKT2] I_{t-1} (prev input) word[1]=0x%08X\n", comp->prev_input.data[1]);

        bitvector_t input_changes;
        bitvector_init(&input_changes, comp->F);
        bitvector_xor(&input_changes, input, &comp->prev_input);
        fprintf(stderr, "[OUR-PKT2] I_t XOR I_{t-1} word[1]=0x%08X\n", input_changes.data[1]);

        fprintf(stderr, "[OUR-PKT2] M_t (current mask) word[1]=0x%08X\n", comp->mask.data[1]);
        fprintf(stderr, "[OUR-PKT2] M_{t-1} (prev mask) word[1]=0x%08X\n", prev_mask.data[1]);
    }

    pocket_compute_change(&change, &comp->mask, &prev_mask, comp->t);

    /* Debug: Show change vector and mask states for packets 0, 1, 19, and 20 */
    if (comp->t <= 1 || comp->t == 19 || comp->t == 20 || comp->t == 20) {
        fprintf(stderr, "\n=== DEBUG packet %zu ===\n", comp->t);

        /* Show input data */
        fprintf(stderr, "It (input) first 64 bits: ");
        for (size_t i = 0; i < 64 && i < input->length; i++) {
            fprintf(stderr, "%d", bitvector_get_bit(input, i));
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");

        if (comp->t > 0) {
            fprintf(stderr, "It-1 (prev input) first 64 bits: ");
            for (size_t i = 0; i < 64 && i < comp->prev_input.length; i++) {
                fprintf(stderr, "%d", bitvector_get_bit(&comp->prev_input, i));
                if ((i+1) % 8 == 0) fprintf(stderr, " ");
            }
            fprintf(stderr, "\n");

            /* Show data changes */
            bitvector_t data_changes;
            bitvector_init(&data_changes, input->length);
            bitvector_xor(&data_changes, input, &comp->prev_input);
            fprintf(stderr, "Data changes (It XOR It-1) first 64 bits: ");
            for (size_t i = 0; i < 64 && i < data_changes.length; i++) {
                fprintf(stderr, "%d", bitvector_get_bit(&data_changes, i));
                if ((i+1) % 8 == 0) fprintf(stderr, " ");
            }
            fprintf(stderr, "\nData changes Hamming weight: %zu\n", bitvector_hamming_weight(&data_changes));
        }

        /* Show mask states */
        fprintf(stderr, "Mt (current mask) first 64 bits: ");
        for (size_t i = 0; i < 64 && i < comp->mask.length; i++) {
            fprintf(stderr, "%d", bitvector_get_bit(&comp->mask, i));
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");

        fprintf(stderr, "Mt-1 (prev mask) first 64 bits: ");
        for (size_t i = 0; i < 64 && i < prev_mask.length; i++) {
            fprintf(stderr, "%d", bitvector_get_bit(&prev_mask, i));
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");

        /* Show change vector */
        fprintf(stderr, "Dt (change vector) first 64 bits: ");
        for (size_t i = 0; i < 64 && i < change.length; i++) {
            fprintf(stderr, "%d", bitvector_get_bit(&change, i));
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\nDt Hamming weight: %zu\n", bitvector_hamming_weight(&change));

        /* Find first '1' bit in Dt */
        for (size_t i = 0; i < change.length; i++) {
            if (bitvector_get_bit(&change, i)) {
                fprintf(stderr, "Dt: First '1' bit at position %zu\n", i);
                fprintf(stderr, "At position %zu: Mt=%d, M_(t-1)=%d\n",
                        i, bitvector_get_bit(&comp->mask, i), bitvector_get_bit(&prev_mask, i));

                /* Show surrounding context */
                fprintf(stderr, "Context around position %zu:\n", i);
                fprintf(stderr, "  It bits [%zu-%zu]: ", i > 4 ? i-4 : 0, i+4 < comp->F ? i+4 : comp->F-1);
                for (size_t j = (i > 4 ? i-4 : 0); j <= (i+4 < comp->F ? i+4 : comp->F-1); j++) {
                    fprintf(stderr, "%d", bitvector_get_bit(input, j));
                }
                fprintf(stderr, "\n");

                if (comp->t > 0) {
                    fprintf(stderr, "  It-1 bits [%zu-%zu]: ", i > 4 ? i-4 : 0, i+4 < comp->F ? i+4 : comp->F-1);
                    for (size_t j = (i > 4 ? i-4 : 0); j <= (i+4 < comp->F ? i+4 : comp->F-1); j++) {
                        fprintf(stderr, "%d", bitvector_get_bit(&comp->prev_input, j));
                    }
                    fprintf(stderr, "\n");

                    fprintf(stderr, "  Data changed at position %zu: %d XOR %d = %d\n",
                            i, bitvector_get_bit(input, i), bitvector_get_bit(&comp->prev_input, i),
                            bitvector_get_bit(input, i) != bitvector_get_bit(&comp->prev_input, i) ? 1 : 0);
                }

                break;
            }
        }
    }

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

    /* Debug: Show flags for packets 0-3 and 19 */
    if (comp->t <= 3 || comp->t == 19 || comp->t == 20) {
        fprintf(stderr, "DEBUG packet %zu: Flags - pt=%d ft=%d rt=%d dt=%d\n",
                comp->t, params->new_mask_flag, params->send_mask_flag,
                params->uncompressed_flag, dt);
    }

    /* ====================================================================
     * Component hₜ: Mask change information
     * hₜ = RLE(Xₜ) ∥ BIT₄(Vₜ) ∥ eₜ ∥ kₜ ∥ cₜ ∥ ḋₜ
     * ==================================================================== */

    /* 1. RLE(Xₜ) - Run-length encode the robustness window */
    size_t before_rle = output->num_bits;
    if (comp->t < 100) {
        fprintf(stderr, "[PKT%zu] Encoding component ht: RLE(Xt) starting at bit %zu\n", comp->t, before_rle);
    }
    pocket_rle_encode(output, &Xt);
    size_t after_rle = output->num_bits;

    /* 2. BIT₄(Vₜ) - 4-bit effective robustness level
     * CCSDS encodes Vt directly (reference implementation confirmed) */
    if (comp->t == 30) {
        fprintf(stderr, "DEBUG packet 30: Vt value = %u (binary: %d%d%d%d)\n",
                Vt, (Vt>>3)&1, (Vt>>2)&1, (Vt>>1)&1, Vt&1);
    }
    for (int i = 3; i >= 0; i--) {
        bitbuffer_append_bit(output, (Vt >> i) & 1);
    }

    /* Debug output for packets 0-3 and 19 */
    if (comp->t >= 0 && (comp->t <= 3 || comp->t == 19 || comp->t == 20)) {
        fprintf(stderr, "DEBUG packet %zu: RLE(Xt)=%zu bits, Vt=%u, H(Xt)=%zu\n",
                comp->t,
                after_rle - before_rle, Vt, bitvector_hamming_weight(&Xt));

        /* Show Xₜ bit pattern (first 64 bits) */
        fprintf(stderr, "DEBUG packet %zu: Xt first 64 bits = ", comp->t);
        for (size_t i = 0; i < 64 && i < Xt.length; i++) {
            fprintf(stderr, "%d", bitvector_get_bit(&Xt, i));
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");

        /* Find the '1' bit position */
        for (size_t i = 0; i < Xt.length; i++) {
            if (bitvector_get_bit(&Xt, i)) {
                fprintf(stderr, "DEBUG packet %zu: Found '1' bit at position %zu (from LSB)\n", comp->t, i);
                break;
            }
        }

        /* Show first 64 bits and bytes of output so far */
        fprintf(stderr, "DEBUG packet %zu: First bits after RLE+Vt: ", comp->t);
        for (size_t i = 0; i < 64 && i < output->num_bits; i++) {
            int bit = (output->data[i/8] >> (7 - (i%8))) & 1;
            fprintf(stderr, "%d", bit);
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");

        /* Show first 3 bytes as hex */
        fprintf(stderr, "DEBUG packet %zu: First 3 bytes: 0x%02X 0x%02X 0x%02X\n",
                comp->t,
                output->num_bits >= 8 ? output->data[0] : 0,
                output->num_bits >= 16 ? output->data[1] : 0,
                output->num_bits >= 24 ? output->data[2] : 0);
    }

    /* 3. eₜ, kₜ, cₜ - Only if Vₜ > 0 and there are mask changes */
    if (Vt > 0 && bitvector_hamming_weight(&Xt) > 0) {
        /* Calculate eₜ */
        int et = pocket_has_positive_updates(&Xt, &comp->mask);
        bitbuffer_append_bit(output, et);

        if (et == 1) {
            /* kₜ - Output '1' for positive updates (mask=0), '0' for negative updates (mask=1)
             * Reference implementation shows kt outputs 1 when mask bit is 0 at changed positions */
            size_t before_kt = output->num_bits;
           if (comp->t == 30) {
                fprintf(stderr, "DEBUG packet 30: Xt '1' positions: ");
                int count = 0;
                for (size_t i = 0; i < Xt.length && count < 10; i++) {
                    if (bitvector_get_bit(&Xt, i)) {
                        fprintf(stderr, "%zu ", i);
                        count++;
                    }
                }
                fprintf(stderr, "\nDEBUG packet 30: MASK values at those positions: ");
                count = 0;
                for (size_t i = 0; i < Xt.length && count < 10; i++) {
                    if (bitvector_get_bit(&Xt, i)) {
                        fprintf(stderr, "%d ", bitvector_get_bit(&comp->mask, i));
                        count++;
                    }
                }
                fprintf(stderr, "\nDEBUG packet 30: kt output (INVERSE of mask): ");
                count = 0;
                for (size_t i = 0; i < Xt.length && count < 10; i++) {
                    if (bitvector_get_bit(&Xt, i)) {
                        fprintf(stderr, "%d ", !bitvector_get_bit(&comp->mask, i));
                        count++;
                    }
                }
                fprintf(stderr, "\n");
            }

            /* Extract INVERTED mask values (1 where mask=0, 0 where mask=1) */
            bitvector_t inverted_mask;
            bitvector_init(&inverted_mask, comp->F);
            for (size_t i = 0; i < comp->mask.length; i++) {
                bitvector_set_bit(&inverted_mask, i, !bitvector_get_bit(&comp->mask, i));
            }
            pocket_bit_extract(output, &inverted_mask, &Xt);
            size_t after_kt = output->num_bits;

            /* Calculate and encode cₜ */
            int ct = pocket_compute_ct_flag(comp, Vt);
            bitbuffer_append_bit(output, ct);

            if (comp->t == 1 || comp->t == 20 || comp->t == 30) {
                fprintf(stderr, "DEBUG packet %zu: et=%d, kt=%zu bits, ct=%d\n",
                        comp->t, et, after_kt - before_kt, ct);
            }
        } else if (comp->t == 1 || comp->t == 20 || comp->t == 30) {
            fprintf(stderr, "DEBUG packet %zu: et=%d (no kt/ct)\n", comp->t, et);
        }
    } else if (comp->t == 1 || comp->t == 20 || comp->t == 30) {
        fprintf(stderr, "DEBUG packet %zu: Skipped et/kt/ct (Vt=%u, H(Xt)=%zu)\n",
                comp->t, Vt, bitvector_hamming_weight(&Xt));
    }

    /* 4. ḋₜ - Flag indicating if both ḟₜ and ṙₜ are zero */
    bitbuffer_append_bit(output, dt);

    /* Debug: Show ht size after dt */
    size_t after_ht = output->num_bits;
    if (comp->t <= 3 || comp->t == 19 || comp->t == 20 || comp->t == 30) {
        fprintf(stderr, "DEBUG packet %zu: ht=%zu bits (RLE=%zu, Vt=4, et+kt+ct+dt=%zu)\n",
                comp->t, after_ht, after_rle - before_rle, after_ht - after_rle - 4);
    }

    /* ====================================================================
     * Component qₜ: Optional full mask
     * qₜ = ∅ if ḋₜ=1, '1' ∥ RLE(<(Mₜ XOR (Mₜ≪))>) if ḟₜ=1, '0' otherwise
     * ==================================================================== */

    if (dt == 0) {  /* Only if ḋₜ = 0 */
        if (params->send_mask_flag) {
            size_t before_qt = output->num_bits;
            bitbuffer_append_bit(output, 1);  /* Flag: mask follows */

            /* Encode mask as RLE(M XOR (M<<)) - no reversal needed */
            bitvector_t mask_shifted, mask_diff;
            bitvector_init(&mask_shifted, comp->F);
            bitvector_init(&mask_diff, comp->F);

            bitvector_left_shift(&mask_shifted, &comp->mask);
            bitvector_xor(&mask_diff, &comp->mask, &mask_shifted);

            if (comp->t == 1) {
                fprintf(stderr, "DEBUG packet 1: mask_diff first 64 bits = ");
                for (size_t i = 0; i < 64 && i < mask_diff.length; i++) {
                    fprintf(stderr, "%d", bitvector_get_bit(&mask_diff, i));
                    if ((i+1) % 8 == 0) fprintf(stderr, " ");
                }
                fprintf(stderr, "\n");
            }

            size_t before_mask_rle = output->num_bits;
            if (comp->t < 100) {
                fprintf(stderr, "[PKT%zu] Encoding component qt: RLE(mask_diff) starting at bit %zu\n", comp->t, before_mask_rle);
            }
            pocket_rle_encode(output, &mask_diff);
            size_t after_qt = output->num_bits;

            if (comp->t <= 3 || comp->t == 19 || comp->t == 20 || comp->t == 30) {
                fprintf(stderr, "DEBUG packet %zu: qt=%zu bits (mask RLE=%zu bits)\n",
                        comp->t, after_qt - before_qt, after_qt - before_mask_rle);
            }
        } else {
            bitbuffer_append_bit(output, 0);  /* Flag: no mask */
            if (comp->t <= 3 || comp->t == 19 || comp->t == 20 || comp->t == 30) {
                fprintf(stderr, "DEBUG packet %zu: qt=1 bit (no mask)\n", comp->t);
            }
        }
    } else if (comp->t <= 3 || comp->t == 19 || comp->t == 20 || comp->t == 30) {
        fprintf(stderr, "DEBUG packet %zu: No qt (dt=1)\n", comp->t);
    }

    /* ====================================================================
     * Component uₜ: Unpredictable bits or full input
     * Per ALGORITHM.md, depends on ḋₜ, ṙₜ, cₜ
     * ==================================================================== */

    if (params->uncompressed_flag) {
        /* '1' ∥ COUNT(F) ∥ Iₜ */
        size_t before_ut = output->num_bits;
        if (comp->t <= 3 || comp->t == 19 || comp->t == 20) {
            fprintf(stderr, "DEBUG packet %zu: Uncompressed mode - comp->F=%zu, input->length=%zu\n",
                    comp->t, comp->F, input->length);
        }
        bitbuffer_append_bit(output, 1);  /* Flag: full input follows */
        size_t after_flag = output->num_bits;

        pocket_count_encode(output, comp->F);
        size_t after_count = output->num_bits;

        bitbuffer_append_bitvector(output, input);
        size_t after_input = output->num_bits;

        if (comp->t <= 3 || comp->t == 19 || comp->t == 20) {
            fprintf(stderr, "DEBUG packet %zu: ut components - flag: %zu bit, COUNT(%zu): %zu bits, input: %zu bits\n",
                    comp->t, after_flag - before_ut, comp->F, after_count - after_flag, after_input - after_count);
            fprintf(stderr, "DEBUG packet %zu: ut total: %zu bits\n",
                    comp->t, after_input - before_ut);
        }
    } else {
        size_t before_ut = output->num_bits;

        if (dt == 0) {
            /* '0' ∥ BE(...) */
            bitbuffer_append_bit(output, 0);  /* Flag: compressed */
        }

        /* Determine extraction mask based on cₜ */
        int ct = pocket_compute_ct_flag(comp, Vt);

        if (ct == 1 && Vt > 0) {
            /* BE(Iₜ, (Xₜ OR Mₜ)) - extract bits where mask OR changes are set */
            bitvector_t extraction_mask;
            bitvector_init(&extraction_mask, comp->F);

            bitvector_or(&extraction_mask, &comp->mask, &Xt);  /* Mₜ OR Xₜ */
            size_t mask_weight = bitvector_hamming_weight(&extraction_mask);

            pocket_bit_extract(output, input, &extraction_mask);

            if (comp->t <= 5 || comp->t == 19 || comp->t == 20 || comp->t == 30) {
                fprintf(stderr, "DEBUG packet %zu: ut=BE(It, Xt|Mt), ct=%d, |Xt|Mt|=%zu, ut=%zu bits\n",
                        comp->t, ct, mask_weight, output->num_bits - before_ut);
            }
        } else {
            /* BE(Iₜ, Mₜ) - extract only unpredictable bits */
            size_t mask_weight = bitvector_hamming_weight(&comp->mask);

            if (comp->t == 20 || comp->t == 30) {
                fprintf(stderr, "DEBUG packet 20: Mask '1' bit positions: ");
                int count = 0;
                for (size_t i = 0; i < comp->mask.length && count < 10; i++) {
                    if (bitvector_get_bit(&comp->mask, i)) {
                        fprintf(stderr, "%zu ", i);
                        count++;
                    }
                }
                fprintf(stderr, "\nDEBUG packet 20: Input bits at those positions: ");
                count = 0;
                for (size_t i = 0; i < comp->mask.length && count < 10; i++) {
                    if (bitvector_get_bit(&comp->mask, i)) {
                        fprintf(stderr, "%d ", bitvector_get_bit(input, i));
                        count++;
                    }
                }
                fprintf(stderr, "\n");
            }

            pocket_bit_extract(output, input, &comp->mask);

            if (comp->t <= 5 || comp->t == 19 || comp->t == 20 || comp->t == 30) {
                fprintf(stderr, "DEBUG packet %zu: ut=BE(It, Mt), ct=%d, |Mt|=%zu, ut=%zu bits\n",
                        comp->t, ct, mask_weight, output->num_bits - before_ut);
            }
        }
    }

    /* Debug: Show total packet size for packet 30 */
    if (comp->t == 30) {
        fprintf(stderr, "DEBUG packet 30: TOTAL packet = %zu bits = %zu bytes\n",
                output->num_bits, (output->num_bits + 7) / 8);
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

    /* Debug: Show final output bytes for packets 0-3 and 19 */
    if (comp->t >= 0 && (comp->t <= 3 || comp->t == 19 || comp->t == 20)) {
        fprintf(stderr, "FINAL packet %zu: Total bits=%zu, First 3 bytes: 0x%02X 0x%02X 0x%02X\n",
                comp->t, output->num_bits,
                output->num_bits >= 8 ? output->data[0] : 0,
                output->num_bits >= 16 ? output->data[1] : 0,
                output->num_bits >= 24 ? output->data[2] : 0);
    }

    /* Advance time */
    comp->t++;

    /* Advance history index (circular buffer) */
    comp->history_index = (comp->history_index + 1) % POCKET_MAX_HISTORY;

    return POCKET_OK;
}
