"""Stub test to verify CI pipeline works."""

import pytest

import pocketplus


def test_version() -> None:
    """Test that version is defined."""
    assert pocketplus.__version__ == "0.1.0"


def test_compress_not_implemented() -> None:
    """Test that compress raises NotImplementedError."""
    with pytest.raises(NotImplementedError):
        pocketplus.compress(b"test", 4)


def test_decompress_not_implemented() -> None:
    """Test that decompress raises NotImplementedError."""
    with pytest.raises(NotImplementedError):
        pocketplus.decompress(b"test", 4)
