# POCKET+ Documentation

This directory contains shared documentation for all POCKET+ implementations.

## Contents

- [Algorithm Overview](algorithm-overview.md) - High-level description of the POCKET+ algorithm
- [CCSDS Specification](ccsds-specification.md) - Information about CCSDS 124.0-B-1 standard
- [Implementation Guide](implementation-guide.md) - Guidelines for implementing POCKET+
- [Test Vectors](../test-vectors/README.md) - Shared test data for validation

## About POCKET+

POCKET+ is an ESA-patented lossless compression algorithm implemented using low-level bitwise operations (OR, XOR, AND). It was specifically designed for spacecraft command and control processors with:

- Low CPU power availability
- Tight real-time constraints
- Unreliable communication channels (packet loss tolerance)

The algorithm has been standardized by CCSDS as **CCSDS 124.0-B-1** for compressing fixed-length spacecraft housekeeping packets.

## Key Features

1. **Lossless Compression** - Perfect reconstruction of original data
2. **Delta Compression** - Each packet can be independently compressed and transmitted
3. **Loss Resilience** - Decompressor can continue even if packets are lost
4. **Low Complexity** - Uses only basic bitwise operations
5. **Real-time Capable** - Suitable for time-critical spacecraft systems

## References

- [ESA POCKET+ Information](https://opssat.esa.int/pocket-plus/)
- CCSDS 124.0-B-1 Standard (Lossless Data Compression)
