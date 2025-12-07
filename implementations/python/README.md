# POCKET+ Python Implementation

[![Python Build](https://github.com/tanagraspace/pocket-plus/actions/workflows/python-build.yml/badge.svg)](https://github.com/tanagraspace/pocket-plus/actions/workflows/python-build.yml)
[![MicroPython Build](https://github.com/tanagraspace/pocket-plus/actions/workflows/micropython-build.yml/badge.svg)](https://github.com/tanagraspace/pocket-plus/actions/workflows/micropython-build.yml)
[![Coverage](https://img.shields.io/badge/coverage-93%25-brightgreen)](build/docs/coverage/)

Python implementation of the ([CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf)) POCKET+ lossless compression algorithm.

## Citation

If POCKET+ contributes to your research, please cite:

> D. Evans, G. Labrèche, D. Marszk, S. Bammens, M. Hernandez-Cabronero, V. Zelenevskiy, V. Shiradhonkar, M. Starcik, and M. Henkel. 2022. "Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1," *Proceedings of the Small Satellite Conference*, Communications, SSC22-XII-03. https://digitalcommons.usu.edu/smallsat/2022/all2022/133/

<details>
<summary>BibTeX</summary>

```bibtex
@inproceedings{evans2022pocketplus,
  author    = {Evans, David and Labrèche, Georges and Marszk, Dominik and Bammens, Samuel and Hernandez-Cabronero, Miguel and Zelenevskiy, Vladimir and Shiradhonkar, Vasundhara and Starcik, Mario and Henkel, Maximilian},
  title     = {Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1},
  booktitle = {Proceedings of the Small Satellite Conference},
  year      = {2022},
  note      = {SSC22-XII-03},
  url       = {https://digitalcommons.usu.edu/smallsat/2022/all2022/133/}
}
```

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
