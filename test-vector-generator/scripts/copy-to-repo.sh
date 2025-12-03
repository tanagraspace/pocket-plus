#!/bin/bash
# Copy generated test vectors to the repository test-vectors directory
# This should be run after successful generation

set -e

WORKSPACE_DIR="/workspace"
OUTPUT_DIR="${WORKSPACE_DIR}/output/vectors"
REPO_TEST_VECTORS="/repo/test-vectors"

echo "=========================================="
echo "Copying Test Vectors to Repository"
echo "=========================================="
echo ""

if [ ! -d "${REPO_TEST_VECTORS}" ]; then
    echo "ERROR: Repository test-vectors directory not found: ${REPO_TEST_VECTORS}"
    echo "Make sure to mount the repo directory when running docker-compose"
    exit 1
fi

copy_vector() {
    local name=$1
    local src="${OUTPUT_DIR}/${name}"
    local dst="${REPO_TEST_VECTORS}"

    if [ ! -d "${src}" ]; then
        echo "WARNING: ${name} not found, skipping"
        return
    fi

    echo "Copying ${name}..."

    # Copy input file (if synthetic)
    if [ -f "${src}"/*.bin ]; then
        cp "${src}"/*.bin "${dst}/input/"
        echo "  ✓ Input: ${dst}/input/$(basename ${src}/*.bin)"
    fi

    # Copy compressed file
    if [ -f "${src}"/*.pkt ]; then
        cp "${src}"/*.pkt "${dst}/expected-output/"
        echo "  ✓ Compressed: ${dst}/expected-output/$(basename ${src}/*.pkt)"
    fi

    # Copy metadata
    if [ -f "${src}/metadata.json" ]; then
        cp "${src}/metadata.json" "${dst}/expected-output/${name}-metadata.json"
        echo "  ✓ Metadata: ${dst}/expected-output/${name}-metadata.json"
    fi

    echo ""
}

# Copy all test vectors
copy_vector "simple"
copy_vector "housekeeping"
copy_vector "edge-cases"
copy_vector "venus-express"

echo "=========================================="
echo "Test vectors copied successfully!"
echo "=========================================="
echo ""
echo "Repository test-vectors directory: ${REPO_TEST_VECTORS}"
echo ""
echo "Next steps:"
echo "  1. Review the copied files in test-vectors/"
echo "  2. Update test-vectors/README.md with actual sizes and checksums"
echo "  3. Commit the test vectors to the repository"
echo ""
