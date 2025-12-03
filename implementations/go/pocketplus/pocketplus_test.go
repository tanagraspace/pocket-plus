package pocketplus

import (
	"testing"
)

func TestCompress(t *testing.T) {
	data := []byte("test data")
	_, err := Compress(data)

	if err != ErrNotImplemented {
		t.Errorf("expected ErrNotImplemented, got %v", err)
	}
}

func TestDecompress(t *testing.T) {
	data := []byte("compressed data")
	_, err := Decompress(data)

	if err != ErrNotImplemented {
		t.Errorf("expected ErrNotImplemented, got %v", err)
	}
}

func TestVersion(t *testing.T) {
	if Version != "0.1.0" {
		t.Errorf("expected version 0.1.0, got %s", Version)
	}
}
