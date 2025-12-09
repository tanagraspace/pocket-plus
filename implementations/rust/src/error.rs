//! Error types for POCKET+ compression/decompression.

use std::fmt;

/// Errors that can occur during POCKET+ compression or decompression.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PocketError {
    /// Invalid packet size (must be > 0 and divisible by 8)
    InvalidPacketSize(usize),

    /// Invalid robustness parameter (must be 0-7)
    InvalidRobustness(usize),

    /// Input data length doesn't match expected packet count
    InvalidInputLength { expected: usize, actual: usize },

    /// Unexpected end of input during decompression
    UnexpectedEndOfInput,

    /// Invalid compressed data format
    InvalidFormat(String),

    /// Buffer overflow during compression
    BufferOverflow,

    /// Not enough bits remaining in input (underflow)
    Underflow,

    /// Invalid length parameter
    InvalidLength,
}

impl fmt::Display for PocketError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InvalidPacketSize(size) => {
                write!(
                    f,
                    "invalid packet size: {size} (must be > 0 and divisible by 8)"
                )
            }
            Self::InvalidRobustness(r) => {
                write!(f, "invalid robustness: {r} (must be 0-7)")
            }
            Self::InvalidInputLength { expected, actual } => {
                write!(f, "invalid input length: expected {expected}, got {actual}")
            }
            Self::UnexpectedEndOfInput => {
                write!(f, "unexpected end of input")
            }
            Self::InvalidFormat(msg) => {
                write!(f, "invalid format: {msg}")
            }
            Self::BufferOverflow => {
                write!(f, "buffer overflow")
            }
            Self::Underflow => {
                write!(f, "not enough bits remaining in input")
            }
            Self::InvalidLength => {
                write!(f, "invalid length parameter")
            }
        }
    }
}

impl std::error::Error for PocketError {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_error_display() {
        let err = PocketError::InvalidPacketSize(0);
        assert!(err.to_string().contains("invalid packet size"));

        let err = PocketError::InvalidRobustness(10);
        assert!(err.to_string().contains("invalid robustness"));

        let err = PocketError::InvalidInputLength {
            expected: 100,
            actual: 50,
        };
        assert!(err.to_string().contains("expected 100"));

        let err = PocketError::UnexpectedEndOfInput;
        assert!(err.to_string().contains("unexpected end"));

        let err = PocketError::InvalidFormat("test".to_string());
        assert!(err.to_string().contains("invalid format"));

        let err = PocketError::BufferOverflow;
        assert!(err.to_string().contains("buffer overflow"));

        let err = PocketError::Underflow;
        assert!(err.to_string().contains("not enough bits"));

        let err = PocketError::InvalidLength;
        assert!(err.to_string().contains("invalid length"));
    }
}
