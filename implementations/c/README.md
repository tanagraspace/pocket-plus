# POCKET+ C Implementation

[![Build](https://github.com/tanagraspace/pocket-plus/actions/workflows/c-build.yml/badge.svg)](https://github.com/tanagraspace/pocket-plus/actions/workflows/c-build.yml)
![Lines](assets/coverage-lines.svg)
![Functions](assets/coverage-functions.svg)

A MISRA-C compliant C implementation of the [CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf) POCKET+ lossless compression algorithm of fixed-length housekeeping data.

## Citation

If POCKET+ contributes to your research, please cite:

> D. Evans, G. Labrèche, D. Marszk, S. Bammens, M. Hernandez-Cabronero, V. Zelenevskiy, V. Shiradhonkar, M. Starcik, and M. Henkel. 2022. "Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1," *Proceedings of the Small Satellite Conference*, Communications, SSC22-XII-03. https://digitalcommons.usu.edu/smallsat/2022/all2022/133/

<details>
<summary>BibTeX</summary>

```bibtex
@inproceedings{evans2022pocketplus,
  author    = {Evans, David and Labrèche, Georges and Marszk, Dominik and Bammens, Samuel and Hernandez-Cabronero, Miguel and Zelenevskiy, Vladimir and Shiradhonkar, Vasundhara and Starcik, Mario and Henkel, Maximilian},
  title     = {Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1},
  booktitle = {Proceedings of the Small Satellite Conference},
  year      = {2022},
  note      = {SSC22-XII-03},
  url       = {https://digitalcommons.usu.edu/smallsat/2022/all2022/133/}
}
```

</details>

## Building

```bash
make               # Build library and CLI
make test          # Run all tests
make valgrind      # Run memory check (requires valgrind)
make coverage      # Run tests with coverage report
make coverage-html # Run tests with coverage report in html (requires lcov)
make misra         # Run MISRA-C:2012 compliance check (requires cppcheck)
make docs          # Generate API documentation (requires doxygen)
make clean         # Clean build artifacts
```

### Docker

```bash
docker-compose run --rm c                    # Build, test, coverage
docker-compose run --rm c make misra         # Run MISRA check
docker-compose run --rm --build c            # Rebuild after changes
```

## CLI

```bash
# Compress
./build/pocketplus <input> <packet_size> <pt> <ft> <rt> <robustness>

# Decompress
./build/pocketplus -d <input.pkt> <packet_size> <robustness>
```

**Example:**
```bash
./build/pocketplus data.bin 90 10 20 50 1      # -> data.bin.pkt
./build/pocketplus -d data.bin.pkt 90 1        # -> data.bin.depkt
```

Run `./build/pocketplus --help` for full usage.

## Library Usage

```c
#include "pocketplus.h"

// Compress
pocket_compressor_t comp;
pocket_compressor_init(&comp, 720, NULL, 1, 10, 20, 50);
pocket_compress(&comp, input, input_size, output, output_max, &output_size);

// Decompress
pocket_decompressor_t decomp;
pocket_decompressor_init(&decomp, 720, NULL, 1);
pocket_decompress(&decomp, compressed, comp_size, output, output_max, &output_size);
```

## Design

- **Zero dependencies** - C99 standard library only
- **Static allocation** - No malloc/free, embedded-friendly (verified via valgrind in CI)
- **MISRA-C compliant** - Suitable for safety-critical systems

## MISRA-C:2012 Compliance

The core library has **zero Required/Mandatory violations**. Remaining violations are all **Advisory** and suppressed with documented rationale in `misra.supp`:

| Rule | Description | Rationale |
|------|-------------|-----------|
| 8.7 | External linkage could be internal | Public API functions require external linkage |
| 15.5 | Multiple return statements | Early returns for error handling in decompression |
| 2.5 | Unused macros | Version macros used in CLI, not library |

Run MISRA checks locally (requires [cppcheck](https://cppcheck.sourceforge.io/)):
```bash
make misra                           # Local
docker-compose run --rm c make misra # Docker
```

CI automatically runs MISRA checks on every push/PR to C code.

## File Structure

```
implementations/c/
├── include/pocketplus.h     # Public API
├── src/
│   ├── bitvector.c          # Fixed-length bit vectors
│   ├── bitbuffer.c          # Variable-length output buffer
│   ├── encode.c             # COUNT, RLE, BE encoding
│   ├── mask.c               # Mask update logic
│   ├── compress.c           # Compression algorithm
│   ├── decompress.c         # Decompression algorithm
│   └── cli.c                # Command-line interface
└── tests/                   # Unit and integration tests
```

## API

### High-Level

- `pocket_compress()` / `pocket_decompress()` - Compress/decompress entire buffer
- `pocket_compressor_init()` / `pocket_decompressor_init()` - Initialize state

### Low-Level

- `pocket_compress_packet()` / `pocket_decompress_packet()` - Single packet
- `pocket_count_encode()` / `pocket_count_decode()` - Counter encoding (Eq. 9)
- `pocket_rle_encode()` / `pocket_rle_decode()` - Run-length encoding (Eq. 10)
- `pocket_bit_extract()` / `pocket_bit_insert()` - Bit extraction (Eq. 11)

## Memory

For maximum packet size (65535 bits):
- `pocket_compressor_t`: ~148 KB
- `pocket_decompressor_t`: ~50 KB

Reduce with `#define POCKET_MAX_PACKET_LENGTH 720` for 90-byte packets.

## References

- [CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf)
- [ESA POCKET+](https://opssat.esa.int/pocket-plus/)

