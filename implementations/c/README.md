# POCKET+ C Implementation

C implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## Version

Current version: **0.1.0**

## Features

- ✅ Bit vector operations (CCSDS Section 1.6.1)
- ✅ Variable-length bit buffer
- ✅ Counter encoding (COUNT)
- ✅ Run-length encoding (RLE)
- ✅ Bit extraction (BE)
- ✅ Mask update logic (Equations 6-8)
- ✅ Core compression algorithm (simplified)
- ⚠️  Full CCSDS encoding (in progress)
- ⏳ Decompression algorithm
- ⏳ Command-line interface

## Implementation Details

### Design Principles

- **Zero dependencies** - Only C99 standard library
- **Static allocation** - No malloc/free, suitable for embedded systems
- **Test-driven** - 64 unit tests covering all core functions
- **Clean code** - Modular design with clear separation of concerns

### File Structure

```
implementations/c/
├── include/
│   └── pocket_plus.h       # Public API header
├── src/
│   ├── bitvector.c         # Bit vector operations
│   ├── bitbuffer.c         # Variable-length bit buffer
│   ├── encode.c            # COUNT, RLE, BE functions
│   ├── mask.c              # Mask update logic
│   └── compress.c          # Main compression algorithm
├── tests/
│   ├── test_bitvector.c    # Bit vector tests (18 tests)
│   ├── test_bitbuffer.c    # Bit buffer tests (11 tests)
│   ├── test_encode.c       # Encoding tests (13 tests)
│   ├── test_mask.c         # Mask logic tests (12 tests)
│   └── test_compress.c     # Compression tests (10 tests)
├── build/                  # Build artifacts (gitignored)
├── Makefile                # Build system
└── README.md               # This file
```

## Building

```bash
make          # Build library (libpocketplus.a)
make test     # Build and run all tests
make clean    # Clean build artifacts
```

## Testing

All 64 unit tests currently passing:

```bash
$ make test
Running build/test_bitvector...
18/18 tests passed

Running build/test_bitbuffer...
11/11 tests passed

Running build/test_encode...
13/13 tests passed

Running build/test_mask...
12/12 tests passed

Running build/test_compress...
10/10 tests passed
```

## Usage

```c
#include "pocket_plus.h"

// Initialize compressor
pocket_compressor_t comp;
pocket_compressor_init(&comp, 720, NULL, 1);  // 720 bits, robustness=1

// Prepare input
bitvector_t input;
bitvector_init(&input, 720);
// ... load input data ...

// Compress packet
bitbuffer_t output;
bitbuffer_init(&output);
pocket_params_t params = {0};
pocket_compress_packet(&comp, &input, &output, &params);

// Extract compressed bytes
uint8_t compressed_data[POCKET_MAX_OUTPUT_BYTES];
size_t num_bytes = bitbuffer_to_bytes(&output, compressed_data, sizeof(compressed_data));
```

## API Overview

### Core Data Structures

- `bitvector_t` - Fixed-length binary vector (up to 65535 bits)
- `bitbuffer_t` - Variable-length bit buffer for encoding
- `pocket_compressor_t` - Compressor state with history tracking
- `pocket_params_t` - Per-packet compression parameters

### Key Functions

#### Bit Operations
- `bitvector_xor()`, `bitvector_or()`, `bitvector_and()`, `bitvector_not()`
- `bitvector_left_shift()`, `bitvector_reverse()`
- `bitvector_hamming_weight()`

#### Encoding
- `pocket_count_encode()` - Counter encoding (CCSDS Equation 9)
- `pocket_rle_encode()` - Run-length encoding (Equation 10)
- `pocket_bit_extract()` - Bit extraction (Equation 11)

#### Mask Update
- `pocket_update_build()` - Build vector update (Equation 6)
- `pocket_update_mask()` - Mask vector update (Equation 7)
- `pocket_compute_change()` - Change vector computation (Equation 8)

#### Compression
- `pocket_compressor_init()` - Initialize compressor
- `pocket_compress_packet()` - Compress one packet
- `pocket_compressor_reset()` - Reset to initial state

## Memory Usage

For maximum packet size (65535 bits = 8192 bytes):
- `bitvector_t`: ~8 KB per instance
- `bitbuffer_t`: ~49 KB (6× max packet size)
- `pocket_compressor_t`: ~148 KB (includes 16 history buffers)

Reduce memory by defining smaller packet size:
```c
#define POCKET_MAX_PACKET_LENGTH 720  // For 90-byte packets
```

## Implementation Status

### Completed ✅
- [x] Bit vector operations with full test coverage
- [x] Variable-length bit buffer
- [x] CCSDS encoding functions (COUNT, RLE, BE)
- [x] Mask update logic (Equations 6, 7, 8)
- [x] Basic compression with state management
- [x] Comprehensive unit test suite (64 tests)

### In Progress ⚠️
- [ ] Full CCSDS output encoding (hₜ ∥ qₜ ∥ uₜ)
- [ ] Robustness window management
- [ ] Flag period handling (pt, ft, rt)

### Planned ⏳
- [ ] Decompression algorithm
- [ ] Reference test vector validation
- [ ] Command-line compression tool
- [ ] Command-line decompression tool
- [ ] Performance optimization

## References

- CCSDS 124.0-B-1 Specification: `../../sandbox/124x0b1.pdf`
- Algorithm Summary: `../../ALGORITHM.md` (root directory)
- ESA Reference Implementation: `../../test-vector-generator/c-reference/`

## License

See [LICENSE](../LICENSE) in the root directory.
