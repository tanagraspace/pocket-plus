FROM rust:alpine

# Install build dependencies
# - musl-dev: Required for compilation
# - make: For Makefile targets
# - gcc: Required by cargo-tarpaulin
# - bash: Required for shell scripts and generate-test-report.sh
# - openssl-dev, openssl-libs-static, pkgconfig, perl: Required by cargo-tarpaulin (openssl-sys)
RUN apk add --no-cache musl-dev make gcc bash openssl-dev openssl-libs-static pkgconfig perl

# Install Rust components and cargo-tarpaulin for coverage
RUN rustup component add rustfmt clippy && \
    cargo install cargo-tarpaulin

WORKDIR /app/implementations/rust

COPY implementations/rust/ ./
COPY test-vectors/ /app/test-vectors/
COPY scripts/ /app/scripts/

# Build, test, coverage, and docs (includes test-report)
CMD ["sh", "-c", "make clean && make fmt-check && make clippy && make build && make coverage && make docs"]
