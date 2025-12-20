#!/usr/bin/env python3
"""
Large-scale test vector generator for POCKET+ comprehensive testing.

Generates GB-scale test data covering:
- All R values (0-7)
- Various packet sizes (F)
- Multiple data patterns (predictable, housekeeping-like, high-entropy, edge)

Usage:
    python generate_large_scale.py --output-dir ../output/large-scale --size 1GB
    python generate_large_scale.py --output-dir ../output/large-scale --size 100MB --r-value 3
"""

import argparse
import os
import sys
import struct
import hashlib
import numpy as np
from pathlib import Path
from typing import List, Dict, Tuple
from common import (
    set_deterministic_seed,
    create_empty_packet,
    write_value,
    write_random_bytes,
    calculate_md5,
    PacketGenerator
)


def parse_size(size_str: str) -> int:
    """Parse size string like '1GB', '500MB', '100KB' to bytes."""
    size_str = size_str.upper().strip()
    # Check longer suffixes first
    multipliers = [
        ('GB', 1024 * 1024 * 1024),
        ('MB', 1024 * 1024),
        ('KB', 1024),
        ('B', 1),
    ]
    for suffix, mult in multipliers:
        if size_str.endswith(suffix):
            return int(float(size_str[:-len(suffix)]) * mult)
    return int(size_str)


class PredictablePatternGenerator(PacketGenerator):
    """Generates highly predictable data (good compression)."""

    def __init__(self, packet_length: int, num_packets: int, seed: int = 42):
        super().__init__(packet_length, num_packets, seed)
        self.base_value = 0x55  # Alternating bits

    def generate_packet(self, packet_num: int) -> bytearray:
        packet = create_empty_packet(self.packet_length)
        # Mostly static with one incrementing byte
        for i in range(self.packet_length):
            if i == 0:
                packet[i] = packet_num & 0xFF  # Counter
            elif i == 1:
                packet[i] = (packet_num >> 8) & 0xFF  # Counter high byte
            else:
                packet[i] = self.base_value
        return packet


class HousekeepingLikeGenerator(PacketGenerator):
    """Generates realistic housekeeping-like telemetry data."""

    def __init__(self, packet_length: int, num_packets: int, seed: int = 42):
        super().__init__(packet_length, num_packets, seed)
        self.temperatures = np.random.uniform(20.0, 30.0, 4)  # 4 temp sensors
        self.voltages = np.random.uniform(3.2, 3.4, 4)  # 4 voltage rails

    def generate_packet(self, packet_num: int) -> bytearray:
        packet = create_empty_packet(self.packet_length)
        pos = 0

        # Packet counter (2 bytes)
        if pos + 2 <= self.packet_length:
            packet[pos:pos+2] = struct.pack('>H', packet_num & 0xFFFF)
            pos += 2

        # Timestamp-like counter (4 bytes)
        if pos + 4 <= self.packet_length:
            timestamp = packet_num * 1000  # ms since start
            packet[pos:pos+4] = struct.pack('>I', timestamp & 0xFFFFFFFF)
            pos += 4

        # Temperatures with small drift (4 x 2 bytes = 8 bytes)
        for i in range(4):
            if pos + 2 > self.packet_length:
                break
            # Small random walk
            self.temperatures[i] += np.random.uniform(-0.1, 0.1)
            self.temperatures[i] = np.clip(self.temperatures[i], 15.0, 40.0)
            temp_raw = int(self.temperatures[i] * 100)  # 0.01 degree resolution
            packet[pos:pos+2] = struct.pack('>H', temp_raw & 0xFFFF)
            pos += 2

        # Voltages with small drift (4 x 2 bytes = 8 bytes)
        for i in range(4):
            if pos + 2 > self.packet_length:
                break
            self.voltages[i] += np.random.uniform(-0.01, 0.01)
            self.voltages[i] = np.clip(self.voltages[i], 3.0, 3.6)
            volt_raw = int(self.voltages[i] * 1000)  # mV resolution
            packet[pos:pos+2] = struct.pack('>H', volt_raw & 0xFFFF)
            pos += 2

        # Fill rest with status flags (mostly static)
        while pos < self.packet_length:
            # Occasional bit flip
            if np.random.random() < 0.01:
                packet[pos] = np.random.randint(0, 256)
            else:
                packet[pos] = 0x00
            pos += 1

        return packet


class HighEntropyGenerator(PacketGenerator):
    """Generates high-entropy random data (worst case for compression)."""

    def generate_packet(self, packet_num: int) -> bytearray:
        packet = create_empty_packet(self.packet_length)
        # Use packet_num as additional seed for variety
        np.random.seed(self.seed + packet_num)
        for i in range(self.packet_length):
            packet[i] = np.random.randint(0, 256)
        return packet


class EdgePatternGenerator(PacketGenerator):
    """Generates edge case patterns (all zeros, all ones, alternating)."""

    def __init__(self, packet_length: int, num_packets: int, seed: int = 42):
        super().__init__(packet_length, num_packets, seed)
        self.patterns = [
            0x00,  # All zeros
            0xFF,  # All ones
            0xAA,  # Alternating 10101010
            0x55,  # Alternating 01010101
            0x0F,  # Half-byte pattern
            0xF0,  # Inverse half-byte
        ]

    def generate_packet(self, packet_num: int) -> bytearray:
        packet = create_empty_packet(self.packet_length)
        pattern = self.patterns[packet_num % len(self.patterns)]
        for i in range(self.packet_length):
            packet[i] = pattern
        return packet


def generate_test_vectors(
    output_dir: Path,
    target_size: int,
    r_values: List[int] = None,
    packet_sizes: List[int] = None,
    patterns: List[str] = None,
    seed: int = 42
) -> Dict[str, str]:
    """
    Generate large-scale test vectors.

    Returns dict mapping filename to MD5 hash.
    """
    if r_values is None:
        r_values = list(range(8))  # All R values 0-7

    if packet_sizes is None:
        # Various packet sizes (in bytes)
        packet_sizes = [8, 16, 32, 64, 90, 180, 360]

    if patterns is None:
        patterns = ['predictable', 'housekeeping', 'entropy', 'edge']

    generator_classes = {
        'predictable': PredictablePatternGenerator,
        'housekeeping': HousekeepingLikeGenerator,
        'entropy': HighEntropyGenerator,
        'edge': EdgePatternGenerator,
    }

    # Calculate per-file size
    num_combinations = len(r_values) * len(packet_sizes) * len(patterns)
    size_per_file = target_size // num_combinations

    output_dir.mkdir(parents=True, exist_ok=True)
    results = {}
    total_generated = 0

    for r in r_values:
        for packet_size in packet_sizes:
            for pattern in patterns:
                # Calculate number of packets
                num_packets = max(1, size_per_file // packet_size)
                actual_size = num_packets * packet_size

                # Generate filename
                filename = f"R{r}_F{packet_size*8}_{pattern}.bin"
                filepath = output_dir / filename

                # Create generator
                gen_class = generator_classes.get(pattern, PredictablePatternGenerator)
                generator = gen_class(packet_size, num_packets, seed + r)

                # Generate and save
                print(f"Generating {filename}: {num_packets} packets, {actual_size:,} bytes...")
                md5 = generator.save_to_file(str(filepath))
                results[filename] = md5
                total_generated += actual_size

    # Generate manifest
    manifest_path = output_dir / "manifest.txt"
    with open(manifest_path, 'w') as f:
        f.write("# Large-scale test vectors manifest\n")
        f.write(f"# Total size: {total_generated:,} bytes ({total_generated / (1024*1024):.2f} MB)\n")
        f.write(f"# Seed: {seed}\n\n")
        for filename, md5 in sorted(results.items()):
            f.write(f"{md5}  {filename}\n")

    print(f"\nGenerated {len(results)} files, total {total_generated:,} bytes")
    print(f"Manifest saved to {manifest_path}")

    return results


def main():
    parser = argparse.ArgumentParser(
        description='Generate large-scale POCKET+ test vectors'
    )
    parser.add_argument(
        '--output-dir', '-o',
        type=Path,
        default=Path('../output/large-scale'),
        help='Output directory for generated files'
    )
    parser.add_argument(
        '--size', '-s',
        type=str,
        default='100MB',
        help='Target total size (e.g., 100MB, 1GB, 5GB)'
    )
    parser.add_argument(
        '--r-value', '-r',
        type=int,
        default=None,
        help='Specific R value (0-7), or all if not specified'
    )
    parser.add_argument(
        '--packet-size', '-p',
        type=int,
        default=None,
        help='Specific packet size in bytes, or various if not specified'
    )
    parser.add_argument(
        '--pattern',
        type=str,
        choices=['predictable', 'housekeeping', 'entropy', 'edge'],
        default=None,
        help='Specific pattern, or all if not specified'
    )
    parser.add_argument(
        '--seed',
        type=int,
        default=42,
        help='Random seed for reproducibility'
    )

    args = parser.parse_args()

    target_size = parse_size(args.size)
    r_values = [args.r_value] if args.r_value is not None else None
    packet_sizes = [args.packet_size] if args.packet_size is not None else None
    patterns = [args.pattern] if args.pattern is not None else None

    print(f"Target size: {target_size:,} bytes ({target_size / (1024*1024):.2f} MB)")
    print(f"Output directory: {args.output_dir}")
    print()

    generate_test_vectors(
        output_dir=args.output_dir,
        target_size=target_size,
        r_values=r_values,
        packet_sizes=packet_sizes,
        patterns=patterns,
        seed=args.seed
    )


if __name__ == '__main__':
    main()
