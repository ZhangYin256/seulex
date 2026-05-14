#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <seulex_exe> <source_dir>" >&2
  exit 2
fi

SEULEX_EXE="$1"
SOURCE_DIR="$2"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

OUT_C="$WORK_DIR/simple1.yy.c"
"$SEULEX_EXE" -o "$OUT_C" "$SOURCE_DIR/test_inputs/simple1.l" >/dev/null

if [[ ! -s "$OUT_C" ]]; then
  echo "Generated scanner file is empty" >&2
  exit 1
fi

if ! grep -q "int yylex(void)" "$OUT_C"; then
  echo "Generated scanner missing yylex function" >&2
  exit 1
fi

echo "OK: generated simple scanner at $OUT_C"
