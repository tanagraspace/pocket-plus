# POCKET+ Python Implementation

Python implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## Version

Current version: **0.1.0**

## Installation

```bash
cd python
pip install -e .
```

For development:
```bash
pip install -e ".[dev]"
```

## Testing

```bash
pytest
```

With coverage:
```bash
pytest --cov=pocket_plus
```

## Usage

```python
from pocket_plus import compress, decompress

# Compress data
compressed = compress(input_data)

# Decompress data
decompressed = decompress(compressed)
```

## Features

- [ ] Core compression algorithm
- [ ] Core decompression algorithm
- [ ] Packet loss resilience
- [ ] Command-line interface
- [ ] Type hints
- [ ] Comprehensive test coverage

## Development

Format code:
```bash
black pocket_plus tests
```

Type checking:
```bash
mypy pocket_plus
```

## License

See [LICENSE](../LICENSE) in the root directory.
