# POCKET+ Implementation Gotchas

**⚠️ READ THIS BEFORE IMPLEMENTING!**

This document contains **critical implementation pitfalls** that will cause your output to diverge from the reference implementation. These are **not obvious** from reading the CCSDS spec and were discovered through extensive debugging.

Each gotcha includes:
- ✅ **What the spec says** (or doesn't say clearly)
- ❌ **Common mistake** that seems reasonable but is wrong
- 🔧 **Correct implementation**
- 📊 **Impact** when you get it wrong

---

## Table of Contents

1. [Initialization Phase: First Rₜ+1 Packets (Not Rₜ+2!)](#1-initialization-phase-first-rₜ1-packets-not-rₜ2)
2. [Flag Timing: Countdown Counters, Not Modulo Arithmetic](#2-flag-timing-countdown-counters-not-modulo-arithmetic)
3. [Vₜ Calculation: Start from Rₜ+1, Not Position 2!](#3-vₜ-calculation-start-from-rₜ1-not-position-2)
4. [Packet Indexing: 0-Based vs 1-Based in Flag Calculations](#4-packet-indexing-0-based-vs-1-based-in-flag-calculations)
5. [Component kₜ: Inverted Mask Values (Not Direct Mask Values!)](#5-component-kₜ-inverted-mask-values-not-direct-mask-values)
6. [Component kₜ: Forward Extraction Order (Not Reverse!)](#6--component-kₜ-forward-extraction-order-not-reverse)
7. [Reference Implementation's Final Padding (FIXED)](#-gotcha-7-reference-implementations-final-padding-fixed)
8. [cₜ Calculation: Include Current Packet's pₜ Flag!](#8-cₜ-calculation-include-current-packets-pₜ-flag)

---

## 1. Initialization Phase: First Rₜ+1 Packets (Not Rₜ+2!)

### ✅ What the Spec Says

The CCSDS spec states that the **first Rₜ+1 packets** must be sent uncompressed with ḟₜ=1, ṙₜ=1, ṗₜ=0.

### ❌ Common Mistake

Applying init phase to Rₜ+2 packets instead of Rₜ+1 packets.

**Example for Rₜ=1:**
- Packet 0 (first packet): init phase → ḟₜ=1, ṙₜ=1, ṗₜ=0
- Packet 1 (second packet): init phase → ḟₜ=1, ṙₜ=1, ṗₜ=0
- Packet 2 (third packet): **❌ WRONG: still in init phase**

### 🔧 Correct Implementation

```c
// Correct condition (0-based indexing)
if (packet_index <= Rt) {
    params.send_mask_flag = 1;
    params.uncompressed_flag = 1;
    params.new_mask_flag = 0;
}
```

**Example for Rₜ=1:**
- Packet 0 (i=0): init phase → ḟₜ=1, ṙₜ=1, ṗₜ=0
- Packet 1 (i=1): init phase → ḟₜ=1, ṙₜ=1, ṗₜ=0
- Packet 2 (i=2): ✅ **normal operation begins**

### 📊 Impact

- **Divergence:** Within first 10-20 packets
- **Symptom:** Flag timing is off by one packet throughout entire stream
- **Detection:** Compare flag values with reference for packets 0-10

---

## 2. Flag Timing: Countdown Counters, Not Modulo Arithmetic

### ✅ What the Spec Says

The reference implementation uses **countdown counters** that start at the period limit and decrement each packet. Flags trigger when the counter reaches 1, then reset.

### ❌ Common Mistake

Using modulo arithmetic: `(packet_num % period) == 0`

This triggers **one packet too early**:
- ṗₜ=1 at packets: **10, 20, 30, 40...** ❌ WRONG
- ḟₜ=1 at packets: **20, 40, 60, 80...** ❌ WRONG
- ṙₜ=1 at packets: **50, 100, 150, 200...** ❌ WRONG

### 🔧 Correct Implementation

**Pattern for Rₜ=1, periods (10, 20, 50):**
- ṗₜ=1 (new mask) at packets: **11, 21, 31, 41...** ✅ CORRECT
- ḟₜ=1 (send mask) at packets: **21, 41, 61, 81...** ✅ CORRECT
- ṙₜ=1 (uncompressed) at packets: **51, 101, 151, 201...** ✅ CORRECT

**Key insight:** First trigger happens at `period + Rₜ`, not at `period`.

**Example for pt_period=10, Rₜ=1:**
```c
int pt_first_trigger = pt_period + Rt;  // 10 + 1 = 11
int packet_num = i + 1;  // Convert 0-based to 1-based

if (packet_num >= pt_first_trigger &&
    packet_num % pt_period == (pt_first_trigger % pt_period)) {
    pt_flag = 1;  // Triggers at packets 11, 21, 31, ...
}
```

### 📊 Impact

- **Divergence:** Byte 30-50 (depending on periods)
- **Symptom:** Wrong mask updates, missing qₜ components, wrong compression modes
- **Size error:** ±10% output size difference
- **Detection:** Check flag values for packets 10-12, 20-22

---

## 3. Vₜ Calculation: Start from Rₜ+1, Not Position 2!

### ✅ What the Spec Says

Per CCSDS Section 5.3.2.2, Cₜ is defined as the largest value where **D_{t-i} = ∅ for all 1 < i ≤ Cₜ + Rₜ**.

### ❌ Common Mistake

Starting from i=2 regardless of Rₜ value:

```c
// WRONG: Always starts from i=2
for (int i = 2; i <= 15; i++) {
    if (Dt[t-i] != 0) break;
    Ct++;
}
```

**Why this seems reasonable:** The spec says "1 < i", which suggests starting from i=2.

**Why it's wrong:** The reference implementation starts from position **Rₜ+1** in the history buffer, not from position 2. For Rₜ=1, this happens to be 2. For Rₜ=2, it's 3. The general formula is:

```
start_position = Rt + 1
```

### 🔧 Correct Implementation

```c
// CORRECT: Start from Rt+1 positions back
int Ct = 0;
for (int i = Rt + 1; i <= 15 && i <= t; i++) {  // i starts from Rt+1!
    size_t hist_idx = (history_index - i + HISTORY_SIZE) % HISTORY_SIZE;
    if (change_history[hist_idx] != 0) break;  // Found a change
    Ct++;
    if (Ct >= 15 - Rt) break;  // Maximum Ct
}
Vt = Rt + Ct;
```

**Example for Rₜ=1:**
- Start from i=2 (Rₜ+1=2)
- Check D_{t-2}, D_{t-3}, ...
- Skip D_{t-1}

**Example for Rₜ=2:**
- Start from i=3 (Rₜ+1=3)
- Check D_{t-3}, D_{t-4}, ...
- Skip D_{t-1} AND D_{t-2}

### 📊 Impact

- **Divergence:** Within first 5-10 packets for Rₜ>1
- **Symptom:** Wrong Vₜ values in component hₜ, byte-level mismatches
- **Size error:** Small (few bits per packet), but compounds over stream
- **Detection:** Print Vₜ values for packets 0-5 and compare with reference
- **Affected tests:** R=2 tests (housekeeping, venus-express) will have byte mismatches

---

## 4. Packet Indexing: 0-Based vs 1-Based in Flag Calculations

### ✅ What the Spec Says

The CCSDS reference implementation uses **1-based packet numbering** (packets 1, 2, 3...).

### ❌ Common Mistake

Using loop index `i` directly for flag calculations when implementing with 0-based indexing:

```c
// WRONG: Uses 0-based index directly
for (int i = 0; i < num_packets; i++) {
    if (i % pt_period == 0) {  // ❌ Triggers at i=10, 20, 30...
        pt_flag = 1;
    }
}
```

**Impact table:**

| Loop Index (i) | Packet Number | Expected ṗₜ | Wrong (using i) | Impact |
|----------------|---------------|-------------|-----------------|---------|
| i=10 | Packet 11 | **1** (trigger!) | 0 | Flag missed! |
| i=20 | Packet 21 | **1** (trigger!) | 0 | Flag missed! |
| i=30 | Packet 31 | **1** (trigger!) | 0 | Flag missed! |

### 🔧 Correct Implementation

```c
// CORRECT: Convert to 1-based packet numbering
for (int i = 0; i < num_packets; i++) {
    int packet_num = i + 1;  // 0-based → 1-based conversion

    if (packet_num >= pt_first_trigger &&
        packet_num % pt_period == (pt_first_trigger % pt_period)) {
        pt_flag = 1;  // ✅ Correctly triggers at packets 11, 21, 31...
    }
}
```

### 📊 Impact

- **Divergence:** Byte 200-300 (30-50% into stream)
- **Symptom:** Wrong flags cause complete corruption
- **Size error:** 20-30% size difference
- **Why this matters:**
  - Wrong flags corrupt the dₜ calculation
  - Wrong ṗₜ causes mask/build updates at incorrect times
  - Wrong ḟₜ omits required mask transmission
  - Wrong ṙₜ sends compressed data when uncompressed is expected

---

## 5. Component kₜ: Inverted Mask Values (Not Direct Mask Values!)

### ✅ What the Spec Says

The kₜ component encodes mask values at changed positions, but **outputs the INVERSE** of the mask bits.

**CCSDS spec says:** "kₜ: mask values for changed positions"

**What this actually means:** Output '1' for positive updates (mask changed to 0), '0' for negative updates (mask changed to 1)

### ❌ Common Mistake

Extracting mask values directly:

```c
// WRONG: Direct mask extraction
kt = BE(mask, Xt);  // ❌ Extracts mask bits directly
```

### 🔧 Correct Implementation

```c
// CORRECT: Extract INVERTED mask values
for (int i = 0; i < mask.length; i++) {
    inverted_mask[i] = !mask[i];  // Invert the entire mask
}
kt = BE(inverted_mask, Xt);  // Extract inverted values at changed positions
```

**Correct encoding:**
- When Xₜ has '1' at position i (bit changed):
  - If mask[i] = 0 (now predictable): output **1** in kₜ
  - If mask[i] = 1 (now unpredictable): output **0** in kₜ
- This is the **INVERSE** of the mask values

**Example:**
- Xₜ = '1' at positions [43, 142]
- Mask values at those positions: [0, 0] (both predictable)
- kₜ output: **11** (not 00!)

### 📊 Impact

- **Divergence:** Byte 200-300 (30-50% into stream)
- **Symptom:** 1-bit error per changed position
- **Size error:** No size change (bit-level corruption)
- **Why this matters:**
  - The eₜ flag indicates if there are "positive updates" (mask bits changed from 1→0)
  - kₜ outputs '1' to mark these positive updates
  - Extracting mask values directly gives the opposite encoding

---

## 6. ⭐ Component kₜ: Forward Extraction Order (Not Reverse!)

**⭐ Latest Discovery - December 2025**

### ✅ What the Spec Says (or Doesn't Say!)

**The CCSDS spec is SILENT on kₜ extraction order.**

CCSDS 124.0-B-1 Section 5.3.2.2 states:
> **kₜ:** mask values for changed positions

That's all. No extraction order, no reference to BE, no algorithm specified.

**However, the BE function IS explicitly defined with reverse order:**
```
BE(a, b) = a_{g_{H(b)-1}} ∥ ... ∥ a_{g₁} ∥ a_{g₀}
```
(gᵢ ordered from highest to lowest position)

**The reference implementation uses FORWARD ORDER for kₜ** (lowest position index to highest), which is DIFFERENT from BE.

### ❌ Common Mistake

Using the same extraction order as BE (Bit Extract) for unpredictable bits, which extracts in **REVERSE ORDER** (highest position to lowest):

```c
// WRONG: Reverse order (works for BE, but NOT for kt!)
for (int i = num_positions - 1; i >= 0; i--) {
    output_bit(mask_values[positions[i]]);
}
```

**Why this seems reasonable:**
- The BE operation (for uₜ component) uses reverse order
- The reference implementation processes words from high to low
- It's natural to assume all bit extraction uses the same order

**Example of wrong output:**
- Xₜ has '1' at positions: 141, 142, 431
- Mask values: [1, 1, 0] (at positions 141, 142, 431)
- Inverted: [0, 0, 1]
- **Wrong (reverse):** Extracts 431, 142, 141 → outputs `100` ❌
- **Correct (forward):** Extracts 141, 142, 431 → outputs `001` ✅

### 🔧 Correct Implementation

**Two separate functions needed:**

```c
// For kₜ component: FORWARD order (low to high position)
int pocket_bit_extract_forward(bitbuffer_t *output,
                                const bitvector_t *data,
                                const bitvector_t *mask) {
    // Collect positions where mask has '1' bits
    for (int i = 0; i < mask->length; i++) {
        if (get_bit(mask, i)) {
            positions[count++] = i;
        }
    }

    // Extract in FORWARD order (low to high position)
    for (int i = 0; i < count; i++) {  // ✅ Forward: i increasing
        output_bit(get_bit(data, positions[i]));
    }
}

// For uₜ component (BE): REVERSE order (high to low position)
int pocket_bit_extract(bitbuffer_t *output,
                        const bitvector_t *data,
                        const bitvector_t *mask) {
    // Collect positions where mask has '1' bits
    for (int i = 0; i < mask->length; i++) {
        if (get_bit(mask, i)) {
            positions[count++] = i;
        }
    }

    // Extract in REVERSE order (high to low position)
    for (int i = count - 1; i >= 0; i--) {  // ✅ Reverse: i decreasing
        output_bit(get_bit(data, positions[i]));
    }
}
```

**Usage:**
```c
// Component kₜ (mask values at changed positions)
inverted_mask = NOT(mask);
pocket_bit_extract_forward(output, inverted_mask, Xt);  // ✅ Forward order

// Component uₜ (unpredictable bits)
pocket_bit_extract(output, input, mask);  // ✅ Reverse order
```

### 📊 Impact

**Before fix (using reverse order for kₜ):**
- **Divergence in late-stream packets** (typically 40-60% into output)
- Symptom: Multi-bit shift errors in packets with mask changes
- Error pattern: Bit-reversed kₜ component causes cumulative offset
- Packets without mask changes still match (Xₜ = ∅, so no kₜ output)

**After fix (using forward order for kₜ):**
- **✅ PERFECT BYTE-FOR-BYTE MATCH!**
- No divergence in compressed data
- Perfect byte-for-byte match with fixed reference implementation
- Prefix match: **100%** of generated compressed data

### 🔍 Why This Was Hard to Find

1. **Spec is completely silent** - CCSDS 124.0-B-1 doesn't specify kₜ extraction order at all
2. **Spec DOES specify BE as reverse** - Natural to assume kₜ uses same order as the explicitly-defined BE function
3. **Reasonable assumption fails** - Using your bit extraction function for kₜ seems logical but is wrong
4. **Late divergence** - Error only appears when kₜ is encoded (packets with H(Xₜ) > 0 and eₜ=1)
5. **Subtle symptom** - Produces valid output, just with bits in wrong order
6. **Reference code complexity** - Reference builds kₜ in a temporary buffer with backwards indexing, then reverses when concatenating
7. **Not derivable from spec** - You MUST examine reference implementation or test against reference output to discover this

### 🎯 Key Lesson

**Don't assume all bit extraction uses the same order!**

- **BE (unpredictable bits):** Reverse order (highest position first)
- **kₜ (mask values):** Forward order (lowest position first)

The reference implementation has different code paths for these operations, and the order matters!

---

## Detection Strategies

### Quick Smoke Tests

1. **Packet 0-2:** Compare first 3 packets byte-by-byte
   - Tests: Init phase, Vₜ calculation
   - Should match perfectly if #1-3 are correct

2. **Packets 10-12:** Check flag values
   - Tests: Flag timing, packet indexing
   - Print pt, ft, rt for each packet

3. **Packets 20-30:** Check for divergence
   - Tests: All flag-related issues
   - Should match if #1-4 are correct

4. **Packets 30-50:** Check kₜ component
   - Tests: kₜ inversion and extraction order
   - Look for systematic 1-bit or 2-bit errors

### Debugging Workflow

```bash
# 1. Generate debug output
./compress input.bin > output.bin 2> debug.log

# 2. Compare sizes
ls -l output.bin reference.bin  # Should match exactly

# 3. Find first divergence
diff -y <(xxd reference.bin) <(xxd output.bin) | head -50

# 4. Check flags
grep "packet [0-9]*: pt=" debug.log | head -30

# 5. Check Vt values
grep "Vt=" debug.log | head -10

# 6. Check kt encoding
grep "kt.*bits" debug.log
```

### Reference Values (Rₜ=1, periods 10/20/50)

**Packet indices (0-based) vs packet numbers (1-based):**
```
i=0  → packet 1  : ft=1, rt=1, pt=0  (init)
i=1  → packet 2  : ft=1, rt=1, pt=0  (init)
i=2  → packet 3  : ft=0, rt=0, pt=0  (normal starts)
i=10 → packet 11 : ft=0, rt=0, pt=1  (first pt trigger)
i=20 → packet 21 : ft=1, rt=0, pt=1  (first ft trigger + pt)
i=30 → packet 31 : ft=0, rt=0, pt=1  (pt only)
i=40 → packet 41 : ft=1, rt=0, pt=1  (ft + pt)
i=50 → packet 51 : ft=0, rt=1, pt=1  (first rt trigger + pt)
```

**Vₜ progression for typical input:**
```
Packet 0: Vt=1 (Rt, init phase)
Packet 1: Vt=1 (Rt, init phase)
Packet 2: Vt=2 (Rt + Ct, where Ct=1 from D0=∅)
Packet 3+: Varies based on mask changes
```

---

## Testing Checklist

Before declaring your implementation "working":

- All compressed bytes match reference output exactly
- Output size matches reference byte-for-byte
- First 10 packets compress correctly
- Flag triggers occur at correct boundaries (test pt, ft, rt periods)
- Mask transmission packets are correct
- Uncompressed packets trigger at correct intervals
- Vₜ values match reference for initialization and steady-state phases
- Vₜ calculation starts from Rₜ+1, not always from i=2
- cₜ calculation includes current packet's pₜ flag (Vₜ+1 total entries)
- No divergence before byte 300 (indicates kₜ issues)
- No systematic bit-shift errors (indicates ordering issues)
- kₜ component uses forward extraction order
- BE operation uses reverse extraction order
- Mask inversion is applied before kₜ extraction
- Both R=1 AND R=2 test vectors pass (different code paths!)

---

## Common Symptoms and Diagnosis

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| Divergence at byte 10-20 | Init phase wrong (#1) | Check Rₜ+1 condition |
| Divergence at byte 30-50 | Flag timing wrong (#2) | Check countdown logic |
| Divergence at byte 5-15 | Vₜ wrong (#3) | Start from Rₜ+1, not i=2 |
| Byte mismatch in R=2 tests | Vₜ start position wrong (#3) | Use i=Rₜ+1 not i=2 |
| Divergence at byte 200-300 | Packet indexing wrong (#4) | Convert i to packet_num |
| 1-bit errors in kₜ | kₜ not inverted (#5) | Invert mask before extraction |
| 2-bit shift at byte 300+ | kₜ extraction order wrong (#6) | Use forward order for kₜ |
| Size off by 10%+ | Multiple flag issues | Check #2 and #4 |
| Size matches but content wrong | Bit-level issues | Check #5 and #6 |
| Size off by ~100 bytes (10KB test) | cₜ missing current flag (#8) | Include current pₜ in cₜ count |
| edge-cases fails, simple passes | cₜ calculation wrong (#8) | Check Vₜ+1 entries for cₜ |

---

## Final Notes

**Trust your test vectors!**

When the first 40 packets match perfectly, your fundamental algorithm is correct. Don't second-guess working code based on later divergences—investigate the specific point of failure.

**The spec is not always clear.**

Many of these gotchas are not explicitly stated in CCSDS 124.0-B-1. The only way to discover them is through:
1. Careful reading of the reference implementation
2. Byte-by-byte comparison with reference output
3. Systematic debugging of divergence points

**Order matters more than you think.**

- Packet numbering: 0-based vs 1-based
- Bit indexing: LSB-first vs MSB-first
- Bit extraction: forward vs reverse order
- Flag timing: countdown vs modulo

Each of these seemingly minor differences will cause your implementation to fail.

---

## 🎯 Gotcha #7: Reference Implementation's Final Padding (FIXED)

### ✅ Status: FIXED

The original ESA/ESOC reference implementation had a bug that added **2 extra null bytes** (`0x00 0x00`) at the end of the compressed output. This has been **fixed** in the current reference implementation.

### 📖 What Was Happening (Historical)

The original reference's `write_to_file()` function:
1. Wrote complete 32-bit words to the file
2. Called `fseek()` to rewind before unused bytes
3. Program exited **WITHOUT** calling `fclose(outputFile)`
4. **Bug:** The fseek didn't take effect, leaving extra padding bytes

### 🔧 The Fix

The reference implementation has been updated with two changes:
1. Added `fclose(outputFile)` before program exit
2. Added `ftruncate()` in `write_to_file()` to properly truncate the file at the correct position

See [../test-vector-generator/c-reference/CHANGES.md](../test-vector-generator/c-reference/CHANGES.md) for details.

### ✅ Current Status

**Test vectors have been regenerated with the fix:**
- All test vectors now contain only compressed data with no spurious padding
- Your implementation should match the reference byte-for-byte
- No workarounds needed for the 2-byte difference

### 💡 Correct Approach

- Your output should be exactly the compressed data
- Byte-boundary padding per packet (if used) is standard
- Expect perfect byte-for-byte matches with current test vectors

---

## 8. cₜ Calculation: Include Current Packet's pₜ Flag!

**⭐ Discovery - December 2025**

### ✅ What the Spec Says

Per CCSDS Section 5.3.2.2 (Equation 20), cₜ = 1 if the new_mask_flag (pₜ) was set **2 or more times** in the last Vₜ iterations.

### ❌ Common Mistake

Only checking historical pₜ flags without including the **current packet's** pₜ flag:

```c
// WRONG: Only checks history, misses current packet
int count = 0;
for (int i = 0; i < Vt; i++) {
    size_t hist_idx = (flag_history_index - 1 - i) % HISTORY_SIZE;
    if (new_mask_flag_history[hist_idx]) count++;
}
return (count >= 2) ? 1 : 0;
```

**Why this seems reasonable:** The current pₜ flag hasn't been stored in history yet when cₜ is computed.

**Why it's wrong:** The reference implementation stores the current pₜ flag BEFORE computing cₜ, so it includes Vₜ+1 total entries (current + Vₜ historical). Your implementation must match this behavior.

### 🔧 Correct Implementation

Either store the current flag before computing cₜ, or include it explicitly:

```c
// CORRECT: Include current packet's flag in the count
int pocket_compute_ct_flag(
    const pocket_compressor_t *comp,
    uint8_t Vt,
    int current_new_mask_flag  // Pass current packet's pt flag
) {
    if (Vt == 0) return 0;

    int count = 0;

    // Include current packet's flag
    if (current_new_mask_flag) count++;

    // Check Vt historical entries
    for (size_t i = 0; i < Vt && i < comp->t; i++) {
        size_t hist_idx = (comp->flag_history_index - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
        if (comp->new_mask_flag_history[hist_idx]) count++;
    }

    return (count >= 2) ? 1 : 0;  // ct=1 if 2+ flags set
}
```

**Reference behavior:**
- Stores `pt_history[pt_history_index] = pt` before computing cₜ
- Loops from `pt_history_index` to `pt_history_index + Vt` (inclusive)
- Total entries checked: Vₜ + 1 (current + Vₜ historical)

### 📊 Impact

- **Divergence:** Mid-stream (typically 500+ bytes into output)
- **Symptom:** Output size mismatch, wrong extraction mask used
- **Size error:** Significant (~100+ bytes on 10KB test)
- **Detection:** Compare with edge-cases test vector (exercises this path)
- **Affected tests:** edge-cases.bin shows ~109 byte size difference without this fix

### 🔍 Why This Was Hard to Find

1. **R=1 tests may pass** - simple.bin passed because of specific data patterns
2. **R=2 tests have different issues** - Vₜ calculation bug masked this in housekeeping/venus-express
3. **Late divergence** - Only triggers when pₜ is set multiple times within Vₜ window
4. **Order of operations** - Reference stores flag then computes cₜ; easy to compute first then store
5. **Size difference misleading** - Looks like encoding bug, not flag counting bug

---

**If you discover new gotchas, please document them here!**
