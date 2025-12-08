"""
POCKET+ Python and MicroPython Implementation

Python and MicroPython compatible implementation of the
[CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf) POCKET+ lossless
compression algorithm of fixed-length housekeeping data.
"""

__version__ = "1.0.0"

from pocketplus.compress import compress
from pocketplus.decompress import decompress

__all__ = ["compress", "decompress", "__version__"]
