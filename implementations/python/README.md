# POCKET+ Python Implementation

[![Python Build](https://github.com/tanagraspace/pocket-plus/actions/workflows/python-build.yml/badge.svg)](https://github.com/tanagraspace/pocket-plus/actions/workflows/python-build.yml)
[![MicroPython Build](https://github.com/tanagraspace/pocket-plus/actions/workflows/micropython-build.yml/badge.svg)](https://github.com/tanagraspace/pocket-plus/actions/workflows/micropython-build.yml)

Python implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## Features

- Full compression/decompression (byte-identical to ESA reference)
- MicroPython compatible (no argparse, no typing module imports)
- CLI matching C implementation interface

## Installation

```bash
pip install -e .
```

For development:
```bash
pip install -e ".[dev]"
```

## Usage

### Python API

```python
from pocketplus import compress, decompress

# Compress: packet_size in bits, robustness 0-7
compressed = compress(data, packet_size=720, robustness=1,
                      pt_limit=10, ft_limit=20, rt_limit=50)

# Decompress
decompressed = decompress(compressed, packet_size=720, robustness=1)
```

### CLI

```bash
# Compress
python cli.py data.bin 90 10 20 50 1

# Decompress
python cli.py -d data.bin.pkt 90 1

# Help
python cli.py --help
```

## Testing

```bash
pytest                           # Run all tests
pytest --cov=pocketplus          # With coverage
pytest tests/test_vectors.py     # Reference validation only
```

## Development

```bash
ruff format .                    # Format code
ruff check .                     # Lint
mypy pocketplus                  # Type check
```

## License

See [LICENSE](../../LICENSE) in the root directory.
