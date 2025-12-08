package pocketplus

// BitBuffer is a variable-length bit buffer for building compressed output.
//
// Bits are appended sequentially using MSB-first ordering as required by
// CCSDS 124.0-B-1.
//
// Bit Ordering:
//   - First bit appended goes to bit position 7
//   - Second bit goes to position 6, etc.
type BitBuffer struct {
	data    []byte
	numBits int
}

// NewBitBuffer creates a new empty bit buffer.
func NewBitBuffer() *BitBuffer {
	return &BitBuffer{
		data:    make([]byte, 0, 256), // Pre-allocate reasonable capacity
		numBits: 0,
	}
}

// Clear resets the buffer to empty.
func (bb *BitBuffer) Clear() {
	bb.data = bb.data[:0]
	bb.numBits = 0
}

// NumBits returns the number of bits in the buffer.
func (bb *BitBuffer) NumBits() int {
	return bb.numBits
}

// AppendBit appends a single bit to the buffer.
func (bb *BitBuffer) AppendBit(bit int) {
	byteIndex := bb.numBits / 8
	bitIndex := bb.numBits % 8

	// Extend buffer if needed
	for byteIndex >= len(bb.data) {
		bb.data = append(bb.data, 0)
	}

	// CCSDS uses MSB-first bit ordering: first bit goes to position 7
	if bit != 0 {
		bb.data[byteIndex] |= 1 << (7 - bitIndex)
	}

	bb.numBits++
}

// AppendBits appends multiple bits from bytes.
// Bits are read MSB-first from the source data.
func (bb *BitBuffer) AppendBits(data []byte, numBits int) {
	for i := 0; i < numBits; i++ {
		byteIndex := i / 8
		bitIndex := i % 8

		// Extract bits MSB-first (bit 7, 6, 5, ..., 0)
		bit := (data[byteIndex] >> (7 - bitIndex)) & 1
		bb.AppendBit(int(bit))
	}
}

// AppendBitVector appends all bits from a BitVector.
func (bb *BitBuffer) AppendBitVector(bv *BitVector) {
	// Calculate number of bytes from bit length
	numBytes := (bv.length + 7) / 8

	// CCSDS MSB-first: bytes in order, bits within each byte from MSB to LSB
	for byteIdx := 0; byteIdx < numBytes; byteIdx++ {
		bitsInThisByte := 8

		// Last byte may have fewer than 8 bits
		if byteIdx == numBytes-1 {
			remainder := bv.length % 8
			if remainder != 0 {
				bitsInThisByte = remainder
			}
		}

		// With MSB-first bitvector indexing: bit 0 is MSB, bit 7 is LSB
		// We want to append bits in order: MSB first, LSB last
		startBit := byteIdx * 8
		for bitOffset := 0; bitOffset < bitsInThisByte; bitOffset++ {
			pos := startBit + bitOffset
			bit := bv.GetBit(pos)
			bb.AppendBit(bit)
		}
	}
}

// AppendBitVectorN appends the first n bits from a BitVector.
func (bb *BitBuffer) AppendBitVectorN(bv *BitVector, n int) {
	if n > bv.length {
		n = bv.length
	}
	for i := 0; i < n; i++ {
		bb.AppendBit(bv.GetBit(i))
	}
}

// AppendValue appends a value as count bits (MSB-first).
func (bb *BitBuffer) AppendValue(value uint64, count int) {
	for i := count - 1; i >= 0; i-- {
		bit := int((value >> i) & 1)
		bb.AppendBit(bit)
	}
}

// ToBytes converts buffer contents to bytes.
func (bb *BitBuffer) ToBytes() []byte {
	if bb.numBits == 0 {
		return []byte{}
	}

	// Calculate number of bytes needed
	numBytes := (bb.numBits + 7) / 8
	result := make([]byte, numBytes)
	copy(result, bb.data[:numBytes])
	return result
}
