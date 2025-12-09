#!/bin/bash
# Generate HTML test report from cargo test output

INPUT_FILE="$1"
OUTPUT_FILE="$2"

if [ -z "$INPUT_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "Usage: $0 <test-output.txt> <report.html>"
    exit 1
fi

# Start HTML
cat > "$OUTPUT_FILE" << 'HTMLEOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>POCKET+ Rust Test Report</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 900px; margin: 0 auto; padding: 2rem; line-height: 1.6; background: #f8f9fa; }
    h1 { color: #333; border-bottom: 2px solid #f46623; padding-bottom: 0.5rem; }
    .summary { background: #28a745; color: white; padding: 1rem; border-radius: 8px; margin: 1rem 0; }
    .summary.fail { background: #dc3545; }
    .module { background: white; padding: 1rem; border-radius: 8px; margin: 1rem 0; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
    .module h2 { margin-top: 0; color: #f46623; font-size: 1.2rem; border-bottom: 1px solid #eee; padding-bottom: 0.5rem; }
    .test { padding: 0.25rem 0; font-family: monospace; font-size: 0.9em; }
    .test.pass::before { content: "ok "; color: #28a745; font-weight: bold; }
    .test.fail::before { content: "FAILED "; color: #dc3545; font-weight: bold; }
    .test.ignored::before { content: "ignored "; color: #ffc107; font-weight: bold; }
    .duration { color: #666; font-size: 0.85em; }
    .stats { color: #666; font-size: 0.9rem; margin-top: 0.5rem; padding-top: 0.5rem; border-top: 1px solid #eee; }
    pre { background: #f1f1f1; padding: 1rem; border-radius: 4px; overflow-x: auto; font-size: 0.85em; }
  </style>
</head>
<body>
  <h1>POCKET+ Rust Test Report</h1>
  <div class="module">
    <h2>Test Results</h2>
HTMLEOF

# Parse test output
total_pass=0
total_fail=0
total_ignored=0

while IFS= read -r line; do
    # Test result: test name ... ok/FAILED/ignored
    if [[ "$line" =~ ^test[[:space:]]+([^[:space:]]+)[[:space:]]+\.\.\.[[:space:]]+(ok|FAILED|ignored) ]]; then
        test_name="${BASH_REMATCH[1]}"
        result="${BASH_REMATCH[2]}"

        case "$result" in
            ok)
                echo "<div class=\"test pass\">$test_name</div>" >> "$OUTPUT_FILE"
                ((total_pass++))
                ;;
            FAILED)
                echo "<div class=\"test fail\">$test_name</div>" >> "$OUTPUT_FILE"
                ((total_fail++))
                ;;
            ignored)
                echo "<div class=\"test ignored\">$test_name</div>" >> "$OUTPUT_FILE"
                ((total_ignored++))
                ;;
        esac
    fi
done < "$INPUT_FILE"

# Close module div
echo "  </div>" >> "$OUTPUT_FILE"

# Calculate totals
total_tests=$((total_pass + total_fail + total_ignored))

# Summary class
if [ $total_fail -eq 0 ]; then
    summary_class="summary"
else
    summary_class="summary fail"
fi

# Add summary
cat >> "$OUTPUT_FILE" << HTMLEOF
  <div class="$summary_class">
    <strong>Total: $total_pass passed, $total_fail failed, $total_ignored ignored ($total_tests tests)</strong>
  </div>
  <div class="stats">
    <p>Generated on $(date -u '+%Y-%m-%d %H:%M:%S UTC')</p>
  </div>
</body>
</html>
HTMLEOF

echo "Test report generated: $OUTPUT_FILE"
echo "Results: $total_pass passed, $total_fail failed, $total_ignored ignored"
