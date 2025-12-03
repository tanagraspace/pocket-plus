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
 * POCKET+ C Implementation
 * CCSDS 124.0-B-1: Robust Compression of Fixed-Length Housekeeping Data
 *
 * Authors:
 *   Georges Labrèche <georges@tanagraspace.com>
 *   Claude Code (claude-sonnet-4-5-20250929) <noreply@anthropic.com>
 *
 * Public API Header
 * ============================================================================
 */

#ifndef POCKET_PLUS_H
#define POCKET_PLUS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Version information */
#define POCKET_VERSION_MAJOR 1
#define POCKET_VERSION_MINOR 0
#define POCKET_VERSION_PATCH 0

/* Error codes */
#define POCKET_OK                0
#define POCKET_ERROR_INVALID_ARG -1
#define POCKET_ERROR_OVERFLOW    -2
#define POCKET_ERROR_UNDERFLOW   -3

/* Configuration constants */
#ifndef POCKET_MAX_PACKET_LENGTH
#define POCKET_MAX_PACKET_LENGTH 65535  /* CCSDS maximum */
#endif

#define POCKET_MAX_PACKET_BYTES ((POCKET_MAX_PACKET_LENGTH + 7) / 8)
#define POCKET_MAX_ROBUSTNESS 7
#define POCKET_MAX_HISTORY 16
#define POCKET_MAX_VT_HISTORY 16  /* History size for Vₜ calculation (per CCSDS) */
#define POCKET_MAX_OUTPUT_BYTES (POCKET_MAX_PACKET_BYTES * 6)

/* Forward declarations */
typedef struct bitvector bitvector_t;
typedef struct bitbuffer bitbuffer_t;
typedef struct pocket_compressor pocket_compressor_t;
typedef struct pocket_decompressor pocket_decompressor_t;

/* Compression parameters (per packet) */
typedef struct {
    uint8_t min_robustness;      /* Rₜ (0-7) */
    uint8_t new_mask_flag;       /* ṗₜ (0 or 1) */
    uint8_t send_mask_flag;      /* ḟₜ (0 or 1) */
    uint8_t uncompressed_flag;   /* ṙₜ (0 or 1) */
} pocket_params_t;

/* ========================================================================
 * Bit Vector API
 * ======================================================================== */

/**
 * Bit vector structure (fixed-length binary vector).
 * Uses 32-bit words with big-endian byte packing to match ESA/ESOC reference.
 */
struct bitvector {
    uint32_t data[(POCKET_MAX_PACKET_BYTES + 3) / 4];  /* 32-bit word array */
    size_t length;                                      /* Number of bits (F) */
    size_t num_words;                                   /* Number of words used */
};

/**
 * Initialize a bit vector with specified length.
 *
 * @param bv Bit vector structure (allocated by caller)
 * @param num_bits Number of bits (1 to POCKET_MAX_PACKET_LENGTH)
 * @return POCKET_OK on success, error code otherwise
 */
int bitvector_init(bitvector_t *bv, size_t num_bits);

/**
 * Zero all bits in the vector.
 */
void bitvector_zero(bitvector_t *bv);

/**
 * Copy one bit vector to another.
 */
void bitvector_copy(bitvector_t *dest, const bitvector_t *src);

/**
 * Get bit value at position (0 = LSB, length-1 = MSB).
 */
int bitvector_get_bit(const bitvector_t *bv, size_t pos);

/**
 * Set bit value at position.
 */
void bitvector_set_bit(bitvector_t *bv, size_t pos, int value);

/**
 * Bitwise XOR: result = a XOR b
 */
void bitvector_xor(bitvector_t *result, const bitvector_t *a, const bitvector_t *b);

/**
 * Bitwise OR: result = a OR b
 */
void bitvector_or(bitvector_t *result, const bitvector_t *a, const bitvector_t *b);

/**
 * Bitwise AND: result = a AND b
 */
void bitvector_and(bitvector_t *result, const bitvector_t *a, const bitvector_t *b);

/**
 * Bitwise NOT: result = ~a
 */
void bitvector_not(bitvector_t *result, const bitvector_t *a);

/**
 * Left shift by 1 bit (inserts 0 at LSB): result = a << 1
 */
void bitvector_left_shift(bitvector_t *result, const bitvector_t *a);

/**
 * Reverse bit order: result = <a>
 */
void bitvector_reverse(bitvector_t *result, const bitvector_t *a);

/**
 * Count number of '1' bits (Hamming weight).
 */
size_t bitvector_hamming_weight(const bitvector_t *bv);

/**
 * Compare two bit vectors for equality.
 */
int bitvector_equals(const bitvector_t *a, const bitvector_t *b);

/**
 * Load bit vector from byte array.
 */
int bitvector_from_bytes(bitvector_t *bv, const uint8_t *data, size_t num_bytes);

/**
 * Store bit vector to byte array.
 */
int bitvector_to_bytes(const bitvector_t *bv, uint8_t *data, size_t num_bytes);

/**
 * Print bit vector as binary string (for debugging).
 */
void bitvector_print(const bitvector_t *bv, FILE *fp);

/* ========================================================================
 * Bit Buffer API (Variable-length output buffer)
 * ======================================================================== */

struct bitbuffer {
    uint8_t data[POCKET_MAX_OUTPUT_BYTES];  /* Static byte array */
    size_t num_bits;                        /* Number of bits written */
};

/**
 * Initialize a bit buffer (empty).
 */
void bitbuffer_init(bitbuffer_t *bb);

/**
 * Clear bit buffer (reset to empty).
 */
void bitbuffer_clear(bitbuffer_t *bb);

/**
 * Append a single bit.
 */
int bitbuffer_append_bit(bitbuffer_t *bb, int bit);

/**
 * Append multiple bits from byte array.
 */
int bitbuffer_append_bits(bitbuffer_t *bb, const uint8_t *data, size_t num_bits);

/**
 * Append a bit vector.
 */
int bitbuffer_append_bitvector(bitbuffer_t *bb, const bitvector_t *bv);

/**
 * Get number of bits in buffer.
 */
size_t bitbuffer_size(const bitbuffer_t *bb);

/**
 * Convert bit buffer to byte array.
 *
 * @param bb Bit buffer
 * @param data Output byte array (allocated by caller)
 * @param max_bytes Maximum bytes to write
 * @return Number of bytes written
 */
size_t bitbuffer_to_bytes(const bitbuffer_t *bb, uint8_t *data, size_t max_bytes);

/* ========================================================================
 * Mask Update API (CCSDS Section 4)
 * ======================================================================== */

/**
 * Update build vector (CCSDS Equation 6).
 */
void pocket_update_build(
    bitvector_t *build,
    const bitvector_t *input,
    const bitvector_t *prev_input,
    int new_mask_flag,
    size_t t
);

/**
 * Update mask vector (CCSDS Equation 7).
 */
void pocket_update_mask(
    bitvector_t *mask,
    const bitvector_t *input,
    const bitvector_t *prev_input,
    const bitvector_t *build_prev,
    int new_mask_flag
);

/**
 * Compute change vector (CCSDS Equation 8).
 */
void pocket_compute_change(
    bitvector_t *change,
    const bitvector_t *mask,
    const bitvector_t *prev_mask,
    size_t t
);

/* ========================================================================
 * Encoding API (CCSDS Section 5.2)
 * ======================================================================== */

/**
 * Counter encoding (CCSDS Section 5.2.2, Equation 9).
 */
int pocket_count_encode(bitbuffer_t *output, uint32_t A);

/**
 * Run-length encoding (CCSDS Section 5.2.3, Equation 10).
 */
int pocket_rle_encode(bitbuffer_t *output, const bitvector_t *input);

/**
 * Bit extraction (CCSDS Section 5.2.4, Equation 11).
 */
int pocket_bit_extract(
    bitbuffer_t *output,
    const bitvector_t *data,
    const bitvector_t *mask
);

/* ========================================================================
 * Compression API (CCSDS Section 5.3)
 * ======================================================================== */

/**
 * Compressor state structure.
 */
struct pocket_compressor {
    /* Configuration (immutable after init) */
    size_t F;                                           /* Input vector length */
    bitvector_t initial_mask;                           /* M₀ - initial mask */
    uint8_t robustness;                                 /* Rₜ - robustness level */

    /* State (updated each cycle) */
    bitvector_t mask;                                   /* Mₜ - current mask */
    bitvector_t prev_mask;                              /* Mₜ₋₁ - previous mask */
    bitvector_t build;                                  /* Bₜ - build vector */
    bitvector_t prev_input;                             /* Iₜ₋₁ - previous input */
    bitvector_t change_history[POCKET_MAX_HISTORY];     /* Dₜ₋ᵢ - recent change vectors */
    size_t history_index;                               /* Current position in circular buffer */

    /* History for cₜ calculation (tracking new_mask_flag) */
    uint8_t new_mask_flag_history[POCKET_MAX_VT_HISTORY];  /* Last Vₜ new_mask_flag values */
    size_t flag_history_index;                              /* Index in flag history */

    /* Cycle counter */
    size_t t;                                           /* Current time index */
};

/**
 * Initialize a compressor.
 *
 * @param comp Compressor structure (allocated by caller)
 * @param F Input vector length (1 to POCKET_MAX_PACKET_LENGTH)
 * @param initial_mask M₀ - initial mask (NULL = all zeros)
 * @param robustness Rₜ - robustness level (0-7)
 * @return POCKET_OK on success, error code otherwise
 */
int pocket_compressor_init(
    pocket_compressor_t *comp,
    size_t F,
    const bitvector_t *initial_mask,
    uint8_t robustness
);

/**
 * Reset compressor to initial state (t=0).
 */
void pocket_compressor_reset(pocket_compressor_t *comp);

/**
 * Compress one input packet (CCSDS Section 5.3).
 *
 * @param comp Compressor state (updated in place)
 * @param input Iₜ - input packet (must be length F)
 * @param output Compressed output buffer
 * @param params Compression parameters (NULL = defaults)
 * @return POCKET_OK on success, error code otherwise
 */
int pocket_compress_packet(
    pocket_compressor_t *comp,
    const bitvector_t *input,
    bitbuffer_t *output,
    const pocket_params_t *params
);

/**
 * Compute robustness window Xₜ (ALGORITHM.md).
 * Xₜ = <(Dₜ₋ᴿₜ OR ... OR Dₜ)> where <a> means reverse
 *
 * @param Xt Output robustness window (reversed, ready for RLE)
 * @param comp Compressor state with change history
 * @param current_change Dₜ - current change vector
 */
void pocket_compute_robustness_window(
    bitvector_t *Xt,
    const pocket_compressor_t *comp,
    const bitvector_t *current_change
);

/**
 * Compute effective robustness Vₜ (ALGORITHM.md).
 * Vₜ = Rₜ + Cₜ where Cₜ = consecutive iterations with no mask changes
 *
 * @param comp Compressor state with change history
 * @param current_change Dₜ - current change vector
 * @return Vₜ (0-15)
 */
uint8_t pocket_compute_effective_robustness(
    const pocket_compressor_t *comp,
    const bitvector_t *current_change
);

/**
 * Check if Xₜ contains positive mask updates (eₜ flag).
 * eₜ = 1 if any changed bits are predictable (mask bit = 0), else 0
 *
 * @param Xt Robustness window (reversed)
 * @param mask Current mask Mₜ
 * @return 1 if positive updates exist, 0 otherwise
 */
int pocket_has_positive_updates(
    const bitvector_t *Xt,
    const bitvector_t *mask
);

/**
 * Compute cₜ flag (multiple new_mask_flag sets).
 * cₜ = 1 if new_mask_flag was set 2+ times in last Vₜ iterations
 *
 * @param comp Compressor state with flag history
 * @param Vt Effective robustness level
 * @return 1 if multiple updates, 0 otherwise
 */
int pocket_compute_ct_flag(
    const pocket_compressor_t *comp,
    uint8_t Vt
);

/* ========================================================================
 * Utility API
 * ======================================================================== */

const char* pocket_version_string(void);
const char* pocket_error_string(int error_code);

#endif /* POCKET_PLUS_H */
