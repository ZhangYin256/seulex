#!/usr/bin/env python3
import json
import pathlib
import sys
from collections import defaultdict


def main() -> int:
    if len(sys.argv) != 4:
        print("Usage: coverage_by_module.py <coverage.json> <repo_root> <output.md>", file=sys.stderr)
        return 2

    json_path = pathlib.Path(sys.argv[1])
    repo_root = pathlib.Path(sys.argv[2]).resolve()
    out_path = pathlib.Path(sys.argv[3])

    data = json.loads(json_path.read_text(encoding="utf-8"))
    files = data.get("files", [])

    module_totals = defaultdict(lambda: {"total": 0, "covered": 0})

    for item in files:
        file_value = item.get("file", "")
        if not file_value:
            continue

        file_path = pathlib.Path(file_value)
        if not file_path.is_absolute():
            file_path = (repo_root / file_path).resolve()

        try:
            rel = file_path.relative_to(repo_root)
        except ValueError:
            continue

        parts = rel.parts
        if len(parts) < 2 or parts[0] != "src":
            continue

        module = parts[1] if len(parts) > 2 else "root"
        lines = item.get("lines", [])
        total = len(lines)
        covered = sum(1 for line in lines if int(line.get("count", 0)) > 0)

        module_totals[module]["total"] += total
        module_totals[module]["covered"] += covered

    lines = []
    lines.append("# Coverage by Module")
    lines.append("")
    lines.append("| Module | Covered Lines | Total Lines | Coverage |")
    lines.append("|---|---:|---:|---:|")

    for module in sorted(module_totals.keys()):
        covered = module_totals[module]["covered"]
        total = module_totals[module]["total"]
        percent = 0.0 if total == 0 else (covered * 100.0 / total)
        lines.append(f"| {module} | {covered} | {total} | {percent:.2f}% |")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
