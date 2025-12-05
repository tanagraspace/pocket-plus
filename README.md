# POCKET+

Multi-language implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## About

POCKET+ is an ESA-patented lossless compression algorithm implemented using very low-level instructions such as OR, XOR, AND, etc. It has been designed to run on spacecraft command and control processors with low CPU power available and tight real-time constraints.

The algorithm has been standardized by CCSDS as **CCSDS 124.0-B-1** for compressing fixed-length spacecraft housekeeping packets.

## Repository Structure

This monorepo contains implementations in multiple programming languages:

```
pocket-plus/
├── implementations/
│   ├── c/              # C implementation
│   ├── python/         # Python implementation
│   └── go/             # Go implementation
├── docs/               # Shared documentation
├── test-vectors/       # Shared test data for validation
├── LICENSE
└── README.md
```

## Implementations

### C Implementation

**Version**: 1.0.0
**Status**: Compression complete (byte-for-byte match with reference)
**Location**: [`implementations/c/`](implementations/c/)

Optimized for embedded systems and spacecraft processors. See [implementations/c/README.md](implementations/c/README.md) for details.

### Python Implementation

**Version**: 0.1.0
**Status**: Planned
**Location**: [`implementations/python/`](implementations/python/)

High-level implementation with type hints. See [implementations/python/README.md](implementations/python/README.md) for details.

### Go Implementation

**Version**: 0.1.0
**Status**: Planned
**Location**: [`implementations/go/`](implementations/go/)

Idiomatic Go implementation. See [implementations/go/README.md](implementations/go/README.md) for details.

## Quick Start

### C

```bash
cd implementations/c
make
make test
```

### Python

```bash
cd implementations/python
pip install -e ".[dev]"
pytest
```

### Go

```bash
cd implementations/go
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
cd implementations/python
# Update version in pyproject.toml
git add pyproject.toml
git commit -m "python: release v1.0.0"
git tag python/v1.0.0
git push origin python/v1.0.0
```

## Documentation

- [Algorithm Specification](docs/ALGORITHM.md) - POCKET+ algorithm details
- [Implementation Guidelines](docs/GUIDELINES.md) - Quick start for implementers
- [Common Gotchas](docs/GOTCHAS.md) - Critical pitfalls to avoid
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

- **C**: Compression complete and validated against all test vectors
- **Python/Go**: Planned
