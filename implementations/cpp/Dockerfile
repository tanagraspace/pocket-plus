FROM alpine:3.20

# Install build tools
RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    lcov \
    doxygen \
    graphviz \
    clang-extra-tools \
    cppcheck \
    valgrind \
    py3-pip \
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
