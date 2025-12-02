# POCKET+

Multi-language implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## About

POCKET+ is an ESA-patented lossless compression algorithm implemented using very low-level instructions such as OR, XOR, AND, etc. It has been designed to run on spacecraft command and control processors with low CPU power available and tight real-time constraints.

The algorithm has been standardized by CCSDS as **CCSDS 124.0-B-1** for compressing fixed-length spacecraft housekeeping packets.

## Repository Structure

This monorepo contains implementations in multiple programming languages:

```
pocket-plus/
├── c/                  # C implementation
├── python/             # Python implementation
├── go/                 # Go implementation
├── docs/               # Shared documentation
├── test-vectors/       # Shared test data for validation
├── LICENSE
└── README.md
```

## Implementations

### C Implementation

**Version**: 0.1.0
**Status**: In development
**Location**: [`c/`](c/)

Optimized for embedded systems and spacecraft processors. See [c/README.md](c/README.md) for details.

### Python Implementation

**Version**: 0.1.0
**Status**: In development
**Location**: [`python/`](python/)

High-level implementation with type hints and comprehensive testing. See [python/README.md](python/README.md) for details.

### Go Implementation

**Version**: 0.1.0
**Status**: In development
**Location**: [`go/`](go/)

Idiomatic Go implementation with streaming and buffer-based APIs. See [go/README.md](go/README.md) for details.

## Quick Start

### C

```bash
cd c
make
make test
```

### Python

```bash
cd python
pip install -e ".[dev]"
pytest
```

### Go

```bash
cd go
go test ./...
```

## Versioning Strategy

This monorepo uses **independent versioning** for each language implementation with prefixed Git tags:

- **C**: `c/v0.1.0`, `c/v0.2.0`, ...
- **Python**: `python/v0.1.0`, `python/v0.2.0`, ...
- **Go**: `go/v0.1.0`, `go/v0.2.0`, ...

Each implementation:
- Follows [Semantic Versioning](https://semver.org/)
- Maintains its own CHANGELOG.md
- Can be released independently
- Shares test vectors for validation

### Releasing

To release a new version of an implementation:

```bash
# Example: releasing Python v1.0.0
cd python
# Update version in pyproject.toml
git add pyproject.toml CHANGELOG.md
git commit -m "python: release v1.0.0"
git tag python/v1.0.0
git push origin python/v1.0.0
```

## Documentation

- [Algorithm Overview](docs/algorithm-overview.md) - How POCKET+ works
- [CCSDS Specification](docs/ccsds-specification.md) - Standards information
- [Implementation Guide](docs/implementation-guide.md) - Guidelines for contributors
- [Test Vectors](test-vectors/README.md) - Validation test data

## Features

- **Lossless Compression** - Perfect reconstruction of original data
- **Delta Compression** - Efficient encoding of slowly-varying data
- **Packet Loss Resilience** - Decompressor continues even with lost packets
- **Low Complexity** - Uses only basic bitwise operations
- **Real-time Capable** - Suitable for time-critical spacecraft systems

## Interoperability

All implementations are designed to be interoperable:
- Data compressed by one implementation can be decompressed by others
- Shared test vectors ensure consistency
- Common documentation and specifications

## Contributing

Contributions are welcome! Please:

1. Read the [Implementation Guide](docs/implementation-guide.md)
2. Ensure your changes pass all test vectors
3. Update relevant CHANGELOG.md
4. Follow language-specific conventions
5. Add tests for new functionality

## License

See [LICENSE](LICENSE) for details.

## References

- [ESA POCKET+ Information](https://opssat.esa.int/pocket-plus/)
- [CCSDS Standards](https://public.ccsds.org/)
- CCSDS 124.0-B-1 - Lossless Data Compression

## Status

All implementations are currently in early development (v0.1.0). The core algorithm implementation is in progress.
