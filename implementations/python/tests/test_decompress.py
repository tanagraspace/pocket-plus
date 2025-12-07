"""Tests for POCKET+ decompression."""

from pocketplus.bitvector import BitVector
from pocketplus.compress import compress
from pocketplus.decompress import Decompressor, decompress


class TestDecompressorInit:
    """Test decompressor initialization."""

    def test_init_basic(self) -> None:
        """Test basic decompressor initialization."""
        decomp = Decompressor(packet_length=720, robustness=1)
        assert decomp.F == 720
        assert decomp.robustness == 1
        assert decomp.t == 0

    def test_init_with_initial_mask(self) -> None:
        """Test initialization with initial mask."""
        initial_mask = BitVector(8)
        initial_mask.from_bytes(bytes([0xFF]))
        decomp = Decompressor(packet_length=8, robustness=1, initial_mask=initial_mask)
        assert decomp.mask.to_bytes() == bytes([0xFF])

    def test_reset(self) -> None:
        """Test decompressor reset."""
        decomp = Decompressor(packet_length=8, robustness=1)
        decomp.t = 10
        decomp.reset()
        assert decomp.t == 0


class TestRoundTrip:
    """Test compression/decompression round-trip."""

    def test_round_trip_single_packet(self) -> None:
        """Test round-trip of a single packet."""
        # Original data
        original = bytes([0xAA, 0x55])  # 16 bits

        # Compress
        compressed = compress(
            original,
            packet_size=16,
            robustness=1,
            pt_limit=10,
            ft_limit=20,
            rt_limit=50,
        )

        # Decompress
        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=1,
        )

        assert decompressed == original

    def test_round_trip_multiple_packets(self) -> None:
        """Test round-trip of multiple packets."""
        # Original data: 5 packets of 2 bytes each
        original = bytes([0x12, 0x34] * 5)

        # Compress
        compressed = compress(
            original,
            packet_size=16,
            robustness=2,
            pt_limit=5,
            ft_limit=10,
            rt_limit=25,
        )

        # Decompress
        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=2,
        )

        assert decompressed == original

    def test_round_trip_all_zeros(self) -> None:
        """Test round-trip of all-zero data."""
        original = bytes([0x00] * 10)

        compressed = compress(
            original,
            packet_size=16,
            robustness=1,
            pt_limit=10,
            ft_limit=20,
            rt_limit=50,
        )

        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=1,
        )

        assert decompressed == original

    def test_round_trip_all_ones(self) -> None:
        """Test round-trip of all-one data."""
        original = bytes([0xFF] * 10)

        compressed = compress(
            original,
            packet_size=16,
            robustness=1,
            pt_limit=10,
            ft_limit=20,
            rt_limit=50,
        )

        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=1,
        )

        assert decompressed == original

    def test_round_trip_identical_packets(self) -> None:
        """Test round-trip of identical packets (high compression)."""
        # Same data repeated - should compress well
        original = bytes([0xDE, 0xAD] * 10)

        compressed = compress(
            original,
            packet_size=16,
            robustness=1,
            pt_limit=5,
            ft_limit=10,
            rt_limit=25,
        )

        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=1,
        )

        assert decompressed == original

    def test_round_trip_varying_data(self) -> None:
        """Test round-trip of varying data."""
        # Different data in each packet
        original = bytes(range(20))  # 0x00 to 0x13

        compressed = compress(
            original,
            packet_size=16,
            robustness=2,
            pt_limit=5,
            ft_limit=10,
            rt_limit=25,
        )

        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=2,
        )

        assert decompressed == original


class TestRoundTripRobustness:
    """Test round-trip with different robustness levels."""

    def test_round_trip_r0(self) -> None:
        """Test round-trip with robustness=0."""
        original = bytes([0xCA, 0xFE] * 5)

        compressed = compress(
            original,
            packet_size=16,
            robustness=0,
            pt_limit=5,
            ft_limit=10,
            rt_limit=25,
        )

        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=0,
        )

        assert decompressed == original

    def test_round_trip_r7(self) -> None:
        """Test round-trip with maximum robustness=7."""
        original = bytes([0xBE, 0xEF] * 10)

        compressed = compress(
            original,
            packet_size=16,
            robustness=7,
            pt_limit=5,
            ft_limit=10,
            rt_limit=25,
        )

        decompressed = decompress(
            compressed,
            packet_size=16,
            robustness=7,
        )

        assert decompressed == original


class TestRoundTripPacketSizes:
    """Test round-trip with different packet sizes."""

    def test_round_trip_small_packet(self) -> None:
        """Test round-trip with small 8-bit packets."""
        original = bytes([0xAA] * 10)

        compressed = compress(
            original,
            packet_size=8,
            robustness=1,
            pt_limit=5,
            ft_limit=10,
            rt_limit=25,
        )

        decompressed = decompress(
            compressed,
            packet_size=8,
            robustness=1,
        )

        assert decompressed == original

    def test_round_trip_standard_packet(self) -> None:
        """Test round-trip with standard 720-bit packets."""
        # 720 bits = 90 bytes
        original = bytes([i % 256 for i in range(90)])

        compressed = compress(
            original,
            packet_size=720,
            robustness=1,
            pt_limit=10,
            ft_limit=20,
            rt_limit=50,
        )

        decompressed = decompress(
            compressed,
            packet_size=720,
            robustness=1,
        )

        assert decompressed == original
