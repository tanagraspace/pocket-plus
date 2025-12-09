#!/bin/bash
# Generate HTML test report from go test output

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
  <title>POCKET+ Go Test Report</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 900px; margin: 0 auto; padding: 2rem; line-height: 1.6; background: #f8f9fa; }
    h1 { color: #333; border-bottom: 2px solid #00ADD8; padding-bottom: 0.5rem; }
    .summary { background: #28a745; color: white; padding: 1rem; border-radius: 8px; margin: 1rem 0; }
    .summary.fail { background: #dc3545; }
    .package { background: white; padding: 1rem; border-radius: 8px; margin: 1rem 0; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
    .package h2 { margin-top: 0; color: #00ADD8; font-size: 1.2rem; border-bottom: 1px solid #eee; padding-bottom: 0.5rem; }
    .test { padding: 0.25rem 0; font-family: monospace; font-size: 0.9em; }
    .test.pass::before { content: "PASS "; color: #28a745; font-weight: bold; }
    .test.fail::before { content: "FAIL "; color: #dc3545; font-weight: bold; }
    .test.skip::before { content: "SKIP "; color: #ffc107; font-weight: bold; }
    .duration { color: #666; font-size: 0.85em; }
    .stats { color: #666; font-size: 0.9rem; margin-top: 0.5rem; padding-top: 0.5rem; border-top: 1px solid #eee; }
    pre { background: #f1f1f1; padding: 1rem; border-radius: 4px; overflow-x: auto; font-size: 0.85em; }
  </style>
</head>
<body>
  <h1>POCKET+ Go Test Report</h1>
HTMLEOF

# Parse test output
total_pass=0
total_fail=0
total_skip=0
current_package=""

while IFS= read -r line; do
    # Package line: === RUN or --- PASS/FAIL
    if [[ "$line" =~ ^"=== RUN"[[:space:]]+(Test[A-Za-z0-9_]+) ]]; then
        test_name="${BASH_REMATCH[1]}"
    fi

    # Test result: --- PASS or --- FAIL or --- SKIP
    if [[ "$line" =~ ^"--- PASS:"[[:space:]]+(Test[A-Za-z0-9_]+)[[:space:]]+\(([0-9.]+)s\) ]]; then
        test_name="${BASH_REMATCH[1]}"
        duration="${BASH_REMATCH[2]}"
        echo "<div class=\"test pass\">$test_name <span class=\"duration\">(${duration}s)</span></div>" >> "$OUTPUT_FILE"
        ((total_pass++))
    elif [[ "$line" =~ ^"--- FAIL:"[[:space:]]+(Test[A-Za-z0-9_]+)[[:space:]]+\(([0-9.]+)s\) ]]; then
        test_name="${BASH_REMATCH[1]}"
        duration="${BASH_REMATCH[2]}"
        echo "<div class=\"test fail\">$test_name <span class=\"duration\">(${duration}s)</span></div>" >> "$OUTPUT_FILE"
        ((total_fail++))
    elif [[ "$line" =~ ^"--- SKIP:"[[:space:]]+(Test[A-Za-z0-9_]+) ]]; then
        test_name="${BASH_REMATCH[1]}"
        echo "<div class=\"test skip\">$test_name</div>" >> "$OUTPUT_FILE"
        ((total_skip++))
    fi

    # Package summary: ok or FAIL followed by package path
    if [[ "$line" =~ ^"ok"[[:space:]]+([^[:space:]]+)[[:space:]]+([0-9.]+)s ]]; then
        pkg="${BASH_REMATCH[1]}"
        pkg_duration="${BASH_REMATCH[2]}"
    elif [[ "$line" =~ ^"FAIL"[[:space:]]+([^[:space:]]+) ]]; then
        pkg="${BASH_REMATCH[1]}"
    fi
done < "$INPUT_FILE"

# Calculate totals
total_tests=$((total_pass + total_fail + total_skip))

# Summary class
if [ $total_fail -eq 0 ]; then
    summary_class="summary"
else
    summary_class="summary fail"
fi

# Add summary
cat >> "$OUTPUT_FILE" << HTMLEOF
  <div class="$summary_class">
    <strong>Total: $total_pass passed, $total_fail failed, $total_skip skipped ($total_tests tests)</strong>
  </div>
  <div class="stats">
    <p>Generated on $(date -u '+%Y-%m-%d %H:%M:%S UTC')</p>
  </div>
</body>
</html>
HTMLEOF

echo "Test report generated: $OUTPUT_FILE"
echo "Results: $total_pass passed, $total_fail failed, $total_skip skipped"
