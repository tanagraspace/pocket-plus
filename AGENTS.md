# Instructions for AI Agents

This file provides guidance for AI agents (like Claude Code) working on the POCKET+ project.

## Project Overview

POCKET+ is a multi-language implementation of the CCSDS 124.0-B-1 lossless compression algorithm designed for spacecraft housekeeping data. This is a **monorepo** with independent implementations in C, Python, and Go.

## Repository Structure

```
pocket-plus/
├── implementations/
│   ├── c/          # C implementation
│   ├── python/     # Python implementation
│   └── go/         # Go implementation
├── docs/           # Shared documentation
└── test-vectors/   # Shared test data
```

## Key Principles

1. **Independent Versioning**: Each language has its own version (e.g., `c/v1.0.0`, `python/v0.9.0`)
2. **Interoperability**: All implementations must be able to compress/decompress each other's output
3. **Shared Test Vectors**: Use common test data in `test-vectors/` for validation
4. **No Over-Engineering**: Keep implementations simple and focused on the task

## Versioning Rules

- Use **prefixed git tags**: `c/vX.Y.Z`, `python/vX.Y.Z`, `go/vX.Y.Z`
- Follow [Semantic Versioning](https://semver.org/)
- Update version in language-specific files:
  - C: `implementations/c/VERSION` and `implementations/c/include/pocket_plus.h`
  - Python: `implementations/python/pyproject.toml` and `implementations/python/pocket_plus/__init__.py`
  - Go: Git tag (Go uses tags directly)

## Commit Conventions

Use [Conventional Commits](https://www.conventionalcommits.org/) with language prefix.

**IMPORTANT**: When working on a GitHub issue, include the issue number in the commit message title:

```
c: add compression function (#5)
python: fix decompression bug (#12)
go: update benchmarks (#8)
docs: clarify algorithm overview (#3)
chore: update build system (#1)
```

For commits without an associated issue:
```
c: add compression function
python: fix decompression bug
```

## When Working on Implementations

### C Implementation
- Target embedded systems and spacecraft processors
- Minimize dynamic memory allocation
- Use fixed-size integer types (`uint8_t`, etc.)
- Provide clear error codes
- Test with Makefile: `cd implementations/c && make test`

### Python Implementation
- Include type hints for all public APIs
- Follow PEP 8 style guidelines
- Use pytest for testing
- Maintain compatibility with Python 3.8+
- Install dev dependencies: `pip install -e ".[dev]"`

### Go Implementation
- Follow Go idioms and conventions
- Return errors, don't panic
- Include benchmarks for performance-critical code
- Run tests: `go test ./...`
- Format code: `go fmt ./...`

## Testing Requirements

1. **Unit Tests**: Test individual functions
2. **Round-trip Tests**: Compress then decompress, verify original data
3. **Test Vectors**: Validate against shared test vectors in `test-vectors/`
4. **Cross-Implementation**: Verify interoperability between languages

## Documentation

- Keep language-specific docs in each implementation's README
- Update shared docs in `docs/` when changing algorithm details
- Reference the CCSDS 124.0-B-1 specification when relevant
- Provide code examples in implementation READMEs

## Important Files to Check

- `docs/implementation-guide.md` - Implementation guidelines
- `docs/algorithm-overview.md` - Algorithm description
- `test-vectors/README.md` - Test data format and usage
- Each implementation's `README.md` and `CHANGELOG.md`

## What NOT to Do

- ❌ Don't create unnecessary abstractions
- ❌ Don't add features beyond what's requested
- ❌ Don't modify the core algorithm without consulting CCSDS 124.0-B-1
- ❌ Don't break interoperability between implementations
- ❌ Don't add dependencies without good reason

## Reference Implementation

The `sandbox/pocket-plus-master/` directory contains a reference C implementation. Use it to understand the algorithm, but don't blindly copy it.

## Questions?

Refer to:
- CCSDS 124.0-B-1 specification
- `docs/` directory for algorithm details
- Existing code in other language implementations
- ESA POCKET+ documentation: https://opssat.esa.int/pocket-plus/
