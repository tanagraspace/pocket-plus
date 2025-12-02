# POCKET+ Test Vectors

This directory contains test vectors for validating POCKET+ implementations across different programming languages.

## Purpose

Test vectors ensure that all implementations:
1. Produce correct compression results
2. Successfully decompress data
3. Maintain interoperability across languages
4. Handle edge cases consistently

## Directory Structure

```
test-vectors/
├── input/              # Original uncompressed test data
│   ├── simple.bin      # Basic test case
│   ├── housekeeping.bin # Typical spacecraft housekeeping data
│   └── edge-cases.bin  # Edge cases and corner conditions
│
└── expected-output/    # Expected compressed results
    ├── simple.compressed
    ├── housekeeping.compressed
    └── edge-cases.compressed
```

## Test Cases

### simple.bin
- **Description**: Simple test case with predictable patterns
- **Size**: TBD
- **Purpose**: Basic validation of compression/decompression

### housekeeping.bin
- **Description**: Realistic spacecraft housekeeping telemetry data
- **Size**: TBD
- **Purpose**: Validate performance on real-world data

### edge-cases.bin
- **Description**: Edge cases and boundary conditions
- **Size**: TBD
- **Purpose**: Ensure robust error handling

## Usage

### Running Tests

Each implementation should validate against these test vectors:

**C:**
```bash
cd c
make test-vectors
```

**Python:**
```bash
cd python
pytest tests/test_vectors.py
```

**Go:**
```bash
cd go
go test ./... -v -run TestVectors
```

## Adding New Test Vectors

When adding new test vectors:

1. Add input file to `input/`
2. Generate expected output using a validated implementation
3. Add expected output to `expected-output/`
4. Document the test case in this README
5. Update test suites in all language implementations

## Validation Criteria

A valid implementation must:
- Compress input files to match expected output (bit-for-bit)
- Decompress output files back to original input
- Handle all edge cases without crashes
- Report errors appropriately for invalid input

## Cross-Implementation Testing

Implementations should also verify:
- Data compressed by C can be decompressed by Python and Go
- Data compressed by Python can be decompressed by C and Go
- Data compressed by Go can be decompressed by C and Python

This ensures true interoperability across implementations.

## Notes

Test vectors will be added as the algorithm implementation progresses. Initial implementations may use placeholder test data until the full algorithm is complete.
