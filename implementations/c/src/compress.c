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

    /* Debug for packet 1 */
    if (comp->t == 1) {
        fprintf(stderr, "\n=== Xt computation for packet 1 ===\n");
        fprintf(stderr, "Robustness=%u, t=%zu, num_changes=%zu, history_index=%zu\n",
                comp->robustness, comp->t, num_changes, comp->history_index);
    }

    /* OR with historical changes (going backwards from current) */
    for (size_t i = 1; i <= num_changes; i++) {
        /* Calculate index of change from i iterations ago */
        size_t hist_idx = (comp->history_index - i + POCKET_MAX_HISTORY) % POCKET_MAX_HISTORY;

        /* Debug for packet 1 */
        if (comp->t == 1) {
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

    /* Don't reverse - RLE will process from LSB to MSB directly */
    bitvector_copy(Xt, &combined);
}

uint8_t pocket_compute_effective_robustness(
    const pocket_compressor_t *comp,
    const bitvector_t *current_change
) {
    /* Vₜ = Rₜ + Cₜ
     * where Cₜ = number of consecutive iterations with no mask changes */

    uint8_t Vt = comp->robustness;

    if (comp->t == 0) {
        return Vt;
    }

    /* Check current change */
    if (bitvector_hamming_weight(current_change) > 0) {
        return Vt;  /* Current iteration has changes, so Cₜ = 0 */
    }

    /* Count consecutive iterations with no changes (going backwards) */
    for (size_t i = 1; i < 15 - comp->robustness && i < comp->t; i++) {
        size_t hist_idx = (comp->history_index - i + POCKET_MAX_HISTORY) % POCKET_MAX_HISTORY;
        if (bitvector_hamming_weight(&comp->change_history[hist_idx]) > 0) {
            break;  /* Found a change, stop counting */
        }
        Vt++;
    }

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

    /* Debug: Show change vector and mask states for packets 0 and 1 */
    if (comp->t <= 1) {
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
                fprintf(stderr, "Dt: First '1' bit at position %zu (from LSB)\n", i);
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

    /* ====================================================================
     * Component hₜ: Mask change information
     * hₜ = RLE(Xₜ) ∥ BIT₄(Vₜ) ∥ eₜ ∥ kₜ ∥ cₜ ∥ ḋₜ
     * ==================================================================== */

    /* 1. RLE(Xₜ) - Run-length encode the robustness window */
    size_t before_rle = output->num_bits;
    pocket_rle_encode(output, &Xt);
    size_t after_rle = output->num_bits;

    /* 2. BIT₄(Vₜ) - 4-bit effective robustness level */
    for (int i = 3; i >= 0; i--) {
        bitbuffer_append_bit(output, (Vt >> i) & 1);
    }

    /* Debug output for packets 1-3 */
    if (comp->t >= 1 && comp->t <= 3) {
        fprintf(stderr, "DEBUG packet %zu: RLE(Xt)=%zu bits, Vt=%u, H(Xt)=%zu\n",
                comp->t,
                after_rle - before_rle, Vt, bitvector_hamming_weight(&Xt));

        /* Show Xₜ bit pattern (first 64 bits) */
        fprintf(stderr, "DEBUG packet 1: Xt first 64 bits = ");
        for (size_t i = 0; i < 64 && i < Xt.length; i++) {
            fprintf(stderr, "%d", bitvector_get_bit(&Xt, i));
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");

        /* Find the '1' bit position */
        for (size_t i = 0; i < Xt.length; i++) {
            if (bitvector_get_bit(&Xt, i)) {
                fprintf(stderr, "DEBUG packet 1: Found '1' bit at position %zu (from LSB)\n", i);
                break;
            }
        }

        /* Show first 64 bits of output so far */
        fprintf(stderr, "DEBUG packet 1: First bits after RLE+Vt: ");
        for (size_t i = 0; i < 64 && i < output->num_bits; i++) {
            int bit = (output->data[i/8] >> (7 - (i%8))) & 1;
            fprintf(stderr, "%d", bit);
            if ((i+1) % 8 == 0) fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
    }

    /* 3. eₜ, kₜ, cₜ - Only if Vₜ > 0 and there are mask changes */
    if (Vt > 0 && bitvector_hamming_weight(&Xt) > 0) {
        /* Calculate eₜ */
        int et = pocket_has_positive_updates(&Xt, &comp->mask);
        bitbuffer_append_bit(output, et);

        if (et == 1) {
            /* kₜ - Extract mask values at changed positions */
            size_t before_kt = output->num_bits;
            pocket_bit_extract(output, &comp->mask, &Xt);
            size_t after_kt = output->num_bits;

            /* Calculate and encode cₜ */
            int ct = pocket_compute_ct_flag(comp, Vt);
            bitbuffer_append_bit(output, ct);

            if (comp->t == 1) {
                fprintf(stderr, "DEBUG packet 1: et=%d, kt=%zu bits, ct=%d\n",
                        et, after_kt - before_kt, ct);
            }
        } else if (comp->t == 1) {
            fprintf(stderr, "DEBUG packet 1: et=%d (no kt/ct)\n", et);
        }
    } else if (comp->t == 1) {
        fprintf(stderr, "DEBUG packet 1: Skipped et/kt/ct (Vt=%u, H(Xt)=%zu)\n",
                Vt, bitvector_hamming_weight(&Xt));
    }

    /* 4. ḋₜ - Flag indicating if both ḟₜ and ṙₜ are zero */
    bitbuffer_append_bit(output, dt);

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
            pocket_rle_encode(output, &mask_diff);
            size_t after_qt = output->num_bits;

            if (comp->t == 1) {
                fprintf(stderr, "DEBUG packet 1: qt=%zu bits (mask RLE=%zu bits)\n",
                        after_qt - before_qt, after_qt - before_mask_rle);
            }
        } else {
            bitbuffer_append_bit(output, 0);  /* Flag: no mask */
            if (comp->t == 1) {
                fprintf(stderr, "DEBUG packet 1: qt=1 bit (no mask)\n");
            }
        }
    } else if (comp->t == 1) {
        fprintf(stderr, "DEBUG packet 1: No qt (dt=1)\n");
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
        int ct = pocket_compute_ct_flag(comp, Vt);

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
