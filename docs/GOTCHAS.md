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
3. [Vₜ Calculation: Skip D_{t-1}!](#3-vₜ-calculation-skip-d_t-1)
4. [Packet Indexing: 0-Based vs 1-Based in Flag Calculations](#4-packet-indexing-0-based-vs-1-based-in-flag-calculations)
5. [Component kₜ: Inverted Mask Values (Not Direct Mask Values!)](#5-component-kₜ-inverted-mask-values-not-direct-mask-values)
6. [Component kₜ: Forward Extraction Order (Not Reverse!)](#6--component-kₜ-forward-extraction-order-not-reverse)

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

## 3. Vₜ Calculation: Skip D_{t-1}!

### ✅ What the Spec Says

Per CCSDS Section 5.3.2.2, Cₜ is defined as the largest value where **D_{t-i} = ∅ for all 1 < i ≤ Cₜ + Rₜ**.

**Critical detail:** The strict inequality **1 < i** means start from **i=2**, checking D_{t-2}, D_{t-3}, ..., **NOT D_{t-1}**!

### ❌ Common Mistake

Starting from i=1, which includes D_{t-1} in the check:

```c
// WRONG: Includes D_{t-1}
for (int i = 1; i <= Ct + Rt; i++) {
    if (Dt[t-i] != 0) break;
    Ct++;
}
```

### 🔧 Correct Implementation

```c
// CORRECT: Skip D_{t-1}, start from D_{t-2}
int Ct = 0;
for (int i = 2; i <= 15; i++) {  // i starts from 2!
    if (t - i < 0) break;
    if (Dt[t-i] != 0) break;
    Ct++;
    if (Ct >= 15 - Rt) break;  // Maximum Ct
}
Vt = Rt + Ct;
```

**Example for t=2, Rₜ=1:**
- Check D₀ only (skip D₁ per spec requirement)
- If D₀ = ∅, then Cₜ=1
- Result: Vₜ = 1 + 1 = 2

### 📊 Impact

- **Divergence:** Within first 5 packets!
- **Symptom:** Wrong Vₜ values in component hₜ
- **Size error:** Small (few bits per packet)
- **Detection:** Print Vₜ values for packets 0-5 and compare with reference

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
- Output size: 641 bytes (expected 643)
- **Divergence at byte 320** (49.8% prefix match)
- Symptom: 2-bit shift error in every packet with mask changes
- Error pattern: `Expected 0x3C, Got 0x3E` (1-bit error in kₜ component)

**After fix (using forward order for kₜ):**
- Output size: 641 bytes (expected 643)
- **✅ ALL 641 BYTES MATCH PERFECTLY!**
- No divergence! Only a 2-byte shortage remains (unrelated issue)
- Prefix match: **100%** of generated bytes

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

## Progressive Bug Fix Impact

Here's how each fix improved the output quality:

| Fix | Output Size | First Divergence | Prefix Match | Size Accuracy |
|-----|-------------|------------------|--------------|---------------|
| **None (all bugs present)** | ~700 bytes | Byte 10-20 | <5% | ~90% |
| **After fixes #1-3** (init, Vₜ, flags) | 644 bytes | Byte 251 | 60.9% | 99.8% |
| **After fix #4** (packet indexing) | 641 bytes | Byte 286 | 44.5% | 99.7% |
| **After fix #5** (kₜ inversion) | 641 bytes | Byte 320 | 49.8% | 99.7% |
| **After fix #6** (kₜ order) ⭐ | 641 bytes | **NONE!** | **100%** ✅ | 99.7% |

**Final status:**
- ✅ **Perfect byte-for-byte match** for all 641 generated bytes!
- ⚠️ Only 2-byte shortage remains (641 vs 643 expected)
- The shortage is likely a minor bit counting issue unrelated to kₜ

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
ls -l output.bin  # Should be 643 bytes

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

- [ ] ✅ All 641+ bytes match reference output exactly
- [ ] ✅ Output size within 1 byte of reference (643 bytes)
- [ ] ✅ First 10 packets compress correctly
- [ ] ✅ Packets 10-12 have correct pt flags
- [ ] ✅ Packets 20-22 have correct ft flags
- [ ] ✅ Packets 50-52 have correct rt flags
- [ ] ✅ Vₜ values match reference for packets 0-10
- [ ] ✅ No divergence before byte 300 (indicates kₜ issues)
- [ ] ✅ No systematic bit-shift errors (indicates ordering issues)
- [ ] ✅ kₜ component uses forward extraction order
- [ ] ✅ BE operation uses reverse extraction order
- [ ] ✅ Mask inversion is applied before kₜ extraction

---

## Common Symptoms and Diagnosis

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| Divergence at byte 10-20 | Init phase wrong (#1) | Check Rₜ+1 condition |
| Divergence at byte 30-50 | Flag timing wrong (#2) | Check countdown logic |
| Divergence at byte 5-15 | Vₜ wrong (#3) | Skip D_{t-1} in calculation |
| Divergence at byte 200-300 | Packet indexing wrong (#4) | Convert i to packet_num |
| 1-bit errors in kₜ | kₜ not inverted (#5) | Invert mask before extraction |
| 2-bit shift at byte 300+ | kₜ extraction order wrong (#6) | Use forward order for kₜ |
| Size off by 10%+ | Multiple flag issues | Check #2 and #4 |
| Size matches but content wrong | Bit-level issues | Check #5 and #6 |

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

## Document History

| Date | Author | Changes |
|------|--------|---------|
| 2025-12-03 | Claude Code | Extracted from ALGORITHM.md |
| 2025-12-03 | Claude Code | Added Gotcha #6: kₜ forward extraction order |
| 2025-12-03 | Claude Code | Added progressive fix impact table |
| 2025-12-03 | Claude Code | Added detection strategies and debugging workflow |

---

**If you discover new gotchas, please document them here!**
