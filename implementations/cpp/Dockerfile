FROM alpine:3.20

# Install build tools
RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    lcov \
    gzip \
    doxygen \
    graphviz \
    clang-extra-tools \
    cppcheck \
    valgrind \
    bash

# Set working directory
WORKDIR /app/implementations/cpp

# Copy source files
COPY implementations/cpp/ .

# Copy test vectors for reference tests
COPY test-vectors/ /app/test-vectors/

# Copy shared scripts
COPY scripts/ /app/scripts/

# Set test vectors path for Docker environment
ENV TEST_VECTORS_DIR=/app/test-vectors

# Configure git for volume mounts and build/test (including lint, CLI tests, valgrind, coverage, and docs)
CMD ["sh", "-c", "git config --global --add safe.directory '*' && rm -rf build/* 2>/dev/null; make build && make lint && make test-report && make test-cli && make valgrind && make coverage-html && make docs"]
