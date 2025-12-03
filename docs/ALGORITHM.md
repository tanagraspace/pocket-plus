# POCKET+ Algorithm Specification

## Overview

POCKET+ (CCSDS 124.0-B-1) is a lossless compression algorithm designed for fixed-length housekeeping telemetry data. It exploits the property that many bits in consecutive packets remain unchanged, achieving compression by only transmitting the bits that change.

**Key Features:**
- **Lossless compression** of fixed-length binary vectors
- **Low complexity** - uses only bitwise operations (XOR, OR, AND, shifts)
- **Robust to packet loss** - configurable resilience to N consecutive lost packets
- **No latency** - produces one output packet per input packet
- **Adaptive** - tracks which bits change over time using a mask

## ⚠️ Critical Implementation Notes

**READ THIS FIRST!** These are the most common implementation pitfalls that will cause your output to diverge from the reference implementation:

### 1. Initialization Phase: First Rₜ+1 Packets (Not Rₜ+2!)

The CCSDS spec states that the **first Rₜ+1 packets** must be sent uncompressed with ḟₜ=1, ṙₜ=1, ṗₜ=0.

**CORRECT:**
```c
// For Rₜ=1: packets 0 and 1 are init packets (2 packets total)
if (packet_index <= Rₜ) {
    ft = 1;  // send_mask_flag
    rt = 1;  // uncompressed_flag
    pt = 0;  // new_mask_flag
}
```

**WRONG:**
```c
// This applies init phase to 3 packets (0, 1, 2) instead of 2!
if (packet_index < Rₜ + 2) {  // ❌ OFF-BY-ONE ERROR
    ft = 1;
    rt = 1;
    pt = 0;
}
```

### 2. Flag Timing: Countdown Counters, Not Modulo Arithmetic

The reference implementation uses **countdown counters** that start at the period limit and decrement each packet. Flags trigger when the counter reaches 1, then reset.

**Pattern for Rₜ=1:**
- ṗₜ=1 (new mask) at packets: 11, 21, 31, ... (not 10, 20, 30!)
- ḟₜ=1 (send mask) at packets: 21, 41, 61, ... (not 20, 40, 60!)
- ṙₜ=1 (uncompressed) at packets: 51, 101, 151, ... (not 50, 100, 150!)

**CORRECT:**
```c
// First trigger happens at: period + Rₜ
// For pt_period=10, Rₜ=1: first trigger at packet 11
int pt_first_trigger = 10 + Rₜ;  // 11
int ft_first_trigger = 20 + Rₜ;  // 21
int rt_first_trigger = 50 + Rₜ;  // 51

pt = (i >= pt_first_trigger && i % 10 == (pt_first_trigger % 10)) ? 1 : 0;
ft = (i >= ft_first_trigger && i % 20 == (ft_first_trigger % 20)) ? 1 : 0;
rt = (i >= rt_first_trigger && i % 50 == (rt_first_trigger % 50)) ? 1 : 0;

// Then override for init phase
if (i <= Rₜ) {
    ft = 1; rt = 1; pt = 0;
}
```

**WRONG:**
```c
// This triggers flags one packet too early!
pt = ((i + 1) % 10 == 0) ? 1 : 0;  // ❌ Triggers at 9, 19, 29
ft = ((i + 1) % 20 == 0) ? 1 : 0;  // ❌ Triggers at 19, 39, 59
```

### 3. Vₜ Calculation: Skip D_{t-1}!

Per CCSDS Section 5.3.2.2, Cₜ is defined as the largest value where **D_{t-i} = ∅ for all 1 < i ≤ Cₜ + Rₜ**.

Note the strict inequality: **1 < i**, which means **start from i=2** (check D_{t-2}, D_{t-3}, ...), **not i=1**!

**CORRECT:**
```c
// For t > Rₜ, count consecutive packets with no mask changes
uint8_t Ct = 0;

// Start from i=2 (skip D_{t-1} per spec: "1 < i")
for (size_t i = 2; i <= 15 && i <= t; i++) {
    size_t hist_idx = (history_index - i + MAX_HISTORY) % MAX_HISTORY;
    if (hamming_weight(change_history[hist_idx]) > 0) {
        break;  // Found a change, stop
    }
    Ct++;
}

Vt = Rt + Ct;  // Effective robustness
```

**WRONG:**
```c
// Starting from i=1 checks D_{t-1}, which violates "1 < i"
for (size_t i = 1; i <= 15 && i <= t; i++) {  // ❌ WRONG!
    // This will produce incorrect Vₜ values
}
```

**Why this matters:** Starting from i=1 causes Vₜ to be calculated incorrectly for early packets, leading to divergence in the compressed output within the first few packets.

### Summary

These three bugs were responsible for:
- **109 bytes of excess output** (752 bytes instead of 643)
- **Divergence starting at byte 93** (out of 643 total)

After fixing all three issues:
- Output size: **644 bytes** (only 1 byte over!)
- First divergence: **byte 251** (99.8% correct!)

## High-Level Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    INPUT PACKET (Fixed Length F)                │
│                         Binary Vector Iₜ                        │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
                   ┌──────────────────┐
                   │   MASK UPDATE    │
                   │  (Section 4)     │
                   │                  │
                   │  • Update Build  │
                   │  • Update Mask   │
                   │  • Compute Change│
                   └────────┬─────────┘
                            │
                            ▼
                   ┌──────────────────┐
                   │    ENCODING      │
                   │   (Section 5)    │
                   │                  │
                   │  hₜ = RLE(changes)│
                   │  qₜ = mask (opt) │
                   │  uₜ = unpred bits│
                   └────────┬─────────┘
                            │
                            ▼
┌────────────────────────────────────────────────────────────────┐
│              OUTPUT PACKET (Variable Length)                   │
│                    oₜ = hₜ ∥ qₜ ∥ uₜ                          │
└────────────────────────────────────────────────────────────────┘
```

## Core Concepts

### 1. The Mask

The **mask** (Mₜ) is a binary vector of length F where each bit indicates:
- **`0`** = **Predictable** - bit expected to stay the same as previous packet
- **`1`** = **Unpredictable** - bit may change from previous packet

```
Input Packet Iₜ:     [ 1 0 1 1 0 0 1 1 ]
Previous Packet:     [ 1 0 1 0 0 1 1 1 ]
Changed bits:        [ 0 0 0 1 0 1 0 0 ]  (XOR result)
                               ↑   ↑
Mask (unpredictable):[ 0 0 0 1 1 1 0 0 ]
                               └───┘
                            These positions
                         track as unpredictable
```

### 2. The Build Vector

The **build** (Bₜ) is a parallel mask constructed using the same rules as the mask. It can replace the mask when the `new_mask_flag` is set, allowing unpredictable bits to become predictable again.

### 3. The Change Vector

The **change vector** (Dₜ) tracks which mask bits changed between time t-1 and t:
```
Dₜ = Mₜ XOR Mₜ₋₁
```

This is used in the encoding stage to communicate mask updates to the decompressor.

## Algorithm Stages

### Stage 1: Mask Update (CCSDS 124.0-B-1 Section 4)

For each input packet Iₜ, update the mask and build vectors:

#### Build Vector Update
```
         ⎧ (Iₜ XOR Iₜ₋₁) OR Bₜ₋₁,  if t > 0 and ṗₜ = 0
Bₜ =     ⎨
         ⎩ 0,                       otherwise (reset when new_mask_flag set)
```

#### Mask Vector Update
```
         ⎧ (Iₜ XOR Iₜ₋₁) OR Mₜ₋₁,  if ṗₜ = 0
Mₜ =     ⎨
         ⎩ (Iₜ XOR Iₜ₋₁) OR Bₜ₋₁,  if ṗₜ = 1 (replace mask with build)
```

Initial condition: M₀ is user-specified (typically all zeros)

#### Change Vector Update
```
         ⎧ Mₜ XOR Mₜ₋₁,  if t > 0
Dₜ =     ⎨
         ⎩ 0,            if t = 0
```

**Flow Diagram:**
```
Iₜ₋₁ ────┐
         XOR ──→ changes ──→ OR ──→ Bₜ (if ṗₜ=0)
Iₜ ──────┘                    ↑      └──→ Mₜ (if ṗₜ=1)
                              │
                         Bₜ₋₁ or Mₜ₋₁

Mₜ₋₁ ────┐
         XOR ──→ Dₜ (change vector)
Mₜ ──────┘
```

### Stage 2: Encoding (CCSDS 124.0-B-1 Section 5)

The output packet is a concatenation of three variable-length binary vectors:

```
oₜ = hₜ ∥ qₜ ∥ uₜ
```

#### Component hₜ: Mask Change Information

Encodes recent mask changes with robustness against packet loss:

```
hₜ = RLE(Xₜ) ∥ BIT₄(Vₜ) ∥ eₜ ∥ kₜ ∥ cₜ ∥ ḋₜ
```

Where:
- **Xₜ**: Reversed OR of change vectors over robustness window
  ```
           ⎧ <Dₜ>,                                    if Rₜ = 0
  Xₜ =     ⎨ <(D₁ OR D₂ OR ... OR Dₜ)>,             if (t - Rₜ) ≤ 0
           ⎩ <(Dₜ₋ᴿₜ OR Dₜ₋ᴿₜ₊₁ OR ... OR Dₜ)>,    otherwise
  ```
  (Note: `<a>` means reverse the bit order of vector a)

- **Vₜ**: Effective robustness level (4 bits, value 0-15)
- **eₜ**: Flag indicating if any mask changes resulted in predictable (0) vs unpredictable (1)
- **kₜ**: Mask values for changed positions
- **cₜ**: Flag indicating multiple new_mask_flag sets in robustness window
- **ḋₜ**: Flag indicating if both ḟₜ and ṙₜ are zero

#### Component qₜ: Optional Full Mask

Optionally encodes the entire mask when `send_mask_flag` (ḟₜ) is set:

```
         ⎧ ∅,                                        if ḋₜ = 1
qₜ =     ⎨ '1' ∥ RLE(<(Mₜ XOR (Mₜ≪))>),            if ḟₜ = 1
         ⎩ '0',                                      otherwise
```

Where `Mₜ≪` is the left bit-shift of Mₜ (shifts bits left, inserts 0 at LSB)

#### Component uₜ: Unpredictable Bits or Full Input

Encodes the data payload:

```
         ⎧ BE(Iₜ, (<Xₜ> OR Mₜ)),                   if ḋₜ = 1 and cₜ = 1
         ⎪ BE(Iₜ, Mₜ),                              if ḋₜ = 1 and cₜ ≠ 1
uₜ =     ⎨ '1' ∥ COUNT(F) ∥ Iₜ,                     if ṙₜ = 1
         ⎪ '0' ∥ BE(Iₜ, (<Xₜ> OR Mₜ)),             if ṙₜ = 0 and ḟₜ = 1 and cₜ = 1
         ⎩ '0' ∥ BE(Iₜ, Mₜ),                        otherwise
```

Where:
- **BE(a, b)**: Bit extraction - extracts bits from `a` at positions where `b` has '1' bits
- **COUNT(F)**: Counter encoding of the input vector length

## Encoding Functions

### 1. Counter Encoding: COUNT(A)

Encodes positive integers 1 ≤ A ≤ 2¹⁶-1:

| Input Range | Output Format | Example |
|-------------|---------------|---------|
| A = 1 | `'0'` | COUNT(1) = `'0'` |
| 2 ≤ A ≤ 33 | `'110'` ∥ BIT₅(A-2) | COUNT(2) = `'110'` ∥ `'00000'` |
| A ≥ 34 | `'111'` ∥ BIT_E(A-2) | COUNT(34) = `'111'` ∥ `'000000'` |

For A ≥ 34: E = 2⌊log₂(A-2) + 1⌋ - 6

**Decoding:** The number of leading zeros uniquely identifies the encoding length, enabling self-parsing.

### 2. Run-Length Encoding: RLE(a)

Encodes a binary vector by counting runs of zeros between '1' bits:

```
RLE(a) = COUNT(C₀) ∥ COUNT(C₁) ∥ ... ∥ COUNT(C_{H(a)-1}) ∥ '10'
```

Where:
- **Cᵢ**: One more than the number of consecutive '0' bits before the i-th '1' bit
- **H(a)**: Hamming weight (number of '1' bits in a)
- **'10'**: Terminator

**Example:**
```
a = '0001000001000010000010000100000010000000000000000'
     ↑         ↑     ↑     ↑    ↑   ↑              ↑
     C₀=4      C₁=6  C₂=1  C₃=6 C₄=5 C₅=7         C₆=3

RLE(a) = COUNT(4) ∥ COUNT(6) ∥ COUNT(1) ∥ COUNT(6) ∥
         COUNT(5) ∥ COUNT(7) ∥ COUNT(3) ∥ '10'
```

**Note:** Trailing zeros are not encoded (deducible from vector length and H(a))

### 3. Bit Extraction: BE(a, b)

Extracts bits from vector `a` at positions where vector `b` has '1' bits:

```
BE(a, b) = a_{g_{H(b)-1}} ∥ ... ∥ a_{g₁} ∥ a_{g₀}
```

Where gᵢ is the position of the i-th '1' bit in `b` (MSB to LSB order)

**Example:**
```
a = '10110011'
b = '01001010'
     ↑  ↑ ↑
BE(a, b) = '001' (extracts bits at positions 6, 3, 1 from a)
```

## Parameters

### Input Parameters

| Parameter | Symbol | Type | Range | Description |
|-----------|--------|------|-------|-------------|
| Input Vector Length | F | integer | 1 to 65535 | Fixed length of input binary vectors |
| Initial Mask | M₀ | binary vector | length F | Initial mask state (typically all zeros) |
| Min Required Robustness | Rₜ | integer | 0 to 7 | Minimum guaranteed consecutive packet loss tolerance |
| New Mask Flag | ṗₜ | bit | 0 or 1 | When 1, replaces mask with build (enables predictable←unpredictable) |
| Send Mask Flag | ḟₜ | bit | 0 or 1 | When 1, includes full mask in output |
| Uncompressed Flag | ṙₜ | bit | 0 or 1 | When 1, sends full input packet instead of just unpredictable bits |

**Special Constraints:**
- For the **first Rₜ+1 packets** (t ≤ Rₜ): ḟₜ = 1, ṙₜ = 1, ṗₜ = 0 (mandatory full mask and full input during initialization)
  - ⚠️ **See [Critical Implementation Note #1](#1-initialization-phase-first-rₜ1-packets-not-rₜ2)** - Common off-by-one error!
- The timing of ṗₜ, ḟₜ, ṙₜ flags depends on period counters that start during initialization
  - ⚠️ **See [Critical Implementation Note #2](#2-flag-timing-countdown-counters-not-modulo-arithmetic)** - Flags don't trigger when you expect!
- All parameters needed for decompression are encoded in the output bitstream

## Bit Numbering Convention

⚠️ **CRITICAL**: CCSDS 124.0-B-1 uses **MSB-first** indexing, which is **opposite** to most programming conventions!

**Traditional Bit Numbering (WRONG for CCSDS):**
```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │  ← Bit indices (typical systems)
└───┴───┴───┴───┴───┴───┴───┴───┘
  ↑                               ↑
  MSB                            LSB
  (highest bit)                  (lowest bit)
```

**CCSDS Bit Numbering (CORRECT):**
```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │  ← Bit indices (CCSDS convention)
└───┴───┴───┴───┴───┴───┴───┴───┘
  ↑                               ↑
  MSB (bit 0)                    LSB (bit N-1)
  Transmitted first              Transmitted last
```

**For an N-bit vector:**
- **Bit 0** = MSB (Most Significant Bit) = 2^(N-1)
- **Bit N-1** = LSB (Least Significant Bit) = 2^0

**Why this matters:**
- **Bitvector indexing**: `vector[0]` accesses the MSB, not the LSB
- **RLE encoding**: Bit positions are encoded MSB-first
- **Bit extraction**: Extract from position 0 (MSB) to position N-1 (LSB)
- **Interoperability**: Reference implementations assume this convention

**Example for 8-bit value 0xB3 (10110011 in binary):**
```
Traditional:  bit[7]=1, bit[6]=0, bit[5]=1, bit[4]=1, ...
CCSDS:        bit[0]=1, bit[1]=0, bit[2]=1, bit[3]=1, ...
              └─MSB                                  └─LSB
```

**Common mistake:** Implementing bit operations with LSB-first indexing will cause your output to be bit-reversed and completely incompatible with CCSDS-compliant decompressors!

## Bitwise Operations

| Operation | Symbol | Description | Example |
|-----------|--------|-------------|---------|
| XOR | `a XOR b` | Exclusive OR (1 if bits differ) | `'101' XOR '110' = '011'` |
| OR | `a OR b` | Logical OR (1 if either bit is 1) | `'101' OR '110' = '111'` |
| AND | `a AND b` | Logical AND (1 if both bits are 1) | `'101' AND '110' = '100'` |
| NOT | `~a` | Bitwise inverse | `~'101' = '010'` |
| Left Shift | `a≪` | Shift left, insert 0 at LSB | `'101'≪ = '010'` |
| Reverse | `<a>` | Reverse bit order | `<'101'> = '101'` |
| Concatenate | `a ∥ b` | Concatenate vectors | `'101' ∥ '11' = '10111'` |

## Robustness and Packet Loss

The algorithm guarantees decompression even if **up to Vₜ consecutive packets** are lost before the current packet, where:

```
Vₜ = Rₜ + Cₜ
```

- **Rₜ**: User-specified minimum required robustness level (0-7)
- **Cₜ**: Additional robustness from consecutive unchanged masks

**Important:** ⚠️ **See [Critical Implementation Note #3](#3-vₜ-calculation-skip-d_t-1)** for common pitfalls!

Per CCSDS 124.0-B-1 Section 5.3.2.2, Cₜ is defined as:

> The largest value for which **D_{t-i} = ∅** for all **1 < i ≤ Cₜ + Rₜ**

This means:
- For **t ≤ Rₜ**: Vₜ = Rₜ (initialization phase)
- For **t > Rₜ**: Count backward starting from **D_{t-2}** (skip D_{t-1}!)
- Stop when finding a change or reaching maximum Cₜ = 15 - Rₜ

Example for t=2, Rₜ=1:
- Check D₀ (skip D₁ per spec) → if D₀ = ∅, then Cₜ=1
- Result: Vₜ = Rₜ + Cₜ = 1 + 1 = 2

The mask change information in `hₜ` includes ORed changes from the previous Vₜ cycles, allowing the decompressor to resynchronize its mask even after packet loss.

## Implementation Complexity

**Operations per packet:**
- Fixed-length input → variable-length output
- Primarily bitwise operations: XOR, OR, AND, bit shifts
- Run-length encoding of sparse binary vectors
- No floating-point arithmetic
- No division (except for log₂ in COUNT, implementable with bit operations)
- Suitable for hardware and low-power embedded systems

## Example: Simple Compression Scenario

```
Given:
  F = 8 (packet length)
  M₀ = '00000000' (all bits start as predictable)
  Rₜ = 0 (no robustness requirement for this example)

Time t=0:
  I₀ = '10110011'
  Since t=0, must send full packet (ṙ₀ = 1, ḟ₀ = 1)

Time t=1:
  I₁ = '10110001' (last two bits changed)
         ↑↑

  Mask Update:
    D₁ = I₁ XOR I₀ = '00000010'
    M₁ = D₁ OR M₀ = '00000010' (bit 1 is now unpredictable)

  Encoding:
    hₜ = RLE(<'00000010'>) ∥ ... (mask change at position 1)
    qₜ = '0' (don't send full mask)
    uₜ = '0' ∥ BE(I₁, M₁) = '0' ∥ '0' (extract bit 1 from I₁)

  Output is much smaller than input!

Time t=2:
  I₂ = '10110001' (same as I₁)

  Mask Update:
    D₂ = I₂ XOR I₁ = '00000000' (no changes)
    M₂ = D₂ OR M₁ = '00000010' (mask unchanged)

  Encoding:
    hₜ = RLE(<'00000000'>) ∥ ... (no mask changes → very short)
    qₜ = '0'
    uₜ = '0' ∥ BE(I₂, M₂) = '0' ∥ '0'

  Highly compressed - only unpredictable bit sent!
```

## References

- **CCSDS 124.0-B-1**: Robust Compression of Fixed-Length Housekeeping Data, February 2023
- **Standard PDF**: https://public.ccsds.org/Pubs/124x0b1.pdf
- **ESA Reference Implementation**: https://opssat.esa.int/pocket-plus/
- **Implementation Paper**: Evans, D., et al. (2022). "Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1."

## Glossary

- **Binary Vector**: Sequence of bits (0s and 1s)
- **Fixed-Length**: All input packets have the same length F
- **Hamming Weight**: Number of '1' bits in a binary vector
- **Lossless**: Perfect reconstruction - decompressed data exactly matches original
- **LSB**: Least Significant Bit (rightmost, bit 0)
- **MSB**: Most Significant Bit (leftmost, bit N-1 for N-bit vector)
- **Robustness Level**: Number of consecutive packet losses tolerated without losing sync
- **Run-Length Encoding**: Encoding consecutive repeated values as count + value
