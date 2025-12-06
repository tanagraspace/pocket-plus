# POCKET+ Implementation Guidelines

## Before You Start

**Read [GOTCHAS.md](GOTCHAS.md) first!** It documents 8 critical pitfalls that will cause your implementation to fail silently.

## Validation Requirements

Your implementation must produce **byte-for-byte identical output** to the reference for all test vectors:

| Test Vector | Input | Expected Output | Parameters |
|-------------|-------|-----------------|------------|
| simple | 9,000 bytes | 641 bytes | R=1, pt=10, ft=20, rt=50 |
| housekeeping | 900,000 bytes | 223,078 bytes | R=2, pt=20, ft=50, rt=100 |
| edge-cases | 45,000 bytes | 10,124 bytes | R=1, pt=10, ft=20, rt=50 |
| venus-express | 13,608,000 bytes | 5,891,500 bytes | R=2, pt=20, ft=50, rt=100 |

Test vectors location: `test-vectors/`

## Critical Implementation Details

These are **not obvious** from the CCSDS spec:

1. **Vt calculation** - Start from position Rt+1, not position 2
2. **ct calculation** - Include current packet's pt flag (Vt+1 total entries)
3. **kt extraction** - Use forward order (low to high position)
4. **BE extraction** - Use reverse order (high to low position)
5. **Flag counters** - Use countdown counters, not modulo arithmetic
6. **Init phase** - First Rt+1 packets have ft=1, rt=1, pt=0

## Implementation Checklist

### Core
- [ ] Compression produces identical output to reference
- [ ] Both R=1 and R=2 robustness levels work
- [ ] All 4 test vectors pass

### Components
- [ ] COUNT encoding (variable-length integer)
- [ ] RLE encoding (run-length with terminator)
- [ ] BE (bit extraction) - reverse order
- [ ] kt extraction - forward order with mask inversion

### State Management
- [ ] Change history buffer (16 entries for Vt)
- [ ] Flag history buffer (16 entries for ct)
- [ ] Countdown counters (pt, ft, rt)

## Testing Strategy

1. **Start with simple.bin** (R=1) - validates basic algorithm
2. **Then housekeeping.bin** (R=2) - validates Vt calculation
3. **Then edge-cases.bin** (R=1) - validates ct calculation
4. **Finally venus-express** (R=2, large) - validates at scale

If simple passes but others fail, check Vt and ct calculations.

## Language-Specific Notes

### C
- Use fixed-size types (`uint8_t`, `uint32_t`)
- Minimize dynamic allocation
- See `implementations/c/` for reference

### Python/Go/Other
- Port from C implementation
- Validate against same test vectors
- Ensure bit-level compatibility

## Documentation

- `ALGORITHM.md` - Detailed algorithm description
- `GOTCHAS.md` - Critical implementation pitfalls
- `implementation-guide.md` - Additional technical details

## Quick Debugging

| Symptom | Likely Cause |
|---------|--------------|
| R=2 tests fail, R=1 passes | Vt starts at wrong position |
| edge-cases fails, simple passes | ct missing current flag |
| Bit-level errors mid-stream | kt using wrong extraction order |
| Size off by 10%+ | Flag timing wrong |

See [GOTCHAS.md](GOTCHAS.md) for detailed diagnosis.
