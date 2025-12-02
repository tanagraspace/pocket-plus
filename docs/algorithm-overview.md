# POCKET+ Algorithm Overview

## Introduction

POCKET+ is a lossless compression algorithm designed for spacecraft housekeeping data. It operates on fixed-length data packets and uses delta compression techniques combined with bitwise operations.

## Core Principles

### 1. Delta Compression

POCKET+ compresses data by encoding the differences (deltas) between successive data values rather than the values themselves. This is particularly effective for housekeeping data which often changes gradually.

### 2. Low-Level Operations

The algorithm is implemented using only basic bitwise operations:
- OR
- XOR
- AND
- Bit shifts

This ensures compatibility with resource-constrained spacecraft processors.

### 3. Independent Packet Processing

Each data packet can be compressed and transmitted independently without waiting for subsequent packets. This enables:
- Immediate transmission ("fire and forget")
- Reduced latency
- Simpler implementation

### 4. Packet Loss Resilience

The algorithm includes mechanisms allowing decompression to continue even when a configurable number of successive packets are lost. This is crucial for:
- Unreliable space-to-ground RF links
- High-noise communication environments
- Mission-critical data transmission

## Implementation Considerations

### Performance Requirements

- **Low CPU Usage** - Must run on processors with limited computational power
- **Real-time Constraints** - Predictable execution time
- **Memory Efficiency** - Minimal RAM footprint

### Data Characteristics

POCKET+ is optimized for:
- Fixed-length housekeeping packets
- Slowly varying telemetry data
- Periodic sensor readings
- Status and health monitoring data

## Next Steps

For detailed implementation guidance, see [Implementation Guide](implementation-guide.md).

For validation, refer to [Test Vectors](../test-vectors/README.md).
