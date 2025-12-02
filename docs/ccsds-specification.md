# CCSDS 124.0-B-1 Specification

## Standard Information

- **Title**: Lossless Data Compression
- **Document ID**: CCSDS 124.0-B-1
- **Status**: Recommended Standard
- **Organization**: Consultative Committee for Space Data Systems (CCSDS)

## Overview

CCSDS 124.0-B-1 standardizes the POCKET+ algorithm for compressing fixed-length spacecraft housekeeping packets. It is part of the CCSDS Blue Book series of recommended standards.

## Scope

The standard defines:
- Compression algorithm specification
- Decompression algorithm specification
- Packet format
- Error handling mechanisms
- Interoperability requirements

## Key Specifications

### Data Format

- Fixed-length input packets
- Variable-length compressed output
- Packet loss tolerance parameters

### Compression Method

- Delta encoding
- Bitwise operations
- Entropy coding (details per standard)

### Quality of Service

- Lossless compression guarantee
- Configurable packet loss resilience
- Deterministic behavior

## Obtaining the Standard

The official CCSDS 124.0-B-1 specification can be obtained from:

**CCSDS Website**: [https://public.ccsds.org/](https://public.ccsds.org/)

Navigate to:
- Publications
- Blue Books (Recommended Standards)
- 124.0-B-1 - Lossless Data Compression

## Implementation Notes

This implementation aims to:
- Comply with CCSDS 124.0-B-1 requirements
- Provide interoperable implementations across languages
- Maintain the spirit of the original POCKET+ algorithm

## Related Standards

- **CCSDS 120.0-G-3** - Lossless Data Compression (Green Book - Informational Report)
- **CCSDS 123.0-B-1** - Lossless Multispectral & Hyperspectral Image Compression

## Patent Information

POCKET+ is patented by the European Space Agency (ESA). The CCSDS standard may include patent licensing information. Users should consult the official standard and ESA documentation for licensing details.

## References

1. CCSDS 124.0-B-1, "Lossless Data Compression," Consultative Committee for Space Data Systems
2. ESA POCKET+ Information: https://opssat.esa.int/pocket-plus/
