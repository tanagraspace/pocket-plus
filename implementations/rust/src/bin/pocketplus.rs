//! POCKET+ Command Line Interface
//!
//! Usage:
//!   pocketplus compress -i input.bin -o output.bin -s 90 -r 2 -p 20 -f 50 -t 100
//!   pocketplus decompress -i input.bin -o output.bin -s 90 -r 2
//!   pocketplus --version
//!   pocketplus --help

use std::env;
use std::process;

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn print_usage() {
    eprintln!("POCKET+ Compression Tool v{VERSION}");
    eprintln!();
    eprintln!("CCSDS 124.0-B-1 POCKET+ lossless compression algorithm");
    eprintln!();
    eprintln!("Usage:");
    eprintln!("  pocketplus compress -i <input> -o <output> -s <size> -r <robustness> -p <pt> -f <ft> -t <rt>");
    eprintln!("  pocketplus decompress -i <input> -o <output> -s <size> -r <robustness>");
    eprintln!("  pocketplus --version");
    eprintln!("  pocketplus --help");
    eprintln!();
    eprintln!("Options:");
    eprintln!("  -i, --input <file>    Input file path");
    eprintln!("  -o, --output <file>   Output file path");
    eprintln!("  -s, --size <bits>     Packet size in bits (must be divisible by 8)");
    eprintln!("  -r, --robustness <R>  Robustness parameter (0-7)");
    eprintln!("  -p, --pt <limit>      Packet threshold limit (compression only)");
    eprintln!("  -f, --ft <limit>      Frame threshold limit (compression only)");
    eprintln!("  -t, --rt <limit>      Reset threshold limit (compression only)");
    eprintln!();
    eprintln!("Examples:");
    eprintln!("  pocketplus compress -i data.bin -o data.pkt -s 720 -r 2 -p 20 -f 50 -t 100");
    eprintln!("  pocketplus decompress -i data.pkt -o data.bin -s 720 -r 2");
}

fn print_version() {
    println!("pocketplus {VERSION}");
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        print_usage();
        process::exit(1);
    }

    match args[1].as_str() {
        "--version" | "-V" => {
            print_version();
        }
        "--help" | "-h" => {
            print_usage();
        }
        "compress" | "decompress" => {
            // TODO: Implement argument parsing and compression/decompression
            eprintln!("Error: Not yet implemented");
            process::exit(1);
        }
        _ => {
            eprintln!("Error: Unknown command '{}'", args[1]);
            eprintln!();
            print_usage();
            process::exit(1);
        }
    }
}
