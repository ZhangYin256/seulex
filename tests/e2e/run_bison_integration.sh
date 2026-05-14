#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "Usage: $0 <seulex_exe> <source_dir> <bison_exe> <cc_exe>" >&2
  exit 2
fi

SEULEX_EXE="$1"
SOURCE_DIR="$2"
BISON_EXE="$3"
CC_EXE="$4"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

"$BISON_EXE" -d "$SOURCE_DIR/test/c99.y" -o "$WORK_DIR/c99.tab.c"
"$SEULEX_EXE" -o "$WORK_DIR/c99.yy.c" "$SOURCE_DIR/test/c99.l" >/dev/null
"$CC_EXE" "$WORK_DIR/c99.tab.c" "$WORK_DIR/c99.yy.c" -lfl -o "$WORK_DIR/c99parser"

printf 'int x;\n' | "$WORK_DIR/c99parser" >/dev/null

echo "OK: bison parser + generated scanner integration passed"
