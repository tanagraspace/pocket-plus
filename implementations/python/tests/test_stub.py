"""Stub test to verify CI pipeline works."""

import pytest

import pocketplus


def test_version() -> None:
    """Test that version is defined."""
    assert pocketplus.__version__ == "0.1.0"


def test_compress_works() -> None:
    """Test that compress produces output."""
    # 8 bytes of data, packet size = 16 bits (2 bytes)
    result = pocketplus.compress(b"\x00\x00\x00\x00\x00\x00\x00\x00", packet_size=16)
    assert len(result) > 0


def test_decompress_not_implemented() -> None:
    """Test that decompress raises NotImplementedError."""
    with pytest.raises(NotImplementedError):
        pocketplus.decompress(b"test", 4)
