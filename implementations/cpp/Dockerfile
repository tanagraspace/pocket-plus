FROM ubuntu:24.04

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    lcov \
    doxygen \
    graphviz \
    clang-format \
    clang-tidy \
    cppcheck \
    valgrind \
    python3-pip \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --break-system-packages junit2html

# Set working directory
WORKDIR /app/implementations/cpp

# Copy source files
COPY implementations/cpp/ .

# Copy test vectors for reference tests
COPY test-vectors/ /app/test-vectors/

# Set test vectors path for Docker environment
ENV TEST_VECTORS_DIR=/app/test-vectors

# Configure git for volume mounts and build/test (including CLI tests, valgrind, coverage, and docs)
CMD ["sh", "-c", "git config --global --add safe.directory '*' && rm -rf build/* 2>/dev/null; make build && make test-report && make test-cli && make valgrind && make coverage-html && make docs"]
