# Known Issues in ESA/ESOC Reference Implementation

This document lists bugs discovered in the ESA/ESOC reference implementation of CCSDS 124.0-B-1 (POCKET+).

## Fixed Issues

Issues that have been patched in this repository. See [CHANGES.md](CHANGES.md) for detailed fix descriptions.

### Issue #1: Compressor Output Padding Bug

| Field | Value |
|-------|-------|
| **File** | `pocket_compress.c` |
| **Status** | Fixed |

**Problem:** Missing `fclose()` causes 2 extra null bytes at end of compressed output.

**Root Cause:** `fseek()` to trim unused bytes not applied when file auto-closed on exit.

**Impact:** Output 643 bytes instead of correct 641 bytes.

---

### Issue #2: Decompressor Loop Termination

| Field | Value |
|-------|-------|
| **File** | `pocket_decompress.c` |
| **Status** | Fixed |

**Problem:** Loop terminates 3 bytes early: `currentByte<(compressed_file_length-3)`

**Root Cause:** Workaround for Issue #1's spurious padding. After fixing #1, decompressor misses last packet(s).

**Impact:** Decompression fails with MD5 mismatch after compressor fix.

**Fix:** Changed to `currentByte<compressed_file_length`

---

## Unfixed Issues

Issues discovered but not patched. Workarounds documented.

### Issue #3: Decompressor Filename Buffer Overflow

| Field | Value |
|-------|-------|
| **File** | `pocket_decompress.c` |
| **Status** | Unfixed (workaround available) |

**Problem:** Segmentation fault when input filename exceeds ~20 characters.

**Root Cause:** Fixed-size buffer for filename handling overflows with longer paths.

**Reproduction:**
```bash
# Works (short name)
pocket_decompress /path/to/hiro.bin.pkt

# Crashes (long name)
pocket_decompress /path/to/high-robustness.bin.pkt
```

**Workaround:** Use short filenames (< 20 characters) for test vectors.

**Impact:** Cannot generate test vectors with long names without modifying decompressor source.
