# POCKET+ Rust Implementation Makefile
# Wrapper around cargo commands for consistency with other implementations

.PHONY: all build test coverage clean fmt fmt-check clippy audit deny docs

all: build

build:
	cargo build --release

test:
	cargo test --release

coverage:
	mkdir -p build/coverage
	cargo tarpaulin --out Html --output-dir build/coverage

fmt:
	cargo fmt

fmt-check:
	cargo fmt --check

clippy:
	cargo clippy -- -D warnings -W clippy::pedantic

audit:
	cargo audit

deny:
	cargo deny check

docs:
	mkdir -p build/docs
	cargo doc --no-deps --document-private-items
	cp -r target/doc build/docs/api

clean:
	cargo clean
	rm -rf build/

# Run all checks (for local development)
check: fmt-check clippy test

# Run all security checks
security: audit deny
