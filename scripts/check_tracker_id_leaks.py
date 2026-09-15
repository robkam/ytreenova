#!/usr/bin/env python3
"""Reject tracker identifiers outside the repository housekeeping documents."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ALLOWED_PATHS = frozenset(
    {
        "docs/ROADMAP.md",
        "docs/BUGS.md",
        "docs/V1_RELEASE_LINE.md",
    }
)
TRACKER_ID_RE = re.compile(
    r"\b(?:task\s*[_-]?\s*\d+(?:\.\d+)*|bugs?\s*[-_]?\s*\d+(?:\.\d+)*)\b",
    re.IGNORECASE,
)


def _tracked_paths(root: Path) -> list[Path]:
    try:
        result = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z"],
            check=True,
            capture_output=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return [path for path in root.rglob("*") if path.is_file() and ".git" not in path.parts]
    return [root / path for path in result.stdout.decode("utf-8").split("\0") if path]


def check_repository(root: Path) -> list[str]:
    failures: list[str] = []
    for path in _tracked_paths(root):
        if not path.is_file():
            continue
        relative_path = path.relative_to(root).as_posix()
        if relative_path in ALLOWED_PATHS:
            continue
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue
        for line_number, line in enumerate(lines, start=1):
            match = TRACKER_ID_RE.search(line)
            if match is not None:
                failures.append(
                    f"{relative_path}:{line_number}: tracker identifier "
                    f"'{match.group(0)}' is allowed only in housekeeping documents"
                )
    return failures


def main() -> int:
    failures = check_repository(Path(__file__).resolve().parents[1])
    if failures:
        print("Tracker identifier leakage found:", file=sys.stderr)
        print("\n".join(failures), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
