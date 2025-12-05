# POCKET+ C Implementation

C implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## Status

**Version**: 1.0.0
**Compression**: Complete (byte-for-byte match with ESA reference)
**Decompression**: Planned

### Test Results

| Test Vector | Input | Output | Ratio | Status |
|-------------|-------|--------|-------|--------|
| simple | 9 KB | 641 bytes | 14.04x | PASS |
| housekeeping | 900 KB | 223 KB | 4.03x | PASS |
| edge-cases | 45 KB | 10 KB | 4.44x | PASS |
| venus-express | 13.6 MB | 5.9 MB | 2.31x | PASS |

76 unit tests passing.

## Building

```bash
make          # Build library (libpocketplus.a)
make test     # Build and run all tests
make clean    # Clean build artifacts
```

## Usage

```c
#include "pocket_plus.h"

// Initialize compressor with automatic parameter management
pocket_compressor_t comp;
pocket_compressor_init(&comp, 720, NULL, 1, 10, 20, 50);
// 720 bits, robustness=1, pt=10, ft=20, rt=50

// Compress entire input
uint8_t output[OUTPUT_SIZE];
size_t output_size;
pocket_compress(&comp, input_data, input_size, output, sizeof(output), &output_size);
```

## Design

- **Zero dependencies** - C99 standard library only
- **Static allocation** - No malloc/free, embedded-friendly
- **Modular** - Separate bitvector, bitbuffer, encode, mask, compress modules

## File Structure

```
implementations/c/
├── include/pocket_plus.h    # Public API
├── src/
│   ├── bitvector.c          # Fixed-length bit vectors
│   ├── bitbuffer.c          # Variable-length output buffer
│   ├── encode.c             # COUNT, RLE, BE encoding
│   ├── mask.c               # Mask update logic
│   └── compress.c           # Main compression algorithm
└── tests/                   # Unit and integration tests
```

## API

### High-Level

- `pocket_compress()` - Compress entire input buffer
- `pocket_compressor_init()` - Initialize compressor state

### Low-Level

- `pocket_compress_packet()` - Compress single packet
- `pocket_count_encode()` - Counter encoding (CCSDS Eq. 9)
- `pocket_rle_encode()` - Run-length encoding (Eq. 10)
- `pocket_bit_extract()` - Bit extraction (Eq. 11)

## Memory

For maximum packet size (65535 bits):
- `bitvector_t`: ~8 KB
- `bitbuffer_t`: ~49 KB
- `pocket_compressor_t`: ~148 KB

Reduce with `#define POCKET_MAX_PACKET_LENGTH 720` for 90-byte packets.

## References

- [CCSDS 124.0-B-1](https://public.ccsds.org/Pubs/124x0b1.pdf)
- [ESA Reference](https://opssat.esa.int/pocket-plus/)
- [docs/GOTCHAS.md](../../docs/GOTCHAS.md) - Implementation pitfalls
