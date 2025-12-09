// Package pocketplus implements the CCSDS 124.0-B-1 POCKET+ lossless
// compression algorithm for fixed-length housekeeping data.
//
// POCKET+ is designed for compressing telemetry data from spacecraft,
// where consecutive packets typically have many bits that remain unchanged
// or follow predictable patterns.
//
// Basic usage:
//
//	// Compress data
//	compressed, err := pocketplus.Compress(data, packetSize, robustness, pt, ft, rt)
//	if err != nil {
//	    log.Fatal(err)
//	}
//
//	// Decompress data
//	decompressed, err := pocketplus.Decompress(compressed, packetSize, robustness)
//	if err != nil {
//	    log.Fatal(err)
//	}
//
// For more information about the POCKET+ algorithm, see:
// https://ccsds.org/Pubs/124x0b1.pdf
package pocketplus
