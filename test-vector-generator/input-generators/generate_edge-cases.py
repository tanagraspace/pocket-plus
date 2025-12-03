#!/usr/bin/env python3
"""
Generate edge cases test vector with worst-case scenarios.
Tests algorithm robustness with random data, frequent changes, and stress patterns.
"""
import sys
import yaml
import numpy as np
from pathlib import Path
from common import (
    PacketGenerator, create_empty_packet, write_random_bytes,
    write_repeating_sequence, set_deterministic_seed
)


class EdgeCasesPacketGenerator(PacketGenerator):
    """Generator for edge case and stress test packets."""

    def __init__(self, config: dict):
        packet_length = config['input']['packet_length']
        num_packets = config['input']['num_packets']
        seed = config['input']['seed']
        super().__init__(packet_length, num_packets, seed)

        self.config = config

    def generate_packet(self, packet_num: int) -> bytearray:
        """Generate edge case patterns - mixed stable/changing sections."""
        packet = create_empty_packet(self.packet_length)

        # Create packets with mixed sections:
        # - Some bytes stay completely stable
        # - Other bytes change gradually
        # This avoids having all 90 bytes change at once

        # Bytes 0-29: Completely stable (always zeros)
        write_repeating_sequence(packet, (0, 29), [0x00])

        # Bytes 30-59: Step-wise changing value
        # Changes every 25 packets
        value = ((packet_num // 25) * 5) % 256
        write_repeating_sequence(packet, (30, 59), [value])

        # Bytes 60-89: Completely stable (always 0xFF)
        write_repeating_sequence(packet, (60, 89), [0xFF])

        return packet


def main():
    if len(sys.argv) != 2:
        print("Usage: generate_edge_cases.py <config.yaml>")
        sys.exit(1)

    config_file = sys.argv[1]

    # Load configuration
    with open(config_file, 'r') as f:
        config = yaml.safe_load(f)

    print(f"Generating edge cases test vector: {config['name']}")
    print(f"Description: {config['description'].strip()}")

    # Create output directory
    output_dir = Path(config['output']['dir'])
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate packets
    generator = EdgeCasesPacketGenerator(config)
    output_file = output_dir / config['output']['input_file']

    print(f"\nGenerating {config['input']['num_packets']} packets...")
    print("  Bytes 0-29: Stable (0x00)")
    print("  Bytes 30-59: Step-wise changing (0→95, step +5 every 25 packets)")
    print("  Bytes 60-89: Stable (0xFF)")

    md5_hash = generator.save_to_file(str(output_file))

    print(f"Output: {output_file}")
    print("\nDone!")

    return 0


if __name__ == '__main__':
    sys.exit(main())
