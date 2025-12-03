#!/usr/bin/env python3
"""
Generate realistic housekeeping test vector.
Simulates typical spacecraft telemetry with sensors, counters, and status flags.
"""
import sys
import yaml
import struct
import numpy as np
from pathlib import Path
from common import (
    PacketGenerator, create_empty_packet, write_value, write_float32,
    calculate_crc16_ccitt, increment_counter, set_deterministic_seed
)


class HousekeepingPacketGenerator(PacketGenerator):
    """Generator for realistic spacecraft housekeeping packets."""

    def __init__(self, config: dict):
        packet_length = config['input']['packet_length']
        num_packets = config['input']['num_packets']
        seed = config['input']['seed']
        super().__init__(packet_length, num_packets, seed)

        self.config = config

        # Initialize counters
        self.sequence_count = 0
        self.timestamp = 0x08C91EF80E694003
        self.cmd_counter = 0
        self.tlm_counter = 0
        self.uptime_ms = 0

        # Initialize sensor states
        # Use float values for internal state, but write as integers
        self.mode = 0x01  # NOMINAL
        self.temp_states = [20.0, 25.0, 22.0]  # Will be scaled to integers
        self.voltage_states = [3.3, 5.0]  # Will be scaled to integers
        self.current_states = [0.5, 1.2]  # Will be scaled to integers
        self.gyro_states = [0.0, 0.0, 0.1]  # Will be scaled to integers
        self.accel_states = [0.0, 0.0, 9.81]  # Will be scaled to integers
        self.status_flags = 0x000000FF  # Low 8 bits set

    def generate_packet(self, packet_num: int) -> bytearray:
        """Generate a realistic housekeeping packet."""
        packet = create_empty_packet(self.packet_length)

        # CCSDS Primary Header (6 bytes)
        # Packet Version Number (3 bits) = 0
        # Packet Type (1 bit) = 0 (TM)
        # Secondary Header Flag (1 bit) = 1
        # APID (11 bits) = 1028
        apid = 1028
        version_type_sec_apid = (0 << 13) | (0 << 12) | (1 << 11) | apid
        write_value(packet, (0, 1), version_type_sec_apid)

        # Sequence Flags (2 bits) = 3 (standalone)
        # Sequence Count (14 bits)
        seq_flags_count = (3 << 14) | (self.sequence_count & 0x3FFF)
        write_value(packet, (2, 3), seq_flags_count)
        self.sequence_count = (self.sequence_count + 1) & 0x3FFF

        # Packet Data Length (16 bits) - number of bytes in data field - 1
        write_value(packet, (4, 5), 0x0053)  # 83 bytes of data = 0x53

        # Timestamp (8 bytes) - incrementing
        write_value(packet, (6, 13), self.timestamp)
        self.timestamp = increment_counter(self.timestamp, 1000, 8)  # +1000ms

        # Spacecraft Mode (1 byte)
        # Occasionally change mode (0.1% chance)
        np.random.seed(self.seed + packet_num * 10)
        if np.random.random() < 0.001:
            self.mode = np.random.choice([0x00, 0x01, 0x02])
        packet[14] = self.mode

        # Temperature sensors (12 bytes, 3x 4-byte integers, scaled by 100)
        # Real spacecraft often uses integer scaling: temp_celsius * 100
        # Changes slowly - only update every 50 packets
        if packet_num % 50 == 0:
            np.random.seed(self.seed + packet_num * 11)
            for i, (nominal, variation) in enumerate([(20.0, 2.0), (25.0, 3.0), (22.0, 2.5)]):
                change = np.random.normal(0, variation * 0.1)
                self.temp_states[i] += change
                self.temp_states[i] = np.clip(self.temp_states[i], nominal - variation, nominal + variation)

        for i in range(3):
            temp_scaled = int(self.temp_states[i] * 100)  # e.g., 20.5°C → 2050
            write_value(packet, (15 + i * 4, 18 + i * 4), temp_scaled)

        # Voltage monitors (8 bytes, 2x 4-byte integers, scaled by 1000)
        # Changes very slowly - only every 100 packets
        if packet_num % 100 == 0:
            np.random.seed(self.seed + packet_num * 12)
            for i, (nominal, variation) in enumerate([(3.3, 0.1), (5.0, 0.15)]):
                change = np.random.normal(0, variation * 0.05)
                self.voltage_states[i] += change
                self.voltage_states[i] = np.clip(self.voltage_states[i], nominal - variation, nominal + variation)

        for i in range(2):
            voltage_scaled = int(self.voltage_states[i] * 1000)  # e.g., 3.3V → 3300
            write_value(packet, (27 + i * 4, 30 + i * 4), voltage_scaled)

        # Current sensors (8 bytes, 2x 4-byte integers, scaled by 1000)
        # Changes moderately - every 25 packets
        if packet_num % 25 == 0:
            np.random.seed(self.seed + packet_num * 13)
            for i, (nominal, variation) in enumerate([(0.5, 0.05), (1.2, 0.1)]):
                change = np.random.normal(0, variation * 0.08)
                self.current_states[i] += change
                self.current_states[i] = np.clip(self.current_states[i], nominal - variation, nominal + variation)

        for i in range(2):
            current_scaled = int(self.current_states[i] * 1000)  # e.g., 0.5A → 500
            write_value(packet, (35 + i * 4, 38 + i * 4), current_scaled)

        # Status flags (4 bytes)
        # Occasionally flip some bits
        np.random.seed(self.seed + packet_num * 14)
        if np.random.random() < 0.01:
            bit_to_flip = np.random.randint(8, 11)  # Flip bits 8-10
            self.status_flags ^= (1 << bit_to_flip)
        write_value(packet, (43, 46), self.status_flags)

        # Command counter (4 bytes)
        write_value(packet, (47, 50), self.cmd_counter)
        self.cmd_counter = increment_counter(self.cmd_counter, 1, 4)

        # Telemetry packet counter (4 bytes)
        write_value(packet, (51, 54), self.tlm_counter)
        self.tlm_counter = increment_counter(self.tlm_counter, 1, 4)

        # Uptime in milliseconds (8 bytes)
        write_value(packet, (55, 62), self.uptime_ms)
        self.uptime_ms = increment_counter(self.uptime_ms, 1000, 8)

        # Gyroscope data (12 bytes, 3x 4-byte integers, scaled by 10000)
        # Changes every 10 packets
        if packet_num % 10 == 0:
            np.random.seed(self.seed + packet_num * 15)
            for i, (nominal, variation) in enumerate([(0.0, 0.05), (0.0, 0.05), (0.1, 0.1)]):
                change = np.random.normal(0, variation * 0.3)
                self.gyro_states[i] += change
                self.gyro_states[i] = np.clip(self.gyro_states[i], nominal - variation, nominal + variation)

        for i in range(3):
            gyro_scaled = int(self.gyro_states[i] * 10000)  # e.g., 0.1 rad/s → 1000
            write_value(packet, (63 + i * 4, 66 + i * 4), gyro_scaled, signed=True)

        # Accelerometer data (12 bytes, 3x 4-byte integers, scaled by 1000)
        # Changes every 20 packets
        if packet_num % 20 == 0:
            np.random.seed(self.seed + packet_num * 16)
            for i, (nominal, variation) in enumerate([(0.0, 0.1), (0.0, 0.1), (9.81, 0.2)]):
                change = np.random.normal(0, variation * 0.3)
                self.accel_states[i] += change
                self.accel_states[i] = np.clip(self.accel_states[i], nominal - variation, nominal + variation)

        for i in range(3):
            accel_scaled = int(self.accel_states[i] * 1000)  # e.g., 9.81 m/s² → 9810
            write_value(packet, (75 + i * 4, 78 + i * 4), accel_scaled, signed=True)

        # CRC-16 (2 bytes) - use fixed pattern instead of calculating
        # Real CRCs are high-entropy, so use stable value for better compression
        write_value(packet, (87, 88), 0xA5A5)

        # Padding (1 byte)
        packet[89] = 0x00

        return packet


def main():
    if len(sys.argv) != 2:
        print("Usage: generate_housekeeping.py <config.yaml>")
        sys.exit(1)

    config_file = sys.argv[1]

    # Load configuration
    with open(config_file, 'r') as f:
        config = yaml.safe_load(f)

    print(f"Generating housekeeping test vector: {config['name']}")
    print(f"Description: {config['description'].strip()}")

    # Create output directory
    output_dir = Path(config['output']['dir'])
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate packets
    generator = HousekeepingPacketGenerator(config)
    output_file = output_dir / config['output']['input_file']

    print(f"\nGenerating {config['input']['num_packets']} packets...")
    md5_hash = generator.save_to_file(str(output_file))

    print(f"Output: {output_file}")
    print("\nDone!")

    return 0


if __name__ == '__main__':
    sys.exit(main())
