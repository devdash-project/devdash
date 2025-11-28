#!/bin/bash
# Format all C++ source files using clang-format
#
# Usage: ./scripts/format-all.sh [--check]
#   --check: Only check formatting, don't modify files (exit 1 if changes needed)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

CHECK_ONLY=false
if [[ "$1" == "--check" ]]; then
    CHECK_ONLY=true
fi

# Find all C++ source and header files
FILES=$(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/tests" \
    -type f \( -name "*.cpp" -o -name "*.h" \) \
    2>/dev/null || true)

if [[ -z "$FILES" ]]; then
    echo "No C++ files found to format"
    exit 0
fi

FILE_COUNT=$(echo "$FILES" | wc -l | tr -d ' ')
echo "Found $FILE_COUNT C++ files"

if $CHECK_ONLY; then
    echo "Checking formatting..."
    FAILED=false

    for file in $FILES; do
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo "  FAIL: $file"
            FAILED=true
        fi
    done

    if $FAILED; then
        echo ""
        echo "Formatting check failed. Run './scripts/format-all.sh' to fix."
        exit 1
    else
        echo "All files are properly formatted"
    fi
else
    echo "Formatting files..."
    for file in $FILES; do
        clang-format -i "$file"
        echo "  Formatted: $file"
    done
    echo "Done"
fi
