FROM rust:alpine

# Install build dependencies
# - musl-dev: Required for compilation
# - make: For Makefile targets
# - gcc: Required by cargo-tarpaulin
# - bash: Required for shell scripts
# - python3, py3-pip: Required for junit2html
# - openssl-dev, openssl-libs-static, pkgconfig, perl: Required by cargo-tarpaulin (openssl-sys)
RUN apk add --no-cache musl-dev make gcc bash python3 py3-pip openssl-dev openssl-libs-static pkgconfig perl

# Install Rust components, cargo-tarpaulin for coverage, and cargo-nextest for test reports
RUN rustup component add rustfmt clippy && \
    cargo install cargo-tarpaulin cargo-nextest && \
    pip install --break-system-packages junit2html

WORKDIR /app/implementations/rust

COPY implementations/rust/ ./
COPY test-vectors/ /app/test-vectors/

# Build, test, coverage, and docs (includes test-report)
CMD ["sh", "-c", "make clean && make fmt-check && make clippy && make build && make coverage && make docs"]
