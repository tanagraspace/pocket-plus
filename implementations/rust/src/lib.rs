//! # POCKET+ Compression Library
//!
//! Rust implementation of the CCSDS 124.0-B-1 POCKET+ lossless compression
//! algorithm for fixed-length housekeeping data.
//!
//! ## Features
//!
//! - Zero external dependencies (standard library only)
//! - Designed for ground systems (64-bit)
//! - Byte-identical output to C reference implementation
//!
//! ## Usage
//!
//! ```rust,ignore
//! use pocketplus::{compress, decompress};
//!
//! // Sample housekeeping data (90 bytes = 720 bits per packet)
//! let data: Vec<u8> = vec![0u8; 90];
//!
//! // Compress data
//! let compressed = compress(
//!     &data,
//!     720,    // packet_size in bits
//!     1,      // robustness (0-7)
//!     10,     // pt_limit
//!     20,     // ft_limit
//!     50,     // rt_limit
//! ).unwrap();
//!
//! // Decompress data
//! let decompressed = decompress(&compressed, 720, 1).unwrap();
//!
//! assert_eq!(data, decompressed);
//! ```
//!
//! ## References
//!
//! - [CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf)

#![forbid(unsafe_code)]
#![warn(clippy::pedantic)]
#![allow(clippy::must_use_candidate)]
#![allow(clippy::missing_errors_doc)]
#![allow(clippy::missing_panics_doc)]

mod compress;
mod decompress;
mod error;

pub use compress::compress;
pub use decompress::decompress;
pub use error::PocketError;

#[cfg(test)]
mod tests {
    #[test]
    fn test_library_loads() {
        // Basic smoke test - ensure library compiles and loads
        assert!(true);
    }
}
