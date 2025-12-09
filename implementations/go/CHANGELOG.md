# Changelog - Go Implementation

All notable changes to the Go implementation will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-12-08

### Added
- Complete POCKET+ compression and decompression implementation
- Full CCSDS 124.0-B-1 compliance with all test vectors passing
- BitVector, BitBuffer, and BitReader primitives with 32-bit word storage
- COUNT, RLE, and BE encoding/decoding functions
- Mask operations (UpdateBuild, UpdateMask, ComputeChange, ComputeKt)
- Compressor with automatic mode (pt, ft, rt counters)
- Decompressor with streaming APIs (PacketIterator, StreamPackets)
- CLI tool (`cmd/pocketplus/`) for compression and decompression
- Comprehensive test suite (150 tests, 93.5% coverage)
- Docker support
- CI/CD with auto-updating coverage badge
- API documentation generation

### Performance
- 1.5x faster than C implementation on benchmark tests
- Optimized bit operations using 32-bit word storage
- Zero external dependencies (standard library only)

### Validated Against
- simple (R=1): 9KB → 641B
- housekeeping (R=2): 900KB → 223KB
- edge-cases (R=1): 45KB → 10KB
- venus-express (R=2): 13.6MB → 5.9MB
- hiro (R=7): 9KB → 1.5KB
