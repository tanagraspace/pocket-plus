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
 * POCKET+ C Implementation - Mask Update Logic
 *
 * Authors:
 *   Georges Labrèche <georges@tanagraspace.com> — https://georges.fyi
 *   Claude Code (claude-sonnet-4-5-20250929) <noreply@anthropic.com>
 *
 * Implements CCSDS 124.0-B-1 Section 4 (Mask Update):
 * - Build Vector Update (Equation 6)
 * - Mask Vector Update (Equation 7)
 * - Change Vector Computation (Equation 8)
 * ============================================================================
 */

#include "pocket_plus.h"

/* ========================================================================
 * Update Build Vector (CCSDS Equation 6)
 *
 * Bₜ = (Iₜ XOR Iₜ₋₁) OR Bₜ₋₁  (if t > 0 and ṗₜ = 0)
 * Bₜ = 0                       (otherwise: t=0 or ṗₜ=1)
 *
 * The build vector accumulates bits that have changed over time.
 * When new_mask_flag is set, the build is used to replace the mask
 * and is then reset to zero.
 * ======================================================================== */

void pocket_update_build(
    bitvector_t *build,
    const bitvector_t *input,
    const bitvector_t *prev_input,
    int new_mask_flag,
    size_t t
) {
    /* Case 1: t=0 or new_mask_flag set → reset build to 0 */
    if (t == 0 || new_mask_flag) {
        bitvector_zero(build);
        return;
    }

    /* Case 2: Normal operation (t > 0 and new_mask_flag = 0)
     * Bₜ = (Iₜ XOR Iₜ₋₁) OR Bₜ₋₁
     */

    /* Create temporary vector for changes */
    bitvector_t changes;
    bitvector_init(&changes, build->length);

    /* Calculate changes: Iₜ XOR Iₜ₋₁ */
    bitvector_xor(&changes, input, prev_input);

    /* Update build: Bₜ = changes OR Bₜ₋₁ */
    bitvector_or(build, &changes, build);
}

/* ========================================================================
 * Update Mask Vector (CCSDS Equation 7)
 *
 * Mₜ = (Iₜ XOR Iₜ₋₁) OR Mₜ₋₁    (if ṗₜ = 0)
 * Mₜ = (Iₜ XOR Iₜ₋₁) OR Bₜ₋₁    (if ṗₜ = 1)
 *
 * The mask tracks which bits are unpredictable.
 * When new_mask_flag is set, the mask is replaced with the build vector.
 * ======================================================================== */

void pocket_update_mask(
    bitvector_t *mask,
    const bitvector_t *input,
    const bitvector_t *prev_input,
    const bitvector_t *build_prev,
    int new_mask_flag
) {
    /* Create temporary vector for changes */
    bitvector_t changes;
    bitvector_init(&changes, mask->length);

    /* Calculate changes: Iₜ XOR Iₜ₋₁ */
    bitvector_xor(&changes, input, prev_input);

    if (new_mask_flag) {
        /* Case 1: new_mask_flag set → Mₜ = (Iₜ XOR Iₜ₋₁) OR Bₜ₋₁ */
        bitvector_or(mask, &changes, build_prev);
    } else {
        /* Case 2: Normal operation → Mₜ = (Iₜ XOR Iₜ₋₁) OR Mₜ₋₁ */
        bitvector_or(mask, &changes, mask);
    }
}

/* ========================================================================
 * Compute Change Vector (CCSDS Equation 8)
 *
 * Dₜ = Mₜ XOR Mₜ₋₁  (if t > 0)
 * Dₜ = Mₜ           (if t = 0, assuming M₋₁ = 0)
 *
 * The change vector tracks which mask bits changed between time steps.
 * This is used in the encoding stage to communicate mask updates.
 * At t=0, all bits in the initial mask are considered "changes" from
 * the implicit all-zero mask before initialization.
 * ======================================================================== */

void pocket_compute_change(
    bitvector_t *change,
    const bitvector_t *mask,
    const bitvector_t *prev_mask,
    size_t t
) {
    if (t == 0) {
        /* At t=0, D₀ = M₀ (all initially predictable bits) */
        bitvector_copy(change, mask);
    } else {
        /* Dₜ = Mₜ XOR Mₜ₋₁ */
        bitvector_xor(change, mask, prev_mask);
    }
}
