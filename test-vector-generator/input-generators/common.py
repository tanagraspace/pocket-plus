"""
Common utilities for test vector generation.
Provides shared functions for creating deterministic binary data.
"""
import struct
import hashlib
import numpy as np
from typing import List, Tuple


def set_deterministic_seed(seed: int):
    """Set seed for reproducible random generation."""
    np.random.seed(seed)


def create_empty_packet(length: int) -> bytearray:
    """Create an empty packet of specified length."""
    return bytearray(length)


def write_bytes(packet: bytearray, positions: Tuple[int, int], data: bytes):
    """Write bytes to packet at specified position range [start, end] inclusive."""
    start, end = positions
    data_len = end - start + 1
    if len(data) != data_len:
        raise ValueError(f"Data length {len(data)} doesn't match position range {data_len}")
    packet[start:end+1] = data


def write_value(packet: bytearray, positions: Tuple[int, int], value: int, byteorder='big', signed=False):
    """Write an integer value to packet at specified positions."""
    start, end = positions
    num_bytes = end - start + 1
    data = value.to_bytes(num_bytes, byteorder=byteorder, signed=signed)
    packet[start:end+1] = data


def write_repeating_sequence(packet: bytearray, positions: Tuple[int, int], sequence: List[int]):
    """Write a repeating sequence to packet."""
    start, end = positions
    length = end - start + 1
    repeated = (sequence * (length // len(sequence) + 1))[:length]
    packet[start:end+1] = bytes(repeated)


def write_random_bytes(packet: bytearray, positions: Tuple[int, int], seed: int = None):
    """Write random bytes to packet."""
    if seed is not None:
        np.random.seed(seed)
    start, end = positions
    length = end - start + 1
    random_data = np.random.randint(0, 256, length, dtype=np.uint8)
    packet[start:end+1] = bytes(random_data)


def write_float32(packet: bytearray, position: int, value: float):
    """Write a 32-bit float at specified position (4 bytes)."""
    data = struct.pack('>f', value)  # Big-endian float
    packet[position:position+4] = data


def calculate_crc16_ccitt(data: bytes) -> int:
    """Calculate CRC-16-CCITT checksum."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


def calculate_md5(data: bytes) -> str:
    """Calculate MD5 hash of data."""
    return hashlib.md5(data).hexdigest()


def increment_counter(value: int, increment: int, num_bytes: int) -> int:
    """Increment counter with wraparound based on byte size."""
    max_value = (1 << (num_bytes * 8)) - 1
    return (value + increment) & max_value


class PacketGenerator:
    """Base class for packet generation."""

    def __init__(self, packet_length: int, num_packets: int, seed: int = 42):
        self.packet_length = packet_length
        self.num_packets = num_packets
        self.seed = seed
        set_deterministic_seed(seed)

    def generate_packet(self, packet_num: int) -> bytearray:
        """Generate a single packet. Override in subclasses."""
        raise NotImplementedError

    def generate_all(self) -> bytes:
        """Generate all packets and return as bytes."""
        all_data = bytearray()
        for i in range(self.num_packets):
            packet = self.generate_packet(i)
            all_data.extend(packet)
        return bytes(all_data)

    def save_to_file(self, filename: str):
        """Generate and save all packets to file."""
        data = self.generate_all()
        with open(filename, 'wb') as f:
            f.write(data)

        # Calculate and print MD5
        md5_hash = calculate_md5(data)
        print(f"Generated {self.num_packets} packets ({len(data)} bytes)")
        print(f"MD5: {md5_hash}")
        return md5_hash
