from __future__ import annotations

import importlib.util
from pathlib import Path


GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_tracker_id_leaks.py"
GUARD_SPEC = importlib.util.spec_from_file_location("check_tracker_id_leaks", GUARD_PATH)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None
guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def _write(root: Path, relative_path: str, content: str) -> None:
    path = root / relative_path
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _tracker_samples() -> list[str]:
    task_number = str(31)
    bug_number = str(10)
    return [
        f"Task {task_number}",
        f"Task{task_number}",
        f"TASK_{task_number}",
        f"TASK-{task_number}",
        f"BUG-{bug_number}",
        f"BUG {bug_number}",
        f"BUG_{bug_number}",
        f"BUG{bug_number}",
        f"bugs {bug_number}",
    ]


def test_tracker_ids_are_allowed_only_in_housekeeping_documents(tmp_path: Path) -> None:
    samples = "\n".join(_tracker_samples()) + "\n"
    _write(tmp_path, "docs/ROADMAP.md", samples)
    _write(tmp_path, "docs/BUGS.md", samples)
    _write(tmp_path, "docs/V1_RELEASE_LINE.md", samples)
    _write(tmp_path, "docs/SPECIFICATION.md", "The split-state contract is mandatory.\n")

    assert guard.check_repository(tmp_path) == []


def test_tracker_ids_in_product_documents_are_rejected(tmp_path: Path) -> None:
    samples = _tracker_samples()
    _write(tmp_path, "docs/SPECIFICATION.md", "\n".join(samples[:4]) + "\n")
    _write(tmp_path, "src/ui/demo.c", "\n".join(samples[4:]) + "\n")

    failures = guard.check_repository(tmp_path)

    expected_locations = {
        *(f"docs/SPECIFICATION.md:{line_number}" for line_number in range(1, 5)),
        *(f"src/ui/demo.c:{line_number}" for line_number in range(1, 6)),
    }
    actual_locations = {failure.split(": tracker identifier", 1)[0] for failure in failures}

    assert actual_locations == expected_locations
