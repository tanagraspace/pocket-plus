//! Variable-length bit buffer for building compressed output.
//!
//! This module provides a dynamically-growing bit buffer for constructing
//! compressed output streams. Bits are appended sequentially using MSB-first
//! ordering as required by CCSDS 124.0-B-1.
//!
//! ## Bit Ordering
//! Bits are appended MSB-first within each byte:
//! - First bit appended goes to bit position 7
//! - Second bit goes to position 6, etc.

#![allow(clippy::cast_possible_truncation)]

use crate::bitvector::BitVector;

/// Maximum output buffer size in bytes.
const MAX_OUTPUT_BYTES: usize = 65535 * 6;

/// Variable-length bit buffer for building compressed output.
///
/// Uses a 64-bit accumulator for efficient bit packing on ground systems.
#[derive(Clone, Debug, Default)]
pub struct BitBuffer {
    /// Byte storage for flushed bits.
    data: Vec<u8>,
    /// Total number of bits in the buffer.
    num_bits: usize,
    /// 64-bit accumulator for pending bits.
    acc: u64,
    /// Number of bits in the accumulator.
    acc_len: usize,
}

impl BitBuffer {
    /// Create a new empty bit buffer.
    pub fn new() -> Self {
        Self {
            data: Vec::with_capacity(1024),
            num_bits: 0,
            acc: 0,
            acc_len: 0,
        }
    }

    /// Clear the buffer, resetting to empty state.
    pub fn clear(&mut self) {
        self.data.clear();
        self.num_bits = 0;
        self.acc = 0;
        self.acc_len = 0;
    }

    /// Get the total number of bits in the buffer.
    #[inline]
    pub fn len(&self) -> usize {
        self.num_bits
    }

    /// Check if the buffer is empty.
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.num_bits == 0
    }

    /// Flush complete bytes from accumulator to data buffer.
    fn flush_acc(&mut self) {
        while self.acc_len >= 8 {
            // Extract top 8 bits
            self.acc_len -= 8;
            let byte = (self.acc >> self.acc_len) as u8;
            self.data.push(byte);
            // Clear extracted bits
            self.acc &= (1u64 << self.acc_len) - 1;
        }
    }

    /// Append a single bit to the buffer.
    ///
    /// # Arguments
    /// * `bit` - Bit value (0 or non-zero for 1)
    ///
    /// # Returns
    /// `true` on success, `false` if buffer would overflow.
    pub fn append_bit(&mut self, bit: u8) -> bool {
        // Check for overflow
        let max_bits = MAX_OUTPUT_BYTES * 8;
        if self.num_bits >= max_bits {
            return false;
        }

        // Accumulate bit in MSB-first order
        let bit_val = u64::from(bit) & 1;
        self.acc = (self.acc << 1) | bit_val;
        self.acc_len += 1;
        self.num_bits += 1;

        // Flush when accumulator has 8+ bits
        if self.acc_len >= 8 {
            self.flush_acc();
        }

        true
    }

    /// Append multiple bits from a value.
    ///
    /// # Arguments
    /// * `value` - Value containing bits (right-justified)
    /// * `num_bits` - Number of bits to append (1-56)
    ///
    /// # Returns
    /// `true` on success, `false` if buffer would overflow.
    pub fn append_value(&mut self, value: u32, num_bits: usize) -> bool {
        if num_bits == 0 || num_bits > 56 {
            return false;
        }

        // Check for overflow
        let max_bits = MAX_OUTPUT_BYTES * 8;
        if self.num_bits + num_bits > max_bits {
            return false;
        }

        // Mask to ensure only the relevant bits are used
        let mask = (1u64 << num_bits) - 1;
        let masked_value = u64::from(value) & mask;

        self.acc = (self.acc << num_bits) | masked_value;
        self.acc_len += num_bits;
        self.num_bits += num_bits;

        // Flush complete bytes
        self.flush_acc();

        true
    }

    /// Append bits from a byte slice.
    ///
    /// # Arguments
    /// * `data` - Source byte slice
    /// * `num_bits` - Number of bits to append
    ///
    /// # Returns
    /// `true` on success, `false` if buffer would overflow.
    pub fn append_bits(&mut self, data: &[u8], num_bits: usize) -> bool {
        // Check for overflow
        let max_bits = MAX_OUTPUT_BYTES * 8;
        if self.num_bits + num_bits > max_bits {
            return false;
        }

        // Append each bit MSB-first
        for i in 0..num_bits {
            let byte_index = i / 8;
            let bit_index = i % 8;

            // Extract bits MSB-first (bit 7, 6, 5, ..., 0)
            let shift_amount = 7 - bit_index;
            let bit = (data[byte_index] >> shift_amount) & 1;

            if !self.append_bit(bit) {
                return false;
            }
        }

        true
    }

    /// Append all bits from a bit vector.
    ///
    /// # Arguments
    /// * `bv` - Source bit vector
    ///
    /// # Returns
    /// `true` on success, `false` if buffer would overflow.
    pub fn append_bitvector(&mut self, bv: &BitVector) -> bool {
        let num_bytes = (bv.len() + 7) / 8;

        for byte_idx in 0..num_bytes {
            let mut bits_in_this_byte = 8;

            // Last byte may have fewer than 8 bits
            if byte_idx == num_bytes - 1 {
                let remainder = bv.len() % 8;
                if remainder != 0 {
                    bits_in_this_byte = remainder;
                }
            }

            // Append bits from this byte position
            let start_bit = byte_idx * 8;
            for bit_offset in 0..bits_in_this_byte {
                let pos = start_bit + bit_offset;
                let bit = bv.get_bit(pos);

                if !self.append_bit(bit) {
                    return false;
                }
            }
        }

        true
    }

    /// Convert buffer to bytes.
    ///
    /// # Returns
    /// A new `Vec<u8>` containing the buffer data.
    pub fn to_bytes(&self) -> Vec<u8> {
        let num_bytes = (self.num_bits + 7) / 8;
        let mut result = Vec::with_capacity(num_bytes);

        // Copy flushed bytes from data buffer
        result.extend_from_slice(&self.data);

        // Handle remaining bits in accumulator
        if self.acc_len > 0 {
            // Shift accumulator bits to MSB position
            let last_byte = (self.acc << (8 - self.acc_len)) as u8;
            result.push(last_byte);
        }

        result
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_new() {
        let bb = BitBuffer::new();
        assert_eq!(bb.len(), 0);
        assert!(bb.is_empty());
    }

    #[test]
    fn test_append_bit() {
        let mut bb = BitBuffer::new();

        // Append 8 bits: 10110010 = 0xB2
        bb.append_bit(1);
        bb.append_bit(0);
        bb.append_bit(1);
        bb.append_bit(1);
        bb.append_bit(0);
        bb.append_bit(0);
        bb.append_bit(1);
        bb.append_bit(0);

        assert_eq!(bb.len(), 8);

        let bytes = bb.to_bytes();
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes[0], 0xB2);
    }

    #[test]
    fn test_append_value() {
        let mut bb = BitBuffer::new();

        // Append 0b1010 (4 bits)
        bb.append_value(0b1010, 4);
        assert_eq!(bb.len(), 4);

        // Append 0b1100 (4 bits) -> full byte 0b10101100 = 0xAC
        bb.append_value(0b1100, 4);
        assert_eq!(bb.len(), 8);

        let bytes = bb.to_bytes();
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes[0], 0xAC);
    }

    #[test]
    fn test_append_bits() {
        let mut bb = BitBuffer::new();

        let data = vec![0xDE, 0xAD];
        bb.append_bits(&data, 16);

        assert_eq!(bb.len(), 16);

        let bytes = bb.to_bytes();
        assert_eq!(bytes, data);
    }

    #[test]
    fn test_append_bitvector() {
        let mut bb = BitBuffer::new();

        let mut bv = BitVector::new(16);
        // Set some bits: make it 0xCAFE
        // 0xCA = 11001010, 0xFE = 11111110
        bv.set_bit(0, 1);
        bv.set_bit(1, 1);
        bv.set_bit(2, 0);
        bv.set_bit(3, 0);
        bv.set_bit(4, 1);
        bv.set_bit(5, 0);
        bv.set_bit(6, 1);
        bv.set_bit(7, 0);
        bv.set_bit(8, 1);
        bv.set_bit(9, 1);
        bv.set_bit(10, 1);
        bv.set_bit(11, 1);
        bv.set_bit(12, 1);
        bv.set_bit(13, 1);
        bv.set_bit(14, 1);
        bv.set_bit(15, 0);

        bb.append_bitvector(&bv);

        assert_eq!(bb.len(), 16);

        let bytes = bb.to_bytes();
        assert_eq!(bytes.len(), 2);
        assert_eq!(bytes[0], 0xCA);
        assert_eq!(bytes[1], 0xFE);
    }

    #[test]
    fn test_partial_byte() {
        let mut bb = BitBuffer::new();

        // Append 5 bits: 10110
        bb.append_bit(1);
        bb.append_bit(0);
        bb.append_bit(1);
        bb.append_bit(1);
        bb.append_bit(0);

        assert_eq!(bb.len(), 5);

        let bytes = bb.to_bytes();
        assert_eq!(bytes.len(), 1);
        // 10110 left-aligned = 10110000 = 0xB0
        assert_eq!(bytes[0], 0xB0);
    }

    #[test]
    fn test_clear() {
        let mut bb = BitBuffer::new();
        bb.append_value(0xFF, 8);
        assert_eq!(bb.len(), 8);

        bb.clear();
        assert_eq!(bb.len(), 0);
        assert!(bb.is_empty());
    }

    #[test]
    fn test_multi_byte() {
        let mut bb = BitBuffer::new();

        // Append 24 bits: 0xDEADBE
        bb.append_value(0xDE, 8);
        bb.append_value(0xAD, 8);
        bb.append_value(0xBE, 8);

        assert_eq!(bb.len(), 24);

        let bytes = bb.to_bytes();
        assert_eq!(bytes.len(), 3);
        assert_eq!(bytes[0], 0xDE);
        assert_eq!(bytes[1], 0xAD);
        assert_eq!(bytes[2], 0xBE);
    }
}
