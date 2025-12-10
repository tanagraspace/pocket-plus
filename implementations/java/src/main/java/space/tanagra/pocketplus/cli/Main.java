/*
 * Copyright (c) 2025 Tanagra Space
 * SPDX-License-Identifier: MIT
 */

package space.tanagra.pocketplus.cli;

import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

import space.tanagra.pocketplus.PocketException;
import space.tanagra.pocketplus.PocketPlus;

/**
 * Command-line interface for POCKET+ compression/decompression.
 *
 * <p>Usage:
 *
 * <ul>
 *   <li>Compress: {@code java -jar pocketplus.jar <input> <packet_size> <pt> <ft> <rt>
 *       <robustness>}
 *   <li>Decompress: {@code java -jar pocketplus.jar -d <input.pkt> <packet_size> <robustness>}
 *   <li>Version: {@code java -jar pocketplus.jar --version}
 * </ul>
 */
public final class Main {

  /** ASCII art banner. */
  private static final String BANNER =
      "\n"
          + "  ____   ___   ____ _  _______ _____     _    \n"
          + " |  _ \\ / _ \\ / ___| |/ / ____|_   _|  _| |_  \n"
          + " | |_) | | | | |   | ' /|  _|   | |   |_   _| \n"
          + " |  __/| |_| | |___| . \\| |___  | |     |_|   \n"
          + " |_|    \\___/ \\____|_|\\_\\_____| |_|           \n"
          + "\n"
          + "         by  T A N A G R A  S P A C E         \n";

  private Main() {
    // Utility class
  }

  /**
   * Main entry point.
   *
   * @param args command-line arguments
   */
  public static void main(String[] args) {
    if (args.length == 0) {
      printUsage();
      System.exit(1);
    }

    try {
      if ("--version".equals(args[0]) || "-v".equals(args[0])) {
        System.out.println("pocketplus " + PocketPlus.version());
        System.exit(0);
      }

      if ("--help".equals(args[0]) || "-h".equals(args[0])) {
        printUsage();
        System.exit(0);
      }

      if ("-d".equals(args[0])) {
        // Decompress mode
        if (args.length < 4) {
          System.err.println("Error: Missing arguments for decompression");
          printUsage();
          System.exit(1);
        }

        String inputPath = args[1];
        int packetSize = Integer.parseInt(args[2]);
        int robustness = Integer.parseInt(args[3]);

        decompress(inputPath, packetSize, robustness);
      } else {
        // Compress mode
        if (args.length < 6) {
          System.err.println("Error: Missing arguments for compression");
          printUsage();
          System.exit(1);
        }

        String inputPath = args[0];
        int packetSize = Integer.parseInt(args[1]);
        int pt = Integer.parseInt(args[2]);
        int ft = Integer.parseInt(args[3]);
        int rt = Integer.parseInt(args[4]);
        int robustness = Integer.parseInt(args[5]);

        compress(inputPath, packetSize, pt, ft, rt, robustness);
      }
    } catch (NumberFormatException e) {
      System.err.println("Error: Invalid number format: " + e.getMessage());
      System.exit(1);
    } catch (PocketException e) {
      System.err.println("Error: " + e.getMessage());
      System.exit(1);
    } catch (IOException e) {
      System.err.println("Error: " + e.getMessage());
      System.exit(1);
    }
  }

  private static void compress(
      String inputPath, int packetSize, int pt, int ft, int rt, int robustness)
      throws IOException, PocketException {
    // Read input file
    byte[] inputData = Files.readAllBytes(Paths.get(inputPath));

    if (inputData.length == 0) {
      throw new PocketException("Input file is empty");
    }

    if (inputData.length % packetSize != 0) {
      throw new PocketException(
          String.format(
              "Input size %d is not a multiple of packet size %d", inputData.length, packetSize));
    }

    int numPackets = inputData.length / packetSize;

    // Compress
    long startTime = System.nanoTime();
    byte[] compressed = PocketPlus.compress(inputData, packetSize, robustness, pt, ft, rt);
    long endTime = System.nanoTime();

    // Write output
    String outputPath = inputPath + ".pkt";
    try (FileOutputStream fos = new FileOutputStream(outputPath)) {
      fos.write(compressed);
    }

    // Print statistics
    double ratio = (double) inputData.length / compressed.length;
    double timeMs = (endTime - startTime) / 1_000_000.0;

    System.out.printf("Compressed %s%n", inputPath);
    System.out.printf("  Packets: %d x %d bytes%n", numPackets, packetSize);
    System.out.printf("  Input:   %d bytes%n", inputData.length);
    System.out.printf("  Output:  %d bytes%n", compressed.length);
    System.out.printf("  Ratio:   %.2fx%n", ratio);
    System.out.printf("  Time:    %.2f ms%n", timeMs);
    System.out.printf("  Output:  %s%n", outputPath);
  }

  private static void decompress(String inputPath, int packetSize, int robustness)
      throws IOException, PocketException {
    // Read input file
    byte[] inputData = Files.readAllBytes(Paths.get(inputPath));

    if (inputData.length == 0) {
      throw new PocketException("Input file is empty");
    }

    // Decompress
    long startTime = System.nanoTime();
    byte[] decompressed = PocketPlus.decompress(inputData, packetSize, robustness);
    long endTime = System.nanoTime();

    // Determine output path
    String outputPath;
    if (inputPath.endsWith(".pkt")) {
      outputPath = inputPath.substring(0, inputPath.length() - 4) + ".depkt";
    } else {
      outputPath = inputPath + ".depkt";
    }

    // Write output
    try (FileOutputStream fos = new FileOutputStream(outputPath)) {
      fos.write(decompressed);
    }

    int numPackets = decompressed.length / packetSize;

    // Print statistics
    double timeMs = (endTime - startTime) / 1_000_000.0;

    System.out.printf("Decompressed %s%n", inputPath);
    System.out.printf("  Packets: %d x %d bytes%n", numPackets, packetSize);
    System.out.printf("  Input:   %d bytes%n", inputData.length);
    System.out.printf("  Output:  %d bytes%n", decompressed.length);
    System.out.printf("  Time:    %.2f ms%n", timeMs);
    System.out.printf("  Output:  %s%n", outputPath);
  }

  private static void printUsage() {
    System.out.println(BANNER);
    System.out.println("CCSDS 124.0-B-1 Lossless Compression (v" + PocketPlus.version() + ")");
    System.out.println("=================================================");
    System.out.println();
    System.out.println("References:");
    System.out.println("  CCSDS 124.0-B-1: https://ccsds.org/Pubs/124x0b1.pdf");
    System.out.println("  ESA POCKET+: https://opssat.esa.int/pocket-plus/");
    System.out.println();
    System.out.println("Citation:");
    System.out.println("  D. Evans, G. Labreche, D. Marszk, S. Bammens, M. Hernandez-Cabronero,");
    System.out.println("  V. Zelenevskiy, V. Shiradhonkar, M. Starcik, and M. Henkel. 2022.");
    System.out.println("  \"Implementing the New CCSDS Housekeeping Data Compression Standard");
    System.out.println("  124.0-B-1 (based on POCKET+) on OPS-SAT-1,\" Proceedings of the");
    System.out.println("  Small Satellite Conference, Communications, SSC22-XII-03.");
    System.out.println("  https://digitalcommons.usu.edu/smallsat/2022/all2022/133/");
    System.out.println();
    System.out.println("Usage:");
    System.out.println(
        "  java -jar pocketplus.jar <input> <packet_size> <pt> <ft> <rt> <robustness>");
    System.out.println("  java -jar pocketplus.jar -d <input.pkt> <packet_size> <robustness>");
    System.out.println();
    System.out.println("Options:");
    System.out.println("  -d             Decompress (default is compress)");
    System.out.println("  -h, --help     Show this help message");
    System.out.println("  -v, --version  Show version information");
    System.out.println();
    System.out.println("Compress arguments:");
    System.out.println("  input          Input file to compress");
    System.out.println("  packet_size    Packet size in bytes (e.g., 90)");
    System.out.println("  pt             New mask period (e.g., 10, 20)");
    System.out.println("  ft             Send mask period (e.g., 20, 50)");
    System.out.println("  rt             Uncompressed period (e.g., 50, 100)");
    System.out.println("  robustness     Robustness level 0-7 (e.g., 1, 2)");
    System.out.println();
    System.out.println("Decompress arguments:");
    System.out.println("  input.pkt      Compressed input file");
    System.out.println("  packet_size    Original packet size in bytes");
    System.out.println("  robustness     Robustness level (must match compression)");
    System.out.println();
    System.out.println("Output:");
    System.out.println("  Compress:   <input>.pkt");
    System.out.println("  Decompress: <input>.depkt (or <base>.depkt if input ends in .pkt)");
    System.out.println();
    System.out.println("Examples:");
    System.out.println("  java -jar pocketplus.jar data.bin 90 10 20 50 1        # compress");
    System.out.println("  java -jar pocketplus.jar -d data.bin.pkt 90 1          # decompress");
    System.out.println();
  }
}
