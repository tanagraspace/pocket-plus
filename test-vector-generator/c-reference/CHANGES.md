# Changes to ESA/ESOC Reference Implementation

This document tracks fixes and modifications made to the ESA/ESOC reference implementation of CCSDS 124.0-B-1 (POCKET+) to correct bugs and improve code quality.

---

## ✅ Fix #1: Added Missing `fclose()` to Prevent Extra Padding Bytes

### Date: 2025-12-03
### Severity: Low (cosmetic only)

### Description

The original reference implementation did not close the output file before program exit, causing the final `fseek()` operation to not be properly applied. This resulted in **2 extra null bytes** (`0x00 0x00`) being written at the end of the compressed output.

### Location

**File:** `pocket_compress.c`
**Function:** `main()`
**Lines:** ~321-330

### Root Cause

1. `write_to_file()` writes a full 32-bit word (4 bytes) to the file
2. Calculates how many bytes to trim: `rest = 3 - (o_bit-1)/8`
3. Calls `fseek(outputFile, -rest, SEEK_CUR)` to position before the unused bytes
4. Function returns **without flushing**
5. Program exits **without calling `fclose(outputFile)`**
6. OS auto-closes the file and flushes buffers
7. **Bug:** The pending `fseek()` operation is not properly applied during the flush
8. Result: More bytes than intended are written

### Original Code

```c
write_to_file(o, outputFile);


}
///////////////////////
// END OF PACKET LOOP//
///////////////////////

printf("%s \n", "SUCCESS! " );
return EXIT_SUCCESS;  // ← File never closed!
```

### Fix Applied

**Part 1:** Added `fclose(outputFile);` before program exit (in `main()`):

```c
write_to_file(o, outputFile);

fclose(outputFile);  // ← Added to ensure file is properly closed

printf("%s \n", "SUCCESS! " );
return EXIT_SUCCESS;
```

**Part 2:** Added file truncation in `write_to_file()` function:

```c
// winds back the write pointer the required number of bytes
int rest;

if (o_bit<1) {
    fseek(outputFile, -4, SEEK_CUR);
} else {
    rest = 3-(o_bit-1)/8;
    fseek(outputFile, -1*rest, SEEK_CUR);
}

// ← Added: Truncate file at current position to remove excess bytes
fflush(outputFile);
long pos = ftell(outputFile);
#ifdef _WIN32
    _chsize(_fileno(outputFile), pos);
#else
    ftruncate(fileno(outputFile), pos);
#endif
```

**Part 3:** Added necessary headers:

```c
#ifdef _WIN32
    #include <io.h>
#else
    #include <unistd.h>
#endif
```

### Impact

**Before Fix:**
- Output file size: 643 bytes (641 bytes compressed data + 2 bytes spurious padding)

**After Fix:**
- Output file size: 641 bytes (correct compressed data only)
- All test vectors regenerated with correct output size
- Implementations now match byte-for-byte without workarounds

### Verification

```bash
# Original reference output (with bug)
ls -l simple.bin.pkt.orig
# 643 bytes

# Fixed reference output
ls -l simple.bin.pkt
# 641 bytes

# Verify last 2 bytes of original were spurious padding
tail -c 2 simple.bin.pkt.orig | hexdump -C
# 00000000  00 00                                             |..|
```

---

## 📝 Notes

- This reference implementation is from ESA/ESOC and is used to generate test vectors
- Fixes are applied to correct bugs while maintaining CCSDS 124.0-B-1 compliance
- Test vectors are regenerated after fixes to ensure correctness
- See [../../docs/GOTCHAS.md](../../docs/GOTCHAS.md) for implementation best practices


---

**Found another bug to fix? Document it here!**
