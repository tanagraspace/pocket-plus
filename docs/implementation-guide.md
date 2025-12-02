# POCKET+ Implementation Guide

## Overview

This guide provides recommendations for implementing POCKET+ across different programming languages while maintaining consistency and correctness.

## Implementation Checklist

### Core Functionality

- [ ] Compression function
- [ ] Decompression function
- [ ] Packet loss handling
- [ ] Error detection and reporting

### Data Structures

- [ ] Packet representation
- [ ] Compression state
- [ ] Decompression state
- [ ] Configuration parameters

### Testing

- [ ] Unit tests for compression
- [ ] Unit tests for decompression
- [ ] Round-trip tests (compress → decompress)
- [ ] Packet loss scenario tests
- [ ] Edge case tests
- [ ] Performance benchmarks

### Validation

All implementations should:
1. Pass the shared test vectors in `../test-vectors/`
2. Produce identical output for identical input
3. Successfully decompress data compressed by other implementations

## Language-Specific Considerations

### C Implementation

- Use fixed-size integer types (`uint8_t`, `uint16_t`, etc.)
- Minimize dynamic memory allocation
- Consider embedded/bare-metal environments
- Provide clear error codes

### Python Implementation

- Include type hints for all public APIs
- Provide both low-level and high-level interfaces
- Consider NumPy integration for array operations
- Include comprehensive documentation

### Go Implementation

- Follow Go idioms and conventions
- Return errors, don't panic
- Provide both streaming and buffer-based APIs
- Include benchmarks

## Testing Strategy

### Test Vector Format

Test vectors are located in `../test-vectors/`:
- `input/` - Original uncompressed data
- `expected-output/` - Expected compressed results

### Cross-Implementation Testing

Implementations should be able to:
1. Compress data that other implementations can decompress
2. Decompress data compressed by other implementations
3. Handle edge cases consistently

## Documentation Requirements

Each implementation should include:

- **README.md** - Installation, usage, and examples
- **CHANGELOG.md** - Version history following Keep a Changelog format
- **API Documentation** - Language-appropriate documentation (docstrings, comments, etc.)
- **Examples** - Working code examples in `examples/` directory

## Versioning

Follow [Semantic Versioning](https://semver.org/):
- **MAJOR** - Incompatible API changes or algorithm changes
- **MINOR** - Backward-compatible functionality additions
- **PATCH** - Backward-compatible bug fixes

Use language-specific version tracking:
- C: `VERSION` file and `#define` in header
- Python: `pyproject.toml` and `__version__`
- Go: Git tags (e.g., `go/v0.1.0`)

## Performance Targets

While specific performance will vary by language and platform, aim for:
- **Compression ratio** - 2:1 to 4:1 for typical housekeeping data
- **Throughput** - Suitable for real-time processing
- **Memory usage** - Minimal, suitable for embedded systems

## Contributing

When adding new features:
1. Implement in one language first
2. Validate with test vectors
3. Document the feature
4. Port to other languages
5. Ensure cross-compatibility
