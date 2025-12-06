#!/usr/bin/env python3
"""
Generate simple synthetic test vector with high compressibility patterns.
This creates predictable data ideal for quick validation and testing.
"""
import sys
import yaml
from pathlib import Path
from common import (
    PacketGenerator, create_empty_packet, write_value,
    write_repeating_sequence, increment_counter
)


class SimplePacketGenerator(PacketGenerator):
    """Generator for simple, highly compressible test packets."""

    def __init__(self, config: dict):
        packet_length = config['input']['packet_length']
        num_packets = config['input']['num_packets']
        seed = config['input']['seed']
        super().__init__(packet_length, num_packets, seed)

        self.config = config
        self.counter_value = 0x0000

    def generate_packet(self, packet_num: int) -> bytearray:
        """Generate a single simple packet with defined patterns."""
        packet = create_empty_packet(self.packet_length)

        # Pattern 1: Repeating sequence at start (bytes 0-3)
        write_repeating_sequence(packet, (0, 3), [0x08, 0xD4, 0xF1, 0xAB])

        # Pattern 2: Incrementing counter (bytes 4-5)
        write_value(packet, (4, 5), self.counter_value)
        self.counter_value = increment_counter(self.counter_value, 1, 2)

        # Pattern 3: Stable values (bytes 6-13)
        write_repeating_sequence(packet, (6, 13), [0x00])

        # Pattern 4: Slow changing value (bytes 14-17)
        # Changes every 10 packets
        slow_value = 0x12345678 + (packet_num // 10)
        write_value(packet, (14, 17), slow_value & 0xFFFFFFFF)

        # Pattern 5: Stable temperature readings (bytes 18-25)
        # Simulates stable sensor readings
        write_repeating_sequence(packet, (18, 25), [0xFF, 0xFF])

        # Pattern 6: More stable zeros (bytes 26-49)
        write_repeating_sequence(packet, (26, 49), [0x00])

        # Pattern 7: Very slow changing value (bytes 50-53)
        # Changes every 25 packets
        slow_value2 = 0xABCDEF00 + (packet_num // 25)
        write_value(packet, (50, 53), slow_value2 & 0xFFFFFFFF)

        # Pattern 8: Repeating pattern (bytes 54-89)
        # Simple repeating sequence that compresses well
        write_repeating_sequence(packet, (54, 89), [0x01, 0x02, 0x03, 0x04])

        return packet


def main():
    if len(sys.argv) != 2:
        print("Usage: generate_simple.py <config.yaml>")
        sys.exit(1)

    config_file = sys.argv[1]

    # Load configuration
    with open(config_file, 'r') as f:
        config = yaml.safe_load(f)

    print(f"Generating simple test vector: {config['name']}")
    print(f"Description: {config['description'].strip()}")

    # Create output directory
    output_dir = Path(config['output']['dir'])
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate packets
    generator = SimplePacketGenerator(config)
    output_file = output_dir / config['output']['input_file']

    print(f"\nGenerating {config['input']['num_packets']} packets...")
    md5_hash = generator.save_to_file(str(output_file))

    print(f"Output: {output_file}")
    print(f"MD5: {md5_hash}")
    print("\nDone!")

    return 0


if __name__ == '__main__':
    sys.exit(main())
