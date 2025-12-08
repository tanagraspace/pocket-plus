package pocketplus

import (
	"bytes"
	"testing"
)

func TestNewBitBuffer(t *testing.T) {
	bb := NewBitBuffer()
	if bb.NumBits() != 0 {
		t.Errorf("Expected 0 bits, got %d", bb.NumBits())
	}
	if len(bb.ToBytes()) != 0 {
		t.Error("Expected empty bytes")
	}
}

func TestBitBufferAppendBit(t *testing.T) {
	bb := NewBitBuffer()

	// Append 8 bits: 10101010
	bb.AppendBit(1)
	bb.AppendBit(0)
	bb.AppendBit(1)
	bb.AppendBit(0)
	bb.AppendBit(1)
	bb.AppendBit(0)
	bb.AppendBit(1)
	bb.AppendBit(0)

	if bb.NumBits() != 8 {
		t.Errorf("Expected 8 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	if len(result) != 1 {
		t.Errorf("Expected 1 byte, got %d", len(result))
	}
	if result[0] != 0xAA {
		t.Errorf("Expected 0xAA, got 0x%02X", result[0])
	}
}

func TestBitBufferAppendBitMSBFirst(t *testing.T) {
	bb := NewBitBuffer()

	// First bit should go to position 7 (MSB)
	bb.AppendBit(1)
	// Pad to 8 bits
	for i := 0; i < 7; i++ {
		bb.AppendBit(0)
	}

	result := bb.ToBytes()
	if result[0] != 0x80 {
		t.Errorf("MSB-first: expected 0x80, got 0x%02X", result[0])
	}
}

func TestBitBufferAppendBits(t *testing.T) {
	bb := NewBitBuffer()

	// Append 16 bits from 2 bytes
	data := []byte{0xAB, 0xCD}
	bb.AppendBits(data, 16)

	if bb.NumBits() != 16 {
		t.Errorf("Expected 16 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	if !bytes.Equal(result, []byte{0xAB, 0xCD}) {
		t.Errorf("Expected [0xAB, 0xCD], got %v", result)
	}
}

func TestBitBufferAppendBitsPartial(t *testing.T) {
	bb := NewBitBuffer()

	// Append only 12 bits from 2 bytes
	data := []byte{0xFF, 0xFF}
	bb.AppendBits(data, 12)

	if bb.NumBits() != 12 {
		t.Errorf("Expected 12 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	// First byte: 0xFF (8 bits)
	// Second byte: top 4 bits of 0xFF = 0xF0
	if !bytes.Equal(result, []byte{0xFF, 0xF0}) {
		t.Errorf("Expected [0xFF, 0xF0], got %v", result)
	}
}

func TestBitBufferAppendBitVector(t *testing.T) {
	bv, _ := NewBitVector(16)
	bv.FromBytes([]byte{0x12, 0x34})

	bb := NewBitBuffer()
	bb.AppendBitVector(bv)

	if bb.NumBits() != 16 {
		t.Errorf("Expected 16 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	if !bytes.Equal(result, []byte{0x12, 0x34}) {
		t.Errorf("Expected [0x12, 0x34], got %v", result)
	}
}

func TestBitBufferAppendBitVectorPartial(t *testing.T) {
	// Test with 12 bits
	bv, _ := NewBitVector(12)
	bv.FromBytes([]byte{0xAB, 0xC0}) // Only top 12 bits valid

	bb := NewBitBuffer()
	bb.AppendBitVector(bv)

	if bb.NumBits() != 12 {
		t.Errorf("Expected 12 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	// Should get the 12 valid bits
	if result[0] != 0xAB {
		t.Errorf("First byte: expected 0xAB, got 0x%02X", result[0])
	}
	// Second byte should have top 4 bits
	if result[1] != 0xC0 {
		t.Errorf("Second byte: expected 0xC0, got 0x%02X", result[1])
	}
}

func TestBitBufferAppendBitVectorN(t *testing.T) {
	bv, _ := NewBitVector(16)
	bv.FromBytes([]byte{0xFF, 0x00})

	bb := NewBitBuffer()
	bb.AppendBitVectorN(bv, 8)

	if bb.NumBits() != 8 {
		t.Errorf("Expected 8 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	if !bytes.Equal(result, []byte{0xFF}) {
		t.Errorf("Expected [0xFF], got %v", result)
	}
}

func TestBitBufferAppendValue(t *testing.T) {
	bb := NewBitBuffer()

	// Append value 5 (101 in binary) as 4 bits -> 0101
	bb.AppendValue(5, 4)

	// Pad to byte boundary
	bb.AppendBit(0)
	bb.AppendBit(0)
	bb.AppendBit(0)
	bb.AppendBit(0)

	result := bb.ToBytes()
	if result[0] != 0x50 { // 0101 0000
		t.Errorf("Expected 0x50, got 0x%02X", result[0])
	}
}

func TestBitBufferAppendValueLarge(t *testing.T) {
	bb := NewBitBuffer()

	// Append 16-bit value
	bb.AppendValue(0xABCD, 16)

	result := bb.ToBytes()
	if !bytes.Equal(result, []byte{0xAB, 0xCD}) {
		t.Errorf("Expected [0xAB, 0xCD], got %v", result)
	}
}

func TestBitBufferClear(t *testing.T) {
	bb := NewBitBuffer()
	bb.AppendBit(1)
	bb.AppendBit(1)
	bb.AppendBit(1)
	bb.AppendBit(1)

	bb.Clear()

	if bb.NumBits() != 0 {
		t.Errorf("Expected 0 bits after clear, got %d", bb.NumBits())
	}
	if len(bb.ToBytes()) != 0 {
		t.Error("Expected empty bytes after clear")
	}
}

func TestBitBufferMultipleAppends(t *testing.T) {
	bb := NewBitBuffer()

	// Mix different append methods
	bb.AppendBit(1)                // 1
	bb.AppendBit(0)                // 10
	bb.AppendBits([]byte{0xFF}, 4) // 10 1111
	bb.AppendValue(3, 2)           // 10 1111 11

	if bb.NumBits() != 8 {
		t.Errorf("Expected 8 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	// 1011 1111 = 0xBF
	if result[0] != 0xBF {
		t.Errorf("Expected 0xBF, got 0x%02X", result[0])
	}
}

func TestBitBufferLargeData(t *testing.T) {
	bb := NewBitBuffer()

	// Append 720 bits (90 bytes) - standard POCKET+ packet size
	data := make([]byte, 90)
	for i := range data {
		data[i] = byte(i)
	}
	bb.AppendBits(data, 720)

	if bb.NumBits() != 720 {
		t.Errorf("Expected 720 bits, got %d", bb.NumBits())
	}

	result := bb.ToBytes()
	if !bytes.Equal(result, data) {
		t.Error("Large data round-trip failed")
	}
}
