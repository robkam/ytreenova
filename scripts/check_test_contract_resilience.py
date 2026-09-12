#!/usr/bin/env python3
"""Generate and enforce the reviewed brittle-test-pattern inventory."""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
BASELINE_PATH = REPO_ROOT / "tests" / "contract_resilience_baseline.json"
SCHEMA_VERSION = 2
DISPOSITIONS = {"remediated", "retained", "out_of_scope"}


@dataclass(frozen=True)
class Match:
    pattern_id: str
    path: str
    symbol: str | None
    line: int
    evidence: str

    @property
    def identity(self) -> str:
        digest = hashlib.sha256(
            "\0".join(
                (self.pattern_id, self.path, self.symbol or "", str(self.line), self.evidence)
            ).encode("utf-8")
        ).hexdigest()[:16]
        return f"{self.pattern_id}:{self.path}:{self.line}:{digest}"


DEFAULT_DISPOSITIONS = {
    "direct-time-sleep": (
        "waiting and navigation remediation",
        "Direct timing waits require event-driven synchronization redesign.",
    ),
    "polling-or-retry-loop": (
        "waiting and navigation remediation",
        "Polling and retry mechanisms belong to the semantic waiting boundary.",
    ),
    "fixed-navigation-loop": (
        "waiting and navigation remediation",
        "Fixed key-count navigation must be replaced with target-identity navigation.",
    ),
    "terminal-geometry": (
        "geometry and presentation remediation",
        "Geometry assertions require presentation-contract remediation.",
    ),
    "screen-slice-or-grid": (
        "geometry and presentation remediation",
        "Screen-grid assertions require semantic visual-state replacement.",
    ),
    "source-read": (
        "external and static contract classification",
        "Source inspection requires observable-contract or static-invariant classification.",
    ),
    "implementation-string-assertion": (
        "external and static contract classification",
        "Implementation-coupled assertions require runtime or static-invariant classification.",
    ),
    "exact-prose-assertion": (
        "geometry and presentation remediation",
        "Editable presentation prose requires a durable behavioural replacement.",
    ),
}

REVIEWED_EXCEPTIONS = {
    (
        "tests/test_f7_preview.py",
        "test_f7_file_name_clipping_at_boundaries",
        "polling-or-retry-loop",
    ): (
        "out_of_scope",
        "geometry and presentation remediation",
        "Bounded scan parses one captured preview snapshot; it performs no waiting, retry, or user navigation.",
    ),
    (
        "tests/test_stats_panel.py",
        "_stats_strip_bounds",
        "polling-or-retry-loop",
    ): (
        "out_of_scope",
        "external and static contract classification",
        "Bounded string search parses one captured stats snapshot; it performs no waiting, retry, user navigation, or fixed-layout assertion.",
    ),
    (
        "tests/tui_harness.py",
        "wait_for_condition",
        "polling-or-retry-loop",
    ): (
        "retained",
        "waiting and navigation remediation",
        "Canonical event-driven PTY-output predicate: waits for observable state with a deadline and diagnostic, never an elapsed test delay or fixed action count.",
    ),
    (
        "tests/ytnova_control.py",
        "wait_for_condition",
        "polling-or-retry-loop",
    ): (
        "retained",
        "waiting and navigation remediation",
        "Canonical control-session event predicate: waits for observable state with a deadline and diagnostic, never an elapsed test delay or fixed action count.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_copy_matrix_fs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies copied fixture bytes; the string is test data, not editable interface prose.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_copy_matrix_vfs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies copied fixture bytes; the string is test data, not editable interface prose.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_move_matrix_fs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies moved fixture bytes; the string is test data, not editable interface prose.",
    ),
    (
        "tests/test_archive_write_parity.py",
        "test_archive_move_matrix_vfs_to_vfs",
        "exact-prose-assertion",
    ): (
        "retained",
        "external and static contract classification",
        "Archive payload comparison verifies moved fixture bytes; the string is test data, not editable interface prose.",
    ),
}

EXTERNAL_CONTRACT_SYMBOLS = {
    ("tests/test_cli_version_flags.py", "test_init_creates_profile_only_if_missing"),
    ("tests/test_cli_version_flags.py", "test_init_with_explicit_profile_path_preserves_target"),
    ("tests/test_theme_config_paths.py", "test_init_uses_xdg_config_home_for_every_config_surface"),
    ("tests/test_theme_config_paths.py", "test_init_ignores_relative_xdg_config_home"),
}

CI_REPAIR_PROTOCOL_SYMBOLS = {
    ("tests/test_ci_repair_loop.py", "test_build_failure_packet_includes_failed_jobs_and_log_excerpt"),
    ("tests/test_ci_repair_loop.py", "test_main_retries_failed_branch_until_green"),
    ("tests/test_ci_repair_loop.py", "test_main_blocks_when_same_failed_run_set_stays_red"),
    ("tests/test_ci_repair_loop.py", "test_main_detach_prints_started_message"),
}

APPSTATE_CONTRACT_SUITE = "tests/test_appstate_contract_guard.py"

RETAINED_TASK_CONTRACT_SUITES = {
    "tests/test_color_config.py", "tests/test_theme_ui_contract.py",
    "tests/test_security_shell_paths.py", "tests/test_archive_ui.py",
    "tests/test_archive_write_parity.py", "tests/test_core.py",
    "tests/test_destination_prompt.py", "tests/test_dir_window_dispatch_regressions.py",
    "tests/test_command_strip_visibility.py",
}



GENERATED_TEMPLATE_SYMBOLS = {
    ("tests/test_profile_template_sync.py", "test_default_profile_template_header_matches_packaged_config"),
    ("tests/test_profile_template_sync.py", "test_default_commands_catalog_header_matches_packaged_commands"),
    (
        "tests/test_profile_template_sync.py",
        "test_default_command_presets_catalog_header_matches_packaged_presets",
    ),
}

STATIC_CONTRACT_SUITES = {
    "tests/test_appstate_contract_guard.py",
    "tests/test_code_quality_hotspots_report.py",
    "tests/test_ci_repair_loop.py",
    "tests/test_cli_version_flags.py",
    "tests/test_dir_window_dispatch_regressions.py",
    "tests/test_fileops_integrity.py",
    "tests/test_file_window_dispatch_regressions.py",
    "tests/test_fuzz_harness_sync_guard.py",
    "tests/test_panel_anchor_contract.py",
    "tests/test_security_gate_contract.py",
    "tests/test_security_shell_paths.py",
    "tests/test_security_tempfiles.py",
    "tests/test_theme_config_paths.py",
    "tests/test_theme_ui_contract.py",
    "tests/test_wsl_notify.py",
    "tests/test_color_config.py",
}

STATIC_CONTRACT_SYMBOLS = {
    ("tests/test_f2_vols.py", "test_f9_applications_menu_navigation_keys_and_edit_action"),
    ("tests/test_archive_exit_ui.py", "test_missing_profile_f10_unchanged_edit_creates_profile"),
    ("tests/test_archive_exit_ui.py", "test_missing_themes_f10_unchanged_edit_keeps_starter_file"),
    ("tests/test_archive_exit_ui.py", "test_missing_commands_f10_unchanged_edit_keeps_starter_file"),
    ("tests/test_archive_exit_ui.py", "test_f10_themes_edits_active_home_dotfile_fallback"),
    ("tests/test_archive_exit_ui.py", "test_removed_legacy_profile_f10_recreates_xdg_not_dotfile"),
    ("tests/test_commands_exhaustive.py", "test_archive_execute_tempfile_cleanup_present"),
    ("tests/test_commands_exhaustive.py", "test_archive_view_tempfile_cleanup_present"),
    ("tests/test_commands_exhaustive.py", "test_archive_hex_tempfile_cleanup_present"),
    ("tests/test_archive_ui.py", "test_archive_mutations_pre_draw_spinner_and_restore_footer_context_contract"),
    ("tests/test_tagged_action_regressions.py", "test_handle_tag_file_action_delegates_file_op_hotspot"),
    ("tests/test_tagged_action_regressions.py", "test_tagged_execute_uses_the_tagged_file_directory_as_its_working_directory"),
    ("tests/test_tagged_action_regressions.py", "test_ctrl_key_dispatch_exposes_only_supported_tagged_operations"),
    ("tests/test_tagged_action_regressions.py", "test_tagged_attribute_prompt_uses_one_date_action_hint"),
    ("tests/test_archive_ui.py", "test_archive_output_flow_writes_selected_entry_to_file"),
    ("tests/test_archive_ui.py", "test_archive_create_overwrite_prompt_respects_no_then_yes"),
    ("tests/test_core.py", "test_tagged_copy_overwrite_all_applies_to_remaining_conflicts"),
    ("tests/test_core.py", "test_tagged_move_overwrite_all_applies_to_remaining_conflicts"),
    ("tests/test_core.py", "test_path_copy"),
    ("tests/test_stats_panel.py", "test_rich_fileinfo_overlay_shows_text_snippet"),
    ("tests/test_stats_panel.py", "test_summary_fileinfo_overlay_uses_file_command_output"),
    ("tests/test_stats_panel.py", "test_long_filename_does_not_hide_preview_or_file_overlays"),
    ("tests/test_stats_panel.py", "test_compact_view_yields_to_visible_rich_and_summary_overlays"),
    ("tests/test_f7_preview.py", "test_f7_preview_search_highlight_contract_uses_tagged_matches"),
    ("tests/test_print_feature.py", "test_framed_output_uses_multiline_fence_around_file_content"),
    ("tests/test_destination_prompt.py", "test_file_copy_missing_destination_yes_creates_directory_and_copies"),
    ("tests/test_panel_isolation.py", "test_dotfiles_toggle_restores_tree_viewport_origin_with_hidden_prefix"),
    ("tests/test_panel_isolation.py", "test_delete_visible_child_restores_tree_viewport_origin_with_hidden_prefix"),
    ("tests/test_panel_isolation.py", "test_split_file_focus_survives_tab_round_trip"),
    ("tests/test_panel_isolation.py", "test_split_panels_keep_independent_file_focus_states"),
    ("tests/test_stats_panel.py", "test_stats_show_current_file_on_entry"),
    ("tests/test_stats_panel.py", "test_stats_in_big_window_mode"),
    ("tests/test_stats_panel.py", "test_stats_show_named_fileinfo_view_state"),
    ("tests/test_stats_panel.py", "test_attributes_view_controls_symlink_targets_in_small_file_window"),
    ("tests/test_archive_exit_ui.py", "test_archive_root_unlogged_right_does_not_show_permission_denied"),
    ("tests/test_archive_exit_ui.py", "test_legacy_six_column_commands_file_does_not_abort_startup"),
    ("tests/test_archive_exit_ui.py", "test_placeholder_dir_shows_unlogged_not_no_files"),
    ("tests/test_archive_ui.py", "test_archive_create_exclusion_empty_payload_shows_status_and_aborts"),
    ("tests/test_archive_ui.py", "test_archive_create_unsupported_format_shows_and_clears_status_error"),
    ("tests/test_contract_resilience_guard.py", "test_reviewed_semantic_wait_exceptions_survive_baseline_generation"),
    ("tests/test_dialog.py", "test_tier_1_footer_prompt"),
    ("tests/test_ghost_bugs.py", "test_screen_wipe_after_error"),
    ("tests/test_help_source_schema.py", "test_help_source_uses_deterministic_topic_block_schema"),
    ("tests/test_i18n_runtime.py", "test_cli_option_errors_support_positional_locale_placeholders"),
    ("tests/test_panel_isolation.py", "test_bug_same_volume_home_mkdir_from_home_root_keeps_inactive_file_state"),
    ("tests/test_print_feature.py", "test_stale_output_commands_conf_does_not_abort_startup"),
    ("tests/test_vi_keys_mode.py", "test_vi_uppercase_d_deletes_tagged_after_single_confirmation"),
    ("tests/test_command_strip_visibility.py", "test_directory_and_file_surfaces_accept_their_actions_at_narrow_width"),
    ("tests/test_command_strip_visibility.py", "test_volume_and_applications_choosers_open_and_cancel_without_stale_modal"),
    ("tests/test_panels.py", "test_f7_visual_layout"),
    ("tests/test_stats_panel.py", "test_footer_shows_fileinfo_band"),
}

SEMANTIC_SNAPSHOT_SYMBOLS = {
    ("tests/repro_real_home_same_volume_split_bug.py", "_stats_current_dir_contains"),
    ("tests/repro_same_volume_home_mkdir_bug.py", "_stats_current_dir_contains"),
    ("tests/test_core.py", "_run_archive_payload_driver"),
    ("tests/test_dir_window_dispatch_regressions.py", "stats_current_dir_contains"),
    ("tests/test_panel_isolation.py", "_stats_current_dir_contains"),
    ("tests/test_panel_isolation.py", "_active_volume_name_from_lines"),
    ("tests/test_panel_isolation.py", "_detect_split_column"),
    ("tests/test_panel_isolation.py", "_assert_split_column_continuous"),
    ("tests/test_panel_isolation.py", "_first_tree_row_segment"),
    ("tests/test_panel_isolation.py", "_tree_segment_rows"),
    ("tests/test_stats_panel.py", "_stats_area"),
    ("tests/test_stats_panel.py", "_stats_strip_bounds"),
    ("tests/test_stats_panel.py", "_stats_strip_texts"),
    ("tests/test_stats_panel.py", "_send_and_wait_for_stats_count"),
}


class PatternVisitor(ast.NodeVisitor):
    def __init__(self, path: str, lines: list[str]) -> None:
        self.path = path
        self.lines = lines
        self.symbols: list[str] = []
        self.matches: list[Match] = []

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        self._visit_symbol(node.name, node)

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        self._visit_symbol(node.name, node)

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        self._visit_symbol(node.name, node)

    def visit_For(self, node: ast.For) -> None:
        if _is_range_loop(node) and _contains_key_send(node):
            self._add("fixed-navigation-loop", node)
        elif _is_range_loop(node) and (_contains_retry_signal(node) or _has_retry_target(node)):
            self._add("polling-or-retry-loop", node)
        self.generic_visit(node)

    def visit_While(self, node: ast.While) -> None:
        if _contains_retry_signal(node) or _contains_time_call(node.test):
            self._add("polling-or-retry-loop", node)
        self.generic_visit(node)

    def visit_Call(self, node: ast.Call) -> None:
        if _dotted_name(node.func) == "time.sleep":
            self._add("direct-time-sleep", node)
        elif _is_source_read(node):
            self._add("source-read", node)
        if _has_terminal_geometry(node):
            self._add("terminal-geometry", node)
        self.generic_visit(node)

    def visit_Subscript(self, node: ast.Subscript) -> None:
        if _looks_like_screen_slice(node):
            self._add("screen-slice-or-grid", node)
        self.generic_visit(node)

    def visit_Assert(self, node: ast.Assert) -> None:
        if _is_implementation_string_assertion(node.test):
            self._add("implementation-string-assertion", node)
        if _is_exact_prose_assertion(node.test):
            self._add("exact-prose-assertion", node)
        self.generic_visit(node)

    def visit_Compare(self, node: ast.Compare) -> None:
        if _looks_like_geometry_comparison(node):
            self._add("terminal-geometry", node)
        self.generic_visit(node)

    def _visit_symbol(self, name: str, node: ast.AST) -> None:
        self.symbols.append(name)
        self.generic_visit(node)
        self.symbols.pop()

    def _add(self, pattern_id: str, node: ast.AST) -> None:
        line = getattr(node, "lineno", 1)
        evidence = self.lines[line - 1].strip() if line <= len(self.lines) else ""
        self.matches.append(
            Match(pattern_id, self.path, self.symbols[-1] if self.symbols else None, line, evidence)
        )


def _dotted_name(node: ast.AST) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        parent = _dotted_name(node.value)
        return f"{parent}.{node.attr}" if parent else node.attr
    return None


def _is_range_loop(node: ast.For) -> bool:
    return isinstance(node.iter, ast.Call) and _dotted_name(node.iter.func) == "range"


def _contains_key_send(node: ast.AST) -> bool:
    return any(
        isinstance(item, ast.Call)
        and (_dotted_name(item.func) or "").split(".")[-1]
        in {"send", "send_keystroke", "send_key", "press"}
        for item in ast.walk(node)
    )


def _contains_retry_signal(node: ast.AST) -> bool:
    return any(
        isinstance(item, (ast.Try, ast.Break, ast.Continue))
        or (isinstance(item, ast.Call) and _contains_time_call(item))
        for item in ast.walk(node)
    )


def _has_retry_target(node: ast.For) -> bool:
    return isinstance(node.target, ast.Name) and any(
        token in node.target.id.lower() for token in ("attempt", "retry", "poll")
    )


def _contains_time_call(node: ast.AST) -> bool:
    return any(
        isinstance(item, ast.Call)
        and (_dotted_name(item.func) or "") in {"time.monotonic", "time.time", "perf_counter"}
        for item in ast.walk(node)
    )


def _is_source_read(node: ast.Call) -> bool:
    method = (_dotted_name(node.func) or "").split(".")[-1]
    if method in {"_read_source", "read_repo_source", "extract_function_block"}:
        return True
    if method not in {"read_text", "read_bytes", "open"}:
        return False
    return any(
        isinstance(value, ast.Constant)
        and isinstance(value.value, str)
        and ("src/" in value.value or value.value.endswith((".c", ".h")))
        for value in ast.walk(node)
    )


def _has_terminal_geometry(node: ast.Call) -> bool:
    return any(
        keyword.arg in {"rows", "cols", "columns", "width", "height"}
        and isinstance(keyword.value, ast.Constant)
        and isinstance(keyword.value.value, int)
        for keyword in node.keywords
    )


def _looks_like_screen_slice(node: ast.Subscript) -> bool:
    value = _dotted_name(node.value) or ""
    if value.split(".")[-1] in {"lines", "screen", "footer_rows", "rows"}:
        return True
    return isinstance(node.slice, ast.Slice) and "screen" in value.lower()


def _string_constants(node: ast.AST) -> list[str]:
    return [item.value for item in ast.walk(node) if isinstance(item, ast.Constant) and isinstance(item.value, str)]


def _is_implementation_string_assertion(node: ast.AST) -> bool:
    strings = _string_constants(node)
    return any(
        token in value
        for value in strings
        for token in ("src/", ".c", ".h", "static ", "void ", "int ")
    )


def _is_exact_prose_assertion(node: ast.AST) -> bool:
    if not isinstance(node, (ast.Compare, ast.BoolOp)):
        return False
    return any(
        len(value.split()) >= 2 or "\n" in value for value in _string_constants(node)
    )


def _looks_like_geometry_comparison(node: ast.Compare) -> bool:
    names = {_dotted_name(value) or "" for value in ast.walk(node)}
    return any(
        name.split(".")[-1] in {"rows", "cols", "columns", "width", "height", "x", "y"}
        for name in names
    )


def scan(root: Path) -> list[Match]:
    matches: list[Match] = []
    for path in sorted((root / "tests").rglob("*.py")):
        relpath = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8")
        visitor = PatternVisitor(relpath, text.splitlines())
        visitor.visit(ast.parse(text, filename=relpath))
        matches.extend(visitor.matches)
    unique = {
        (match.pattern_id, match.path, match.symbol, match.line, match.evidence): match
        for match in matches
    }
    return sorted(
        unique.values(), key=lambda match: (match.path, match.line, match.pattern_id, match.evidence)
    )


def _baseline_row(match: Match) -> dict[str, object]:
    disposition = "out_of_scope"
    owner, reason = DEFAULT_DISPOSITIONS[match.pattern_id]
    exception = REVIEWED_EXCEPTIONS.get((match.path, match.symbol, match.pattern_id))
    if exception:
        disposition, owner, reason = exception
    elif match.path == APPSTATE_CONTRACT_SUITE:
        disposition = "retained"
        owner = "generated AppState ownership and transition contracts"
        reason = (
            "The named AppState registry or source-boundary invariant prevents unregistered "
            "writes, stale state reuse, or fail-open dispatch; runtime execution cannot safely "
            "prove their global absence across every generated registry and call path."
        )
    elif match.path in RETAINED_TASK_CONTRACT_SUITES:
        disposition = "retained"
        owner = "classified Task 99.5 behavioral and static contracts"
        reason = (
            "The named test protects a published configuration, non-observable security, or "
            "filesystem/archive end-state contract; its exact fixture payload or static guard is not editable presentation prose."
        )
    elif (match.path, match.symbol) in GENERATED_TEMPLATE_SYMBOLS:
        disposition = "retained"
        owner = "published template generation contracts"
        reason = (
            "Packaged source and generated starter-header content must remain synchronized; "
            "runtime execution cannot safely prove pre-build generated-artifact drift."
        )
    elif (match.path, match.symbol) in EXTERNAL_CONTRACT_SYMBOLS:
        disposition = "retained"
        owner = "published command and configuration contracts"
        reason = (
            "The assertion verifies a documented CLI bootstrap or XDG configuration "
            "contract, not incidental display prose or whitespace."
        )
    elif (match.path, match.symbol) in CI_REPAIR_PROTOCOL_SYMBOLS:
        disposition = "retained"
        owner = "machine-consumed CI recovery protocol"
        reason = (
            "The assertion verifies failure-packet, handoff, or detached-loop protocol "
            "tokens consumed by automated CI recovery rather than editable presentation prose."
        )
    elif (
        (match.path in STATIC_CONTRACT_SUITES or (match.path, match.symbol) in STATIC_CONTRACT_SYMBOLS)
        and (
            match.pattern_id in {"exact-prose-assertion", "implementation-string-assertion"}
            or (match.path, match.symbol) in STATIC_CONTRACT_SYMBOLS
        )
    ):
        owner = "external and static contract classification"
        reason = (
            "This suite validates machine-readable theme/configuration invariants; "
            "the source-derived assertion is classified separately from runtime presentation."
        )
    elif (match.path, match.symbol) in SEMANTIC_SNAPSHOT_SYMBOLS:
        owner = "external and static contract classification"
        reason = (
            "The helper parses semantic fixture identity from one captured runtime snapshot; "
            "it is not a fixed terminal-layout assertion."
        )
    return {
        "id": match.identity,
        "pattern_id": match.pattern_id,
        "path": match.path,
        "symbol": match.symbol,
        "line": match.line,
        "evidence": match.evidence,
        "disposition": disposition,
        "reason": reason,
        "owner": owner,
        "removal_condition": (
            "Remove this exception when the test is replaced by an observable behavioural "
            "contract or the underlying stable external contract is retired."
        ),
    }


def build_baseline(root: Path) -> dict[str, object]:
    return {
        "schema_version": SCHEMA_VERSION,
        "contract": "Reviewed test-contract exception allowlist for Python tests",
        "matches": [_baseline_row(match) for match in scan(root)],
    }


def validate_baseline(document: object, root: Path) -> list[str]:
    if not isinstance(document, dict):
        return ["baseline must be a JSON object"]
    if document.get("schema_version") != SCHEMA_VERSION:
        return [f"baseline schema_version must be {SCHEMA_VERSION}"]
    rows = document.get("matches")
    if not isinstance(rows, list):
        return ["baseline matches must be a list"]

    failures: list[str] = []
    expected = {match.identity: match for match in scan(root)}
    observed: dict[str, dict[str, object]] = {}
    required = {
        "id",
        "pattern_id",
        "path",
        "symbol",
        "line",
        "evidence",
        "disposition",
        "reason",
        "owner",
        "removal_condition",
    }
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            failures.append(f"matches[{index}] must be an object")
            continue
        missing = sorted(required - row.keys())
        if missing:
            failures.append(f"matches[{index}] lacks required field(s): {', '.join(missing)}")
            continue
        row_id = row["id"]
        if not isinstance(row_id, str):
            failures.append(f"matches[{index}].id must be a string")
            continue
        if row_id in observed:
            failures.append(f"duplicate baseline match: {row_id}")
        observed[row_id] = row
        expected_match = expected.get(row_id)
        if expected_match is not None:
            expected_fields = {
                "pattern_id": expected_match.pattern_id,
                "path": expected_match.path,
                "symbol": expected_match.symbol,
                "line": expected_match.line,
                "evidence": expected_match.evidence,
            }
            for field, value in expected_fields.items():
                if row.get(field) != value:
                    failures.append(f"{row_id}: {field} does not match scanned evidence")
        if row.get("disposition") not in DISPOSITIONS:
            failures.append(f"{row_id}: invalid disposition")
        for field in ("reason", "owner", "removal_condition"):
            if not isinstance(row.get(field), str) or not row[field].strip():
                failures.append(f"{row_id}: {field} must be non-empty")

    for row_id in sorted(expected.keys() - observed.keys()):
        match = expected[row_id]
        failures.append(f"unreviewed match: {match.path}:{match.line}: {match.pattern_id}")
    for row_id in sorted(observed.keys() - expected.keys()):
        failures.append(f"stale baseline match: {row_id}")
    return failures


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    parser.add_argument("--baseline", type=Path, default=BASELINE_PATH)
    parser.add_argument("--write-baseline", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    if args.write_baseline:
        args.baseline.write_text(
            json.dumps(build_baseline(root), indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        print(f"WROTE: {args.baseline}")
        return 0
    try:
        document = json.loads(args.baseline.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"FAIL: unable to read baseline: {exc}", file=sys.stderr)
        return 1
    failures = validate_baseline(document, root)
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1
    print("PASS: test-contract exception allowlist is fully reconciled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
