FROM eclipse-temurin:21-alpine

# Install build tools
RUN apk add --no-cache \
    maven \
    bash \
    make \
    bc

# Set working directory
WORKDIR /app/implementations/java

# Copy source files (context is repo root)
COPY implementations/java/pom.xml implementations/java/checkstyle.xml implementations/java/Makefile implementations/java/VERSION ./
COPY implementations/java/src ./src
COPY implementations/java/tests ./tests

# Make test scripts executable
RUN chmod +x tests/*.sh

# Build and test
ENTRYPOINT ["sh", "-c", "make clean && make fmt-check && make checkstyle && make build && make test && make coverage && make docs"]
