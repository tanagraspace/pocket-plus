"""
POCKET+ Lossless Compression Algorithm

Implementation of CCSDS 124.0-B-1 lossless compression algorithm
designed for spacecraft housekeeping data.

MicroPython compatible - no typing module imports.
"""

__version__ = "0.1.0"

from pocketplus.compress import compress


def decompress(data: bytes, packet_size: int, robustness: int = 1) -> bytes:
    """
    Decompress data using POCKET+ algorithm.

    Args:
        data: Compressed input data
        packet_size: Size of each original packet in bytes
        robustness: Robustness level R used during compression

    Returns:
        Decompressed data

    Raises:
        NotImplementedError: Implementation pending
    """
    raise NotImplementedError("Decompression not yet implemented")


__all__ = ["compress", "decompress", "__version__"]
