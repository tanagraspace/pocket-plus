# POCKET+

The definitive implementation of the POCKET+ lossless compression algorithm of fixed-length housekeeping data ([CCSDS 124.0-B-1](https://ccsds.org/Pubs/124x0b1.pdf)).

## Citation

If POCKET+ contributes to your research, please cite:

> D. Evans, G. Labrèche, D. Marszk, S. Bammens, M. Hernandez-Cabronero, V. Zelenevskiy, V. Shiradhonkar, M. Starcik, and M. Henkel. 2022. "Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1," *Proceedings of the Small Satellite Conference*, Communications, SSC22-XII-03. https://digitalcommons.usu.edu/smallsat/2022/all2022/133/

<details>
<summary>BibTeX</summary>

```bibtex
@inproceedings{evans2022pocketplus,
  author    = {Evans, David and Labrèche, Georges and Marszk, Dominik and Bammens, Samuel and Hernandez-Cabronero, Miguel and Zelenevskiy, Vladimir and Shiradhonkar, Vasundhara and Starcik, Mario and Henkel, Maximilian},
  title     = {Implementing the New CCSDS Housekeeping Data Compression Standard 124.0-B-1 (based on POCKET+) on OPS-SAT-1},
  booktitle = {Proceedings of the Small Satellite Conference},
  year      = {2022},
  note      = {SSC22-XII-03},
  url       = {https://digitalcommons.usu.edu/smallsat/2022/all2022/133/}
}
```

</details>

## About

POCKET+ is an European Space Agency (ESA) patented lossless compression algorithm implemented using very low-level instructions such as OR, XOR, AND, etc. It has been designed to run on spacecraft command and control processors with low CPU power available and tight real-time constraints. An earlier version of POCKET+ was flight-proven onboard both the Nanomind 3200 flight computer and SEPP payload computer of ESA's OPS-SAT-1 spacecraft.

The algorithm has been standardized by CCSDS as **CCSDS 124.0-B-1** for compressing fixed-length spacecraft housekeeping packets.

## Implementations

| Language | Version | Status | Location |
|----------|---------|--------|----------|
| C | 1.0.0 | Complete | [`implementations/c/`](implementations/c/) |
| Python | 1.0.0 | Complete | [`implementations/python/`](implementations/python/) |
| Go | 0.1.0 | Planned | [`implementations/go/`](implementations/go/) |

## Repository Structure

```
pocket-plus/
├── implementations/
│   ├── c/              # C implementation
│   ├── python/         # Python implementation
│   └── go/             # Go implementation
├── docs/               # Shared documentation
├── test-vectors/       # Shared test data for validation
├── LICENSE
└── README.md
```

## Quick Start

### Docker (recommended)

Build, test, and generate coverage for any implementation:

```bash
docker-compose run --rm c          # C implementation
docker-compose run --rm python     # Python implementation
# docker-compose run --rm go       # Go (planned)
```

Artifacts are written to `implementations/<lang>/build/`.

### Local Build

#### C

```bash
cd implementations/c
make          # Build library and CLI
make test     # Run tests
make coverage # Run tests with coverage report
```

#### Python

```bash
cd implementations/python
pip install -e ".[dev]"
pytest
```

#### Go

```bash
cd implementations/go
go test ./...
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for versioning strategy, release process, and contribution guidelines.

## License

See [LICENSE](LICENSE) for details.
