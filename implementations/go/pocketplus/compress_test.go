package pocketplus

import "testing"

func TestVersion(t *testing.T) {
	if Version == "" {
		t.Error("Version should not be empty")
	}
}

func TestCompressEmptyInput(t *testing.T) {
	result, err := Compress([]byte{}, 90, 1, 10, 20, 50)
	if err != nil {
		t.Errorf("Compress with empty input should not error: %v", err)
	}
	if len(result) != 0 {
		t.Errorf("Expected empty result, got %d bytes", len(result))
	}
}

func TestCompressInvalidPacketSize(t *testing.T) {
	_, err := Compress([]byte{1, 2, 3}, 0, 1, 10, 20, 50)
	if err == nil {
		t.Error("Expected error for zero packet size")
	}
}

func TestCompressInvalidDataLength(t *testing.T) {
	_, err := Compress([]byte{1, 2, 3}, 2, 1, 10, 20, 50)
	if err == nil {
		t.Error("Expected error for data length not multiple of packet size")
	}
}

func TestCompressInvalidRobustness(t *testing.T) {
	data := make([]byte, 90)
	_, err := Compress(data, 90, 0, 10, 20, 50)
	if err == nil {
		t.Error("Expected error for robustness < 1")
	}

	_, err = Compress(data, 90, 8, 10, 20, 50)
	if err == nil {
		t.Error("Expected error for robustness > 7")
	}
}

func TestDecompressEmptyInput(t *testing.T) {
	result, err := Decompress([]byte{}, 90, 1)
	if err != nil {
		t.Errorf("Decompress with empty input should not error: %v", err)
	}
	if len(result) != 0 {
		t.Errorf("Expected empty result, got %d bytes", len(result))
	}
}

func TestDecompressInvalidPacketSize(t *testing.T) {
	_, err := Decompress([]byte{1, 2, 3}, 0, 1)
	if err == nil {
		t.Error("Expected error for zero packet size")
	}
}

func TestDecompressInvalidRobustness(t *testing.T) {
	_, err := Decompress([]byte{1, 2, 3}, 90, 0)
	if err == nil {
		t.Error("Expected error for robustness < 1")
	}

	_, err = Decompress([]byte{1, 2, 3}, 90, 8)
	if err == nil {
		t.Error("Expected error for robustness > 7")
	}
}
