"""
POCKET+ Lossless Compression Algorithm

Implementation of CCSDS 124.0-B-1 lossless compression algorithm
designed for spacecraft housekeeping data.

MicroPython compatible - no typing module imports.
"""

__version__ = "1.0.0"

from pocketplus.compress import compress
from pocketplus.decompress import decompress

__all__ = ["compress", "decompress", "__version__"]
