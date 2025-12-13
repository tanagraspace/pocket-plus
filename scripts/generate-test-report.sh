#!/bin/bash
# Generate HTML test report from test output
# Usage: ./generate-test-report.sh <input-file> <output-file> <title>
#
# Supports two test output formats:
# 1. C format: Suite headers with === separator, test_* lines with checkmarks
# 2. C++ Catch2 format: "All tests passed" summary with assertion counts

set -e

INPUT_FILE="$1"
OUTPUT_FILE="$2"
TITLE="${3:-Test Report}"

if [ -z "$INPUT_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "Usage: $0 <input-file> <output-file> [title]"
    exit 1
fi

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file not found: $INPUT_FILE"
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT_FILE")"

# Detect format based on content
if grep -q "^=\+$" "$INPUT_FILE"; then
    FORMAT="c"
elif grep -q "test cases\|assertions" "$INPUT_FILE"; then
    FORMAT="catch2"
else
    FORMAT="unknown"
fi

# Start HTML
cat > "$OUTPUT_FILE" << 'HTMLHEADER'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TITLE_PLACEHOLDER</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 900px; margin: 0 auto; padding: 2rem; line-height: 1.6; background: #f8f9fa; }
        h1 { color: #333; border-bottom: 2px solid #28a745; padding-bottom: 0.5rem; }
        h2 { color: #555; font-size: 1.2rem; margin-top: 1.5rem; }
        .summary { background: #28a745; color: white; padding: 1rem; border-radius: 8px; margin: 1rem 0; }
        .summary.fail { background: #dc3545; }
        .suite { background: white; padding: 1rem; border-radius: 8px; margin: 1rem 0; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
        .suite h2 { margin-top: 0; border-bottom: 1px solid #eee; padding-bottom: 0.5rem; }
        .test { padding: 0.25rem 0; font-family: monospace; font-size: 0.9rem; }
        .test.pass::before { content: "✓ "; color: #28a745; }
        .test.fail::before { content: "✗ "; color: #dc3545; }
        .stats { color: #666; font-size: 0.9rem; margin-top: 0.5rem; padding-top: 0.5rem; border-top: 1px solid #eee; }
        pre { background: #1e1e1e; color: #d4d4d4; padding: 1rem; border-radius: 4px; overflow-x: auto; font-size: 0.85rem; }
        .timestamp { color: #666; font-size: 0.8rem; margin-top: 2rem; }
    </style>
</head>
<body>
    <h1>TITLE_PLACEHOLDER</h1>
HTMLHEADER

# Replace title placeholder
sed -i.bak "s/TITLE_PLACEHOLDER/$TITLE/g" "$OUTPUT_FILE" && rm -f "$OUTPUT_FILE.bak"

if [ "$FORMAT" = "c" ]; then
    # Parse C-style test output
    total_passed=0
    total_tests=0
    current_suite=""
    prev_line=""
    pending_test=""
    
    while IFS= read -r line; do
        # Check if current line is === separator (indicates prev_line was a suite header)
        if [[ "$line" =~ ^=+$ ]] && [[ "$prev_line" != "" ]]; then
            if [[ "$current_suite" != "" ]]; then
                echo '</div>' >> "$OUTPUT_FILE"
            fi
            suite_name=$(echo "$prev_line" | sed 's/ *Tests$//' | sed 's/ *$//')
            current_suite="$suite_name"
            echo "<div class=\"suite\"><h2>$suite_name</h2>" >> "$OUTPUT_FILE"
        fi
        
        # Check for test result (handles both inline checkmarks and test_vector format)
        if [[ "$line" =~ "test_" ]]; then
            test_name=$(echo "$line" | sed 's/^[[:space:]]*//' | sed 's/\.\.\..*//')
            if [[ "$line" =~ "✓" ]]; then
                echo "<div class=\"test pass\">$test_name</div>" >> "$OUTPUT_FILE"
            elif [[ "$line" =~ "✗" ]]; then
                echo "<div class=\"test fail\">$test_name</div>" >> "$OUTPUT_FILE"
            else
                # Store pending test name for test_vector format (checkmark on next line)
                pending_test="$test_name"
            fi
        fi
        
        # Check for verified checkmark on separate line (test_vector format)
        if [[ "$line" =~ "verified" ]] && [[ "$line" =~ "✓" ]] && [[ "$pending_test" != "" ]]; then
            echo "<div class=\"test pass\">$pending_test</div>" >> "$OUTPUT_FILE"
            pending_test=""
        fi
        
        # Check for summary line
        if [[ "$line" =~ ([0-9]+)/([0-9]+)" tests passed" ]]; then
            passed="${BASH_REMATCH[1]}"
            tests="${BASH_REMATCH[2]}"
            total_passed=$((total_passed + passed))
            total_tests=$((total_tests + tests))
            echo "<div class=\"stats\">$passed/$tests tests passed</div>" >> "$OUTPUT_FILE"
        fi
        
        prev_line="$line"
    done < "$INPUT_FILE"
    
    # Close last suite
    if [[ "$current_suite" != "" ]]; then
        echo '</div>' >> "$OUTPUT_FILE"
    fi
    
    # Add summary
    if [[ $total_tests -gt 0 ]]; then
        if [[ $total_passed -eq $total_tests ]]; then
            echo "<div class=\"summary\"><strong>Total: $total_passed/$total_tests tests passed</strong></div>" >> "$OUTPUT_FILE"
        else
            echo "<div class=\"summary fail\"><strong>Total: $total_passed/$total_tests tests passed</strong></div>" >> "$OUTPUT_FILE"
        fi
    fi

elif [ "$FORMAT" = "catch2" ]; then
    # Parse Catch2 test output
    if grep -q "All tests passed" "$INPUT_FILE"; then
        PASSED=$(grep -oE "[0-9]+ assertions" "$INPUT_FILE" | head -1 | grep -oE "[0-9]+")
        TESTS=$(grep -oE "[0-9]+ test cases" "$INPUT_FILE" | head -1 | grep -oE "[0-9]+")
        echo "<div class=\"summary\"><strong>All $TESTS test cases passed ($PASSED assertions)</strong></div>" >> "$OUTPUT_FILE"
    else
        echo "<div class=\"summary fail\"><strong>Some tests failed</strong></div>" >> "$OUTPUT_FILE"
    fi
    
    echo "<h2>Full Output</h2>" >> "$OUTPUT_FILE"
    echo "<pre>" >> "$OUTPUT_FILE"
    cat "$INPUT_FILE" >> "$OUTPUT_FILE"
    echo "</pre>" >> "$OUTPUT_FILE"

else
    # Unknown format - just dump the output
    echo "<h2>Test Output</h2>" >> "$OUTPUT_FILE"
    echo "<pre>" >> "$OUTPUT_FILE"
    cat "$INPUT_FILE" >> "$OUTPUT_FILE"
    echo "</pre>" >> "$OUTPUT_FILE"
fi

# Add timestamp footer
echo "<p class=\"timestamp\">Generated on $(date -u '+%Y-%m-%d %H:%M:%S UTC')</p>" >> "$OUTPUT_FILE"
echo "</body></html>" >> "$OUTPUT_FILE"

echo "Generated: $OUTPUT_FILE"
