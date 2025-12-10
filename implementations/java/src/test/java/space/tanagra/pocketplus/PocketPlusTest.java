/*
 * Copyright (c) 2025 Tanagra Space
 * SPDX-License-Identifier: MIT
 */

package space.tanagra.pocketplus;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;

/** Unit tests for PocketPlus high-level API. */
class PocketPlusTest {

  @Test
  void testVersion() {
    String version = PocketPlus.version();
    assertNotNull(version);
    assertTrue(version.matches("\\d+\\.\\d+\\.\\d+"));
    assertEquals("1.0.0", version);
  }

  @Test
  void testCompressNullInput() {
    assertThrows(PocketException.class, () -> PocketPlus.compress(null, 90, 2, 20, 50, 100));
  }

  @Test
  void testCompressInvalidPacketSize() {
    byte[] data = new byte[90];
    assertThrows(PocketException.class, () -> PocketPlus.compress(data, 0, 2, 20, 50, 100));
    assertThrows(PocketException.class, () -> PocketPlus.compress(data, -1, 2, 20, 50, 100));
  }

  @Test
  void testCompressInvalidRobustness() {
    byte[] data = new byte[90];
    assertThrows(PocketException.class, () -> PocketPlus.compress(data, 90, -1, 20, 50, 100));
    assertThrows(PocketException.class, () -> PocketPlus.compress(data, 90, 8, 20, 50, 100));
  }

  @Test
  void testCompressNonMultiplePacketSize() {
    byte[] data = new byte[100]; // Not a multiple of 90
    assertThrows(PocketException.class, () -> PocketPlus.compress(data, 90, 2, 20, 50, 100));
  }

  @Test
  void testDecompressNullInput() {
    assertThrows(PocketException.class, () -> PocketPlus.decompress(null, 90, 2));
  }

  @Test
  void testDecompressInvalidPacketSize() {
    byte[] data = new byte[100];
    assertThrows(PocketException.class, () -> PocketPlus.decompress(data, 0, 2));
    assertThrows(PocketException.class, () -> PocketPlus.decompress(data, -1, 2));
  }

  @Test
  void testDecompressInvalidRobustness() {
    byte[] data = new byte[100];
    assertThrows(PocketException.class, () -> PocketPlus.decompress(data, 90, -1));
    assertThrows(PocketException.class, () -> PocketPlus.decompress(data, 90, 8));
  }

  @Test
  void testSimpleRoundTrip() throws PocketException {
    // Create simple test data - 2 identical packets
    byte[] original = new byte[180]; // 2 packets of 90 bytes
    for (int i = 0; i < original.length; i++) {
      original[i] = (byte) (i % 256);
    }

    // Compress
    byte[] compressed = PocketPlus.compress(original, 90, 1, 10, 20, 50);
    assertNotNull(compressed);
    assertTrue(compressed.length > 0);

    // Decompress
    byte[] decompressed = PocketPlus.decompress(compressed, 90, 1);
    assertNotNull(decompressed);

    // Verify round-trip
    assertArrayEquals(original, decompressed);
  }

  @Test
  void testConstants() {
    assertEquals(65535, PocketPlus.MAX_PACKET_LENGTH);
    assertEquals(8192, PocketPlus.MAX_PACKET_BYTES); // ceil(65535/8) = 8192 bytes
    assertEquals(7, PocketPlus.MAX_ROBUSTNESS);
  }

  // ========== Compressor validation tests ==========

  @Test
  void testCompressorInvalidLength() {
    assertThrows(IllegalArgumentException.class, () -> new Compressor(0, null, 1, 10, 20, 50));
    assertThrows(IllegalArgumentException.class, () -> new Compressor(-1, null, 1, 10, 20, 50));
    assertThrows(
        IllegalArgumentException.class,
        () -> new Compressor(PocketPlus.MAX_PACKET_LENGTH + 1, null, 1, 10, 20, 50));
  }

  @Test
  void testCompressorInvalidRobustness() {
    assertThrows(IllegalArgumentException.class, () -> new Compressor(720, null, -1, 10, 20, 50));
    assertThrows(IllegalArgumentException.class, () -> new Compressor(720, null, 8, 10, 20, 50));
  }

  @Test
  void testCompressorValidBoundary() {
    // Should not throw - boundary values
    new Compressor(1, null, 0, 10, 20, 50);
    new Compressor(PocketPlus.MAX_PACKET_LENGTH, null, 7, 10, 20, 50);
  }

  // ========== Decompressor validation tests ==========

  @Test
  void testDecompressorInvalidLength() {
    assertThrows(IllegalArgumentException.class, () -> new Decompressor(0, null, 1));
    assertThrows(IllegalArgumentException.class, () -> new Decompressor(-1, null, 1));
    assertThrows(
        IllegalArgumentException.class,
        () -> new Decompressor(PocketPlus.MAX_PACKET_LENGTH + 1, null, 1));
  }

  @Test
  void testDecompressorInvalidRobustness() {
    assertThrows(IllegalArgumentException.class, () -> new Decompressor(720, null, -1));
    assertThrows(IllegalArgumentException.class, () -> new Decompressor(720, null, 8));
  }

  @Test
  void testDecompressorValidBoundary() {
    // Should not throw - boundary values
    new Decompressor(1, null, 0);
    new Decompressor(PocketPlus.MAX_PACKET_LENGTH, null, 7);
  }

  @Test
  void testDecompressorReset() {
    Decompressor decomp = new Decompressor(720, null, 1);
    assertEquals(0, decomp.getTimeIndex());
    decomp.reset();
    assertEquals(0, decomp.getTimeIndex());
  }

  @Test
  void testCompressorReset() {
    Compressor comp = new Compressor(720, null, 1, 10, 20, 50);
    assertEquals(0, comp.getTimeIndex());
    comp.reset();
    assertEquals(0, comp.getTimeIndex());
  }
}
