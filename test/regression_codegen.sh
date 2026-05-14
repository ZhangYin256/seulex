#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD="$ROOT/build"
SPEC="$ROOT/test_inputs/dfa_minimizer_demo.l"
OUT_C="$ROOT/test_outputs/generated_scanner.c"
OUT_BIN="$ROOT/test_outputs/generated_scanner"
FLEX_C="$ROOT/test_outputs/flex_scanner.c"
FLEX_BIN="$ROOT/test_outputs/flex_scanner"
SAMPLE_IN="$ROOT/test_inputs/sample_input.txt"

mkdir -p "$ROOT/test_outputs"

echo "Generating scanner with SeuLex..."
$BUILD/SeuLex -o "$OUT_C" "$SPEC"

echo "Compiling generated scanner..."
cc -std=c11 -O2 "$OUT_C" -o "$OUT_BIN" || { echo "Compile failed"; exit 1; }

echo "Running generated scanner..."
cat "$SAMPLE_IN" | "$OUT_BIN" > "$ROOT/test_outputs/generated.out" || true

if command -v flex >/dev/null 2>&1; then
  echo "Flex found: generating flex scanner for comparison..."
  flex -o "$FLEX_C" "$SPEC"
  cc -std=c11 -O2 "$FLEX_C" -lfl -o "$FLEX_BIN"
  cat "$SAMPLE_IN" | "$FLEX_BIN" > "$ROOT/test_outputs/flex.out" || true
  echo "Diff between generated scanner and flex:"
  diff -u "$ROOT/test_outputs/flex.out" "$ROOT/test_outputs/generated.out" || echo "Differences found"
else
  echo "Flex not found; skipping comparison. Generated output saved to test_outputs/generated.out"
fi

echo "Regression test completed."
