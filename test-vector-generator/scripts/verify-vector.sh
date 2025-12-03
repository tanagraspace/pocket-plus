#!/bin/bash
# Compress, decompress, and verify a single test vector
# Usage: verify-vector.sh <name> <config.yaml>

set -e

if [ $# -ne 2 ]; then
    echo "Usage: $0 <name> <config.yaml>"
    exit 1
fi

NAME=$1
CONFIG=$2

# Read configuration using Python
read_config() {
    python3 -c "import yaml; config = yaml.safe_load(open('${CONFIG}')); print(config$1)"
}

INPUT_TYPE=$(read_config "['input']['type']")
PACKET_LENGTH=$(read_config "['compression']['packet_length']")
PT=$(read_config "['compression']['pt']")
FT=$(read_config "['compression']['ft']")
RT=$(read_config "['compression']['rt']")
ROBUSTNESS=$(read_config "['compression']['robustness']")
OUTPUT_DIR=$(read_config "['output']['dir']")

# Determine input file
if [ "${INPUT_TYPE}" = "real_data" ]; then
    INPUT_FILE=$(read_config "['input']['source']")
else
    INPUT_FILE="${OUTPUT_DIR}/$(read_config "['output']['input_file']")"
fi

COMPRESSED_FILE="${OUTPUT_DIR}/$(read_config "['output']['compressed_file']")"
DECOMPRESSED_FILE="${OUTPUT_DIR}/$(read_config "['output']['decompressed_file']")"
METADATA_FILE="${OUTPUT_DIR}/metadata.json"

echo "Configuration:"
echo "  Input:       ${INPUT_FILE}"
echo "  Packet len:  ${PACKET_LENGTH} bytes"
echo "  Parameters:  pt=${PT} ft=${FT} rt=${RT} robustness=${ROBUSTNESS}"
echo ""

# Check input file exists
if [ ! -f "${INPUT_FILE}" ]; then
    echo "ERROR: Input file not found: ${INPUT_FILE}"
    exit 1
fi

# Calculate input statistics
INPUT_SIZE=$(stat -c%s "${INPUT_FILE}")
INPUT_MD5=$(md5sum "${INPUT_FILE}" | awk '{print $1}')

echo "Compressing..."
pocket_compress "${INPUT_FILE}" ${PACKET_LENGTH} ${PT} ${FT} ${RT} ${ROBUSTNESS}

# Check compressed file was created
if [ ! -f "${INPUT_FILE}.pkt" ]; then
    echo "ERROR: Compression failed - no output file"
    exit 1
fi

# Move to expected location if different
if [ "${INPUT_FILE}.pkt" != "${COMPRESSED_FILE}" ]; then
    mv "${INPUT_FILE}.pkt" "${COMPRESSED_FILE}"
fi

COMPRESSED_SIZE=$(stat -c%s "${COMPRESSED_FILE}")
COMPRESSED_MD5=$(md5sum "${COMPRESSED_FILE}" | awk '{print $1}')
COMPRESSION_RATIO=$(echo "scale=2; ${INPUT_SIZE} / ${COMPRESSED_SIZE}" | bc)

echo "✓ Compressed: ${INPUT_SIZE} → ${COMPRESSED_SIZE} bytes (${COMPRESSION_RATIO}x)"
echo ""

echo "Decompressing..."
pocket_decompress "${COMPRESSED_FILE}"

# Check decompressed file was created
if [ ! -f "${COMPRESSED_FILE}.depkt" ]; then
    echo "ERROR: Decompression failed - no output file"
    exit 1
fi

# Move to expected location if different
if [ "${COMPRESSED_FILE}.depkt" != "${DECOMPRESSED_FILE}" ]; then
    mv "${COMPRESSED_FILE}.depkt" "${DECOMPRESSED_FILE}"
fi

DECOMPRESSED_MD5=$(md5sum "${DECOMPRESSED_FILE}" | awk '{print $1}')

echo "✓ Decompressed"
echo ""

# Verify round-trip
echo "Verifying round-trip..."
if [ "${INPUT_MD5}" = "${DECOMPRESSED_MD5}" ]; then
    echo "✓ SUCCESS: MD5 match!"
    echo "  Original:     ${INPUT_MD5}"
    echo "  Decompressed: ${DECOMPRESSED_MD5}"
else
    echo "✗ FAILED: MD5 mismatch!"
    echo "  Original:     ${INPUT_MD5}"
    echo "  Decompressed: ${DECOMPRESSED_MD5}"
    exit 1
fi
echo ""

# Generate metadata
echo "Generating metadata..."
cat > "${METADATA_FILE}" <<EOF
{
  "name": "${NAME}",
  "generated_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "input": {
    "file": "$(basename ${INPUT_FILE})",
    "size": ${INPUT_SIZE},
    "md5": "${INPUT_MD5}",
    "type": "${INPUT_TYPE}"
  },
  "compression": {
    "packet_length": ${PACKET_LENGTH},
    "parameters": {
      "pt": ${PT},
      "ft": ${FT},
      "rt": ${RT},
      "robustness": ${ROBUSTNESS}
    }
  },
  "output": {
    "compressed": {
      "file": "$(basename ${COMPRESSED_FILE})",
      "size": ${COMPRESSED_SIZE},
      "md5": "${COMPRESSED_MD5}"
    },
    "decompressed": {
      "file": "$(basename ${DECOMPRESSED_FILE})",
      "size": $(stat -c%s "${DECOMPRESSED_FILE}"),
      "md5": "${DECOMPRESSED_MD5}"
    }
  },
  "results": {
    "compression_ratio": ${COMPRESSION_RATIO},
    "roundtrip_verified": true
  }
}
EOF

echo "✓ Metadata saved: ${METADATA_FILE}"
echo ""
