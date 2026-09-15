from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "scripts" / "ci_repair_loop.py"
SCRIPT_SPEC = importlib.util.spec_from_file_location("ci_repair_loop", SCRIPT_PATH)
assert SCRIPT_SPEC is not None and SCRIPT_SPEC.loader is not None
ci_repair = importlib.util.module_from_spec(SCRIPT_SPEC)
SCRIPT_SPEC.loader.exec_module(ci_repair)


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def test_discover_handoff_requires_explicit_choice_when_multiple_currents(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    handoff_dir = tmp_path / ".agent" / "handoffs"
    _write(handoff_dir / "task-a.current.md", "a")
    _write(handoff_dir / "task-b.current.md", "b")

    monkeypatch.setattr(ci_repair, "HANDOFF_DIR", handoff_dir)
    monkeypatch.setattr(ci_repair, "FAILURE_PACKET_PATH", handoff_dir / "ci-failure.current.md")
    monkeypatch.setattr(ci_repair, "STATE_PATH", handoff_dir / "ci-repair.current.md")

    with pytest.raises(RuntimeError, match="multiple current handoffs found"):
        ci_repair._discover_handoff(None)


def test_build_failure_packet_includes_failed_jobs_and_log_excerpt() -> None:
    failed_runs = [
        {
            "workflowName": "C/C++ Baseline CI",
            "event": "push",
            "conclusion": "failure",
            "url": "https://example.invalid/run/1",
        }
    ]
    run_details = [
        {
            "workflowName": "C/C++ Baseline CI",
            "jobs": [
                {
                    "name": "Full coverage baseline gate",
                    "url": "https://example.invalid/job/1",
                    "conclusion": "failure",
                    "steps": [
                        {
                            "name": "Run coverage baseline gate",
                            "conclusion": "failure",
                        }
                    ],
                }
            ],
        }
    ]
    logs = [
        "Full coverage baseline gate\tRun coverage baseline gate\tpytest -q tests/test_example.py\n"
        "E   assert 1 == 2\n"
    ]

    packet = ci_repair._build_failure_packet(
        repo="robkam/ytreenova",
        branch="feat/test",
        sha="abc123",
        attempt=2,
        handoff_path=Path(".agent/handoffs/task.current.md"),
        failed_runs=failed_runs,
        run_details=run_details,
        logs=logs,
    )

    assert "authoritative current failure context" in packet
    assert "Full coverage baseline gate" in packet
    assert "Run coverage baseline gate" in packet
    assert "assert 1 == 2" in packet


def test_main_retries_failed_branch_until_green(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    handoff_dir = tmp_path / ".agent" / "handoffs"
    handoff_path = handoff_dir / "preview-helper-boundary.current.md"
    _write(handoff_path, "# task\n")

    repo_root = tmp_path / "repo"
    repo_root.mkdir()

    monkeypatch.setattr(ci_repair, "REPO_ROOT", repo_root)
    monkeypatch.setattr(ci_repair, "HANDOFF_DIR", handoff_dir)
    monkeypatch.setattr(ci_repair, "FAILURE_PACKET_PATH", handoff_dir / "ci-failure.current.md")
    monkeypatch.setattr(ci_repair, "PROMPT_PATH", handoff_dir / "ci-repair.prompt.current.txt")
    monkeypatch.setattr(ci_repair, "RESPONSE_PATH", handoff_dir / "ci-repair.response.current.txt")
    monkeypatch.setattr(ci_repair, "STATE_PATH", handoff_dir / "ci-repair.current.md")

    sha_values = iter(["sha-old", "sha-old", "sha-new", "sha-new"])
    git_map = {
        ("branch", "--show-current"): "feat/branch",
        ("remote", "get-url", "origin"): "https://github.com/robkam/ytreenova.git",
    }

    def fake_git(args: list[str], *, repo_root: Path) -> str:
        key = tuple(args)
        if key == ("rev-parse", "HEAD"):
            return next(sha_values)
        return git_map[key]

    runs_by_sha = {
        "sha-old": [
            {
                "databaseId": 1001,
                "workflowName": "C/C++ Baseline CI",
                "headSha": "sha-old",
                "status": "completed",
                "conclusion": "failure",
                "event": "push",
                "displayTitle": "test",
                "createdAt": "2026-07-18T00:00:00Z",
                "updatedAt": "2026-07-18T00:01:00Z",
                "url": "https://example.invalid/run/1001",
            }
        ],
        "sha-new": [
            {
                "databaseId": 1002,
                "workflowName": "C/C++ Baseline CI",
                "headSha": "sha-new",
                "status": "completed",
                "conclusion": "success",
                "event": "push",
                "displayTitle": "test",
                "createdAt": "2026-07-18T00:02:00Z",
                "updatedAt": "2026-07-18T00:03:00Z",
                "url": "https://example.invalid/run/1002",
            }
        ],
    }

    def fake_load_runs(
        repo: str, *, repo_root: Path, branch: str, sha: str
    ) -> list[dict[str, object]]:
        return runs_by_sha[sha]

    monkeypatch.setattr(ci_repair, "_git", fake_git)
    monkeypatch.setattr(ci_repair, "_load_runs", fake_load_runs)
    monkeypatch.setattr(
        ci_repair,
        "_load_run_detail",
        lambda repo, *, repo_root, run_id: {
            "workflowName": "C/C++ Baseline CI",
            "jobs": [
                {
                    "name": "Full coverage baseline gate",
                    "url": "https://example.invalid/job/1001",
                    "conclusion": "failure",
                    "steps": [
                        {"name": "Run coverage baseline gate", "conclusion": "failure"}
                    ],
                }
            ],
        },
    )
    monkeypatch.setattr(
        ci_repair,
        "_load_failed_log",
        lambda repo, *, repo_root, run_id: "pytest -q tests/test_example.py\nE assert 1 == 2\n",
    )
    codex_prompts: list[str] = []
    monkeypatch.setattr(
        ci_repair,
        "_invoke_codex",
        lambda *, repo_root, prompt_text, model, profile, dry_run: codex_prompts.append(
            prompt_text
        )
        or 0,
    )
    notifications: list[str] = []
    monkeypatch.setattr(ci_repair, "_notify", lambda repo_root, message: notifications.append(message))
    monkeypatch.setattr(ci_repair.time, "sleep", lambda seconds: None)
    monkeypatch.setattr(sys, "argv", ["ci_repair_loop.py", "--poll-seconds", "0"])

    rc = ci_repair.main()
    output = capsys.readouterr().out

    assert rc == 0
    assert len(codex_prompts) == 1
    assert "Failure packet (authoritative current truth)" in codex_prompts[0]
    assert "launching fresh repair attempt 1/3" in output
    assert notifications == ["feat/branch CI green at sha-new"]
    assert (handoff_dir / "ci-failure.current.md").exists()
    assert (handoff_dir / "ci-repair.current.md").read_text(encoding="utf-8").find("completed") != -1


def test_main_blocks_when_same_failed_run_set_stays_red(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    handoff_dir = tmp_path / ".agent" / "handoffs"
    _write(handoff_dir / "preview-helper-boundary.current.md", "# task\n")
    repo_root = tmp_path / "repo"
    repo_root.mkdir()

    monkeypatch.setattr(ci_repair, "REPO_ROOT", repo_root)
    monkeypatch.setattr(ci_repair, "HANDOFF_DIR", handoff_dir)
    monkeypatch.setattr(ci_repair, "FAILURE_PACKET_PATH", handoff_dir / "ci-failure.current.md")
    monkeypatch.setattr(ci_repair, "PROMPT_PATH", handoff_dir / "ci-repair.prompt.current.txt")
    monkeypatch.setattr(ci_repair, "RESPONSE_PATH", handoff_dir / "ci-repair.response.current.txt")
    monkeypatch.setattr(ci_repair, "STATE_PATH", handoff_dir / "ci-repair.current.md")

    monkeypatch.setattr(
        ci_repair,
        "_git",
        lambda args, *, repo_root: {
            ("branch", "--show-current"): "feat/branch",
            ("rev-parse", "HEAD"): "sha-stuck",
            ("remote", "get-url", "origin"): "https://github.com/robkam/ytreenova.git",
        }[tuple(args)],
    )
    failing_runs = [
        {
            "databaseId": 2001,
            "workflowName": "C/C++ Baseline CI",
            "headSha": "sha-stuck",
            "status": "completed",
            "conclusion": "failure",
            "event": "push",
            "displayTitle": "test",
            "createdAt": "2026-07-18T00:00:00Z",
            "updatedAt": "2026-07-18T00:01:00Z",
            "url": "https://example.invalid/run/2001",
        }
    ]
    monkeypatch.setattr(
        ci_repair,
        "_load_runs",
        lambda repo, *, repo_root, branch, sha: list(failing_runs),
    )
    monkeypatch.setattr(
        ci_repair,
        "_load_run_detail",
        lambda repo, *, repo_root, run_id: {"workflowName": "C/C++ Baseline CI", "jobs": []},
    )
    monkeypatch.setattr(
        ci_repair,
        "_load_failed_log",
        lambda repo, *, repo_root, run_id: "same failure\n",
    )
    monkeypatch.setattr(ci_repair, "_invoke_codex", lambda **kwargs: 0)
    notifications: list[str] = []
    monkeypatch.setattr(ci_repair, "_notify", lambda repo_root, message: notifications.append(message))
    monkeypatch.setattr(ci_repair.time, "sleep", lambda seconds: None)
    monkeypatch.setattr(sys, "argv", ["ci_repair_loop.py", "--poll-seconds", "0"])

    rc = ci_repair.main()
    output = capsys.readouterr().out

    assert rc == 1
    assert "same failed run set is still red" in output
    assert notifications == [
        "feat/branch CI repair blocked: same failed run set is still red after the last repair attempt"
    ]


def test_base_command_omits_default_values() -> None:
    command = ci_repair._base_command(
        handoff=None,
        poll_seconds=ci_repair.DEFAULT_POLL_SECONDS,
        max_attempts=ci_repair.DEFAULT_MAX_ATTEMPTS,
        model=None,
        profile=None,
        dry_run=False,
    )

    assert command == ["python3", str(ci_repair.REPO_ROOT / "scripts" / "ci_repair_loop.py")]


def test_main_detach_prints_started_message(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    handoff_dir = tmp_path / ".agent" / "handoffs"
    handoff_path = handoff_dir / "preview-helper-boundary.current.md"
    _write(handoff_path, "# task\n")
    repo_root = tmp_path / "repo"
    repo_root.mkdir()

    monkeypatch.setattr(ci_repair, "REPO_ROOT", repo_root)
    monkeypatch.setattr(ci_repair, "HANDOFF_DIR", handoff_dir)
    monkeypatch.setattr(ci_repair, "FAILURE_PACKET_PATH", handoff_dir / "ci-failure.current.md")
    monkeypatch.setattr(ci_repair, "PROMPT_PATH", handoff_dir / "ci-repair.prompt.current.txt")
    monkeypatch.setattr(ci_repair, "RESPONSE_PATH", handoff_dir / "ci-repair.response.current.txt")
    monkeypatch.setattr(ci_repair, "STATE_PATH", handoff_dir / "ci-repair.current.md")
    monkeypatch.setattr(ci_repair, "LOG_PATH", handoff_dir / "ci-repair.current.log")
    monkeypatch.setattr(ci_repair, "LAUNCH_INFO_PATH", handoff_dir / "ci-repair.launch.current.json")
    monkeypatch.setattr(
        ci_repair,
        "_git",
        lambda args, *, repo_root: {
            ("branch", "--show-current"): "feat/branch",
            ("rev-parse", "HEAD"): "sha-detach",
            ("remote", "get-url", "origin"): "https://github.com/robkam/ytreenova.git",
        }[tuple(args)],
    )
    monkeypatch.setattr(
        ci_repair,
        "_start_detached",
        lambda **kwargs: ("tmux", "ytnova-ci-repair-feat-branch"),
    )
    monkeypatch.setattr(
        sys,
        "argv",
        ["ci_repair_loop.py", "--detach", "--handoff", str(handoff_path)],
    )

    rc = ci_repair.main()
    output = capsys.readouterr().out

    assert rc == 0
    assert "started ci-repair loop in tmux" in output
    assert "ytnova-ci-repair-feat-branch" in output
