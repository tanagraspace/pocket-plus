
# Pocket+

## Getting Started

The example `1028packets.ccsds` file includes a week’s worth of ADCS packets (ID 1028) from Venus Express.
The packets have a length of 90 bytes each. The executables can be compiled using the `Makefile`:

```bash
$ make all
```

After compiling you can execute the compressor, so for example to compress the example file from the command line:

```bash
$ ./pocket_compress 1028packets.ccsds 90 20 50 100 2
```

where,
* `1028packets.ccsds` is the filename to compress,
* `90` is the packet lenght in bytes,
* `20` is how often flag `pt` will be set,
* `50` is how often flag `ft` with be set,
* `100` is how often flag `rt` with be set,
* `2` is the minimum robustness level (required).

This creates a `1028packets.ccsds.pkt` compressed file, the operation can be reversed by executing
the decompressor, for example from a command line run:

```bash
$ ./pocket_decompress 1028packets.ccsds.pkt
```

This creates a `1028packets.ccsds.pkt.depkt` decompressed file, that can be compared to the original
file to verify they have the same content, for example by calculating the md5sum:

```bash
$ md5sum 1028packets.ccsds 1028packets.ccsds.pkt.depkt
ac0ce25d660efa5ee18f92b71aa9a85d  1028packets.ccsds
ac0ce25d660efa5ee18f92b71aa9a85d  1028packets.ccsds.pkt.depkt

```

