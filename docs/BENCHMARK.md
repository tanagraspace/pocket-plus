# POCKET+ Benchmark Results

Performance comparison across POCKET+ implementations.

## Environment

| Property | Value |
|----------|-------|
| OS | Alpine Linux v3.20 |
| Architecture | aarch64 |
| CPU | ARMv8 |
| CPU Cores | 2 |
| Memory | 1.9 GB |
| Container | Docker (Alpine 3.20) |
| Date | 2025-12-12 03:27:38 UTC |

## Test Vectors

All vectors are synthetic test data except for venus-express, which contains real housekeeping telemetry from ESA's [Venus Express](https://www.esa.int/Science_Exploration/Space_Science/Venus_Express) mission.

| Vector | Packets | Size | R | pt | ft | rt |
|--------|---------|------|---|----|----|-----|
| simple | 100 | 9.0 KB | 1 | 10 | 20 | 50 |
| hiro | 100 | 9.0 KB | 7 | 10 | 20 | 50 |
| housekeeping | 10000 | 900.0 KB | 2 | 20 | 50 | 100 |
| venus-express | 151200 | 13.6 MB | 2 | 20 | 50 | 100 |

## Compression Results

### Compression: simple (100 packets, 9.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |      .088 |     1128413 |    .88 |             96.85 |
| C++            |      .025 |     3921568 |    .25 |            336.59 |
| Go             |      .102 |      979048 |   1.02 |             84.03 |
| Rust           |      .061 |     1631853 |    .61 |            140.06 |
| Java           |      .217 |      459791 |   2.17 |             39.46 |

### Compression: hiro (100 packets, 9.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |      .096 |     1036484 |    .96 |             88.96 |
| C++            |      .031 |     3152585 |    .31 |            270.58 |
| Go             |      .216 |      462299 |   2.16 |             39.67 |
| Rust           |      .093 |     1070778 |    .93 |             91.90 |
| Java           |      .143 |      695265 |   1.43 |             59.67 |

### Compression: housekeeping (10000 packets, 900.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |    11.379 |      878738 |   1.13 |             75.42 |
| C++            |     4.692 |     2131200 |    .46 |            182.92 |
| Go             |    14.928 |      669861 |   1.49 |             57.49 |
| Rust           |     9.536 |     1048569 |    .95 |             89.99 |
| Java           |     7.865 |     1271404 |    .78 |            109.12 |

### Compression: venus-express (151200 packets, 13608.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |   222.156 |      680599 |   1.46 |             58.41 |
| C++            |    92.506 |     1634478 |    .61 |            140.28 |
| Go             |   266.338 |      567698 |   1.76 |             48.72 |
| Rust           |   198.289 |      762519 |   1.31 |             65.44 |
| Java           |   149.881 |     1008795 |    .99 |             86.58 |

## Decompression Results

### Decompression: simple (100 packets, 9.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |      .064 |     1539882 |    .64 |            132.16 |
| C++            |      .023 |     4306632 |    .23 |            369.64 |
| Go             |      .100 |      998302 |   1.00 |             85.68 |
| Rust           |      .101 |      982994 |   1.01 |             84.37 |
| Java           |      .328 |      304247 |   3.28 |             26.11 |

### Decompression: hiro (100 packets, 9.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |      .130 |      767106 |   1.30 |             65.84 |
| C++            |      .040 |     2455192 |    .40 |            210.73 |
| Go             |      .200 |      498181 |   2.00 |             42.75 |
| Rust           |      .137 |      727166 |   1.37 |             62.41 |
| Java           |      .651 |      153385 |   6.51 |             13.16 |

### Decompression: housekeeping (10000 packets, 900.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |    13.764 |      726504 |   1.37 |             62.35 |
| C++            |     3.585 |     2788661 |    .35 |            239.35 |
| Go             |    16.908 |      591406 |   1.69 |             50.76 |
| Rust           |    13.701 |      729830 |   1.37 |             62.64 |
| Java           |    15.495 |      645363 |   1.54 |             55.39 |

### Decompression: venus-express (151200 packets, 13608.0 KB)

| Implementation | Time (ms) | Packets/sec | µs/pkt | Throughput (MB/s) |
|----------------|-----------|-------------|--------|-------------------|
| C              |   241.568 |      625909 |   1.59 |             53.72 |
| C++            |    88.603 |     1706472 |    .58 |            146.46 |
| Go             |   395.231 |      382561 |   2.61 |             32.83 |
| Rust           |   305.679 |      494636 |   2.02 |             42.45 |
| Java           |   317.050 |      476895 |   2.09 |             40.93 |

## Methodology

- **Iterations**: 100 (with 1 warmup iteration)
- **Packet size**: 90 bytes (720 bits)
- **Timing**: Wall-clock time for complete file processing

## Notes

- **Python** is excluded from benchmarks as it is not performance-focused.
  The Python implementation prioritizes readability and correctness over speed.
- Results may vary based on system load and hardware.
- Go benchmarks use the native `go test -bench` framework.
- All other implementations use custom benchmark binaries with consistent output format.
