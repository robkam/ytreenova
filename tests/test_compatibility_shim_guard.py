from __future__ import annotations

import importlib.util
import json
from pathlib import Path


GUARD_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_compatibility_shims.py"
GUARD_SPEC = importlib.util.spec_from_file_location(
    "check_compatibility_shims", GUARD_PATH
)
assert GUARD_SPEC is not None and GUARD_SPEC.loader is not None

guard = importlib.util.module_from_spec(GUARD_SPEC)
GUARD_SPEC.loader.exec_module(guard)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _write_fixture(
    root: Path, *, status: str, entries: list[dict[str, str]], marker: str = ""
) -> None:
    _write(
        root / "docs" / "ROADMAP.md",
        f"### Task {51}.2.2: Runtime launch hardening\n* - [{status}] **Status:** "
        f"{'Complete' if status == 'x' else 'Not Started'}.\n",
    )
    _write(
        root / "registry" / "compatibility_shims.json",
        json.dumps({"shims": entries}),
    )
    _write(root / "src" / "demo.c", f"/* {marker} */\nint demo(void) {{ return 0; }}\n")


def test_orphaned_compatibility_shim_marker_is_rejected(tmp_path: Path) -> None:
    marker = "YTNOVA_COMPAT_SHIM: id=demo owner=51.2.2 removal=replace-demo"
    _write_fixture(tmp_path, status=" ", entries=[], marker=marker)

    failures = guard.check_repository(tmp_path)

    assert any("unregistered shim marker" in failure for failure in failures)


def test_completed_owner_compatibility_shim_marker_is_rejected(tmp_path: Path) -> None:
    marker = "YTNOVA_COMPAT_SHIM: id=demo owner=51.2.2 removal=replace-demo"
    entry = {
        "id": "demo",
        "file": "src/demo.c",
        "symbol": "demo",
        "owner_task": "51.2.2",
        "removal_condition": "replace-demo",
    }
    _write_fixture(tmp_path, status="x", entries=[entry], marker=marker)

    failures = guard.check_repository(tmp_path)

    assert any("completed owner task" in failure for failure in failures)


def test_active_compatibility_shim_inventory_is_accepted(tmp_path: Path) -> None:
    marker = "YTNOVA_COMPAT_SHIM: id=demo owner=51.2.2 removal=replace-demo"
    entry = {
        "id": "demo",
        "file": "src/demo.c",
        "symbol": "demo",
        "owner_task": "51.2.2",
        "removal_condition": "replace-demo",
    }
    _write_fixture(tmp_path, status=" ", entries=[entry], marker=marker)

    assert guard.check_repository(tmp_path) == []
