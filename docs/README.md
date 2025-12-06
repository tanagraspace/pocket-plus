# POCKET+ Documentation

Shared documentation for all POCKET+ implementations.

## Contents

- [ALGORITHM.md](ALGORITHM.md) - Algorithm specification and details
- [GUIDELINES.md](GUIDELINES.md) - Implementation quick start
- [GOTCHAS.md](GOTCHAS.md) - Critical pitfalls (read this first!)
- [Test Vectors](../test-vectors/README.md) - Validation data

## About POCKET+

POCKET+ (CCSDS 124.0-B-1) is an ESA-patented lossless compression algorithm using low-level bitwise operations (OR, XOR, AND). Designed for spacecraft processors with limited CPU and real-time constraints.

## Key Features

- **Lossless** - Perfect data reconstruction
- **Low complexity** - Bitwise operations only
- **Packet loss resilient** - Configurable robustness level
- **No latency** - One output per input packet

## References

- [ESA POCKET+](https://opssat.esa.int/pocket-plus/)
- [CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf)
