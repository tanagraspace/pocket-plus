FROM alpine:edge

RUN apk add --no-cache gcc musl-dev make lcov bc gzip doxygen curl ca-certificates cppcheck --repository=http://dl-cdn.alpinelinux.org/alpine/edge/testing

WORKDIR /app/implementations/c

COPY implementations/c/ ./
COPY test-vectors/ /app/test-vectors/

# Copy root README for Doxygen main page (creates ../../README.md relative to WORKDIR)
COPY README.md /app/README.md

# Download Doxygen Awesome CSS theme (allow failure for offline builds)
RUN mkdir -p doxygen-awesome-css && \
    (curl -sL --retry 3 --connect-timeout 10 \
        https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.3.4/doxygen-awesome.css \
        -o doxygen-awesome-css/doxygen-awesome.css && \
     curl -sL --retry 3 --connect-timeout 10 \
        https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.3.4/doxygen-awesome-sidebar-only.css \
        -o doxygen-awesome-css/doxygen-awesome-sidebar-only.css) || \
    echo "Warning: Could not download Doxygen theme, using default"

CMD ["sh", "-c", "make clean && make && make test-cli && make coverage && make coverage-html && make docs"]
