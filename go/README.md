# POCKET+ Go Implementation

Go implementation of the POCKET+ lossless compression algorithm (CCSDS 124.0-B-1).

## Version

Current version: **0.1.0**

## Installation

```bash
go get github.com/tanagraspace/pocket-plus/go/pocketplus
```

## Testing

```bash
cd go
go test ./...
```

With coverage:
```bash
go test -cover ./...
```

## Usage

```go
package main

import (
    "github.com/tanagraspace/pocket-plus/go/pocketplus"
)

func main() {
    // Compress data
    compressed, err := pocketplus.Compress(inputData)
    if err != nil {
        panic(err)
    }

    // Decompress data
    decompressed, err := pocketplus.Decompress(compressed)
    if err != nil {
        panic(err)
    }
}
```

## Features

- [ ] Core compression algorithm
- [ ] Core decompression algorithm
- [ ] Packet loss resilience
- [ ] Command-line interface
- [ ] Benchmarks
- [ ] Example programs

## Development

Format code:
```bash
go fmt ./...
```

Run linter:
```bash
golangci-lint run
```

## License

See [LICENSE](../LICENSE) in the root directory.
