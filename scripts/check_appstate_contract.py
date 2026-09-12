#!/usr/bin/env python3
"""Validate the documented AppState transition registries."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REGISTRY_DIR = REPO_ROOT / "registry" / "appstate"
DEFAULT_TRANSITIONS = DEFAULT_REGISTRY_DIR / "appstate_transition_matrix.json"
DEFAULT_ACTION_COVERAGE = DEFAULT_REGISTRY_DIR / "appstate_action_coverage.json"
DEFAULT_EVENT_COVERAGE = DEFAULT_REGISTRY_DIR / "appstate_event_coverage.json"
DEFAULT_OWNER_FIELDS = DEFAULT_REGISTRY_DIR / "appstate_owner_fields.json"
DEFAULT_DISPATCH_SURFACES = DEFAULT_REGISTRY_DIR / "appstate_dispatch_surfaces.json"
DEFAULT_INVARIANTS = DEFAULT_REGISTRY_DIR / "appstate_invariants.json"
DEFAULT_GENERATION_DOMAINS = DEFAULT_REGISTRY_DIR / "appstate_generation_domains.json"
DEFAULT_DIFF_HARNESS = DEFAULT_REGISTRY_DIR / "appstate_diff_harness.json"
DEFAULT_TRANSITION_SEQUENCES = DEFAULT_REGISTRY_DIR / "appstate_transition_sequences.json"
DEFAULT_ACTION_HEADER = REPO_ROOT / "include" / "ytnova_defs.h"
DEFAULT_ACTION_RUNTIME = REPO_ROOT / "src" / "core" / "appstate_actions.c"

STALE_RUNTIME_REGISTRY_NOTE_PATTERNS = (
    re.compile(r"\bnon[- ]runtime\b", re.IGNORECASE),
    re.compile(r"\bfuture[- ]only\b", re.IGNORECASE),
    re.compile(r"\bfor future\b", re.IGNORECASE),
    re.compile(r"before AppState runtime migration", re.IGNORECASE),
    re.compile(r"\bduring migration\b", re.IGNORECASE),
    re.compile(r"\bfuture state-sequence enforcement\b", re.IGNORECASE),
    re.compile(r"\bcontinues to migrate incrementally\b", re.IGNORECASE),
    re.compile(r"runtime behavior is unchanged", re.IGNORECASE),
)

REQUIRED_TRANSITION_CATEGORIES = {
    "keybinding",
    "menu_action",
    "modal_action",
    "refresh_rebuild",
    "volume_operation",
    "terminal_signal_or_resize",
    "filesystem_mutation_result",
    "command_completion",
    "rebuild_rebind_callback",
    "render_reflow",
}

REQUIRED_TRANSITION_FIELDS = {
    "id",
    "category",
    "source_state",
    "event",
    "guard",
    "allowed_result",
    "blocked_result",
    "target_state",
    "owner",
    "declared_write_set",
    "generation_effect",
    "side_effects",
    "render_invalidation",
    "boundary_status",
    "notes_follow_up",
}

REQUIRED_ACTION_FIELDS = {
    "action",
    "transition_id",
    "category",
    "owner",
    "declared_write_set",
    "owner_field_refs",
    "transition_sequence_refs",
    "dispatch_surface_refs",
    "generation_domain_refs",
    "diff_harness_refs",
    "invariant_refs",
    "boundary_status",
    "migration_notes",
}

REQUIRED_EVENT_CLASSES = {
    "terminal_resize_signal",
    "refresh_rebuild",
    "rebuild_rebind_callback",
    "filesystem_mutation_result",
    "watcher_live_refresh",
    "command_completion",
    "modal_completion",
    "volume_lifecycle",
    "render_reflow",
}

REQUIRED_DISPATCH_SURFACE_CATEGORIES = {
    "key_decode_input_dispatch",
    "directory_window_action_dispatch",
    "file_window_action_dispatch",
    "menu_modal_completion",
    "resize_signal_handling",
    "refresh_rebuild_rebind",
    "filesystem_mutation_result",
    "volume_operation",
    "watcher_live_refresh",
    "render_reflow_projection",
    "command_completion_dispatch",
    "volume_menu_selection",
    "rebuild_rebind_callback",
}

REQUIRED_INVARIANT_CATEGORIES = {
    "inactive_panel_frozen",
    "render_projection_read_only",
    "hidden_entry_visible_navigation",
    "panel_local_focus_restore",
    "viewport_identity_rebind",
    "shared_state_panel_local_isolation",
    "stale_snapshot_fail_closed",
    "blocked_transition_determinism",
}

REQUIRED_GENERATION_DOMAIN_CATEGORIES = {
    "panel_generation",
    "volume_generation",
    "directory_identity",
    "file_identity",
    "focus_shape",
    "modal_command_target",
    "visibility_filter_state",
    "topology_state",
    "file_payload_state",
    "volume_lifecycle",
    "layout_reflow",
}

REQUIRED_DIFF_HARNESS_CATEGORIES = {
    "transition_before_after_snapshot",
    "declared_write_set_diff",
    "render_projection_read_only_diff",
    "generation_mismatch_check",
    "blocked_transition_no_unrelated_mutation",
}

REQUIRED_SEQUENCE_CATEGORIES = {
    "directory_file_transition",
    "display_mode",
    "filesystem_mutation",
    "layout_split",
    "menu_action",
    "modal_command",
    "panel_navigation",
    "refresh_rebuild",
    "render_reflow",
    "search_jump",
    "terminal_resize",
    "visibility_filter",
    "volume_lifecycle",
}

REQUIRED_SEQUENCE_FLOWS = {
    "dotfile_reveal_conceal",
    "enter_directory_file_transition",
    "esc_modal_dismissal",
    "file_small_big_transitions",
    "filesystem_mutation_result",
    "modal_completion",
    "volume_menu_select",
    "refresh_rebuild",
    "render_reflow_projection",
    "search_jump",
    "showall_global_tagged_only",
    "split_close_reopen",
    "split_toggle_f8",
    "tab_panel_switch",
    "terminal_resize_reflow",
    "volume_cycling_release",
    "watcher_live_refresh",
}

REQUIRED_DISPATCH_SURFACE_FIELDS = {
    "surface_id",
    "category",
    "source_path",
    "entry_symbol_or_path",
    "transition_id",
    "transition_sequence_refs",
    "boundary_status",
    "allowed_direct_writes",
    "migration_notes",
}

REQUIRED_EVENT_FIELDS = {
    "event_id",
    "event_class",
    "transition_id",
    "category",
    "source",
    "owner",
    "declared_write_set",
    "owner_field_refs",
    "transition_sequence_refs",
    "dispatch_surface_refs",
    "generation_domain_refs",
    "diff_harness_refs",
    "invariant_refs",
    "boundary_status",
    "trigger_paths",
    "migration_notes",
}

REQUIRED_OWNER_FIELDS = {
    "field",
    "owner_region",
    "canonical_owner",
    "runtime_carrier",
    "mutation_rule",
    "migration_status",
    "invariant_checks",
}

REQUIRED_INVARIANT_FIELDS = {
    "invariant_id",
    "category",
    "owner_region",
    "protected_fields",
    "transition_ids",
    "dispatch_surface_ids",
    "failure_mode",
    "enforcement_status",
    "test_strategy",
    "migration_notes",
}

REQUIRED_GENERATION_DOMAIN_FIELDS = {
    "domain_id",
    "category",
    "owner_region",
    "generation_owner_field",
    "identity_fields",
    "coverage_transition_ids",
    "advances_on_transition_ids",
    "stale_snapshot_policy",
    "fail_closed_fallback",
    "restore_boundary",
    "enforcement_status",
    "migration_notes",
}

REQUIRED_DIFF_HARNESS_FIELDS = {
    "harness_id",
    "check_category",
    "snapshot_phases",
    "snapshot_regions",
    "transition_ids",
    "owner_field_refs",
    "invariant_ids",
    "generation_domain_ids",
    "expected_behavior",
    "failure_mode",
    "enforcement_status",
    "migration_notes",
}

REQUIRED_SEQUENCE_FIELDS = {
    "scenario_id",
    "category",
    "flow",
    "description",
    "coverage_status",
    "steps",
}

REQUIRED_SEQUENCE_STEP_FIELDS = {
    "ordinal",
    "step_id",
    "transition_id",
    "stimulus",
    "expected_result",
    "invariant_ids",
    "diff_harness_ids",
    "generation_domain_expectations",
}

REQUIRED_GENERATION_EXPECTATION_FIELDS = {
    "domain_id",
    "expectation",
}

REQUIRED_FALLBACK_FIELDS = {
    "outcome",
    "allowed_mutation_scope",
}

REQUIRED_NO_UNRELATED_MUTATION_FIELDS = {
    "diff_harness_id",
    "expectation",
}

LIST_FIELDS = {
    "declared_write_set",
    "side_effects",
    "invariant_checks",
    "migration_notes",
}

ACTION_LIST_FIELDS = LIST_FIELDS | {
    "owner_field_refs",
    "transition_sequence_refs",
    "dispatch_surface_refs",
    "generation_domain_refs",
    "diff_harness_refs",
    "invariant_refs",
}
EVENT_LIST_FIELDS = LIST_FIELDS | {
    "trigger_paths",
    "owner_field_refs",
    "transition_sequence_refs",
    "dispatch_surface_refs",
    "generation_domain_refs",
    "diff_harness_refs",
    "invariant_refs",
}
VALID_BOUNDARY_STATUSES = {
    "covered_by_transition_record",
    "documented_foundation_only",
    "mapped_to_existing_broad_transition",
}
VALID_ENFORCEMENT_STATUSES = {
    "documented_foundation_only",
    "covered_by_runtime_registry",
}
VALID_RUNTIME_REGISTRY_STATUSES = {
    "covered_by_runtime_registry",
}
VALID_MIGRATION_STATUSES = VALID_RUNTIME_REGISTRY_STATUSES
VALID_SEQUENCE_COVERAGE_STATUSES = VALID_RUNTIME_REGISTRY_STATUSES
DISPATCH_LIST_FIELDS = {"migration_notes", "transition_sequence_refs"}
INVARIANT_LIST_FIELDS = {
    "protected_fields",
    "transition_ids",
    "migration_notes",
}
GENERATION_DOMAIN_LIST_FIELDS = {
    "identity_fields",
    "coverage_transition_ids",
    "migration_notes",
}
DIFF_HARNESS_LIST_FIELDS = {
    "snapshot_phases",
    "snapshot_regions",
    "transition_ids",
    "owner_field_refs",
    "invariant_ids",
    "generation_domain_ids",
    "migration_notes",
}
SEQUENCE_LIST_FIELDS = {
    "action_coverage_refs",
    "event_coverage_refs",
    "invariant_ids",
    "diff_harness_ids",
    "generation_domain_expectations",
}
DISPATCH_SURFACE_SOURCE_ROOT = REPO_ROOT / "src"
ENTRY_SYMBOL_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
GENERATION_FIELD_RE = re.compile(r"\b(?:[A-Za-z0-9_]+\.)?[A-Za-z0-9_]+_generation\b")
DISPATCH_SURFACE_CALLSITE_RE = re.compile(
    r'AppStateValidatedDispatchSurface\s*\(\s*"([^"]+)"\s*\)'
)
EVENT_CALLSITE_RE = re.compile(r'AppStateValidatedEvent\s*\(\s*"([^"]+)"\s*\)')
KEY_ACTION_CALLSITE_RE = re.compile(r"\bAppStateValidatedKeyAction\s*\(")
DECODED_ACTION_DISPATCH_SURFACE_CATEGORIES = {
    "key_decode_input_dispatch",
    "directory_window_action_dispatch",
    "file_window_action_dispatch",
}


def _load_json(path: Path) -> tuple[Any | None, list[str]]:
    try:
        return json.loads(path.read_text(encoding="utf-8")), []
    except OSError as exc:
        return None, [f"{path}: failed to read: {exc}"]
    except json.JSONDecodeError as exc:
        return None, [f"{path}: invalid JSON: {exc}"]


def _is_non_empty(value: Any) -> bool:
    if isinstance(value, str):
        return bool(value.strip())
    if isinstance(value, list):
        return len(value) > 0 and all(_is_non_empty(item) for item in value)
    if isinstance(value, dict):
        return bool(value)
    return value is not None


def _validate_list_field(*, value: Any, label: str, field: str) -> list[str]:
    failures: list[str] = []
    if not isinstance(value, list):
        return [f"{label}: {field} must be a non-empty list"]
    if not value:
        failures.append(f"{label}: {field} must be non-empty")
    for index, item in enumerate(value):
        if not isinstance(item, str) or not item.strip():
            failures.append(f"{label}: {field}[{index}] must be a non-empty string")
    return failures


def _validate_required_fields(
    *,
    record: Any,
    required_fields: set[str],
    list_fields: set[str],
    label: str,
) -> list[str]:
    failures: list[str] = []
    if not isinstance(record, dict):
        return [f"{label}: record must be an object"]

    missing = sorted(required_fields - set(record))
    if missing:
        failures.append(f"{label}: missing required field(s): {', '.join(missing)}")

    for field in sorted((required_fields | list_fields) & set(record)):
        value = record[field]
        if field in list_fields:
            failures.extend(_validate_list_field(value=value, label=label, field=field))
        elif not _is_non_empty(value):
            failures.append(f"{label}: {field} must be non-empty")

    return failures


def _validate_status_field(
    *,
    record: dict[str, Any],
    label: str,
    field: str,
    valid_statuses: set[str],
) -> list[str]:
    value = record.get(field)
    if not isinstance(value, str) or not value.strip():
        return []
    if value not in valid_statuses:
        return [f"{label}: unknown {field}: {value}"]
    return []


def _validate_runtime_backed_registry_notes(
    *,
    doc: Any,
    path: Path,
    runtime_records: list[dict[str, Any]],
) -> list[str]:
    if not runtime_records or not isinstance(doc, dict):
        return []
    failures: list[str] = []
    contract = doc.get("contract")
    if not isinstance(contract, str) or not contract.strip():
        failures.append(
            f"{path}: runtime-backed registry contract must be a non-empty string"
        )
    notes = doc.get("notes")
    if not isinstance(notes, str) or not notes.strip():
        failures.append(
            f"{path}: runtime-backed registry notes must be a non-empty string"
        )
        return failures
    for pattern in STALE_RUNTIME_REGISTRY_NOTE_PATTERNS:
        if pattern.search(notes):
            failures.append(
                f"{path}: runtime-backed registry notes must not describe the "
                "registry as non-runtime, future-only, still migrating, or runtime-unchanged"
            )
            break
    return failures


def _validate_event_boundary_status(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    failures = _validate_status_field(
        record=record,
        label=label,
        field="boundary_status",
        valid_statuses=VALID_BOUNDARY_STATUSES,
    )
    if record.get("boundary_status") == "mapped_to_existing_broad_transition":
        failures.append(
            f"{label}: boundary_status must use covered_by_transition_record, "
            "not mapped_to_existing_broad_transition"
        )
    return failures


def _validate_runtime_action_coverage_boundary_status(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    if record.get("boundary_status") == "documented_foundation_only":
        return [
            f"{label}: boundary_status must use covered_by_transition_record "
            "once runtime action coverage is registered"
        ]
    return []


def _validate_runtime_transition_boundary_status(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    if record.get("boundary_status") == "documented_foundation_only":
        return [
            f"{label}: boundary_status must use covered_by_transition_record "
            "once runtime transition is registered"
        ]
    return []


def _validate_runtime_diff_harness_enforcement_status(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    if record.get("enforcement_status") == "documented_foundation_only":
        return [
            f"{label}: enforcement_status must use covered_by_runtime_registry "
            "once runtime diff harness is registered"
        ]
    return []


def _validate_runtime_generation_domain_enforcement_status(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    if record.get("enforcement_status") == "documented_foundation_only":
        return [
            f"{label}: enforcement_status must use covered_by_runtime_registry "
            "once runtime generation domain is registered"
        ]
    return []


def _validate_runtime_invariant_enforcement_status(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    if record.get("enforcement_status") == "documented_foundation_only":
        return [
            f"{label}: enforcement_status must use covered_by_runtime_registry "
            "once runtime invariant is registered"
        ]
    return []


def _parse_ytnova_actions(header_path: Path) -> tuple[list[str], list[str]]:
    try:
        source = header_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{header_path}: failed to read: {exc}"]

    match = re.search(r"typedef\s+enum\s*\{(?P<body>.*?)\}\s*YtreeNovaAction\s*;", source, re.S)
    if match is None:
        return [], [f"{header_path}: failed to find YtreeNovaAction enum"]

    body = re.sub(r"/\*.*?\*/", "", match.group("body"), flags=re.S)
    body = re.sub(r"//.*", "", body)
    actions: list[str] = []
    for item in body.split(","):
        action = item.split("=", 1)[0].strip()
        if not action:
            continue
        if not re.fullmatch(r"ACTION_[A-Z0-9_]+", action):
            return [], [f"{header_path}: invalid YtreeNovaAction enum member: {action}"]
        actions.append(action)

    if not actions:
        return [], [f"{header_path}: YtreeNovaAction enum must not be empty"]
    return actions, []


def _compact_initializer_snippet(row: str) -> str:
    snippet = re.sub(r"\s+", " ", row).strip()
    if len(snippet) > 160:
        return snippet[:157] + "..."
    return snippet


def _split_top_level_initializer_rows(
    body: str,
    label: str,
) -> tuple[list[str], list[str]]:
    rows: list[str] = []
    failures: list[str] = []
    index = 0
    while index < len(body):
        while index < len(body) and body[index] in " \t\r\n,":
            index += 1
        if index >= len(body):
            break
        if body[index] != "{":
            start = index
            while index < len(body) and body[index] not in ",\n":
                index += 1
            failures.append(
                f"{label}: unexpected content outside initializer row: "
                f"{_compact_initializer_snippet(body[start:index])}"
            )
            continue

        start = index
        depth = 0
        in_string = False
        escaped = False
        while index < len(body):
            char = body[index]
            if in_string:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    in_string = False
            else:
                if char == '"':
                    in_string = True
                elif char == "{":
                    depth += 1
                elif char == "}":
                    depth -= 1
                    if depth == 0:
                        index += 1
                        rows.append(body[start:index])
                        break
            index += 1
        else:
            failures.append(
                f"{label}[{len(rows)}]: unterminated initializer row: "
                f"{_compact_initializer_snippet(body[start:])}"
            )
            break

    return rows, failures


def _parse_string_initializer_array(
    body: str,
    table_name: str,
) -> tuple[list[str], list[str]]:
    values: list[str] = []
    failures: list[str] = []
    index = 0
    entry_index = 0
    while index < len(body):
        while index < len(body) and body[index] in " \t\r\n":
            index += 1
        if index >= len(body):
            break

        start = index
        if body[index] == ",":
            failures.append(
                f"{table_name}[{entry_index}]: malformed string literal entry: "
                f"{_compact_initializer_snippet(body[start:index + 1])}"
            )
            index += 1
            entry_index += 1
            continue
        if body[index] != '"':
            while index < len(body) and body[index] != ",":
                index += 1
            failures.append(
                f"{table_name}[{entry_index}]: malformed string literal entry: "
                f"{_compact_initializer_snippet(body[start:index])}"
            )
            if index < len(body) and body[index] == ",":
                index += 1
            entry_index += 1
            continue

        index += 1
        value_start = index
        escaped = False
        while index < len(body):
            char = body[index]
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                break
            index += 1
        else:
            failures.append(
                f"{table_name}[{entry_index}]: unterminated string literal entry: "
                f"{_compact_initializer_snippet(body[start:])}"
            )
            break

        value = body[value_start:index]
        index += 1
        while index < len(body) and body[index] in " \t\r\n":
            index += 1
        if index < len(body) and body[index] != ",":
            while index < len(body) and body[index] not in ",\n":
                index += 1
            failures.append(
                f"{table_name}[{entry_index}]: malformed string literal entry: "
                f"{_compact_initializer_snippet(body[start:index])}"
            )
            if index < len(body) and body[index] == ",":
                index += 1
            entry_index += 1
            continue
        if index < len(body) and body[index] == ",":
            index += 1
        values.append(value)
        entry_index += 1

    return values, failures


def _parse_runtime_action_transitions(
    runtime_path: Path,
) -> tuple[list[dict[str, str]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    match = re.search(
        r"kAppStateActionTransitions\s*\[[^\]]*\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime action transition table"]

    records: list[dict[str, str]] = []
    row_re = re.compile(
        r"\{\s*(ACTION_[A-Z0-9_]+)\s*,\s*\"([^\"]+)\"\s*,\s*\"([^\"]+)\"\s*\}"
    )
    for row_match in row_re.finditer(match.group("body")):
        records.append(
            {
                "action": row_match.group(1),
                "transition_id": row_match.group(2),
                "category": row_match.group(3),
            }
        )

    if not records:
        return [], [f"{runtime_path}: runtime action transition table must not be empty"]
    return records, []


def _parse_runtime_action_coverage_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    failures: list[str] = []
    write_sets: dict[str, list[str]] = {}
    write_set_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppState(?:TransitionWriteSet|ActionCoverageWriteSet)[0-9]+)"
        r"\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    for write_set_match in write_set_re.finditer(source):
        table_name = write_set_match.group(1)
        values, array_failures = _parse_string_initializer_array(
            write_set_match.group("body"),
            table_name,
        )
        write_sets[table_name] = values
        failures.extend(array_failures)

    arrays: dict[str, list[str]] = {}
    array_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppStateActionCoverage(?:OwnerFieldRefs|TransitionSequenceRefs|DispatchSurfaceRefs|InvariantRefs|GenerationDomainRefs|DiffHarnessRefs|MigrationNotes)[0-9]+)"
        r"\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    for array_match in array_re.finditer(source):
        table_name = array_match.group(1)
        values, array_failures = _parse_string_initializer_array(
            array_match.group("body"),
            table_name,
        )
        arrays[table_name] = values
        failures.extend(array_failures)

    match = re.search(
        r"kAppStateActionCoverages\s*\[[^\]]*\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime action coverage registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*(?P<action>ACTION_[A-Z0-9_]+)\s*,"
        r"\s*\"(?P<action_name>[^\"]*)\"\s*,"
        r"\s*\"(?P<transition_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<category>[^\"]*)\"\s*,"
        r"\s*\"(?P<owner>[^\"]*)\"\s*,"
        r"\s*(?P<write_set>kAppState(?:TransitionWriteSet|ActionCoverageWriteSet)[0-9]+)\s*,"
        r"\s*sizeof\((?P=write_set)\)\s*/\s*sizeof\((?P=write_set)\[0\]\)\s*,"
        r"\s*(?P<owner_field_refs>kAppStateActionCoverageOwnerFieldRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=owner_field_refs)\)\s*/"
        r"\s*sizeof\((?P=owner_field_refs)\[0\]\)\s*,"
        r"\s*(?P<transition_sequence_refs>kAppStateActionCoverageTransitionSequenceRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=transition_sequence_refs)\)\s*/"
        r"\s*sizeof\((?P=transition_sequence_refs)\[0\]\)\s*,"
        r"\s*(?P<dispatch_surface_refs>kAppStateActionCoverageDispatchSurfaceRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=dispatch_surface_refs)\)\s*/"
        r"\s*sizeof\((?P=dispatch_surface_refs)\[0\]\)\s*,"
        r"\s*(?P<invariant_refs>kAppStateActionCoverageInvariantRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=invariant_refs)\)\s*/"
        r"\s*sizeof\((?P=invariant_refs)\[0\]\)\s*,"
        r"\s*(?P<generation_domain_refs>kAppStateActionCoverageGenerationDomainRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=generation_domain_refs)\)\s*/"
        r"\s*sizeof\((?P=generation_domain_refs)\[0\]\)\s*,"
        r"\s*(?P<diff_harness_refs>kAppStateActionCoverageDiffHarnessRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=diff_harness_refs)\)\s*/"
        r"\s*sizeof\((?P=diff_harness_refs)\[0\]\)\s*,"
        r"\s*\"(?P<boundary_status>[^\"]*)\"\s*,"
        r"\s*(?P<migration_notes>kAppStateActionCoverageMigrationNotes[0-9]+)\s*,"
        r"\s*sizeof\((?P=migration_notes)\)\s*/"
        r"\s*sizeof\((?P=migration_notes)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_action_coverage",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_action_coverage[{index}]: malformed runtime action "
                f"coverage row: {_compact_initializer_snippet(row)}"
            )
            continue
        write_set_name = row_match.group("write_set")
        declared_write_set = write_sets.get(write_set_name)
        if declared_write_set is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown declared-write-set "
                f"table: {write_set_name}"
            )
            declared_write_set = []
        owner_field_refs_name = row_match.group("owner_field_refs")
        owner_field_refs = arrays.get(owner_field_refs_name)
        if owner_field_refs is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown owner-field-refs "
                f"table: {owner_field_refs_name}"
            )
            owner_field_refs = []
        sequence_refs_name = row_match.group("transition_sequence_refs")
        sequence_refs = arrays.get(sequence_refs_name)
        if sequence_refs is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown transition-sequence-refs "
                f"table: {sequence_refs_name}"
            )
            sequence_refs = []
        dispatch_surface_refs_name = row_match.group("dispatch_surface_refs")
        dispatch_surface_refs = arrays.get(dispatch_surface_refs_name)
        if dispatch_surface_refs is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown dispatch-surface-refs "
                f"table: {dispatch_surface_refs_name}"
            )
            dispatch_surface_refs = []
        invariant_refs_name = row_match.group("invariant_refs")
        invariant_refs = arrays.get(invariant_refs_name)
        if invariant_refs is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown invariant-refs "
                f"table: {invariant_refs_name}"
            )
            invariant_refs = []
        generation_domain_refs_name = row_match.group("generation_domain_refs")
        generation_domain_refs = arrays.get(generation_domain_refs_name)
        if generation_domain_refs is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown generation-domain-refs "
                f"table: {generation_domain_refs_name}"
            )
            generation_domain_refs = []
        diff_harness_refs_name = row_match.group("diff_harness_refs")
        diff_harness_refs = arrays.get(diff_harness_refs_name)
        if diff_harness_refs is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown diff-harness-refs "
                f"table: {diff_harness_refs_name}"
            )
            diff_harness_refs = []
        notes_name = row_match.group("migration_notes")
        notes = arrays.get(notes_name)
        if notes is None:
            failures.append(
                f"runtime_action_coverage[{index}]: unknown migration-notes "
                f"table: {notes_name}"
            )
            notes = []
        records.append(
            {
                "action": row_match.group("action"),
                "action_name": row_match.group("action_name"),
                "transition_id": row_match.group("transition_id"),
                "category": row_match.group("category"),
                "owner": row_match.group("owner"),
                "declared_write_set": declared_write_set,
                "owner_field_refs": owner_field_refs,
                "transition_sequence_refs": sequence_refs,
                "dispatch_surface_refs": dispatch_surface_refs,
                "invariant_refs": invariant_refs,
                "generation_domain_refs": generation_domain_refs,
                "diff_harness_refs": diff_harness_refs,
                "boundary_status": row_match.group("boundary_status"),
                "migration_notes": notes,
            }
        )

    if not records:
        failures.append(
            f"{runtime_path}: runtime action coverage registry must not be empty"
        )
    return records, failures


def _parse_runtime_event_coverage_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    failures: list[str] = []
    arrays: dict[str, list[str]] = {}
    array_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppState(?:TransitionWriteSet|EventCoverageOwnerFieldRefs|EventCoverageTriggerPaths|EventCoverageTransitionSequenceRefs|EventCoverageDispatchSurfaceRefs|EventCoverageInvariantRefs|EventCoverageGenerationDomainRefs|EventCoverageDiffHarnessRefs|EventCoverageMigrationNotes)[0-9]+)"
        r"\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    for array_match in array_re.finditer(source):
        table_name = array_match.group(1)
        values, array_failures = _parse_string_initializer_array(
            array_match.group("body"),
            table_name,
        )
        arrays[table_name] = values
        failures.extend(array_failures)

    match = re.search(
        r"kAppStateEventCoverages\s*\[[^\]]*\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime event coverage registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<event_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<event_class>[^\"]*)\"\s*,"
        r"\s*\"(?P<transition_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<category>[^\"]*)\"\s*,"
        r"\s*\"(?P<source>[^\"]*)\"\s*,"
        r"\s*\"(?P<owner>[^\"]*)\"\s*,"
        r"\s*(?P<write_set>kAppStateTransitionWriteSet[0-9]+)\s*,"
        r"\s*sizeof\((?P=write_set)\)\s*/\s*sizeof\((?P=write_set)\[0\]\)\s*,"
        r"\s*(?P<owner_field_refs>kAppStateEventCoverageOwnerFieldRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=owner_field_refs)\)\s*/"
        r"\s*sizeof\((?P=owner_field_refs)\[0\]\)\s*,"
        r"\s*\"(?P<boundary_status>[^\"]*)\"\s*,"
        r"\s*(?P<trigger_paths>kAppStateEventCoverageTriggerPaths[0-9]+)\s*,"
        r"\s*sizeof\((?P=trigger_paths)\)\s*/\s*sizeof\((?P=trigger_paths)\[0\]\)\s*,"
        r"\s*(?P<transition_sequence_refs>kAppStateEventCoverageTransitionSequenceRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=transition_sequence_refs)\)\s*/\s*sizeof\((?P=transition_sequence_refs)\[0\]\)\s*,"
        r"\s*(?P<dispatch_surface_refs>kAppStateEventCoverageDispatchSurfaceRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=dispatch_surface_refs)\)\s*/\s*sizeof\((?P=dispatch_surface_refs)\[0\]\)\s*,"
        r"\s*(?P<invariant_refs>kAppStateEventCoverageInvariantRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=invariant_refs)\)\s*/\s*sizeof\((?P=invariant_refs)\[0\]\)\s*,"
        r"\s*(?P<generation_domain_refs>kAppStateEventCoverageGenerationDomainRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=generation_domain_refs)\)\s*/\s*sizeof\((?P=generation_domain_refs)\[0\]\)\s*,"
        r"\s*(?P<diff_harness_refs>kAppStateEventCoverageDiffHarnessRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=diff_harness_refs)\)\s*/\s*sizeof\((?P=diff_harness_refs)\[0\]\)\s*,"
        r"\s*(?P<migration_notes>kAppStateEventCoverageMigrationNotes[0-9]+)\s*,"
        r"\s*sizeof\((?P=migration_notes)\)\s*/\s*sizeof\((?P=migration_notes)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_event_coverage",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_event_coverage[{index}]: malformed runtime event "
                f"coverage row: {_compact_initializer_snippet(row)}"
            )
            continue
        write_set = arrays.get(row_match.group("write_set"))
        owner_field_refs = arrays.get(row_match.group("owner_field_refs"))
        trigger_paths = arrays.get(row_match.group("trigger_paths"))
        transition_sequence_refs = arrays.get(row_match.group("transition_sequence_refs"))
        migration_notes = arrays.get(row_match.group("migration_notes"))
        if write_set is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown declared-write-set table: "
                f"{row_match.group('write_set')}"
            )
            write_set = []
        if owner_field_refs is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown owner-field-refs table: "
                f"{row_match.group('owner_field_refs')}"
            )
            owner_field_refs = []
        if trigger_paths is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown trigger-paths table: "
                f"{row_match.group('trigger_paths')}"
            )
            trigger_paths = []
        if transition_sequence_refs is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown transition-sequence-refs table: "
                f"{row_match.group('transition_sequence_refs')}"
            )
            transition_sequence_refs = []
        dispatch_surface_refs = arrays.get(row_match.group("dispatch_surface_refs"))
        if dispatch_surface_refs is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown dispatch-surface-refs table: "
                f"{row_match.group('dispatch_surface_refs')}"
            )
            dispatch_surface_refs = []
        invariant_refs = arrays.get(row_match.group("invariant_refs"))
        if invariant_refs is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown invariant-refs table: "
                f"{row_match.group('invariant_refs')}"
            )
            invariant_refs = []
        generation_domain_refs = arrays.get(row_match.group("generation_domain_refs"))
        if generation_domain_refs is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown generation-domain-refs table: "
                f"{row_match.group('generation_domain_refs')}"
            )
            generation_domain_refs = []
        diff_harness_refs = arrays.get(row_match.group("diff_harness_refs"))
        if diff_harness_refs is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown diff-harness-refs table: "
                f"{row_match.group('diff_harness_refs')}"
            )
            diff_harness_refs = []
        if migration_notes is None:
            failures.append(
                f"runtime_event_coverage[{index}]: unknown migration-notes table: "
                f"{row_match.group('migration_notes')}"
            )
            migration_notes = []
        records.append(
            {
                "event_id": row_match.group("event_id"),
                "event_class": row_match.group("event_class"),
                "transition_id": row_match.group("transition_id"),
                "category": row_match.group("category"),
                "source": row_match.group("source"),
                "owner": row_match.group("owner"),
                "declared_write_set": write_set,
                "owner_field_refs": owner_field_refs,
                "boundary_status": row_match.group("boundary_status"),
                "trigger_paths": trigger_paths,
                "transition_sequence_refs": transition_sequence_refs,
                "dispatch_surface_refs": dispatch_surface_refs,
                "invariant_refs": invariant_refs,
                "generation_domain_refs": generation_domain_refs,
                "diff_harness_refs": diff_harness_refs,
                "migration_notes": migration_notes,
            }
        )

    if not records:
        failures.append(f"{runtime_path}: runtime event coverage registry must not be empty")
    return records, failures


def _parse_runtime_transition_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    write_sets: dict[str, list[str]] = {}
    write_set_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppStateTransitionWriteSet[0-9]+)\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    for write_set_match in write_set_re.finditer(source):
        write_sets[write_set_match.group(1)] = re.findall(
            r'"([^"]+)"', write_set_match.group("body")
        )

    side_effects: dict[str, list[str]] = {}
    side_effect_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppStateTransitionSideEffects[0-9]+)\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    for side_effect_match in side_effect_re.finditer(source):
        side_effects[side_effect_match.group(1)] = re.findall(
            r'"([^"]+)"', side_effect_match.group("body")
        )

    match = re.search(
        r"kAppStateTransitions\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime transition registry"]

    records: list[dict[str, Any]] = []
    failures: list[str] = []
    row_re = re.compile(
        r"\{\s*\"(?P<id>[^\"]+)\"\s*,\s*\"(?P<category>[^\"]+)\"\s*,"
        r"\s*\"(?P<source_state>[^\"]+)\"\s*,\s*\"(?P<event>[^\"]+)\"\s*,"
        r"\s*\"(?P<guard>[^\"]+)\"\s*,\s*\"(?P<allowed_result>[^\"]+)\"\s*,"
        r"\s*\"(?P<blocked_result>[^\"]+)\"\s*,"
        r"\s*\"(?P<target_state>[^\"]+)\"\s*,\s*\"(?P<owner>[^\"]+)\"\s*,"
        r"\s*\"(?P<generation_effect>[^\"]+)\"\s*,"
        r"\s*(?P<side_effects>kAppStateTransitionSideEffects[0-9]+)\s*,"
        r"\s*sizeof\((?P=side_effects)\)\s*/\s*sizeof\((?P=side_effects)\[0\]\)\s*,"
        r"\s*\"(?P<render_invalidation>[^\"]+)\"\s*,"
        r"\s*\"(?P<boundary_status>[^\"]+)\"\s*,"
        r"\s*\"(?P<notes_follow_up>[^\"]+)\"\s*,"
        r"\s*(?P<write_set>kAppStateTransitionWriteSet[0-9]+)\s*,"
        r"\s*sizeof\((?P=write_set)\)\s*/\s*sizeof\((?P=write_set)\[0\]\)\s*\}",
        re.S,
    )
    for index, row_match in enumerate(row_re.finditer(match.group("body"))):
        write_set_name = row_match.group("write_set")
        declared_write_set = write_sets.get(write_set_name)
        if declared_write_set is None:
            failures.append(
                f"runtime_transition[{index}]: unknown write-set table: {write_set_name}"
            )
            declared_write_set = []
        side_effect_name = row_match.group("side_effects")
        side_effect_values = side_effects.get(side_effect_name)
        if side_effect_values is None:
            failures.append(
                f"runtime_transition[{index}]: unknown side-effects table: "
                f"{side_effect_name}"
            )
            side_effect_values = []
        records.append(
            {
                "id": row_match.group("id"),
                "category": row_match.group("category"),
                "source_state": row_match.group("source_state"),
                "event": row_match.group("event"),
                "guard": row_match.group("guard"),
                "allowed_result": row_match.group("allowed_result"),
                "blocked_result": row_match.group("blocked_result"),
                "target_state": row_match.group("target_state"),
                "owner": row_match.group("owner"),
                "generation_effect": row_match.group("generation_effect"),
                "side_effects": side_effect_values,
                "render_invalidation": row_match.group("render_invalidation"),
                "boundary_status": row_match.group("boundary_status"),
                "notes_follow_up": row_match.group("notes_follow_up"),
                "declared_write_set": declared_write_set,
            }
        )

    if not records:
        failures.append(f"{runtime_path}: runtime transition registry must not be empty")
    return records, failures


def _parse_runtime_owner_field_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    invariant_tables: dict[str, list[str]] = {}
    failures: list[str] = []
    invariant_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppStateOwnerFieldInvariantChecks[0-9]+)\[\]\s*=\s*\{"
        r"(?P<body>.*?)\};",
        re.S,
    )
    for invariant_match in invariant_re.finditer(source):
        table_name = invariant_match.group(1)
        values, array_failures = _parse_string_initializer_array(
            invariant_match.group("body"),
            table_name,
        )
        invariant_tables[table_name] = values
        failures.extend(array_failures)

    match = re.search(
        r"kAppStateOwnerFields\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime owner field registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<field>[^\"]*)\"\s*,"
        r"\s*\"(?P<owner_region>[^\"]*)\"\s*,"
        r"\s*\"(?P<canonical_owner>[^\"]*)\"\s*,"
        r"\s*\"(?P<runtime_carrier>[^\"]*)\"\s*,"
        r"\s*\"(?P<mutation_rule>[^\"]*)\"\s*,"
        r"\s*\"(?P<migration_status>[^\"]*)\"\s*,"
        r"\s*(?P<invariant_checks>kAppStateOwnerFieldInvariantChecks[0-9]+)\s*,"
        r"\s*sizeof\((?P=invariant_checks)\)\s*/"
        r"\s*sizeof\((?P=invariant_checks)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_owner_field",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_owner_field[{index}]: malformed runtime owner field "
                f"registry row: {_compact_initializer_snippet(row)}"
            )
            continue

        table_name = row_match.group("invariant_checks")
        invariant_checks = invariant_tables.get(table_name)
        if invariant_checks is None:
            failures.append(
                f"runtime_owner_field[{index}]: unknown invariant_checks "
                f"table: {table_name}"
            )
            invariant_checks = []
        records.append(
            {
                "field": row_match.group("field"),
                "owner_region": row_match.group("owner_region"),
                "canonical_owner": row_match.group("canonical_owner"),
                "runtime_carrier": row_match.group("runtime_carrier"),
                "mutation_rule": row_match.group("mutation_rule"),
                "migration_status": row_match.group("migration_status"),
                "invariant_checks": invariant_checks,
            }
        )

    if not records:
        failures.append(f"{runtime_path}: runtime owner field registry must not be empty")
    return records, failures


def _parse_runtime_dispatch_surface_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    array_fields = {
        "AllowedDirectWrites": "allowed_direct_writes",
        "TransitionSequenceRefs": "transition_sequence_refs",
        "MigrationNotes": "migration_notes",
    }
    arrays: dict[str, list[str]] = {}
    failures: list[str] = []
    for table_prefix in array_fields:
        array_re = re.compile(
            r"static\s+const\s+char\s+\*const\s+"
            rf"(kAppStateDispatchSurface{table_prefix}[0-9]+)\[\]\s*=\s*\{{"
            r"(?P<body>.*?)\};",
            re.S,
        )
        for array_match in array_re.finditer(source):
            table_name = array_match.group(1)
            values, array_failures = _parse_string_initializer_array(
                array_match.group("body"),
                table_name,
            )
            arrays[table_name] = values
            failures.extend(array_failures)

    match = re.search(
        r"kAppStateDispatchSurfaces\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime dispatch surface registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<surface_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<category>[^\"]*)\"\s*,"
        r"\s*\"(?P<source_path>[^\"]*)\"\s*,"
        r"\s*\"(?P<entry_symbol_or_path>[^\"]*)\"\s*,"
        r"\s*\"(?P<transition_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<boundary_status>[^\"]*)\"\s*,"
        r"\s*(?P<allowed_direct_writes>"
        r"kAppStateDispatchSurfaceAllowedDirectWrites[0-9]+|NULL)\s*,"
        r"\s*(?:(?P<allowed_direct_write_zero>0)|"
        r"sizeof\((?P=allowed_direct_writes)\)\s*/"
        r"\s*sizeof\((?P=allowed_direct_writes)\[0\]\))\s*,"
        r"\s*(?P<transition_sequence_refs>"
        r"kAppStateDispatchSurfaceTransitionSequenceRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=transition_sequence_refs)\)\s*/"
        r"\s*sizeof\((?P=transition_sequence_refs)\[0\]\)\s*,"
        r"\s*(?P<migration_notes>kAppStateDispatchSurfaceMigrationNotes[0-9]+)\s*,"
        r"\s*sizeof\((?P=migration_notes)\)\s*/"
        r"\s*sizeof\((?P=migration_notes)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_dispatch_surface",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_dispatch_surface[{index}]: malformed runtime dispatch "
                f"surface registry row: {_compact_initializer_snippet(row)}"
            )
            continue
        record: dict[str, Any] = {
            "surface_id": row_match.group("surface_id"),
            "category": row_match.group("category"),
            "source_path": row_match.group("source_path"),
            "entry_symbol_or_path": row_match.group("entry_symbol_or_path"),
            "transition_id": row_match.group("transition_id"),
            "boundary_status": row_match.group("boundary_status"),
        }
        for field in (
            "allowed_direct_writes",
            "transition_sequence_refs",
            "migration_notes",
        ):
            table_name = row_match.group(field)
            if table_name == "NULL":
                values = []
            else:
                values = arrays.get(table_name)
                if values is None:
                    failures.append(
                        f"runtime_dispatch_surface[{index}]: unknown {field} "
                        f"table: {table_name}"
                    )
                    values = []
            record[field] = values
        if row_match.group("allowed_direct_writes") == "NULL" and not row_match.group(
            "allowed_direct_write_zero"
        ):
            failures.append(
                f"runtime_dispatch_surface[{index}]: NULL allowed_direct_writes "
                "must have zero count"
            )
        records.append(record)

    if not records:
        failures.append(f"{runtime_path}: runtime dispatch surface registry must not be empty")
    return records, failures


def _parse_runtime_invariant_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    array_fields = {
        "ProtectedFields": "protected_fields",
        "TransitionIds": "transition_ids",
        "DispatchSurfaceIds": "dispatch_surface_ids",
        "MigrationNotes": "migration_notes",
    }
    arrays: dict[str, list[str]] = {}
    failures: list[str] = []
    for table_prefix in array_fields:
        array_re = re.compile(
            r"static\s+const\s+char\s+\*const\s+"
            rf"(kAppStateInvariant{table_prefix}[0-9]+)\[\]\s*=\s*\{{"
            r"(?P<body>.*?)\};",
            re.S,
        )
        for array_match in array_re.finditer(source):
            table_name = array_match.group(1)
            values, array_failures = _parse_string_initializer_array(
                array_match.group("body"),
                table_name,
            )
            arrays[table_name] = values
            failures.extend(array_failures)

    match = re.search(
        r"kAppStateInvariants\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime invariant registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<invariant_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<category>[^\"]*)\"\s*,"
        r"\s*\"(?P<owner_region>[^\"]*)\"\s*,"
        r"\s*(?P<protected_fields>kAppStateInvariantProtectedFields[0-9]+)\s*,"
        r"\s*sizeof\((?P=protected_fields)\)\s*/"
        r"\s*sizeof\((?P=protected_fields)\[0\]\)\s*,"
        r"\s*(?P<transition_ids>kAppStateInvariantTransitionIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=transition_ids)\)\s*/"
        r"\s*sizeof\((?P=transition_ids)\[0\]\)\s*,"
        r"\s*(?P<dispatch_surface_ids>kAppStateInvariantDispatchSurfaceIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=dispatch_surface_ids)\)\s*/"
        r"\s*sizeof\((?P=dispatch_surface_ids)\[0\]\)\s*,"
        r"\s*\"(?P<failure_mode>[^\"]*)\"\s*,"
        r"\s*\"(?P<enforcement_status>[^\"]*)\"\s*,"
        r"\s*\"(?P<test_strategy>[^\"]*)\"\s*,"
        r"\s*(?P<migration_notes>kAppStateInvariantMigrationNotes[0-9]+)\s*,"
        r"\s*sizeof\((?P=migration_notes)\)\s*/"
        r"\s*sizeof\((?P=migration_notes)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_invariant",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_invariant[{index}]: malformed runtime invariant "
                f"registry row: {_compact_initializer_snippet(row)}"
            )
            continue
        record: dict[str, Any] = {
            "invariant_id": row_match.group("invariant_id"),
            "category": row_match.group("category"),
            "owner_region": row_match.group("owner_region"),
            "failure_mode": row_match.group("failure_mode"),
            "enforcement_status": row_match.group("enforcement_status"),
            "test_strategy": row_match.group("test_strategy"),
        }
        for table_prefix, field in array_fields.items():
            table_name = row_match.group(field)
            values = arrays.get(table_name)
            if values is None:
                failures.append(
                    f"runtime_invariant[{index}]: unknown {field} table: {table_name}"
                )
                values = []
            record[field] = values
        records.append(record)

    if not records:
        failures.append(f"{runtime_path}: runtime invariant registry must not be empty")
    return records, failures


def _parse_runtime_generation_domain_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    array_fields = {
        "IdentityFields": "identity_fields",
        "CoverageTransitionIds": "coverage_transition_ids",
        "AdvancesOnTransitionIds": "advances_on_transition_ids",
        "MigrationNotes": "migration_notes",
    }
    arrays: dict[str, list[str]] = {}
    failures: list[str] = []
    for table_prefix in array_fields:
        array_re = re.compile(
            r"static\s+const\s+char\s+\*const\s+"
            rf"(kAppStateGenerationDomain{table_prefix}[0-9]+)\[\]\s*=\s*\{{"
            r"(?P<body>.*?)\};",
            re.S,
        )
        for array_match in array_re.finditer(source):
            table_name = array_match.group(1)
            values, array_failures = _parse_string_initializer_array(
                array_match.group("body"),
                table_name,
            )
            arrays[table_name] = values
            failures.extend(array_failures)

    match = re.search(
        r"kAppStateGenerationDomains\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime generation domain registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<domain_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<category>[^\"]*)\"\s*,"
        r"\s*\"(?P<owner_region>[^\"]*)\"\s*,"
        r"\s*\"(?P<generation_owner_field>[^\"]*)\"\s*,"
        r"\s*(?P<identity_fields>kAppStateGenerationDomainIdentityFields[0-9]+)\s*,"
        r"\s*sizeof\((?P=identity_fields)\)\s*/"
        r"\s*sizeof\((?P=identity_fields)\[0\]\)\s*,"
        r"\s*(?P<coverage_transition_ids>"
        r"kAppStateGenerationDomainCoverageTransitionIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=coverage_transition_ids)\)\s*/"
        r"\s*sizeof\((?P=coverage_transition_ids)\[0\]\)\s*,"
        r"\s*(?P<advances_on_transition_ids>"
        r"kAppStateGenerationDomainAdvancesOnTransitionIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=advances_on_transition_ids)\)\s*/"
        r"\s*sizeof\((?P=advances_on_transition_ids)\[0\]\)\s*,"
        r"\s*\"(?P<stale_snapshot_policy>[^\"]*)\"\s*,"
        r"\s*\"(?P<fail_closed_fallback>[^\"]*)\"\s*,"
        r"\s*\"(?P<restore_boundary>[^\"]*)\"\s*,"
        r"\s*\"(?P<enforcement_status>[^\"]*)\"\s*,"
        r"\s*(?P<migration_notes>kAppStateGenerationDomainMigrationNotes[0-9]+)\s*,"
        r"\s*sizeof\((?P=migration_notes)\)\s*/"
        r"\s*sizeof\((?P=migration_notes)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_generation_domain",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_generation_domain[{index}]: malformed runtime generation "
                f"domain registry row: {_compact_initializer_snippet(row)}"
            )
            continue
        record: dict[str, Any] = {
            "domain_id": row_match.group("domain_id"),
            "category": row_match.group("category"),
            "owner_region": row_match.group("owner_region"),
            "generation_owner_field": row_match.group("generation_owner_field"),
            "stale_snapshot_policy": row_match.group("stale_snapshot_policy"),
            "fail_closed_fallback": row_match.group("fail_closed_fallback"),
            "restore_boundary": row_match.group("restore_boundary"),
            "enforcement_status": row_match.group("enforcement_status"),
        }
        for table_prefix, field in array_fields.items():
            table_name = row_match.group(field)
            values = arrays.get(table_name)
            if values is None:
                failures.append(
                    f"runtime_generation_domain[{index}]: unknown {field} "
                    f"table: {table_name}"
                )
                values = []
            record[field] = values
        records.append(record)

    if not records:
        failures.append(f"{runtime_path}: runtime generation domain registry must not be empty")
    return records, failures


def _parse_runtime_diff_harness_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    array_fields = {
        "SnapshotPhases": "snapshot_phases",
        "SnapshotRegions": "snapshot_regions",
        "TransitionIds": "transition_ids",
        "OwnerFieldRefs": "owner_field_refs",
        "InvariantIds": "invariant_ids",
        "GenerationDomainIds": "generation_domain_ids",
        "MigrationNotes": "migration_notes",
    }
    arrays: dict[str, list[str]] = {}
    failures: list[str] = []
    for table_prefix in array_fields:
        array_re = re.compile(
            r"static\s+const\s+char\s+\*const\s+"
            rf"(kAppStateDiffHarness{table_prefix}[0-9]+)\[\]\s*=\s*\{{"
            r"(?P<body>.*?)\};",
            re.S,
        )
        for array_match in array_re.finditer(source):
            table_name = array_match.group(1)
            values, array_failures = _parse_string_initializer_array(
                array_match.group("body"),
                table_name,
            )
            arrays[table_name] = values
            failures.extend(array_failures)

    match = re.search(
        r"kAppStateDiffHarnesses\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime diff harness registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<harness_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<check_category>[^\"]*)\"\s*,"
        r"\s*(?P<snapshot_phases>kAppStateDiffHarnessSnapshotPhases[0-9]+)\s*,"
        r"\s*sizeof\((?P=snapshot_phases)\)\s*/"
        r"\s*sizeof\((?P=snapshot_phases)\[0\]\)\s*,"
        r"\s*(?P<snapshot_regions>kAppStateDiffHarnessSnapshotRegions[0-9]+)\s*,"
        r"\s*sizeof\((?P=snapshot_regions)\)\s*/"
        r"\s*sizeof\((?P=snapshot_regions)\[0\]\)\s*,"
        r"\s*(?P<transition_ids>kAppStateDiffHarnessTransitionIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=transition_ids)\)\s*/"
        r"\s*sizeof\((?P=transition_ids)\[0\]\)\s*,"
        r"\s*(?P<owner_field_refs>kAppStateDiffHarnessOwnerFieldRefs[0-9]+)\s*,"
        r"\s*sizeof\((?P=owner_field_refs)\)\s*/"
        r"\s*sizeof\((?P=owner_field_refs)\[0\]\)\s*,"
        r"\s*(?P<invariant_ids>kAppStateDiffHarnessInvariantIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=invariant_ids)\)\s*/"
        r"\s*sizeof\((?P=invariant_ids)\[0\]\)\s*,"
        r"\s*(?P<generation_domain_ids>"
        r"kAppStateDiffHarnessGenerationDomainIds[0-9]+)\s*,"
        r"\s*sizeof\((?P=generation_domain_ids)\)\s*/"
        r"\s*sizeof\((?P=generation_domain_ids)\[0\]\)\s*,"
        r"\s*\"(?P<expected_behavior>[^\"]*)\"\s*,"
        r"\s*\"(?P<failure_mode>[^\"]*)\"\s*,"
        r"\s*\"(?P<enforcement_status>[^\"]*)\"\s*,"
        r"\s*(?P<migration_notes>kAppStateDiffHarnessMigrationNotes[0-9]+)\s*,"
        r"\s*sizeof\((?P=migration_notes)\)\s*/"
        r"\s*sizeof\((?P=migration_notes)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_diff_harness",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_diff_harness[{index}]: malformed runtime diff harness "
                f"registry row: {_compact_initializer_snippet(row)}"
            )
            continue
        record: dict[str, Any] = {
            "harness_id": row_match.group("harness_id"),
            "check_category": row_match.group("check_category"),
            "expected_behavior": row_match.group("expected_behavior"),
            "failure_mode": row_match.group("failure_mode"),
            "enforcement_status": row_match.group("enforcement_status"),
        }
        for table_prefix, field in array_fields.items():
            table_name = row_match.group(field)
            values = arrays.get(table_name)
            if values is None:
                failures.append(
                    f"runtime_diff_harness[{index}]: unknown {field} "
                    f"table: {table_name}"
                )
                values = []
            record[field] = values
        records.append(record)

    if not records:
        failures.append(f"{runtime_path}: runtime diff harness registry must not be empty")
    return records, failures



def _parse_runtime_nullable_string(value: str) -> str | None:
    value = value.strip()
    if value == "NULL":
        return None
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        return value[1:-1]
    return None


def _parse_runtime_transition_sequence_registry(
    runtime_path: Path,
) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        source = runtime_path.read_text(encoding="utf-8")
    except OSError as exc:
        return [], [f"{runtime_path}: failed to read: {exc}"]

    failures: list[str] = []
    string_arrays: dict[str, list[str]] = {}
    string_array_re = re.compile(
        r"static\s+const\s+char\s+\*const\s+"
        r"(kAppStateTransitionSequenceStep(?:ActionCoverageRefs|EventCoverageRefs|InvariantIds|DiffHarnessIds)[0-9]+_[0-9]+)"
        r"\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    for array_match in string_array_re.finditer(source):
        table_name = array_match.group(1)
        values, array_failures = _parse_string_initializer_array(
            array_match.group("body"),
            table_name,
        )
        string_arrays[table_name] = values
        failures.extend(array_failures)

    generation_expectations: dict[str, list[dict[str, str]]] = {}
    generation_re = re.compile(
        r"static\s+const\s+AppStateTransitionSequenceGenerationExpectationMetadata\s+"
        r"(kAppStateTransitionSequenceStepGenerationExpectations[0-9]+_[0-9]+)"
        r"\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    generation_row_re = re.compile(
        r"\{\s*\"(?P<domain_id>[^\"]*)\"\s*,\s*\"(?P<expectation>[^\"]*)\"\s*\}",
        re.S,
    )
    for generation_match in generation_re.finditer(source):
        table_name = generation_match.group(1)
        rows, row_failures = _split_top_level_initializer_rows(
            generation_match.group("body"),
            table_name,
        )
        failures.extend(row_failures)
        values: list[dict[str, str]] = []
        for index, row in enumerate(rows):
            row_match = generation_row_re.fullmatch(row.strip())
            if row_match is None:
                failures.append(
                    f"{table_name}[{index}]: malformed generation expectation row: "
                    f"{_compact_initializer_snippet(row)}"
                )
                continue
            values.append(
                {
                    "domain_id": row_match.group("domain_id"),
                    "expectation": row_match.group("expectation"),
                }
            )
        generation_expectations[table_name] = values

    no_unrelated_mutations: dict[str, dict[str, str]] = {}
    no_unrelated_re = re.compile(
        r"static\s+const\s+AppStateTransitionSequenceNoUnrelatedMutationMetadata\s+"
        r"(kAppStateTransitionSequenceStepNoUnrelatedMutation[0-9]+_[0-9]+)"
        r"\s*=\s*\{\s*\"(?P<diff_harness_id>[^\"]*)\"\s*,\s*"
        r"\"(?P<expectation>[^\"]*)\"\s*\};",
        re.S,
    )
    for no_unrelated_match in no_unrelated_re.finditer(source):
        no_unrelated_mutations[no_unrelated_match.group(1)] = {
            "diff_harness_id": no_unrelated_match.group("diff_harness_id"),
            "expectation": no_unrelated_match.group("expectation"),
        }

    deterministic_fallbacks: dict[str, dict[str, str]] = {}
    fallback_re = re.compile(
        r"static\s+const\s+AppStateTransitionSequenceDeterministicFallbackMetadata\s+"
        r"(kAppStateTransitionSequenceStepDeterministicFallback[0-9]+_[0-9]+)"
        r"\s*=\s*\{\s*\"(?P<outcome>[^\"]*)\"\s*,\s*"
        r"\"(?P<allowed_mutation_scope>[^\"]*)\"\s*\};",
        re.S,
    )
    for fallback_match in fallback_re.finditer(source):
        deterministic_fallbacks[fallback_match.group(1)] = {
            "outcome": fallback_match.group("outcome"),
            "allowed_mutation_scope": fallback_match.group("allowed_mutation_scope"),
        }

    step_arrays: dict[str, list[dict[str, Any]]] = {}
    step_array_re = re.compile(
        r"static\s+const\s+AppStateTransitionSequenceStepMetadata\s+"
        r"(kAppStateTransitionSequenceSteps[0-9]+)\[\]\s*=\s*\{(?P<body>.*?)\};",
        re.S,
    )
    step_row_re = re.compile(
        r"\{\s*(?P<ordinal>[0-9]+)\s*,"
        r"\s*\"(?P<step_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<transition_id>[^\"]*)\"\s*,"
        r"\s*(?P<stimulus_action_id>NULL|\"[^\"]*\")\s*,"
        r"\s*(?P<stimulus_event_id>NULL|\"[^\"]*\")\s*,"
        r"\s*(?P<action_coverage_refs>NULL|kAppStateTransitionSequenceStepActionCoverageRefs[0-9]+_[0-9]+)\s*,"
        r"\s*(?P<action_coverage_ref_count>0|sizeof\((?P=action_coverage_refs)\)\s*/\s*sizeof\((?P=action_coverage_refs)\[0\]\))\s*,"
        r"\s*(?P<event_coverage_refs>NULL|kAppStateTransitionSequenceStepEventCoverageRefs[0-9]+_[0-9]+)\s*,"
        r"\s*(?P<event_coverage_ref_count>0|sizeof\((?P=event_coverage_refs)\)\s*/\s*sizeof\((?P=event_coverage_refs)\[0\]\))\s*,"
        r"\s*\"(?P<expected_result>[^\"]*)\"\s*,"
        r"\s*(?P<invariant_ids>kAppStateTransitionSequenceStepInvariantIds[0-9]+_[0-9]+)\s*,"
        r"\s*sizeof\((?P=invariant_ids)\)\s*/\s*sizeof\((?P=invariant_ids)\[0\]\)\s*,"
        r"\s*(?P<diff_harness_ids>kAppStateTransitionSequenceStepDiffHarnessIds[0-9]+_[0-9]+)\s*,"
        r"\s*sizeof\((?P=diff_harness_ids)\)\s*/\s*sizeof\((?P=diff_harness_ids)\[0\]\)\s*,"
        r"\s*(?P<generation_domain_expectations>"
        r"kAppStateTransitionSequenceStepGenerationExpectations[0-9]+_[0-9]+)\s*,"
        r"\s*sizeof\((?P=generation_domain_expectations)\)\s*/"
        r"\s*sizeof\((?P=generation_domain_expectations)\[0\]\)\s*,"
        r"\s*(?P<no_unrelated_mutation>NULL|&?kAppStateTransitionSequenceStepNoUnrelatedMutation[0-9]+_[0-9]+)\s*,"
        r"\s*(?P<precondition>NULL|\"[^\"]*\")\s*,"
        r"\s*(?P<deterministic_fallback>NULL|&?kAppStateTransitionSequenceStepDeterministicFallback[0-9]+_[0-9]+)\s*\}",
        re.S,
    )
    for step_array_match in step_array_re.finditer(source):
        table_name = step_array_match.group(1)
        rows, row_failures = _split_top_level_initializer_rows(
            step_array_match.group("body"),
            f"runtime_transition_sequence_step_array.{table_name}",
        )
        failures.extend(row_failures)
        steps: list[dict[str, Any]] = []
        for index, row in enumerate(rows):
            row_match = step_row_re.fullmatch(row.strip())
            if row_match is None:
                failures.append(
                    f"runtime_transition_sequence_step[{table_name}][{index}]: "
                    f"malformed runtime transition sequence step row: "
                    f"{_compact_initializer_snippet(row)}"
                )
                continue

            invariant_table = row_match.group("invariant_ids")
            diff_harness_table = row_match.group("diff_harness_ids")
            action_coverage_table = row_match.group("action_coverage_refs")
            event_coverage_table = row_match.group("event_coverage_refs")
            generation_table = row_match.group("generation_domain_expectations")
            no_unrelated_table = row_match.group("no_unrelated_mutation").lstrip("&")
            fallback_table = row_match.group("deterministic_fallback").lstrip("&")

            if action_coverage_table != "NULL" and action_coverage_table not in string_arrays:
                failures.append(
                    f"runtime_transition_sequence_step[{table_name}][{index}]: "
                    f"unknown action_coverage_refs table: {action_coverage_table}"
                )
            if event_coverage_table != "NULL" and event_coverage_table not in string_arrays:
                failures.append(
                    f"runtime_transition_sequence_step[{table_name}][{index}]: "
                    f"unknown event_coverage_refs table: {event_coverage_table}"
                )
            if invariant_table not in string_arrays:
                failures.append(
                    f"runtime_transition_sequence_step[{table_name}][{index}]: "
                    f"unknown invariant_ids table: {invariant_table}"
                )
            if diff_harness_table not in string_arrays:
                failures.append(
                    f"runtime_transition_sequence_step[{table_name}][{index}]: "
                    f"unknown diff_harness_ids table: {diff_harness_table}"
                )
            if generation_table not in generation_expectations:
                failures.append(
                    f"runtime_transition_sequence_step[{table_name}][{index}]: "
                    "unknown generation_domain_expectations table: "
                    f"{generation_table}"
                )

            no_unrelated = None
            if no_unrelated_table != "NULL":
                no_unrelated = no_unrelated_mutations.get(no_unrelated_table)
                if no_unrelated is None:
                    failures.append(
                        f"runtime_transition_sequence_step[{table_name}][{index}]: "
                        "unknown no_unrelated_mutation table: "
                        f"{no_unrelated_table}"
                    )
            deterministic_fallback = None
            if fallback_table != "NULL":
                deterministic_fallback = deterministic_fallbacks.get(fallback_table)
                if deterministic_fallback is None:
                    failures.append(
                        f"runtime_transition_sequence_step[{table_name}][{index}]: "
                        "unknown deterministic_fallback table: "
                        f"{fallback_table}"
                    )

            steps.append(
                {
                    "ordinal": int(row_match.group("ordinal")),
                    "step_id": row_match.group("step_id"),
                    "transition_id": row_match.group("transition_id"),
                    "stimulus_action_id": _parse_runtime_nullable_string(
                        row_match.group("stimulus_action_id")
                    ),
                    "stimulus_event_id": _parse_runtime_nullable_string(
                        row_match.group("stimulus_event_id")
                    ),
                    "action_coverage_refs": string_arrays.get(
                        action_coverage_table, []
                    ),
                    "event_coverage_refs": string_arrays.get(
                        event_coverage_table, []
                    ),
                    "expected_result": row_match.group("expected_result"),
                    "invariant_ids": string_arrays.get(invariant_table, []),
                    "diff_harness_ids": string_arrays.get(diff_harness_table, []),
                    "generation_domain_expectations": generation_expectations.get(
                        generation_table,
                        [],
                    ),
                    "no_unrelated_mutation": no_unrelated,
                    "precondition": _parse_runtime_nullable_string(
                        row_match.group("precondition")
                    ),
                    "deterministic_fallback": deterministic_fallback,
                }
            )
        step_arrays[table_name] = steps

    match = re.search(
        r"kAppStateTransitionSequences\s*\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.S,
    )
    if match is None:
        return [], [f"{runtime_path}: failed to find runtime transition sequence registry"]

    records: list[dict[str, Any]] = []
    row_re = re.compile(
        r"\{\s*\"(?P<scenario_id>[^\"]*)\"\s*,"
        r"\s*\"(?P<category>[^\"]*)\"\s*,"
        r"\s*\"(?P<flow>[^\"]*)\"\s*,"
        r"\s*\"(?P<description>[^\"]*)\"\s*,"
        r"\s*\"(?P<coverage_status>[^\"]*)\"\s*,"
        r"\s*(?P<steps>kAppStateTransitionSequenceSteps[0-9]+)\s*,"
        r"\s*sizeof\((?P=steps)\)\s*/\s*sizeof\((?P=steps)\[0\]\)\s*\}",
        re.S,
    )
    rows, row_failures = _split_top_level_initializer_rows(
        match.group("body"),
        "runtime_transition_sequence",
    )
    failures.extend(row_failures)
    for index, row in enumerate(rows):
        row_match = row_re.fullmatch(row.strip())
        if row_match is None:
            failures.append(
                f"runtime_transition_sequence[{index}]: malformed runtime transition "
                f"sequence registry row: {_compact_initializer_snippet(row)}"
            )
            continue
        steps_table = row_match.group("steps")
        steps = step_arrays.get(steps_table)
        if steps is None:
            failures.append(
                f"runtime_transition_sequence[{index}]: unknown steps table: {steps_table}"
            )
            steps = []
        records.append(
            {
                "scenario_id": row_match.group("scenario_id"),
                "category": row_match.group("category"),
                "flow": row_match.group("flow"),
                "description": row_match.group("description"),
                "coverage_status": row_match.group("coverage_status"),
                "steps": steps,
            }
        )

    if not records:
        failures.append(
            f"{runtime_path}: runtime transition sequence registry must not be empty"
        )
    return records, failures

def _validate_runtime_action_lookup(
    *,
    runtime_records: list[dict[str, str]],
    runtime_path: Path,
    enum_actions: list[str],
    action_coverage_by_action: dict[str, dict[str, Any]],
    transition_ids: dict[str, dict[str, Any]],
    runtime_transition_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    expected_actions = set(enum_actions)
    covered_actions: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_action[{index}]"
        action = record["action"]
        if action in covered_actions:
            failures.append(f"{label}: duplicate runtime action: {action}")
        covered_actions.add(action)
        if action not in expected_actions:
            failures.append(f"{label}: unknown YtreeNovaAction enum member: {action}")
        elif index >= len(enum_actions) or action != enum_actions[index]:
            expected = enum_actions[index] if index < len(enum_actions) else "<none>"
            failures.append(
                f"{label}: runtime row order does not match YtreeNovaAction enum: "
                f"expected {expected}, found {action}"
            )

        transition_id = record["transition_id"]
        transition_record = transition_ids.get(transition_id)
        if transition_record is None:
            failures.append(
                f"{label}: transition_id does not match a transition id: {transition_id}"
            )
        if transition_id not in runtime_transition_ids:
            failures.append(
                f"{label}: transition_id does not match runtime transition "
                f"registry: {transition_id}"
            )

        category = record["category"]
        if (
            transition_record is not None
            and category != transition_record.get("category")
        ):
            failures.append(
                f"{label}: category does not match transition {transition_id}: {category}"
            )

        coverage_record = action_coverage_by_action.get(action)
        if coverage_record is None:
            failures.append(
                f"{label}: runtime action missing from action coverage: {action}"
            )
            continue
        if transition_id != coverage_record.get("transition_id"):
            failures.append(
                f"{label}: runtime transition_id does not match action coverage "
                f"for {action}: {transition_id}"
            )
        if category != coverage_record.get("category"):
            failures.append(
                f"{label}: runtime category does not match action coverage "
                f"for {action}: {category}"
            )

    missing_actions = sorted(expected_actions - covered_actions)
    if missing_actions:
        failures.append(
            f"{runtime_path}: runtime action lookup missing YtreeNovaAction enum member(s): "
            + ", ".join(missing_actions)
        )

    return failures


def _validate_runtime_action_coverage_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    enum_actions: list[str],
    action_coverage_by_action: dict[str, dict[str, Any]],
    transition_ids: dict[str, dict[str, Any]],
    transition_sequence_records: list[Any],
    runtime_transition_ids: set[str],
    runtime_transition_sequence_records: list[Any],
    runtime_action_transitions: list[dict[str, str]],
    runtime_dispatch_surface_records: list[Any],
    runtime_generation_domain_records: list[Any],
    registered_owner_fields: set[str],
    runtime_invariant_ids: set[str],
    runtime_invariant_transition_ids: dict[str, set[str]],
    runtime_invariant_protected_fields: dict[str, set[str]],
    runtime_diff_harness_ids: set[str],
    runtime_diff_harness_transition_ids: dict[str, set[str]],
    runtime_diff_harness_owner_field_refs: dict[str, set[str]],
    runtime_diff_harness_invariant_ids: dict[str, set[str]],
    runtime_diff_harness_generation_domain_ids: dict[str, set[str]],
) -> list[str]:
    failures: list[str] = []
    expected_actions = set(enum_actions)
    covered_actions: set[str] = set()
    runtime_action_by_action = {
        record["action"]: record for record in runtime_action_transitions
    }

    for index, record in enumerate(runtime_records):
        label = f"runtime_action_coverage[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_ACTION_FIELDS,
                list_fields=ACTION_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        action_name = record.get("action_name")
        if not isinstance(action_name, str) or not action_name.strip():
            failures.append(f"{label}: action_name must be non-empty")
        failures.extend(_validate_event_boundary_status(record=record, label=label))
        failures.extend(
            _validate_runtime_action_coverage_boundary_status(
                record=record, label=label
            )
        )

        action = record.get("action")
        if isinstance(action, str) and action.strip():
            if action in covered_actions:
                failures.append(f"{label}: duplicate runtime action coverage: {action}")
            else:
                covered_actions.add(action)
            if action not in expected_actions:
                failures.append(f"{label}: unknown YtreeNovaAction enum member: {action}")
            elif index >= len(enum_actions) or action != enum_actions[index]:
                expected = enum_actions[index] if index < len(enum_actions) else "<none>"
                failures.append(
                    f"{label}: runtime row order does not match YtreeNovaAction "
                    f"enum: expected {expected}, found {action}"
                )
            if isinstance(action_name, str) and action_name.strip() and action_name != action:
                failures.append(
                    f"{label}: action_name does not match action: {action_name}"
                )

        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )
        failures.extend(
            _validate_owner_field_coverage_refs(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
                registry_label="owner-field registry",
            )
        )
        failures.extend(
            _validate_transition_sequence_refs(
                record=record,
                transition_sequence_records=runtime_transition_sequence_records,
                label=label,
            )[0]
        )
        failures.extend(
            _validate_dispatch_surface_refs(
                record=record,
                dispatch_surface_records=runtime_dispatch_surface_records,
                label=label,
            )
        )
        failures.extend(
            _validate_generation_domain_refs(
                generation_domain_refs=record.get("generation_domain_refs"),
                generation_domain_records=runtime_generation_domain_records,
                transition_id=record.get("transition_id"),
                label=label,
            )
        )
        failures.extend(
            _validate_record_invariant_refs(
                invariant_refs=record.get("invariant_refs"),
                invariant_ids=runtime_invariant_ids,
                invariant_transition_ids=runtime_invariant_transition_ids,
                invariant_protected_fields=runtime_invariant_protected_fields,
                transition_id=record.get("transition_id"),
                declared_write_set=record.get("declared_write_set"),
                label=label,
            )
        )
        failures.extend(
            _validate_coverage_diff_harness_refs(
                record=record,
                diff_harness_ids=runtime_diff_harness_ids,
                diff_harness_transition_ids=runtime_diff_harness_transition_ids,
                diff_harness_owner_field_refs=(
                    runtime_diff_harness_owner_field_refs
                ),
                diff_harness_invariant_ids=runtime_diff_harness_invariant_ids,
                diff_harness_generation_domain_ids=(
                    runtime_diff_harness_generation_domain_ids
                ),
                label=label,
            )
        )

        transition_id = record.get("transition_id")
        transition_record = None
        if isinstance(transition_id, str) and transition_id.strip():
            transition_record = transition_ids.get(transition_id)
            if transition_record is None:
                failures.append(
                    f"{label}: transition_id does not match a transition id: {transition_id}"
                )
            if transition_id not in runtime_transition_ids:
                failures.append(
                    f"{label}: transition_id does not match runtime transition "
                    f"registry: {transition_id}"
                )

        category = record.get("category")
        if (
            isinstance(category, str)
            and category.strip()
            and transition_record is not None
            and category != transition_record.get("category")
        ):
            failures.append(
                f"{label}: category does not match transition {transition_id}: {category}"
            )
        owner = record.get("owner")
        if (
            isinstance(owner, str)
            and owner.strip()
            and transition_record is not None
            and owner != transition_record.get("owner")
        ):
            failures.append(
                f"{label}: owner does not match transition {transition_id}: {owner}"
            )

        if isinstance(action, str) and action.strip():
            coverage_record = action_coverage_by_action.get(action)
            if coverage_record is None:
                failures.append(
                    f"{label}: runtime action coverage missing from docs: {action}"
                )
            else:
                for field in (
                    "transition_id",
                    "category",
                    "owner",
                    "declared_write_set",
                    "owner_field_refs",
                    "transition_sequence_refs",
                    "dispatch_surface_refs",
                    "generation_domain_refs",
                    "diff_harness_refs",
                    "invariant_refs",
                    "boundary_status",
                    "migration_notes",
                ):
                    if record.get(field) != coverage_record.get(field):
                        failures.append(
                            f"{label}: runtime {field} does not match action "
                            f"coverage for {action}: {record.get(field)!r}"
                        )
                if record.get("transition_sequence_refs") != coverage_record.get(
                    "transition_sequence_refs"
                ):
                    failures.extend(
                        _validate_transition_sequence_refs(
                            record=coverage_record,
                            transition_sequence_records=transition_sequence_records,
                            label=f"action coverage for {action}",
                        )[0]
                    )

            action_transition = runtime_action_by_action.get(action)
            if action_transition is None:
                failures.append(
                    f"{label}: action missing from runtime action transition table: {action}"
                )
            else:
                if transition_id != action_transition.get("transition_id"):
                    failures.append(
                        f"{label}: transition_id does not match runtime action "
                        f"transition table for {action}: {transition_id}"
                    )
                if category != action_transition.get("category"):
                    failures.append(
                        f"{label}: category does not match runtime action "
                        f"transition table for {action}: {category}"
                    )

    missing_actions = sorted(expected_actions - covered_actions)
    if missing_actions:
        failures.append(
            f"{runtime_path}: runtime action coverage missing YtreeNovaAction "
            f"enum member(s): {', '.join(missing_actions)}"
        )

    return failures


def _validate_runtime_transition_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
    runtime_invariant_protected_fields_by_transition: dict[str, set[str]],
) -> list[str]:
    failures: list[str] = []
    expected_ids = set(transition_ids)
    covered_ids: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_transition[{index}]"
        runtime_id = record["id"]
        if runtime_id in covered_ids:
            failures.append(f"{label}: duplicate runtime transition id: {runtime_id}")
        covered_ids.add(runtime_id)

        matrix_record = transition_ids.get(runtime_id)
        if matrix_record is None:
            failures.append(
                f"{label}: id does not match a transition matrix id: {runtime_id}"
            )
        else:
            for field in (
                "category",
                "source_state",
                "event",
                "guard",
                "allowed_result",
                "blocked_result",
                "target_state",
                "owner",
                "generation_effect",
                "side_effects",
                "render_invalidation",
                "boundary_status",
                "notes_follow_up",
                "declared_write_set",
            ):
                if record.get(field) != matrix_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match transition "
                        f"{runtime_id}: {record.get(field)}"
                    )

        failures.extend(_validate_event_boundary_status(record=record, label=label))
        failures.extend(
            _validate_runtime_transition_boundary_status(record=record, label=label)
        )
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )
        failures.extend(
            _validate_transition_write_set_has_invariant_coverage(
                record=record,
                protected_fields_by_transition=(
                    runtime_invariant_protected_fields_by_transition
                ),
                label=label,
            )
        )

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime transition registry missing transition id(s): "
            + ", ".join(missing_ids)
        )

    return failures


def _generation_write_coverage_by_owner_field(
    generation_domain_records: list[Any],
) -> dict[str, set[str]]:
    coverage: dict[str, set[str]] = {}
    for record in generation_domain_records:
        if not isinstance(record, dict):
            continue
        owner_field = record.get("generation_owner_field")
        transition_refs = record.get("advances_on_transition_ids")
        if not isinstance(owner_field, str) or not owner_field.strip():
            continue
        if not isinstance(transition_refs, list):
            continue
        owner_coverage = coverage.setdefault(owner_field, set())
        for transition_id in transition_refs:
            if isinstance(transition_id, str) and transition_id.strip():
                owner_coverage.add(transition_id)
    return coverage


def _validate_generation_write_coverage(
    *,
    transitions: list[Any],
    generation_domain_records: list[Any],
    label_prefix: str,
) -> list[str]:
    failures: list[str] = []
    coverage_by_owner = _generation_write_coverage_by_owner_field(
        generation_domain_records
    )

    for index, record in enumerate(transitions):
        if not isinstance(record, dict):
            continue
        transition_id = record.get("id")
        write_set = record.get("declared_write_set")
        if not isinstance(transition_id, str) or not transition_id.strip():
            continue
        if not isinstance(write_set, list):
            continue

        for write_index, field in enumerate(write_set):
            if not isinstance(field, str) or not field.strip():
                continue
            covered_transitions = coverage_by_owner.get(field)
            if covered_transitions is None:
                continue
            if transition_id not in covered_transitions:
                failures.append(
                    f"{label_prefix}[{index}]: declared_write_set[{write_index}] "
                    "writes generation owner field without generation domain "
                    f"coverage for transition {transition_id}: {field}"
                )

    return failures


def _diff_harness_write_coverage_by_transition(
    diff_harness_records: list[Any],
) -> dict[str, set[str]]:
    coverage: dict[str, set[str]] = {}
    for record in diff_harness_records:
        if not isinstance(record, dict):
            continue
        transition_refs = record.get("transition_ids")
        owner_field_refs = record.get("owner_field_refs")
        if not isinstance(transition_refs, list) or not isinstance(
            owner_field_refs, list
        ):
            continue
        for transition_id in transition_refs:
            if not isinstance(transition_id, str) or not transition_id.strip():
                continue
            transition_coverage = coverage.setdefault(transition_id, set())
            for owner_field in owner_field_refs:
                if isinstance(owner_field, str) and owner_field.strip():
                    transition_coverage.add(owner_field)
    return coverage


def _validate_diff_harness_write_coverage(
    *,
    transitions: list[Any],
    diff_harness_records: list[Any],
    label_prefix: str,
) -> list[str]:
    failures: list[str] = []
    coverage_by_transition = _diff_harness_write_coverage_by_transition(
        diff_harness_records
    )

    for index, record in enumerate(transitions):
        if not isinstance(record, dict):
            continue
        transition_id = record.get("id")
        write_set = record.get("declared_write_set")
        if not isinstance(transition_id, str) or not transition_id.strip():
            continue
        if not isinstance(write_set, list):
            continue

        covered_fields = coverage_by_transition.get(transition_id, set())
        for write_index, field in enumerate(write_set):
            if not isinstance(field, str) or not field.strip():
                continue
            if field not in covered_fields:
                failures.append(
                    f"{label_prefix}[{index}]: declared_write_set[{write_index}] "
                    "lacks diff harness coverage for transition "
                    f"{transition_id}: {field}"
                )

    return failures


def _diff_harness_generation_coverage_by_domain(
    diff_harness_records: list[Any],
) -> dict[str, set[str]]:
    coverage: dict[str, set[str]] = {}
    for record in diff_harness_records:
        if not isinstance(record, dict):
            continue
        domain_refs = record.get("generation_domain_ids")
        transition_refs = record.get("transition_ids")
        if not isinstance(domain_refs, list) or not isinstance(
            transition_refs, list
        ):
            continue
        for domain_id in domain_refs:
            if not isinstance(domain_id, str) or not domain_id.strip():
                continue
            domain_coverage = coverage.setdefault(domain_id, set())
            for transition_id in transition_refs:
                if isinstance(transition_id, str) and transition_id.strip():
                    domain_coverage.add(transition_id)
    return coverage


def _validate_generation_diff_harness_coverage(
    *,
    generation_domain_records: list[Any],
    diff_harness_records: list[Any],
    label_prefix: str,
) -> list[str]:
    failures: list[str] = []
    coverage_by_domain = _diff_harness_generation_coverage_by_domain(
        diff_harness_records
    )

    for index, record in enumerate(generation_domain_records):
        if not isinstance(record, dict):
            continue
        domain_id = record.get("domain_id")
        transition_refs = record.get("advances_on_transition_ids")
        if not isinstance(domain_id, str) or not domain_id.strip():
            continue
        if not isinstance(transition_refs, list):
            continue

        covered_transitions = coverage_by_domain.get(domain_id, set())
        for transition_id in transition_refs:
            if not isinstance(transition_id, str) or not transition_id.strip():
                continue
            if transition_id not in covered_transitions:
                failures.append(
                    f"{label_prefix}[{index}] {domain_id}: "
                    "advances_on_transition_ids lacks same-domain/same-transition "
                    "diff harness coverage for transition "
                    f"{transition_id}"
                )

    return failures


def _validate_runtime_owner_field_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    owner_field_records: list[Any],
    runtime_invariant_ids: set[str],
    runtime_invariant_protected_fields: dict[str, set[str]],
) -> list[str]:
    failures: list[str] = []
    expected_owner_fields = {
        record["field"]: record
        for record in owner_field_records
        if isinstance(record, dict)
        and isinstance(record.get("field"), str)
        and record["field"].strip()
    }
    expected_fields = set(expected_owner_fields)
    covered_fields: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_owner_field[{index}]"
        runtime_field = record["field"]
        if runtime_field in covered_fields:
            failures.append(f"{label}: duplicate runtime owner field: {runtime_field}")
        covered_fields.add(runtime_field)

        owner_field_record = expected_owner_fields.get(runtime_field)
        if owner_field_record is None:
            failures.append(
                f"{label}: field does not match an owner field: {runtime_field}"
            )
        else:
            for field in REQUIRED_OWNER_FIELDS:
                if record.get(field) != owner_field_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match owner field "
                        f"{runtime_field}: {record.get(field)}"
                    )

        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="migration_status",
                valid_statuses=VALID_MIGRATION_STATUSES,
            )
        )
        for field in sorted(REQUIRED_OWNER_FIELDS - {"invariant_checks"}):
            value = record.get(field)
            if not isinstance(value, str) or not value.strip():
                failures.append(f"{label}: {field} must be a non-empty string")

        invariant_checks = record.get("invariant_checks")
        if not isinstance(invariant_checks, list) or not invariant_checks:
            failures.append(f"{label}: invariant_checks must be non-empty")
        else:
            for check_index, check in enumerate(invariant_checks):
                if not isinstance(check, str) or not check.strip():
                    failures.append(
                        f"{label}: invariant_checks[{check_index}] "
                        "must be a non-empty string"
                    )
            failures.extend(
                _validate_invariant_check_refs(
                    invariant_checks=invariant_checks,
                    runtime_invariant_ids=runtime_invariant_ids,
                    label=label,
                )
            )
            failures.extend(
                _validate_owner_field_invariant_alignment(
                    invariant_checks=invariant_checks,
                    owner_field=runtime_field,
                    invariant_protected_fields=runtime_invariant_protected_fields,
                    label=label,
                )
            )

    missing_fields = sorted(expected_fields - covered_fields)
    if missing_fields:
        failures.append(
            f"{runtime_path}: runtime owner field registry missing field(s): "
            + ", ".join(missing_fields)
        )

    return failures


def _validate_runtime_dispatch_surface_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    dispatch_surface_records: list[Any],
    runtime_transition_records: dict[str, dict[str, Any]],
    runtime_transition_ids: set[str],
    runtime_invariant_protected_fields_by_surface: dict[str, set[str]],
    runtime_transition_sequence_records: list[Any],
    runtime_diff_harness_owner_field_refs: dict[str, set[str]],
    runtime_invariant_protected_fields: dict[str, set[str]],
) -> list[str]:
    failures: list[str] = []
    expected_surfaces = {
        record["surface_id"]: record
        for record in dispatch_surface_records
        if isinstance(record, dict)
        and isinstance(record.get("surface_id"), str)
        and record["surface_id"].strip()
    }
    expected_ids = set(expected_surfaces)
    covered_ids: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_dispatch_surface[{index}]"
        runtime_id = record["surface_id"]
        if runtime_id in covered_ids:
            failures.append(f"{label}: duplicate runtime dispatch surface id: {runtime_id}")
        covered_ids.add(runtime_id)

        surface_record = expected_surfaces.get(runtime_id)
        if surface_record is None:
            failures.append(
                f"{label}: surface_id does not match a dispatch surface id: {runtime_id}"
            )
        else:
            for field in (
                "category",
                "source_path",
                "entry_symbol_or_path",
                "transition_id",
                "boundary_status",
                "allowed_direct_writes",
                "transition_sequence_refs",
                "migration_notes",
            ):
                if record.get(field) != surface_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match dispatch "
                        f"surface {runtime_id}: {record.get(field)}"
                    )

        failures.extend(_validate_event_boundary_status(record=record, label=label))
        writes = record.get("allowed_direct_writes")
        if not isinstance(writes, list):
            failures.append(f"{label}: allowed_direct_writes must be a list")
        else:
            for write_index, write in enumerate(writes):
                if not isinstance(write, str) or not write.strip():
                    failures.append(
                        f"{label}: allowed_direct_writes[{write_index}] "
                        "must be a non-empty string"
                    )

        notes = record.get("migration_notes")
        if not isinstance(notes, list) or not notes:
            failures.append(f"{label}: migration_notes must be non-empty")
        elif any(not isinstance(note, str) or not note.strip() for note in notes):
            failures.append(f"{label}: migration_notes must contain non-empty strings")

        failures.extend(
            _validate_dispatch_surface_transition_sequence_coverage(
                record=record,
                transition_sequence_records=runtime_transition_sequence_records,
                diff_harness_owner_field_refs=runtime_diff_harness_owner_field_refs,
                invariant_protected_fields=runtime_invariant_protected_fields,
                label=label,
            )
        )

        transition_id = record.get("transition_id")
        if (
            isinstance(transition_id, str)
            and transition_id.strip()
            and transition_id not in runtime_transition_ids
        ):
            failures.append(
                f"{label}: transition_id does not match runtime transition "
                f"registry: {transition_id}"
            )
        failures.extend(
            _validate_allowed_direct_writes_within_transition(
                record=record,
                transition_records=runtime_transition_records,
                label=label,
            )
        )
        failures.extend(
            _validate_allowed_direct_writes_have_invariant_coverage(
                record=record,
                protected_fields_by_surface=runtime_invariant_protected_fields_by_surface,
                label=label,
            )
        )

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime dispatch surface registry missing "
            "surface id(s): " + ", ".join(missing_ids)
        )

    return failures


def _validate_runtime_invariant_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    invariant_records: list[Any],
    runtime_transition_ids: set[str],
    dispatch_surface_ids: set[str],
    runtime_diff_harness_owner_field_refs: set[str],
) -> list[str]:
    failures: list[str] = []
    expected_invariants = {
        record["invariant_id"]: record
        for record in invariant_records
        if isinstance(record, dict)
        and isinstance(record.get("invariant_id"), str)
        and record["invariant_id"].strip()
    }
    expected_ids = set(expected_invariants)
    covered_ids: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_invariant[{index}]"
        runtime_id = record["invariant_id"]
        if runtime_id in covered_ids:
            failures.append(f"{label}: duplicate runtime invariant id: {runtime_id}")
        covered_ids.add(runtime_id)

        invariant_record = expected_invariants.get(runtime_id)
        if invariant_record is None:
            failures.append(f"{label}: invariant_id does not match an invariant id: {runtime_id}")
        else:
            for field in (
                "category",
                "owner_region",
                "protected_fields",
                "transition_ids",
                "dispatch_surface_ids",
                "failure_mode",
                "enforcement_status",
                "test_strategy",
                "migration_notes",
            ):
                if record.get(field) != invariant_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match invariant "
                        f"{runtime_id}: {record.get(field)}"
                    )

        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="enforcement_status",
                valid_statuses=VALID_ENFORCEMENT_STATUSES,
            )
        )
        failures.extend(
            _validate_runtime_invariant_enforcement_status(
                record=record,
                label=label,
            )
        )
        for field in (
            "protected_fields",
            "transition_ids",
            "migration_notes",
        ):
            values = record.get(field)
            if not isinstance(values, list) or not values:
                failures.append(f"{label}: {field} must be non-empty")

        failures.extend(
            _validate_invariant_diff_harness_owner_field_coverage(
                protected_fields=record.get("protected_fields"),
                invariant_id=runtime_id,
                diff_harness_owner_field_refs=runtime_diff_harness_owner_field_refs,
                label=label,
                coverage_label="runtime diff harness",
            )
        )

        runtime_surface_refs = record.get("dispatch_surface_ids")
        if not isinstance(runtime_surface_refs, list):
            failures.append(f"{label}: dispatch_surface_ids must be non-empty")
        elif not runtime_surface_refs:
            migration_notes = record.get("migration_notes")
            has_cross_cutting_note = (
                isinstance(migration_notes, list)
                and any(
                    isinstance(note, str)
                    and "cross-cutting" in note.lower()
                    for note in migration_notes
                )
            )
            if not has_cross_cutting_note:
                failures.append(f"{label}: dispatch_surface_ids must be non-empty")

        transition_refs = record.get("transition_ids")
        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in runtime_transition_ids
                ):
                    failures.append(
                        f"{label}: transition_ids does not match runtime transition "
                        f"registry: {transition_id}"
                    )

        surface_refs = record.get("dispatch_surface_ids")
        if isinstance(surface_refs, list):
            for surface_id in surface_refs:
                if (
                    isinstance(surface_id, str)
                    and surface_id.strip()
                    and surface_id not in dispatch_surface_ids
                ):
                    failures.append(
                        f"{label}: dispatch_surface_ids references unknown dispatch "
                        f"surface id: {surface_id}"
                    )

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime invariant registry missing invariant id(s): "
            + ", ".join(missing_ids)
        )

    return failures


def _validate_runtime_generation_domain_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    generation_domain_records: list[Any],
    runtime_owner_fields: set[str],
    runtime_transition_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    expected_domains = {
        record["domain_id"]: record
        for record in generation_domain_records
        if isinstance(record, dict)
        and isinstance(record.get("domain_id"), str)
        and record["domain_id"].strip()
    }
    expected_ids = set(expected_domains)
    covered_ids: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_generation_domain[{index}]"
        runtime_id = record["domain_id"]
        if runtime_id in covered_ids:
            failures.append(
                f"{label}: duplicate runtime generation domain id: {runtime_id}"
            )
        covered_ids.add(runtime_id)

        domain_record = expected_domains.get(runtime_id)
        if domain_record is None:
            failures.append(
                f"{label}: domain_id does not match a generation domain id: {runtime_id}"
            )
        else:
            for field in (
                "category",
                "owner_region",
                "generation_owner_field",
                "identity_fields",
                "coverage_transition_ids",
                "advances_on_transition_ids",
                "stale_snapshot_policy",
                "fail_closed_fallback",
                "restore_boundary",
                "enforcement_status",
                "migration_notes",
            ):
                if record.get(field) != domain_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match generation "
                        f"domain {runtime_id}: {record.get(field)}"
                    )

        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="enforcement_status",
                valid_statuses=VALID_ENFORCEMENT_STATUSES,
            )
        )
        failures.extend(
            _validate_runtime_generation_domain_enforcement_status(
                record=record,
                label=label,
            )
        )
        for field in (
            "domain_id",
            "category",
            "owner_region",
            "generation_owner_field",
            "stale_snapshot_policy",
            "fail_closed_fallback",
            "restore_boundary",
            "enforcement_status",
        ):
            value = record.get(field)
            if not isinstance(value, str) or not value.strip():
                failures.append(f"{label}: {field} must be a non-empty string")

        for field in ("identity_fields", "coverage_transition_ids", "migration_notes"):
            values = record.get(field)
            if not isinstance(values, list) or not values:
                failures.append(f"{label}: {field} must be non-empty")
                continue
            for value_index, value in enumerate(values):
                if not isinstance(value, str) or not value.strip():
                    failures.append(
                        f"{label}: {field}[{value_index}] must be a non-empty string"
                    )

        for transition_field in (
            "coverage_transition_ids",
            "advances_on_transition_ids",
        ):
            transition_refs = record.get(transition_field)
            if not isinstance(transition_refs, list):
                failures.append(f"{label}: {transition_field} must be a list")
                continue
            for transition_index, transition_id in enumerate(transition_refs):
                if not isinstance(transition_id, str) or not transition_id.strip():
                    failures.append(
                        f"{label}: {transition_field}[{transition_index}] "
                        "must be a non-empty string"
                    )
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in runtime_transition_ids
                ):
                    failures.append(
                        f"{label}: {transition_field} does not match "
                        f"runtime transition registry: {transition_id}"
                    )

        coverage_transition_refs = record.get("coverage_transition_ids")
        transition_refs = record.get("advances_on_transition_ids")
        if isinstance(coverage_transition_refs, list) and isinstance(
            transition_refs, list
        ):
            declared_coverage = {
                transition_id
                for transition_id in coverage_transition_refs
                if isinstance(transition_id, str) and transition_id.strip()
            }
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in declared_coverage
                ):
                    failures.append(
                        f"{label}: advances_on_transition_ids must be covered by "
                        f"coverage_transition_ids: {transition_id}"
                    )
            declared_advances = {
                transition_id
                for transition_id in transition_refs
                if isinstance(transition_id, str) and transition_id.strip()
            }
            for transition_id in coverage_transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in declared_advances
                    and not _has_projection_only_note(record.get("migration_notes"))
                ):
                    failures.append(
                        f"{label}: coverage_transition_ids without "
                        "advances_on_transition_ids requires a read-only/"
                        f"projection-only migration_notes explanation: {transition_id}"
                    )
        generation_owner_field = record.get("generation_owner_field")
        if (
            isinstance(generation_owner_field, str)
            and generation_owner_field.strip()
            and generation_owner_field not in runtime_owner_fields
        ):
            failures.append(
                f"{label}: generation_owner_field does not match runtime owner "
                f"field registry: {generation_owner_field}"
            )

        identity_fields = record.get("identity_fields")
        if isinstance(identity_fields, list):
            for field in identity_fields:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in runtime_owner_fields
                ):
                    failures.append(
                        f"{label}: identity_fields does not match runtime owner "
                        f"field registry: {field}"
                    )

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime generation domain registry missing "
            "domain id(s): " + ", ".join(missing_ids)
        )

    return failures


def _validate_runtime_diff_harness_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    diff_harness_records: list[Any],
    runtime_transition_ids: set[str],
    runtime_owner_fields: set[str],
    runtime_invariant_ids: set[str],
    runtime_invariant_transition_ids: dict[str, set[str]],
    runtime_invariant_protected_fields: dict[str, set[str]],
    runtime_generation_domain_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    expected_harnesses = {
        record["harness_id"]: record
        for record in diff_harness_records
        if isinstance(record, dict)
        and isinstance(record.get("harness_id"), str)
        and record["harness_id"].strip()
    }
    expected_ids = set(expected_harnesses)
    covered_ids: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_diff_harness[{index}]"
        runtime_id = record["harness_id"]
        harness_label = f"{label} {runtime_id}" if runtime_id else label
        if runtime_id in covered_ids:
            failures.append(f"{label}: duplicate runtime diff harness id: {runtime_id}")
        covered_ids.add(runtime_id)

        harness_record = expected_harnesses.get(runtime_id)
        if harness_record is None:
            failures.append(
                f"{label}: harness_id does not match a diff harness id: {runtime_id}"
            )
        else:
            for field in REQUIRED_DIFF_HARNESS_FIELDS:
                if record.get(field) != harness_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match diff harness "
                        f"{runtime_id}: {record.get(field)}"
                    )

        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="enforcement_status",
                valid_statuses=VALID_ENFORCEMENT_STATUSES,
            )
        )
        failures.extend(
            _validate_runtime_diff_harness_enforcement_status(
                record=record,
                label=label,
            )
        )
        for field in (
            "harness_id",
            "check_category",
            "expected_behavior",
            "failure_mode",
            "enforcement_status",
        ):
            value = record.get(field)
            if not isinstance(value, str) or not value.strip():
                failures.append(f"{label}: {field} must be a non-empty string")

        for field in DIFF_HARNESS_LIST_FIELDS:
            values = record.get(field)
            if not isinstance(values, list) or not values:
                failures.append(f"{label}: {field} must be non-empty")
                continue
            for value_index, value in enumerate(values):
                if not isinstance(value, str) or not value.strip():
                    failures.append(
                        f"{label}: {field}[{value_index}] must be a non-empty string"
                    )

        transition_refs = record.get("transition_ids")
        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in runtime_transition_ids
                ):
                    failures.append(
                        f"{label}: transition_ids does not match runtime transition "
                        f"registry: {transition_id}"
                    )

        owner_field_refs = record.get("owner_field_refs")
        if isinstance(owner_field_refs, list):
            for field in owner_field_refs:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in runtime_owner_fields
                ):
                    failures.append(
                        f"{label}: owner_field_refs does not match runtime owner "
                        f"field registry: {field}"
                    )

        invariant_refs = record.get("invariant_ids")
        if isinstance(invariant_refs, list):
            for invariant_id in invariant_refs:
                if (
                    isinstance(invariant_id, str)
                    and invariant_id.strip()
                    and invariant_id not in runtime_invariant_ids
                ):
                    failures.append(
                        f"{label}: invariant_ids does not match runtime invariant "
                        f"registry: {invariant_id}"
                    )

        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                failures.extend(
                    _validate_invariant_transition_alignment(
                        invariant_refs=invariant_refs,
                        transition_id=transition_id,
                        invariant_transition_ids=runtime_invariant_transition_ids,
                        label=harness_label,
                        invariant_field="invariant_ids",
                        transition_field="transition_id",
                    )
                )

        if isinstance(owner_field_refs, list):
            failures.extend(
                _validate_diff_harness_owner_field_invariant_alignment(
                    owner_field_refs=owner_field_refs,
                    invariant_refs=invariant_refs,
                    invariant_protected_fields=runtime_invariant_protected_fields,
                    valid_owner_fields=runtime_owner_fields,
                    valid_invariant_ids=runtime_invariant_ids,
                    label=harness_label,
                )
            )

        generation_domain_refs = record.get("generation_domain_ids")
        if isinstance(generation_domain_refs, list):
            for domain_id in generation_domain_refs:
                if (
                    isinstance(domain_id, str)
                    and domain_id.strip()
                    and domain_id not in runtime_generation_domain_ids
                ):
                    failures.append(
                        f"{label}: generation_domain_ids does not match runtime "
                        f"generation domain registry: {domain_id}"
                    )

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime diff harness registry missing harness id(s): "
            + ", ".join(missing_ids)
        )

    return failures


def _validate_required_string_list(
    *,
    value: Any,
    required_values: set[str],
    label: str,
    item_label: str,
) -> list[str]:
    failures = _validate_list_field(value=value, label=label, field=item_label)
    if failures:
        return failures

    seen: set[str] = set()
    declared = set()
    assert isinstance(value, list)
    for index, item in enumerate(value):
        assert isinstance(item, str)
        if item in seen:
            failures.append(f"{label}: duplicate {item_label}[{index}]: {item}")
        seen.add(item)
        declared.add(item)

    missing_values = sorted(required_values - declared)
    if missing_values:
        failures.append(f"{label}: missing required value(s): {', '.join(missing_values)}")

    unknown_values = sorted(declared - required_values)
    if unknown_values:
        failures.append(f"{label}: unknown value(s): {', '.join(unknown_values)}")

    return failures


def _validate_owner_field_ref_list(
    *,
    refs: Any,
    registered_fields: set[str],
    label: str,
    registry_label: str,
    allow_empty: bool = False,
) -> list[str]:
    if not isinstance(refs, list):
        return [f"{label}: owner_field_refs must be a non-empty list"]
    failures: list[str] = []
    if not refs and not allow_empty:
        failures.append(f"{label}: owner_field_refs must be non-empty")
    for index, ref in enumerate(refs):
        if not isinstance(ref, str) or not ref.strip():
            failures.append(
                f"{label}: owner_field_refs[{index}] must be a non-empty string"
            )
    if failures:
        return failures

    seen: set[str] = set()
    assert isinstance(refs, list)
    for index, ref in enumerate(refs):
        assert isinstance(ref, str)
        if ref in seen:
            failures.append(f"{label}: duplicate owner_field_refs[{index}]: {ref}")
        seen.add(ref)
        if ref not in registered_fields:
            failures.append(
                f"{label}: owner_field_refs does not match {registry_label}: {ref}"
            )

    return failures


def _validate_coverage_diff_harness_refs(
    *,
    record: dict[str, Any],
    diff_harness_ids: set[str],
    diff_harness_transition_ids: dict[str, set[str]],
    diff_harness_owner_field_refs: dict[str, set[str]],
    diff_harness_invariant_ids: dict[str, set[str]],
    diff_harness_generation_domain_ids: dict[str, set[str]],
    label: str,
) -> list[str]:
    refs = record.get("diff_harness_refs")
    failures = _validate_list_field(
        value=refs,
        label=label,
        field="diff_harness_refs",
    )
    if failures:
        return failures

    assert isinstance(refs, list)
    transition_id = record.get("transition_id")
    seen: set[str] = set()
    covered_owner_fields: set[str] = set()
    covered_invariants: set[str] = set()
    covered_generation_domains: set[str] = set()

    for index, ref in enumerate(refs):
        assert isinstance(ref, str)
        if ref in seen:
            failures.append(f"{label}: duplicate diff_harness_refs[{index}]: {ref}")
        seen.add(ref)
        if ref not in diff_harness_ids:
            failures.append(
                f"{label}: diff_harness_refs references unknown diff harness id: {ref}"
            )
            continue
        if (
            isinstance(transition_id, str)
            and transition_id.strip()
            and transition_id not in diff_harness_transition_ids.get(ref, set())
        ):
            failures.append(
                f"{label}: diff_harness_refs[{index}] transition_id does not match "
                f"{transition_id}: {ref}"
            )
        covered_owner_fields.update(diff_harness_owner_field_refs.get(ref, set()))
        covered_invariants.update(diff_harness_invariant_ids.get(ref, set()))
        covered_generation_domains.update(
            diff_harness_generation_domain_ids.get(ref, set())
        )

    for field_name, covered_refs in (
        ("owner_field_refs", covered_owner_fields),
        ("invariant_refs", covered_invariants),
        ("generation_domain_refs", covered_generation_domains),
    ):
        record_refs = record.get(field_name)
        if not isinstance(record_refs, list):
            continue
        for index, ref in enumerate(record_refs):
            if not isinstance(ref, str) or not ref.strip():
                continue
            if ref not in covered_refs:
                failures.append(
                    f"{label}: {field_name}[{index}] lacks referenced "
                    f"diff_harness_refs coverage: {ref}"
                )

    return failures




def _validate_owner_fields(
    *,
    owner_fields_doc: Any,
    owner_fields_path: Path,
    runtime_invariant_ids: set[str],
    invariant_protected_fields: dict[str, set[str]],
) -> tuple[set[str], list[str]]:
    failures: list[str] = []
    registered_fields: set[str] = set()
    if not isinstance(owner_fields_doc, dict):
        failures.append(f"{owner_fields_path}: top-level value must be an object")
        return registered_fields, failures

    owner_records = owner_fields_doc.get("owner_fields")
    if not isinstance(owner_records, list) or not owner_records:
        failures.append(f"{owner_fields_path}: owner_fields must be a non-empty list")
        owner_records = []

    for index, record in enumerate(owner_records):
        label = f"owner_field[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_OWNER_FIELDS,
                list_fields={"invariant_checks"},
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="migration_status",
                valid_statuses=VALID_MIGRATION_STATUSES,
            )
        )

        field = record.get("field")
        if isinstance(field, str) and field.strip():
            if field in registered_fields:
                failures.append(f"{label}: duplicate field: {field}")
            registered_fields.add(field)
        failures.extend(
            _validate_invariant_check_refs(
                invariant_checks=record.get("invariant_checks"),
                runtime_invariant_ids=runtime_invariant_ids,
                label=label,
            )
        )
        failures.extend(
            _validate_owner_field_invariant_alignment(
                invariant_checks=record.get("invariant_checks"),
                owner_field=field,
                invariant_protected_fields=invariant_protected_fields,
                label=label,
            )
        )

    return registered_fields, failures


def _validate_registered_write_set(
    *,
    record: dict[str, Any],
    registered_fields: set[str],
    label: str,
) -> list[str]:
    write_set = record.get("declared_write_set")
    if not isinstance(write_set, list):
        return []

    failures: list[str] = []
    for field in write_set:
        if isinstance(field, str) and field.strip() and field not in registered_fields:
            failures.append(
                f"{label}: declared_write_set references unregistered owner field: {field}"
            )
    return failures


def _has_empty_write_set_owner_field_rationale(record: dict[str, Any]) -> bool:
    migration_notes = record.get("migration_notes")
    if not isinstance(migration_notes, list):
        return False
    return any(
        isinstance(note, str)
        and (
            "empty write set" in note.lower()
            or "read-only" in note.lower()
            or "projection-only" in note.lower()
            or "no-write" in note.lower()
        )
        for note in migration_notes
    )


def _validate_owner_field_coverage_refs(
    *,
    record: dict[str, Any],
    registered_fields: set[str],
    label: str,
    registry_label: str,
) -> list[str]:
    refs = record.get("owner_field_refs")
    failures = _validate_owner_field_ref_list(
        refs=refs,
        registered_fields=registered_fields,
        label=label,
        registry_label=registry_label,
        allow_empty=True,
    )
    if not isinstance(refs, list):
        return failures

    write_set = record.get("declared_write_set")
    if not isinstance(write_set, list):
        return failures

    declared_writes = [
        field for field in write_set if isinstance(field, str) and field.strip()
    ]
    owner_refs = [ref for ref in refs if isinstance(ref, str) and ref.strip()]
    if not declared_writes and not owner_refs:
        if not _has_empty_write_set_owner_field_rationale(record):
            failures.append(
                f"{label}: empty declared_write_set requires an owner_field_refs "
                "rationale in migration_notes"
            )
        return failures

    for ref_index, ref in enumerate(owner_refs):
        if ref not in declared_writes:
            failures.append(
                f"{label}: owner_field_refs[{ref_index}] must be declared by "
                f"declared_write_set: {ref}"
            )

    for write_index, field in enumerate(declared_writes):
        if field not in owner_refs:
            failures.append(
                f"{label}: owner_field_refs missing declared_write_set[{write_index}]: "
                f"{field}"
            )

    return failures


def _collect_string_ids(doc: Any, *, collection_key: str, id_field: str) -> set[str]:
    if not isinstance(doc, dict):
        return set()
    records = doc.get(collection_key)
    if not isinstance(records, list):
        return set()
    return {
        value
        for record in records
        if isinstance(record, dict)
        for value in [record.get(id_field)]
        if isinstance(value, str) and value.strip()
    }


def _collect_transition_ids_by_string_id(
    doc: Any, *, collection_key: str, id_field: str
) -> dict[str, str]:
    if not isinstance(doc, dict):
        return {}
    records = doc.get(collection_key)
    if not isinstance(records, list):
        return {}

    transition_ids: dict[str, str] = {}
    for record in records:
        if not isinstance(record, dict):
            continue
        value = record.get(id_field)
        transition_id = record.get("transition_id")
        if (
            isinstance(value, str)
            and value.strip()
            and isinstance(transition_id, str)
            and transition_id.strip()
        ):
            transition_ids[value] = transition_id
    return transition_ids


def _validate_invariant_check_refs(
    *,
    invariant_checks: Any,
    runtime_invariant_ids: set[str],
    label: str,
) -> list[str]:
    if not isinstance(invariant_checks, list):
        return []

    failures: list[str] = []
    for check_index, invariant_id in enumerate(invariant_checks):
        if (
            isinstance(invariant_id, str)
            and invariant_id.strip()
            and invariant_id not in runtime_invariant_ids
        ):
            failures.append(
                f"{label}: invariant_checks[{check_index}] does not match "
                f"runtime invariant registry: {invariant_id}"
            )
    return failures


def _validate_record_invariant_refs(
    *,
    invariant_refs: Any,
    invariant_ids: set[str],
    invariant_transition_ids: dict[str, set[str]],
    invariant_protected_fields: dict[str, set[str]],
    transition_id: Any,
    declared_write_set: Any,
    label: str,
) -> list[str]:
    if not isinstance(invariant_refs, list):
        return []

    failures: list[str] = []
    seen: set[str] = set()
    protected_fields: set[str] = set()
    has_transition_id = isinstance(transition_id, str) and bool(transition_id.strip())

    for index, invariant_id in enumerate(invariant_refs):
        if not isinstance(invariant_id, str) or not invariant_id.strip():
            continue
        if invariant_id in seen:
            failures.append(f"{label}: invariant_refs[{index}] duplicates {invariant_id}")
            continue
        seen.add(invariant_id)
        if invariant_id not in invariant_ids:
            failures.append(
                f"{label}: invariant_refs[{index}] does not match invariant registry: "
                f"{invariant_id}"
            )
            continue
        if (
            has_transition_id
            and transition_id not in invariant_transition_ids.get(invariant_id, set())
        ):
            failures.append(
                f"{label}: invariant_refs[{index}] transition_id does not match "
                f"{transition_id}: {invariant_id}"
            )
            continue
        protected_fields.update(invariant_protected_fields.get(invariant_id, set()))

    if not isinstance(declared_write_set, list):
        return failures

    for index, field in enumerate(declared_write_set):
        if not isinstance(field, str) or not field.strip():
            continue
        if field not in protected_fields:
            failures.append(
                f"{label}: invariant_refs lack collective coverage for "
                f"declared_write_set[{index}] owner field: {field}"
            )

    return failures


def _validate_allowed_direct_writes(
    *,
    record: dict[str, Any],
    registered_fields: set[str],
    label: str,
) -> list[str]:
    writes = record.get("allowed_direct_writes")
    if not isinstance(writes, list):
        return [f"{label}: allowed_direct_writes must be a list"]

    failures: list[str] = []
    seen: set[str] = set()
    for index, field in enumerate(writes):
        if not isinstance(field, str) or not field.strip():
            failures.append(
                f"{label}: allowed_direct_writes[{index}] must be a non-empty string"
            )
            continue
        if field in seen:
            failures.append(
                f"{label}: duplicate allowed_direct_writes[{index}]: {field}"
            )
        seen.add(field)
        if field not in registered_fields:
            failures.append(
                f"{label}: allowed_direct_writes references unregistered owner field: {field}"
            )
    return failures


def _validate_allowed_direct_writes_within_transition(
    *,
    record: dict[str, Any],
    transition_records: dict[str, dict[str, Any]],
    label: str,
) -> list[str]:
    writes = record.get("allowed_direct_writes")
    transition_id = record.get("transition_id")
    if not isinstance(writes, list):
        return []
    if not isinstance(transition_id, str) or not transition_id.strip():
        return []

    transition_record = transition_records.get(transition_id)
    if transition_record is None:
        return []

    declared_write_set = transition_record.get("declared_write_set")
    if not isinstance(declared_write_set, list):
        return []

    declared_fields = {
        field
        for field in declared_write_set
        if isinstance(field, str) and field.strip()
    }
    failures: list[str] = []
    for index, field in enumerate(writes):
        if not isinstance(field, str) or not field.strip():
            continue
        if field not in declared_fields:
            failures.append(
                f"{label}: allowed_direct_writes[{index}] outside transition "
                f"declared_write_set for {transition_id}: {field}"
            )
    return failures


def _validate_transition_sequence_refs(
    *,
    record: dict[str, Any],
    transition_sequence_records: list[Any],
    label: str,
) -> tuple[list[str], list[dict[str, Any]]]:
    refs = record.get("transition_sequence_refs")
    transition_id = record.get("transition_id")
    if not isinstance(refs, list) or not refs:
        return [f"{label}: transition_sequence_refs must be a non-empty list"], []

    sequences: dict[str, dict[str, Any]] = {
        sequence["scenario_id"]: sequence
        for sequence in transition_sequence_records
        if isinstance(sequence, dict)
        and isinstance(sequence.get("scenario_id"), str)
        and sequence["scenario_id"].strip()
    }
    failures: list[str] = []
    seen: set[str] = set()
    matching_steps: list[dict[str, Any]] = []

    for index, sequence_ref in enumerate(refs):
        if not isinstance(sequence_ref, str) or not sequence_ref.strip():
            failures.append(
                f"{label}: transition_sequence_refs[{index}] must be a non-empty string"
            )
            continue
        if sequence_ref in seen:
            failures.append(
                f"{label}: duplicate transition_sequence_refs[{index}]: {sequence_ref}"
            )
        seen.add(sequence_ref)
        sequence = sequences.get(sequence_ref)
        if sequence is None:
            failures.append(
                f"{label}: transition_sequence_refs references unknown transition "
                f"sequence: {sequence_ref}"
            )
            continue
        if not isinstance(transition_id, str) or not transition_id.strip():
            continue
        steps = sequence.get("steps")
        sequence_matching_steps = (
            [
                step
                for step in steps
                if isinstance(step, dict) and step.get("transition_id") == transition_id
            ]
            if isinstance(steps, list)
            else []
        )
        if not sequence_matching_steps:
            failures.append(
                f"{label}: transition_sequence_refs must include at least one step "
                f"covering transition_id {transition_id} in "
                f"transition_sequence_refs[{index}]: {sequence_ref}"
            )
        matching_steps.extend(sequence_matching_steps)

    if isinstance(transition_id, str) and transition_id.strip() and not matching_steps:
        failures.append(
            f"{label}: transition_sequence_refs must include at least one step "
            f"covering transition_id {transition_id}"
        )

    return failures, matching_steps


def _dispatch_surfaces_by_id(
    dispatch_surface_records: list[Any],
) -> dict[str, dict[str, Any]]:
    return {
        surface["surface_id"]: surface
        for surface in dispatch_surface_records
        if isinstance(surface, dict)
        and isinstance(surface.get("surface_id"), str)
        and surface["surface_id"].strip()
    }


def _generation_domains_by_id(
    generation_domain_records: list[Any],
) -> dict[str, dict[str, Any]]:
    return {
        domain["domain_id"]: domain
        for domain in generation_domain_records
        if isinstance(domain, dict)
        and isinstance(domain.get("domain_id"), str)
        and domain["domain_id"].strip()
    }


def _generation_domain_covers_transition(
    domain: dict[str, Any], transition_id: Any
) -> bool:
    coverage_transition_ids = domain.get("coverage_transition_ids")
    return (
        isinstance(transition_id, str)
        and transition_id.strip()
        and isinstance(coverage_transition_ids, list)
        and transition_id in coverage_transition_ids
    )


def _validate_generation_domain_refs(
    *,
    generation_domain_refs: Any,
    generation_domain_records: list[Any],
    transition_id: Any,
    label: str,
) -> list[str]:
    failures = _validate_list_field(
        value=generation_domain_refs,
        label=label,
        field="generation_domain_refs",
    )
    if failures:
        return failures

    assert isinstance(generation_domain_refs, list)
    domains = _generation_domains_by_id(generation_domain_records)
    seen: set[str] = set()

    for index, domain_id in enumerate(generation_domain_refs):
        if not isinstance(domain_id, str) or not domain_id.strip():
            failures.append(
                f"{label}: generation_domain_refs[{index}] must be a non-empty string"
            )
            continue
        if domain_id in seen:
            failures.append(
                f"{label}: duplicate generation_domain_refs[{index}]: {domain_id}"
            )
        seen.add(domain_id)
        domain = domains.get(domain_id)
        if domain is None:
            failures.append(
                f"{label}: generation_domain_refs references unknown generation domain: {domain_id}"
            )
            continue
        if not _generation_domain_covers_transition(domain, transition_id):
            failures.append(
                f"{label}: generation_domain_refs[{index}] transition_id does not match "
                f"{transition_id}: {domain_id}"
            )

    return failures


def _validate_dispatch_surface_refs(
    *,
    record: dict[str, Any],
    dispatch_surface_records: list[Any],
    label: str,
) -> list[str]:
    refs = record.get("dispatch_surface_refs")
    transition_id = record.get("transition_id")
    failures = _validate_list_field(
        value=refs,
        label=label,
        field="dispatch_surface_refs",
    )
    if failures:
        return failures

    assert isinstance(refs, list)
    surfaces = _dispatch_surfaces_by_id(dispatch_surface_records)
    seen: set[str] = set()
    for index, surface_id in enumerate(refs):
        if not isinstance(surface_id, str) or not surface_id.strip():
            failures.append(
                f"{label}: dispatch_surface_refs[{index}] must be a non-empty string"
            )
            continue
        if surface_id in seen:
            failures.append(f"{label}: duplicate dispatch_surface_refs[{index}]: {surface_id}")
        seen.add(surface_id)
        surface = surfaces.get(surface_id)
        if surface is None:
            failures.append(
                f"{label}: dispatch_surface_refs references unknown dispatch surface: {surface_id}"
            )
            continue
        if (
            isinstance(transition_id, str)
            and transition_id.strip()
            and surface.get("transition_id") != transition_id
        ):
            failures.append(
                f"{label}: dispatch_surface_refs[{index}] transition_id does not match "
                f"{transition_id}: {surface_id}"
            )

    return failures


def _validate_dispatch_surface_transition_sequence_coverage(
    *,
    record: dict[str, Any],
    transition_sequence_records: list[Any],
    diff_harness_owner_field_refs: dict[str, set[str]],
    invariant_protected_fields: dict[str, set[str]],
    label: str,
) -> list[str]:
    failures, matching_steps = _validate_transition_sequence_refs(
        record=record,
        transition_sequence_records=transition_sequence_records,
        label=label,
    )
    writes = record.get("allowed_direct_writes")
    if not isinstance(writes, list):
        return failures

    diff_covered_fields: set[str] = set()
    invariant_covered_fields: set[str] = set()
    for step in matching_steps:
        diff_harness_ids = step.get("diff_harness_ids")
        if isinstance(diff_harness_ids, list):
            for harness_id in diff_harness_ids:
                if isinstance(harness_id, str) and harness_id.strip():
                    diff_covered_fields.update(
                        diff_harness_owner_field_refs.get(harness_id, set())
                    )
        invariant_ids = step.get("invariant_ids")
        if isinstance(invariant_ids, list):
            for invariant_id in invariant_ids:
                if isinstance(invariant_id, str) and invariant_id.strip():
                    invariant_covered_fields.update(
                        invariant_protected_fields.get(invariant_id, set())
                    )

    for write_index, field in enumerate(writes):
        if not isinstance(field, str) or not field.strip():
            continue
        if field not in diff_covered_fields:
            failures.append(
                f"{label}: allowed_direct_writes[{write_index}] lacks "
                f"transition-sequence diff harness coverage for owner field: {field}"
            )
        if field not in invariant_covered_fields:
            failures.append(
                f"{label}: allowed_direct_writes[{write_index}] lacks "
                f"transition-sequence invariant coverage for owner field: {field}"
            )

    return failures


def _validate_appstate_diff_harness(
    *,
    diff_harness_doc: Any,
    diff_harness_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
    invariant_ids: set[str],
    invariant_transition_ids: dict[str, set[str]],
    invariant_protected_fields: dict[str, set[str]],
    generation_domain_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(diff_harness_doc, dict):
        failures.append(f"{diff_harness_path}: top-level value must be an object")
        return failures

    records = diff_harness_doc.get("diff_harness_checks")
    if not isinstance(records, list) or not records:
        failures.append(
            f"{diff_harness_path}: diff_harness_checks must be a non-empty list"
        )
        records = []

    harness_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(records):
        label = f"diff_harness_check[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_DIFF_HARNESS_FIELDS,
                list_fields=DIFF_HARNESS_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="enforcement_status",
                valid_statuses=VALID_ENFORCEMENT_STATUSES,
            )
        )

        harness_id = record.get("harness_id")
        harness_label = label
        if isinstance(harness_id, str) and harness_id.strip():
            harness_label = f"{label} {harness_id}"
            if harness_id in harness_ids:
                failures.append(f"{label}: duplicate harness_id: {harness_id}")
            harness_ids.add(harness_id)

        check_category = record.get("check_category")
        if isinstance(check_category, str) and check_category.strip():
            covered_categories.add(check_category)
            if check_category not in REQUIRED_DIFF_HARNESS_CATEGORIES:
                failures.append(f"{label}: unknown check_category: {check_category}")

        transition_refs = record.get("transition_ids")
        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in transition_ids
                ):
                    failures.append(
                        f"{label}: transition_ids references unknown transition id: {transition_id}"
                    )

        owner_field_refs = record.get("owner_field_refs")
        if isinstance(owner_field_refs, list):
            for field in owner_field_refs:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in registered_owner_fields
                ):
                    failures.append(
                        f"{label}: owner_field_refs references unregistered owner field: {field}"
                    )

        invariant_refs = record.get("invariant_ids")
        if isinstance(invariant_refs, list):
            for invariant_id in invariant_refs:
                if (
                    isinstance(invariant_id, str)
                    and invariant_id.strip()
                    and invariant_id not in invariant_ids
                ):
                    failures.append(
                        f"{label}: invariant_ids references unknown invariant id: {invariant_id}"
                    )

        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                failures.extend(
                    _validate_invariant_transition_alignment(
                        invariant_refs=invariant_refs,
                        transition_id=transition_id,
                        invariant_transition_ids=invariant_transition_ids,
                        label=harness_label,
                        invariant_field="invariant_ids",
                        transition_field="transition_id",
                    )
                )

        if isinstance(owner_field_refs, list):
            failures.extend(
                _validate_diff_harness_owner_field_invariant_alignment(
                    owner_field_refs=owner_field_refs,
                    invariant_refs=invariant_refs,
                    invariant_protected_fields=invariant_protected_fields,
                    valid_owner_fields=registered_owner_fields,
                    valid_invariant_ids=invariant_ids,
                    label=harness_label,
                )
            )

        generation_domain_refs = record.get("generation_domain_ids")
        if isinstance(generation_domain_refs, list):
            for domain_id in generation_domain_refs:
                if (
                    isinstance(domain_id, str)
                    and domain_id.strip()
                    and domain_id not in generation_domain_ids
                ):
                    failures.append(
                        f"{label}: generation_domain_ids references unknown generation domain id: {domain_id}"
                    )

    missing_categories = sorted(REQUIRED_DIFF_HARNESS_CATEGORIES - covered_categories)
    if missing_categories:
        failures.append(
            "diff harness missing required check_category/categories: "
            + ", ".join(missing_categories)
        )

    return failures


def _validate_source_path(
    value: Any, *, label: str, repository_root: Path = REPO_ROOT
) -> list[str]:
    if not isinstance(value, str) or not value.strip():
        return [f"{label}: source_path must be a non-empty string"]

    source_path = value.strip()
    path = Path(source_path)
    if (
        path.is_absolute()
        or "\\" in source_path
        or any(part == ".." for part in path.parts)
    ):
        return [f"{label}: source_path must be a relative repository path"]

    source_file = (repository_root / source_path).resolve()
    source_root = (repository_root / "src").resolve()
    try:
        source_file.relative_to(source_root)
    except ValueError:
        return [f"{label}: source_path must point inside src/: {source_path}"]

    if not source_file.is_file():
        return [f"{label}: source_path does not exist: {source_path}"]
    return []


def _validate_source_boundary_refs(
    value: Any, *, label: str, repository_root: Path = REPO_ROOT
) -> list[str]:
    failures = _validate_list_field(
        value=value,
        label=label,
        field="source_boundary_refs",
    )
    if failures:
        return failures

    assert isinstance(value, list)
    seen: set[str] = set()
    for index, source_path in enumerate(value):
        if not isinstance(source_path, str) or not source_path.strip():
            failures.append(
                f"{label}: source_boundary_refs[{index}] must be a non-empty string"
            )
            continue
        normalized = source_path.strip()
        if normalized in seen:
            failures.append(
                f"{label}: duplicate source_boundary_refs[{index}]: {normalized}"
            )
        seen.add(normalized)
        failures.extend(
            _validate_source_path(
                normalized,
                label=f"{label}: source_boundary_refs[{index}]",
                repository_root=repository_root,
            )
        )
    return failures


def _validate_entry_symbol_or_path(value: Any, *, label: str) -> list[str]:
    if not isinstance(value, str) or not value.strip():
        return [f"{label}: entry_symbol_or_path must be a non-empty string"]

    entry = value.strip()
    path = Path(entry)
    if (
        path.is_absolute()
        or "\\" in entry
        or any(part == ".." for part in path.parts)
        or re.search(r"\s", entry)
    ):
        return [f"{label}: entry_symbol_or_path is malformed: {entry}"]
    return []


def _entry_symbol_or_path_is_anchored(source: str, entry: str) -> bool:
    if ENTRY_SYMBOL_RE.fullmatch(entry):
        return (
            re.search(
                rf"(?<![A-Za-z0-9_]){re.escape(entry)}\s*\(",
                source,
            )
            is not None
        )
    return entry in source


def _validate_dispatch_surface_source_anchor(
    *,
    source_path: str,
    entry_symbol_or_path: str,
    label: str,
    repository_root: Path = REPO_ROOT,
) -> list[str]:
    source_file = repository_root / source_path.strip()
    entry = entry_symbol_or_path.strip()
    try:
        source = source_file.read_text(encoding="utf-8")
    except OSError as exc:
        return [f"{label}: source_path could not be read: {source_path}: {exc}"]

    if not _entry_symbol_or_path_is_anchored(source, entry):
        return [
            f"{label}: entry_symbol_or_path not found in source_path "
            f"{source_path}: {entry}"
        ]
    return []


def _runtime_validation_callsites_by_id(
    source_root: Path, pattern: re.Pattern[str]
) -> tuple[dict[str, set[str]], list[str]]:
    callsites_by_id: dict[str, set[str]] = {}
    failures: list[str] = []
    if not source_root.is_dir():
        return {}, [f"{source_root}: src root does not exist"]

    for source_file in sorted(source_root.rglob("*")):
        if not source_file.is_file() or source_file.suffix not in {".c", ".h"}:
            continue
        try:
            source = source_file.read_text(encoding="utf-8")
        except OSError as exc:
            failures.append(f"{source_file}: failed to read source file: {exc}")
            continue

        relative_path = source_file.relative_to(source_root.parent).as_posix()
        for match in pattern.finditer(source):
            callsites_by_id.setdefault(match.group(1), set()).add(relative_path)
    return callsites_by_id, failures


def _runtime_validation_callsite_ids_in_file(
    source_file: Path, pattern: re.Pattern[str]
) -> tuple[set[str], list[str]]:
    try:
        source = source_file.read_text(encoding="utf-8")
    except OSError as exc:
        return set(), [f"{source_file}: failed to read source file: {exc}"]
    return {match.group(1) for match in pattern.finditer(source)}, []


def _runtime_validation_callsite_exists_in_file(
    source_file: Path, pattern: re.Pattern[str]
) -> tuple[bool, list[str]]:
    try:
        source = source_file.read_text(encoding="utf-8")
    except OSError as exc:
        return False, [f"{source_file}: failed to read source file: {exc}"]
    return bool(pattern.search(source)), []


def _runtime_dispatch_surface_callsites_by_id(
    source_root: Path,
) -> tuple[dict[str, set[str]], list[str]]:
    return _runtime_validation_callsites_by_id(
        source_root, DISPATCH_SURFACE_CALLSITE_RE
    )


def _validate_runtime_dispatch_surface_callsites(
    *,
    runtime_records: list[dict[str, Any]],
    repository_root: Path,
    action_runtime_path: Path,
) -> list[str]:
    if (
        action_runtime_path.parent.name != "core"
        or action_runtime_path.parent.parent.name != "src"
    ):
        return []
    failures: list[str] = []
    for record in runtime_records:
        if not isinstance(record, dict):
            continue
        surface_id = record.get("surface_id")
        source_path = record.get("source_path")
        if not isinstance(surface_id, str) or not surface_id.strip():
            continue
        if not isinstance(source_path, str) or not source_path.strip():
            continue
        source_file = repository_root / source_path
        callsite_ids, read_failures = _runtime_validation_callsite_ids_in_file(
            source_file, DISPATCH_SURFACE_CALLSITE_RE
        )
        failures.extend(read_failures)
        if read_failures:
            continue
        if surface_id not in callsite_ids:
            failures.append(
                f"{source_path}: {surface_id}: missing runtime validation callsite"
            )
    return failures


def _decoded_action_source_boundary_paths(
    action_records: list[dict[str, Any]],
    dispatch_surface_records: list[Any],
) -> list[str]:
    dispatch_surfaces = _dispatch_surfaces_by_id(dispatch_surface_records)
    source_paths: list[str] = []
    seen_paths: set[str] = set()
    for record in action_records:
        if not isinstance(record, dict):
            continue
        refs = record.get("dispatch_surface_refs")
        if not isinstance(refs, list):
            continue
        for surface_id in refs:
            if not isinstance(surface_id, str) or not surface_id.strip():
                continue
            surface = dispatch_surfaces.get(surface_id)
            if surface is None:
                continue
            if (
                surface.get("category")
                not in DECODED_ACTION_DISPATCH_SURFACE_CATEGORIES
            ):
                continue
            source_path = surface.get("source_path")
            if (
                not isinstance(source_path, str)
                or not source_path.strip()
                or source_path in seen_paths
            ):
                continue
            source_paths.append(source_path)
            seen_paths.add(source_path)
    return source_paths


def _validate_runtime_action_callsites(
    *,
    action_records: list[dict[str, Any]],
    dispatch_surface_records: list[Any],
    repository_root: Path,
    action_runtime_path: Path,
) -> list[str]:
    if (
        action_runtime_path.parent.name != "core"
        or action_runtime_path.parent.parent.name != "src"
    ):
        return []
    failures: list[str] = []
    for source_path in _decoded_action_source_boundary_paths(
        action_records, dispatch_surface_records
    ):
        source_file = repository_root / source_path
        has_callsite, read_failures = _runtime_validation_callsite_exists_in_file(
            source_file, KEY_ACTION_CALLSITE_RE
        )
        failures.extend(read_failures)
        if read_failures:
            continue
        if not has_callsite:
            failures.append(
                f"{source_path}: AppStateValidatedKeyAction: missing runtime "
                f"validation callsite"
            )
    return failures


def _validate_runtime_event_callsites(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_dispatch_surface_records: list[dict[str, Any]],
    repository_root: Path,
    action_runtime_path: Path,
) -> list[str]:
    if (
        action_runtime_path.parent.name != "core"
        or action_runtime_path.parent.parent.name != "src"
    ):
        return []
    failures: list[str] = []
    dispatch_surfaces = _dispatch_surfaces_by_id(runtime_dispatch_surface_records)
    for record in runtime_records:
        if not isinstance(record, dict):
            continue
        event_id = record.get("event_id")
        refs = record.get("dispatch_surface_refs")
        if not isinstance(event_id, str) or not event_id.strip():
            continue
        if not isinstance(refs, list):
            continue
        source_paths: list[str] = []
        seen_paths: set[str] = set()
        for surface_id in refs:
            if not isinstance(surface_id, str) or not surface_id.strip():
                continue
            surface = dispatch_surfaces.get(surface_id)
            if surface is None:
                continue
            source_path = surface.get("source_path")
            if (
                not isinstance(source_path, str)
                or not source_path.strip()
                or source_path in seen_paths
            ):
                continue
            source_paths.append(source_path)
            seen_paths.add(source_path)
        if not source_paths:
            continue

        event_found = False
        for source_path in source_paths:
            source_file = repository_root / source_path
            callsite_ids, read_failures = _runtime_validation_callsite_ids_in_file(
                source_file, EVENT_CALLSITE_RE
            )
            failures.extend(read_failures)
            if read_failures:
                continue
            if event_id in callsite_ids:
                event_found = True
                break
        if not event_found:
            failures.append(
                f"{', '.join(source_paths)}: {event_id}: missing runtime validation callsite"
            )
    return failures


def _validate_dispatch_surfaces(
    *,
    dispatch_surfaces_doc: Any,
    dispatch_surfaces_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    runtime_dispatch_surface_ids: set[str],
    registered_owner_fields: set[str],
    invariant_protected_fields_by_surface: dict[str, set[str]],
    transition_sequence_records: list[Any],
    diff_harness_owner_field_refs: dict[str, set[str]],
    invariant_protected_fields: dict[str, set[str]],
    repository_root: Path = REPO_ROOT,
) -> list[str]:
    failures: list[str] = []
    if not isinstance(dispatch_surfaces_doc, dict):
        failures.append(f"{dispatch_surfaces_path}: top-level value must be an object")
        return failures

    surface_records = dispatch_surfaces_doc.get("dispatch_surfaces")
    if not isinstance(surface_records, list) or not surface_records:
        failures.append(
            f"{dispatch_surfaces_path}: dispatch_surfaces must be a non-empty list"
        )
        surface_records = []

    surface_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(surface_records):
        label = f"dispatch_surface[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_DISPATCH_SURFACE_FIELDS
                - {"allowed_direct_writes"},
                list_fields=DISPATCH_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(_validate_event_boundary_status(record=record, label=label))
        if "allowed_direct_writes" not in record:
            failures.append(f"{label}: missing required field(s): allowed_direct_writes")
        else:
            failures.extend(
                _validate_allowed_direct_writes(
                    record=record,
                    registered_fields=registered_owner_fields,
                    label=label,
                )
            )

        surface_id = record.get("surface_id")
        if isinstance(surface_id, str) and surface_id.strip():
            if surface_id in surface_ids:
                failures.append(f"{label}: duplicate surface_id: {surface_id}")
            surface_ids.add(surface_id)
            if (
                surface_id in runtime_dispatch_surface_ids
                and record.get("boundary_status") != "covered_by_transition_record"
            ):
                failures.append(
                    f"{label}: runtime-backed dispatch surface {surface_id} must use "
                    "covered_by_transition_record, not "
                    f"{record.get('boundary_status')}"
                )

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            covered_categories.add(category)
            if category not in REQUIRED_DISPATCH_SURFACE_CATEGORIES:
                failures.append(f"{label}: unknown category: {category}")

        transition_id = record.get("transition_id")
        if isinstance(transition_id, str) and transition_id.strip():
            if transition_id not in transition_ids:
                failures.append(
                    f"{label}: transition_id does not match a transition id: {transition_id}"
                )
            failures.extend(
                _validate_allowed_direct_writes_within_transition(
                    record=record,
                    transition_records=transition_ids,
                    label=label,
                )
            )
            failures.extend(
                _validate_allowed_direct_writes_have_invariant_coverage(
                    record=record,
                    protected_fields_by_surface=invariant_protected_fields_by_surface,
                    label=label,
                )
            )
        failures.extend(
            _validate_dispatch_surface_transition_sequence_coverage(
                record=record,
                transition_sequence_records=transition_sequence_records,
                diff_harness_owner_field_refs=diff_harness_owner_field_refs,
                invariant_protected_fields=invariant_protected_fields,
                label=label,
            )
        )

        source_path = record.get("source_path")
        entry_symbol_or_path = record.get("entry_symbol_or_path")
        source_failures = _validate_source_path(
            source_path, label=label, repository_root=repository_root
        )
        entry_failures = _validate_entry_symbol_or_path(
            entry_symbol_or_path, label=label
        )
        failures.extend(source_failures)
        failures.extend(entry_failures)
        if (
            not source_failures
            and not entry_failures
            and isinstance(source_path, str)
            and isinstance(entry_symbol_or_path, str)
        ):
            failures.extend(
                _validate_dispatch_surface_source_anchor(
                    source_path=source_path,
                    entry_symbol_or_path=entry_symbol_or_path,
                    label=label,
                    repository_root=repository_root,
                )
            )

    missing_categories = sorted(
        REQUIRED_DISPATCH_SURFACE_CATEGORIES - covered_categories
    )
    if missing_categories:
        failures.append(
            "dispatch surfaces missing required category/categories: "
            + ", ".join(missing_categories)
        )

    return failures


def _validate_invariants(
    *,
    invariants_doc: Any,
    invariants_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
    dispatch_surface_ids: set[str],
    diff_harness_owner_field_refs: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(invariants_doc, dict):
        failures.append(f"{invariants_path}: top-level value must be an object")
        return failures

    invariant_records = invariants_doc.get("invariants")
    if not isinstance(invariant_records, list) or not invariant_records:
        failures.append(f"{invariants_path}: invariants must be a non-empty list")
        invariant_records = []

    invariant_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(invariant_records):
        label = f"invariant[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_INVARIANT_FIELDS - {"dispatch_surface_ids"},
                list_fields=INVARIANT_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="enforcement_status",
                valid_statuses=VALID_ENFORCEMENT_STATUSES,
            )
        )
        if "dispatch_surface_ids" not in record:
            failures.append(f"{label}: missing required field(s): dispatch_surface_ids")
        else:
            surface_refs_value = record.get("dispatch_surface_ids")
            if not isinstance(surface_refs_value, list):
                failures.append(
                    f"{label}: dispatch_surface_ids must be a list"
                )
            else:
                for surface_index, surface_id in enumerate(surface_refs_value):
                    if not isinstance(surface_id, str) or not surface_id.strip():
                        failures.append(
                            f"{label}: dispatch_surface_ids[{surface_index}] must be a non-empty string"
                        )

        invariant_id = record.get("invariant_id")
        if isinstance(invariant_id, str) and invariant_id.strip():
            if invariant_id in invariant_ids:
                failures.append(f"{label}: duplicate invariant_id: {invariant_id}")
            invariant_ids.add(invariant_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            covered_categories.add(category)
            if category not in REQUIRED_INVARIANT_CATEGORIES:
                failures.append(f"{label}: unknown category: {category}")

        protected_fields = record.get("protected_fields")
        if isinstance(protected_fields, list):
            for field in protected_fields:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in registered_owner_fields
                ):
                    failures.append(
                        f"{label}: protected_fields references unregistered owner field: {field}"
                    )
        failures.extend(
            _validate_invariant_diff_harness_owner_field_coverage(
                protected_fields=protected_fields,
                invariant_id=invariant_id,
                diff_harness_owner_field_refs=diff_harness_owner_field_refs,
                label=label,
                coverage_label="diff harness",
                registered_owner_fields=registered_owner_fields,
            )
        )

        transition_refs = record.get("transition_ids")
        if isinstance(transition_refs, list):
            for transition_id in transition_refs:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in transition_ids
                ):
                    failures.append(
                        f"{label}: transition_ids references unknown transition id: {transition_id}"
                    )

        surface_refs = record.get("dispatch_surface_ids")
        if isinstance(surface_refs, list):
            if not surface_refs:
                migration_notes = record.get("migration_notes")
                has_cross_cutting_note = (
                    isinstance(migration_notes, list)
                    and any(
                        isinstance(note, str)
                        and "cross-cutting" in note.lower()
                        for note in migration_notes
                    )
                )
                if not has_cross_cutting_note:
                    failures.append(
                        f"{label}: empty dispatch_surface_ids requires a cross-cutting migration_notes explanation"
                    )
            for surface_id in surface_refs:
                if (
                    isinstance(surface_id, str)
                    and surface_id.strip()
                    and surface_id not in dispatch_surface_ids
                ):
                    failures.append(
                        f"{label}: dispatch_surface_ids references unknown dispatch surface id: {surface_id}"
                    )

    missing_categories = sorted(REQUIRED_INVARIANT_CATEGORIES - covered_categories)
    if missing_categories:
        failures.append(
            "invariants missing required category/categories: "
            + ", ".join(missing_categories)
        )

    return failures


def _validate_generation_transition_refs(
    *,
    value: Any,
    label: str,
    transition_ids: dict[str, dict[str, Any]],
    migration_notes: Any,
    field: str,
) -> list[str]:
    failures: list[str] = []
    if not isinstance(value, list):
        return [f"{label}: {field} must be a list"]

    for index, transition_id in enumerate(value):
        if not isinstance(transition_id, str) or not transition_id.strip():
            failures.append(f"{label}: {field}[{index}] must be a non-empty string")
            continue
        if transition_id not in transition_ids:
            failures.append(
                f"{label}: {field} references unknown transition id: {transition_id}"
            )

    if field == "advances_on_transition_ids" and not value:
        has_projection_note = (
            isinstance(migration_notes, list)
            and any(
                isinstance(note, str)
                and (
                    "read-only" in note.lower()
                    or "projection-only" in note.lower()
                )
                for note in migration_notes
            )
        )
        if not has_projection_note:
            failures.append(
                f"{label}: empty {field} requires a read-only/projection-only migration_notes explanation"
            )

    return failures


def _has_projection_only_note(migration_notes: Any) -> bool:
    return isinstance(migration_notes, list) and any(
        isinstance(note, str)
        and ("read-only" in note.lower() or "projection-only" in note.lower())
        for note in migration_notes
    )


def _validate_generation_domains(
    *,
    generation_domains_doc: Any,
    generation_domains_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    registered_owner_fields: set[str],
) -> tuple[set[str], list[str]]:
    failures: list[str] = []
    generation_owner_fields: set[str] = set()
    if not isinstance(generation_domains_doc, dict):
        failures.append(f"{generation_domains_path}: top-level value must be an object")
        return generation_owner_fields, failures

    domain_records = generation_domains_doc.get("generation_domains")
    if not isinstance(domain_records, list) or not domain_records:
        failures.append(
            f"{generation_domains_path}: generation_domains must be a non-empty list"
        )
        domain_records = []

    domain_ids: set[str] = set()
    covered_categories: set[str] = set()
    for index, record in enumerate(domain_records):
        label = f"generation_domain[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_GENERATION_DOMAIN_FIELDS
                - {"advances_on_transition_ids"},
                list_fields=GENERATION_DOMAIN_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="enforcement_status",
                valid_statuses=VALID_ENFORCEMENT_STATUSES,
            )
        )

        if "advances_on_transition_ids" not in record:
            failures.append(
                f"{label}: missing required field(s): advances_on_transition_ids"
            )
        else:
            failures.extend(
                _validate_generation_transition_refs(
                    value=record.get("advances_on_transition_ids"),
                    label=label,
                    transition_ids=transition_ids,
                    migration_notes=record.get("migration_notes"),
                    field="advances_on_transition_ids",
                )
            )

        if "coverage_transition_ids" not in record:
            failures.append(
                f"{label}: missing required field(s): coverage_transition_ids"
            )
        else:
            failures.extend(
                _validate_generation_transition_refs(
                    value=record.get("coverage_transition_ids"),
                    label=label,
                    transition_ids=transition_ids,
                    migration_notes=record.get("migration_notes"),
                    field="coverage_transition_ids",
                )
            )

        domain_id = record.get("domain_id")
        if isinstance(domain_id, str) and domain_id.strip():
            if domain_id in domain_ids:
                failures.append(f"{label}: duplicate domain_id: {domain_id}")
            domain_ids.add(domain_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            covered_categories.add(category)
            if category not in REQUIRED_GENERATION_DOMAIN_CATEGORIES:
                failures.append(f"{label}: unknown category: {category}")

        generation_owner_field = record.get("generation_owner_field")
        if isinstance(generation_owner_field, str) and generation_owner_field.strip():
            generation_owner_fields.add(generation_owner_field)
            if generation_owner_field not in registered_owner_fields:
                failures.append(
                    f"{label}: generation_owner_field references unregistered owner field: {generation_owner_field}"
                )

        identity_fields = record.get("identity_fields")
        if isinstance(identity_fields, list):
            for field in identity_fields:
                if (
                    isinstance(field, str)
                    and field.strip()
                    and field not in registered_owner_fields
                ):
                    failures.append(
                        f"{label}: identity_fields references unregistered owner field: {field}"
                    )

        coverage_transition_ids = record.get("coverage_transition_ids")
        advances_on_transition_ids = record.get("advances_on_transition_ids")
        if isinstance(coverage_transition_ids, list) and isinstance(
            advances_on_transition_ids, list
        ):
            declared_coverage = {
                transition_id
                for transition_id in coverage_transition_ids
                if isinstance(transition_id, str) and transition_id.strip()
            }
            for transition_id in advances_on_transition_ids:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in declared_coverage
                ):
                    failures.append(
                        f"{label}: advances_on_transition_ids must be covered by "
                        f"coverage_transition_ids: {transition_id}"
                    )
            declared_advances = {
                transition_id
                for transition_id in advances_on_transition_ids
                if isinstance(transition_id, str) and transition_id.strip()
            }
            for transition_id in coverage_transition_ids:
                if (
                    isinstance(transition_id, str)
                    and transition_id.strip()
                    and transition_id not in declared_advances
                    and not _has_projection_only_note(record.get("migration_notes"))
                ):
                    failures.append(
                        f"{label}: coverage_transition_ids without "
                        "advances_on_transition_ids requires a read-only/"
                        f"projection-only migration_notes explanation: {transition_id}"
                    )

    missing_categories = sorted(
        REQUIRED_GENERATION_DOMAIN_CATEGORIES - covered_categories
    )
    if missing_categories:
        failures.append(
            "generation domains missing required category/categories: "
            + ", ".join(missing_categories)
        )

    return generation_owner_fields, failures


def _validate_transition_generation_effects(
    *,
    transitions: list[Any],
    generation_owner_fields: set[str],
) -> list[str]:
    failures: list[str] = []
    registered_names = set(generation_owner_fields)
    registered_names.update(field.rsplit(".", 1)[-1] for field in generation_owner_fields)

    for index, record in enumerate(transitions):
        if not isinstance(record, dict):
            continue
        effect = record.get("generation_effect")
        if not isinstance(effect, str):
            continue
        for field in sorted(set(GENERATION_FIELD_RE.findall(effect))):
            if field not in registered_names:
                failures.append(
                    f"transition[{index}]: generation_effect names unregistered generation field: {field}"
                )

    return failures


def _validate_runtime_event_coverage_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    event_coverage_doc: Any,
    transition_ids: dict[str, dict[str, Any]],
    runtime_transition_sequence_records: list[Any],
    runtime_dispatch_surface_records: list[Any],
    runtime_generation_domain_records: list[Any],
    registered_owner_fields: set[str],
    runtime_transition_ids: set[str],
    runtime_invariant_ids: set[str],
    runtime_invariant_transition_ids: dict[str, set[str]],
    runtime_invariant_protected_fields: dict[str, set[str]],
    runtime_diff_harness_ids: set[str],
    runtime_diff_harness_transition_ids: dict[str, set[str]],
    runtime_diff_harness_owner_field_refs: dict[str, set[str]],
    runtime_diff_harness_invariant_ids: dict[str, set[str]],
    runtime_diff_harness_generation_domain_ids: dict[str, set[str]],
) -> list[str]:
    failures: list[str] = []
    doc_records = event_coverage_doc.get("events") if isinstance(event_coverage_doc, dict) else []
    event_by_id = {
        record.get("event_id"): record
        for record in doc_records
        if isinstance(record, dict) and isinstance(record.get("event_id"), str)
    }
    expected_ids = set(event_by_id)
    covered_ids: set[str] = set()
    covered_classes: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_event_coverage[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_EVENT_FIELDS,
                list_fields=EVENT_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(_validate_event_boundary_status(record=record, label=label))
        failures.extend(
            _validate_owner_field_coverage_refs(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
                registry_label="owner-field registry",
            )
        )
        failures.extend(
            _validate_transition_sequence_refs(
                record=record,
                transition_sequence_records=runtime_transition_sequence_records,
                label=label,
            )[0]
        )
        failures.extend(
            _validate_dispatch_surface_refs(
                record=record,
                dispatch_surface_records=runtime_dispatch_surface_records,
                label=label,
            )
        )
        failures.extend(
            _validate_generation_domain_refs(
                generation_domain_refs=record.get("generation_domain_refs"),
                generation_domain_records=runtime_generation_domain_records,
                transition_id=record.get("transition_id"),
                label=label,
            )
        )
        failures.extend(
            _validate_record_invariant_refs(
                invariant_refs=record.get("invariant_refs"),
                invariant_ids=runtime_invariant_ids,
                invariant_transition_ids=runtime_invariant_transition_ids,
                invariant_protected_fields=runtime_invariant_protected_fields,
                transition_id=record.get("transition_id"),
                declared_write_set=record.get("declared_write_set"),
                label=label,
            )
        )
        failures.extend(
            _validate_coverage_diff_harness_refs(
                record=record,
                diff_harness_ids=runtime_diff_harness_ids,
                diff_harness_transition_ids=runtime_diff_harness_transition_ids,
                diff_harness_owner_field_refs=(
                    runtime_diff_harness_owner_field_refs
                ),
                diff_harness_invariant_ids=runtime_diff_harness_invariant_ids,
                diff_harness_generation_domain_ids=(
                    runtime_diff_harness_generation_domain_ids
                ),
                label=label,
            )
        )
        event_id = record.get("event_id")
        if isinstance(event_id, str) and event_id.strip():
            if event_id in covered_ids:
                failures.append(f"{label}: duplicate event_id: {event_id}")
            covered_ids.add(event_id)
            doc_record = event_by_id.get(event_id)
            if doc_record is None:
                failures.append(f"{label}: runtime event coverage missing from docs: {event_id}")
            else:
                for field in REQUIRED_EVENT_FIELDS:
                    if record.get(field) != doc_record.get(field):
                        failures.append(f"{label}: {field} does not match docs for {event_id}")
        event_class = record.get("event_class")
        if isinstance(event_class, str) and event_class.strip():
            if event_class in covered_classes:
                failures.append(f"{label}: duplicate event_class: {event_class}")
            covered_classes.add(event_class)
            if event_class not in REQUIRED_EVENT_CLASSES:
                failures.append(f"{label}: unknown event_class: {event_class}")
        transition_id = record.get("transition_id")
        transition_record = transition_ids.get(transition_id) if isinstance(transition_id, str) else None
        if transition_record is None:
            failures.append(f"{label}: transition_id does not match a transition id: {transition_id}")
        elif transition_id not in runtime_transition_ids:
            failures.append(f"{label}: transition_id missing from runtime transition registry: {transition_id}")
        else:
            if record.get("category") != transition_record.get("category"):
                failures.append(
                    f"{label}: category does not match transition "
                    f"{transition_id}: {record.get('category')}"
                )
            if record.get("owner") != transition_record.get("owner"):
                failures.append(
                    f"{label}: owner does not match transition "
                    f"{transition_id}: {record.get('owner')}"
                )
            if record.get("declared_write_set") != transition_record.get("declared_write_set"):
                failures.append(
                    f"{label}: declared_write_set does not match "
                    f"transition {transition_id}"
                )

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime event coverage missing event id(s): "
            + ", ".join(missing_ids)
        )
    missing_classes = sorted(REQUIRED_EVENT_CLASSES - covered_classes)
    if missing_classes:
        failures.append(
            f"{runtime_path}: runtime event coverage missing event_class(es): "
            + ", ".join(missing_classes)
        )
    return failures


def _validate_event_coverage(
    *,
    event_coverage_doc: Any,
    event_coverage_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    transition_sequence_records: list[Any],
    dispatch_surface_records: list[Any],
    generation_domain_records: list[Any],
    registered_owner_fields: set[str],
    invariant_ids: set[str],
    invariant_transition_ids: dict[str, set[str]],
    invariant_protected_fields: dict[str, set[str]],
    diff_harness_ids: set[str],
    diff_harness_transition_ids: dict[str, set[str]],
    diff_harness_owner_field_refs: dict[str, set[str]],
    diff_harness_invariant_ids: dict[str, set[str]],
    diff_harness_generation_domain_ids: dict[str, set[str]],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(event_coverage_doc, dict):
        failures.append(f"{event_coverage_path}: top-level value must be an object")
        return failures

    required_event_classes = event_coverage_doc.get("required_event_classes")
    failures.extend(
        _validate_required_string_list(
            value=required_event_classes,
            required_values=REQUIRED_EVENT_CLASSES,
            label="event coverage required_event_classes",
            item_label="required_event_classes",
        )
    )

    event_records = event_coverage_doc.get("events")
    if not isinstance(event_records, list) or not event_records:
        failures.append(f"{event_coverage_path}: events must be a non-empty list")
        event_records = []

    event_ids: set[str] = set()
    covered_event_classes: set[str] = set()
    for index, record in enumerate(event_records):
        label = f"event[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_EVENT_FIELDS,
                list_fields=EVENT_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(_validate_event_boundary_status(record=record, label=label))
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )
        failures.extend(
            _validate_owner_field_coverage_refs(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
                registry_label="owner-field registry",
            )
        )
        failures.extend(
            _validate_transition_sequence_refs(
                record=record,
                transition_sequence_records=transition_sequence_records,
                label=label,
            )[0]
        )
        failures.extend(
            _validate_dispatch_surface_refs(
                record=record,
                dispatch_surface_records=dispatch_surface_records,
                label=label,
            )
        )
        failures.extend(
            _validate_generation_domain_refs(
                generation_domain_refs=record.get("generation_domain_refs"),
                generation_domain_records=generation_domain_records,
                transition_id=record.get("transition_id"),
                label=label,
            )
        )
        failures.extend(
            _validate_record_invariant_refs(
                invariant_refs=record.get("invariant_refs"),
                invariant_ids=invariant_ids,
                invariant_transition_ids=invariant_transition_ids,
                invariant_protected_fields=invariant_protected_fields,
                transition_id=record.get("transition_id"),
                declared_write_set=record.get("declared_write_set"),
                label=label,
            )
        )
        failures.extend(
            _validate_coverage_diff_harness_refs(
                record=record,
                diff_harness_ids=diff_harness_ids,
                diff_harness_transition_ids=diff_harness_transition_ids,
                diff_harness_owner_field_refs=diff_harness_owner_field_refs,
                diff_harness_invariant_ids=diff_harness_invariant_ids,
                diff_harness_generation_domain_ids=(
                    diff_harness_generation_domain_ids
                ),
                label=label,
            )
        )

        event_id = record.get("event_id")
        if isinstance(event_id, str) and event_id.strip():
            if event_id in event_ids:
                failures.append(f"{label}: duplicate event_id: {event_id}")
            event_ids.add(event_id)

        event_class = record.get("event_class")
        if isinstance(event_class, str) and event_class.strip():
            if event_class in covered_event_classes:
                failures.append(f"{label}: duplicate event_class: {event_class}")
            covered_event_classes.add(event_class)
            if event_class not in REQUIRED_EVENT_CLASSES:
                failures.append(f"{label}: unknown event_class: {event_class}")

        transition_id = record.get("transition_id")
        transition_record = None
        if isinstance(transition_id, str) and transition_id.strip():
            transition_record = transition_ids.get(transition_id)
            if transition_record is None:
                failures.append(
                    f"{label}: transition_id does not match a transition id: {transition_id}"
                )

        category = record.get("category")
        if (
            isinstance(category, str)
            and category.strip()
            and transition_record is not None
            and category != transition_record.get("category")
        ):
            failures.append(
                f"{label}: category does not match transition {transition_id}: {category}"
            )
        owner = record.get("owner")
        if (
            isinstance(owner, str)
            and owner.strip()
            and transition_record is not None
            and owner != transition_record.get("owner")
        ):
            failures.append(
                f"{label}: owner does not match transition {transition_id}: {owner}"
            )

    missing_event_classes = sorted(REQUIRED_EVENT_CLASSES - covered_event_classes)
    if missing_event_classes:
        failures.append(
            "event coverage missing required event_class(es): "
            + ", ".join(missing_event_classes)
        )

    return failures


def _validate_step_stimulus(
    *,
    stimulus: Any,
    label: str,
    transition_id: Any,
    action_ids: set[str],
    action_transition_ids: dict[str, str],
    event_ids: set[str],
    event_transition_ids: dict[str, str],
) -> list[str]:
    if not isinstance(stimulus, dict) or not stimulus:
        return [f"{label}: stimulus must be a non-empty object"]

    failures: list[str] = []
    action_id = stimulus.get("action_id")
    event_id = stimulus.get("event_id")
    if action_id is None and event_id is None:
        failures.append(f"{label}: stimulus must include action_id or event_id")

    if action_id is not None:
        if not isinstance(action_id, str) or not action_id.strip():
            failures.append(f"{label}: stimulus.action_id must be a non-empty string")
        elif action_id not in action_ids:
            failures.append(f"{label}: stimulus.action_id references unknown action: {action_id}")
        elif isinstance(transition_id, str) and transition_id.strip():
            action_transition_id = action_transition_ids.get(action_id)
            if action_transition_id != transition_id:
                failures.append(
                    f"{label}: stimulus.action_id {action_id} maps to transition_id "
                    f"{action_transition_id}, not step.transition_id {transition_id}"
                )

    if event_id is not None:
        if not isinstance(event_id, str) or not event_id.strip():
            failures.append(f"{label}: stimulus.event_id must be a non-empty string")
        elif event_id not in event_ids:
            failures.append(f"{label}: stimulus.event_id references unknown event id: {event_id}")
        elif isinstance(transition_id, str) and transition_id.strip():
            event_transition_id = event_transition_ids.get(event_id)
            if event_transition_id != transition_id:
                failures.append(
                    f"{label}: stimulus.event_id {event_id} maps to transition_id "
                    f"{event_transition_id}, not step.transition_id {transition_id}"
                )

    return failures


def _validate_step_reference_list(
    *,
    value: Any,
    valid_ids: set[str],
    label: str,
    field: str,
    reference_label: str,
) -> list[str]:
    failures = _validate_list_field(value=value, label=label, field=field)
    if failures:
        return failures

    assert isinstance(value, list)
    seen: set[str] = set()
    for index, item in enumerate(value):
        assert isinstance(item, str)
        if item in seen:
            failures.append(f"{label}: duplicate {field}[{index}]: {item}")
        seen.add(item)
        if item not in valid_ids:
            failures.append(f"{label}: {field} references unknown {reference_label}: {item}")
    return failures


def _validate_after_step_contract_evaluation(
    *, step: dict[str, Any], label: str
) -> list[str]:
    failures: list[str] = []
    for field, evaluation_name in (
        ("invariant_ids", "invariant"),
        ("diff_harness_ids", "diff harness"),
    ):
        value = step.get(field)
        if isinstance(value, list) and not value:
            failures.append(
                f"{label}: after-step contract scenario must evaluate "
                f"at least one {evaluation_name}"
            )
    return failures


def _non_empty_string_set(value: Any) -> set[str] | None:
    if not isinstance(value, list):
        return None
    refs: set[str] = set()
    for item in value:
        if not isinstance(item, str) or not item.strip():
            return None
        refs.add(item)
    return refs


def _step_generation_domain_ids(value: Any) -> set[str] | None:
    if not isinstance(value, list):
        return None
    domain_ids: set[str] = set()
    for record in value:
        if not isinstance(record, dict):
            return None
        domain_id = record.get("domain_id")
        if not isinstance(domain_id, str) or not domain_id.strip():
            return None
        domain_ids.add(domain_id)
    return domain_ids


def _validate_step_coverage_ref_overlap(
    *,
    label: str,
    field: str,
    index: int,
    ref: str,
    coverage_record: dict[str, Any],
    step_invariant_ids: Any,
    step_diff_harness_ids: Any,
    step_generation_domain_expectations: Any,
) -> list[str]:
    failures: list[str] = []
    overlap_checks = (
        (
            "invariant_refs",
            _non_empty_string_set(step_invariant_ids),
            _non_empty_string_set(coverage_record.get("invariant_refs")),
            "step invariant_ids",
        ),
        (
            "diff_harness_refs",
            _non_empty_string_set(step_diff_harness_ids),
            _non_empty_string_set(coverage_record.get("diff_harness_refs")),
            "step diff_harness_ids",
        ),
        (
            "generation_domain_refs",
            _step_generation_domain_ids(step_generation_domain_expectations),
            _non_empty_string_set(coverage_record.get("generation_domain_refs")),
            "step generation_domain_expectations",
        ),
    )
    for coverage_field, step_refs, coverage_refs, step_field in overlap_checks:
        if step_refs is None or coverage_refs is None:
            continue
        if not step_refs or not coverage_refs or not step_refs.intersection(coverage_refs):
            failures.append(
                f"{label}: {field}[{index}] {ref} {coverage_field} must overlap "
                f"{step_field}"
            )
    return failures


def _validate_step_coverage_refs(
    *,
    value: Any,
    valid_ids: set[str],
    coverage_records_by_id: dict[str, dict[str, Any]],
    label: str,
    field: str,
    reference_label: str,
    stimulus_id: Any,
    transition_id: Any,
    step_invariant_ids: Any,
    step_diff_harness_ids: Any,
    step_generation_domain_expectations: Any,
) -> list[str]:
    if not isinstance(value, list):
        return [f"{label}: {field} must be a list"]

    failures: list[str] = []
    seen: set[str] = set()
    for index, item in enumerate(value):
        if not isinstance(item, str) or not item.strip():
            failures.append(f"{label}: {field}[{index}] must be a non-empty string")
            continue
        if item in seen:
            failures.append(f"{label}: duplicate {field}[{index}]: {item}")
        seen.add(item)
        if item not in valid_ids:
            failures.append(
                f"{label}: {field} references unknown {reference_label}: {item}"
            )
    if not isinstance(stimulus_id, str) or not stimulus_id.strip():
        if value:
            failures.append(
                f"{label}: {field} must be empty when the step has no matching stimulus"
            )
        return failures
    if failures:
        return failures

    if not value:
        failures.append(
            f"{label}: {field} must include coverage for stimulus {stimulus_id}"
        )
        return failures

    for index, ref in enumerate(value):
        assert isinstance(ref, str)
        if ref != stimulus_id:
            failures.append(
                f"{label}: {field}[{index}] {ref} does not match stimulus {stimulus_id}"
            )
            continue
        coverage_record = coverage_records_by_id.get(ref)
        if coverage_record is None:
            continue
        coverage_transition_id = coverage_record.get("transition_id")
        if (
            isinstance(transition_id, str)
            and transition_id.strip()
            and coverage_transition_id != transition_id
        ):
            failures.append(
                f"{label}: {field}[{index}] transition_id does not match "
                f"step.transition_id {transition_id}: {coverage_transition_id}"
            )
        failures.extend(
            _validate_step_coverage_ref_overlap(
                label=label,
                field=field,
                index=index,
                ref=ref,
                coverage_record=coverage_record,
                step_invariant_ids=step_invariant_ids,
                step_diff_harness_ids=step_diff_harness_ids,
                step_generation_domain_expectations=step_generation_domain_expectations,
            )
        )

    return failures


def _diff_harness_transition_ids_by_harness(
    diff_harness_records: list[Any],
) -> dict[str, set[str]]:
    transition_ids_by_harness: dict[str, set[str]] = {}
    for record in diff_harness_records:
        if not isinstance(record, dict):
            continue
        harness_id = record.get("harness_id")
        transition_ids = record.get("transition_ids")
        if not isinstance(harness_id, str) or not harness_id.strip():
            continue
        if not isinstance(transition_ids, list):
            continue
        transition_ids_by_harness[harness_id] = {
            transition_id
            for transition_id in transition_ids
            if isinstance(transition_id, str) and transition_id.strip()
        }
    return transition_ids_by_harness


def _diff_harness_string_refs_by_harness(
    diff_harness_records: list[Any],
    field: str,
) -> dict[str, set[str]]:
    refs_by_harness: dict[str, set[str]] = {}
    for record in diff_harness_records:
        if not isinstance(record, dict):
            continue
        harness_id = record.get("harness_id")
        refs = record.get(field)
        if not isinstance(harness_id, str) or not harness_id.strip():
            continue
        if not isinstance(refs, list):
            continue
        refs_by_harness[harness_id] = {
            ref for ref in refs if isinstance(ref, str) and ref.strip()
        }
    return refs_by_harness


def _invariant_transition_ids_by_invariant(
    invariant_records: list[Any],
) -> dict[str, set[str]]:
    transition_ids_by_invariant: dict[str, set[str]] = {}
    for record in invariant_records:
        if not isinstance(record, dict):
            continue
        invariant_id = record.get("invariant_id")
        transition_ids = record.get("transition_ids")
        if not isinstance(invariant_id, str) or not invariant_id.strip():
            continue
        if not isinstance(transition_ids, list):
            continue
        transition_ids_by_invariant[invariant_id] = {
            transition_id
            for transition_id in transition_ids
            if isinstance(transition_id, str) and transition_id.strip()
        }
    return transition_ids_by_invariant


def _invariant_protected_fields_by_invariant(
    invariant_records: list[Any],
) -> dict[str, set[str]]:
    protected_fields_by_invariant: dict[str, set[str]] = {}
    for record in invariant_records:
        if not isinstance(record, dict):
            continue
        invariant_id = record.get("invariant_id")
        protected_fields = record.get("protected_fields")
        if not isinstance(invariant_id, str) or not invariant_id.strip():
            continue
        if not isinstance(protected_fields, list):
            continue
        protected_fields_by_invariant[invariant_id] = {
            field
            for field in protected_fields
            if isinstance(field, str) and field.strip()
        }
    return protected_fields_by_invariant


def _diff_harness_owner_field_refs(diff_harness_records: list[Any]) -> set[str]:
    owner_field_refs: set[str] = set()
    for record in diff_harness_records:
        if not isinstance(record, dict):
            continue
        refs = record.get("owner_field_refs")
        if not isinstance(refs, list):
            continue
        owner_field_refs.update(
            field for field in refs if isinstance(field, str) and field.strip()
        )
    return owner_field_refs


def _validate_invariant_diff_harness_owner_field_coverage(
    *,
    protected_fields: Any,
    invariant_id: Any,
    diff_harness_owner_field_refs: set[str],
    label: str,
    coverage_label: str,
    registered_owner_fields: set[str] | None = None,
) -> list[str]:
    if not isinstance(invariant_id, str) or not invariant_id.strip():
        return []
    if not isinstance(protected_fields, list):
        return []

    failures: list[str] = []
    for field in protected_fields:
        if not isinstance(field, str) or not field.strip():
            continue
        if registered_owner_fields is not None and field not in registered_owner_fields:
            continue
        if field not in diff_harness_owner_field_refs:
            failures.append(
                f"{label}: invariant {invariant_id} protected field lacks "
                f"{coverage_label} owner_field_refs coverage: {field}"
            )
    return failures


def _invariant_protected_fields_by_dispatch_surface(
    invariant_records: list[Any],
) -> dict[str, set[str]]:
    protected_fields_by_surface: dict[str, set[str]] = {}
    for record in invariant_records:
        if not isinstance(record, dict):
            continue
        surface_refs = record.get("dispatch_surface_ids")
        protected_fields = record.get("protected_fields")
        if not isinstance(surface_refs, list) or not isinstance(
            protected_fields, list
        ):
            continue
        valid_fields = {
            field
            for field in protected_fields
            if isinstance(field, str) and field.strip()
        }
        if not valid_fields:
            continue
        for surface_id in surface_refs:
            if not isinstance(surface_id, str) or not surface_id.strip():
                continue
            protected_fields_by_surface.setdefault(surface_id, set()).update(
                valid_fields
            )
    return protected_fields_by_surface


def _invariant_protected_fields_by_transition(
    invariant_records: list[Any],
) -> dict[str, set[str]]:
    protected_fields_by_transition: dict[str, set[str]] = {}
    for record in invariant_records:
        if not isinstance(record, dict):
            continue
        transition_refs = record.get("transition_ids")
        protected_fields = record.get("protected_fields")
        if not isinstance(transition_refs, list) or not isinstance(
            protected_fields, list
        ):
            continue
        valid_fields = {
            field
            for field in protected_fields
            if isinstance(field, str) and field.strip()
        }
        if not valid_fields:
            continue
        for transition_id in transition_refs:
            if not isinstance(transition_id, str) or not transition_id.strip():
                continue
            protected_fields_by_transition.setdefault(transition_id, set()).update(
                valid_fields
            )
    return protected_fields_by_transition


def _validate_allowed_direct_writes_have_invariant_coverage(
    *,
    record: dict[str, Any],
    protected_fields_by_surface: dict[str, set[str]],
    label: str,
) -> list[str]:
    surface_id = record.get("surface_id")
    writes = record.get("allowed_direct_writes")
    if not isinstance(surface_id, str) or not surface_id.strip():
        return []
    if not isinstance(writes, list):
        return []

    protected_fields = protected_fields_by_surface.get(surface_id, set())
    failures: list[str] = []
    for index, field in enumerate(writes):
        if not isinstance(field, str) or not field.strip():
            continue
        if field not in protected_fields:
            failures.append(
                f"{label}: {surface_id} allowed_direct_writes[{index}] "
                f"lacks same-surface invariant coverage for owner field: {field}"
            )
    return failures


def _validate_transition_write_set_has_invariant_coverage(
    *,
    record: dict[str, Any],
    protected_fields_by_transition: dict[str, set[str]],
    label: str,
) -> list[str]:
    transition_id = record.get("id")
    write_set = record.get("declared_write_set")
    if not isinstance(transition_id, str) or not transition_id.strip():
        return []
    if not isinstance(write_set, list):
        return []

    protected_fields = protected_fields_by_transition.get(transition_id, set())
    failures: list[str] = []
    for index, field in enumerate(write_set):
        if not isinstance(field, str) or not field.strip():
            continue
        if field not in protected_fields:
            failures.append(
                f"{label}: declared_write_set[{index}] lacks same-transition "
                f"invariant coverage for transition {transition_id} "
                f"owner field: {field}"
            )
    return failures


def _validate_owner_field_invariant_alignment(
    *,
    invariant_checks: Any,
    owner_field: Any,
    invariant_protected_fields: dict[str, set[str]],
    label: str,
) -> list[str]:
    if not isinstance(owner_field, str) or not owner_field.strip():
        return []
    if not isinstance(invariant_checks, list):
        return []

    for invariant_id in invariant_checks:
        if not isinstance(invariant_id, str) or not invariant_id.strip():
            continue
        if owner_field in invariant_protected_fields.get(invariant_id, set()):
            return []

    return [
        f"{label}: invariant_checks must include at least one invariant "
        f"protecting owner field {owner_field}"
    ]


def _validate_diff_harness_owner_field_invariant_alignment(
    *,
    owner_field_refs: Any,
    invariant_refs: Any,
    invariant_protected_fields: dict[str, set[str]],
    valid_owner_fields: set[str],
    valid_invariant_ids: set[str],
    label: str,
) -> list[str]:
    if not isinstance(owner_field_refs, list):
        return []
    if not isinstance(invariant_refs, list):
        return []

    protected_fields: set[str] = set()
    for invariant_id in invariant_refs:
        if not isinstance(invariant_id, str) or not invariant_id.strip():
            continue
        if invariant_id not in valid_invariant_ids:
            continue
        protected_fields.update(invariant_protected_fields.get(invariant_id, set()))

    failures: list[str] = []
    for field in owner_field_refs:
        if not isinstance(field, str) or not field.strip():
            continue
        if field not in valid_owner_fields:
            continue
        if field not in protected_fields:
            failures.append(
                f"{label}: invariant_ids must include at least one invariant "
                f"protecting owner_field_refs field {field}"
            )
    return failures


def _invariant_refs_cover_transition(
    *,
    invariant_refs: list[str],
    transition_id: str,
    invariant_transition_ids: dict[str, set[str]],
) -> bool:
    for invariant_id in invariant_refs:
        if not isinstance(invariant_id, str) or not invariant_id.strip():
            continue
        if transition_id in invariant_transition_ids.get(invariant_id, set()):
            return True
    return False


def _validate_invariant_transition_alignment(
    *,
    invariant_refs: Any,
    transition_id: Any,
    invariant_transition_ids: dict[str, set[str]],
    label: str,
    invariant_field: str,
    transition_field: str,
) -> list[str]:
    if not isinstance(transition_id, str) or not transition_id.strip():
        return []
    if not isinstance(invariant_refs, list):
        return []

    if _invariant_refs_cover_transition(
        invariant_refs=invariant_refs,
        transition_id=transition_id,
        invariant_transition_ids=invariant_transition_ids,
    ):
        return []

    return [
        f"{label}: {invariant_field} must include at least one invariant "
        f"covering {transition_field} {transition_id}"
    ]


def _validate_step_invariant_transition_alignment(
    *,
    invariant_refs: Any,
    transition_id: Any,
    invariant_transition_ids: dict[str, set[str]],
    label: str,
) -> list[str]:
    return _validate_invariant_transition_alignment(
        invariant_refs=invariant_refs,
        transition_id=transition_id,
        invariant_transition_ids=invariant_transition_ids,
        label=label,
        invariant_field="invariant_ids",
        transition_field="transition_id",
    )


def _validate_step_diff_harness_transition_alignment(
    *,
    diff_harness_refs: Any,
    transition_id: Any,
    diff_harness_transition_ids: dict[str, set[str]],
    label: str,
) -> list[str]:
    if not isinstance(transition_id, str) or not transition_id.strip():
        return []
    if not isinstance(diff_harness_refs, list):
        return []

    for harness_id in diff_harness_refs:
        if not isinstance(harness_id, str) or not harness_id.strip():
            continue
        if transition_id in diff_harness_transition_ids.get(harness_id, set()):
            return []

    return [
        f"{label}: diff_harness_ids must include at least one diff harness "
        f"covering transition_id {transition_id}"
    ]


def _validate_generation_expectations(
    *,
    value: Any,
    label: str,
    generation_domain_ids: set[str],
) -> list[str]:
    if not isinstance(value, list) or not value:
        return [f"{label}: generation_domain_expectations must be a non-empty list"]

    failures: list[str] = []
    seen: set[str] = set()
    for index, record in enumerate(value):
        expectation_label = f"{label}.generation_domain_expectations[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_GENERATION_EXPECTATION_FIELDS,
                list_fields=set(),
                label=expectation_label,
            )
        )
        if not isinstance(record, dict):
            continue

        domain_id = record.get("domain_id")
        if isinstance(domain_id, str) and domain_id.strip():
            if domain_id in seen:
                failures.append(
                    f"{expectation_label}: duplicate domain_id: {domain_id}"
                )
            seen.add(domain_id)
            if domain_id not in generation_domain_ids:
                failures.append(
                    f"{expectation_label}: domain_id references unknown generation domain id: {domain_id}"
                )

    return failures


def _requires_deterministic_fallback(record: dict[str, Any]) -> bool:
    precondition = record.get("precondition")
    expected_result = record.get("expected_result")
    return (
        precondition in {"generation_mismatch", "stale_snapshot"}
        or expected_result == "fallback"
    )


def _requires_no_unrelated_mutation(record: dict[str, Any]) -> bool:
    precondition = record.get("precondition")
    expected_result = record.get("expected_result")
    return (
        precondition in {"generation_mismatch", "stale_snapshot"}
        or expected_result in {"blocked", "fallback", "invalid"}
    )


def _validate_deterministic_fallback(
    *,
    record: dict[str, Any],
    label: str,
) -> list[str]:
    fallback = record.get("deterministic_fallback")
    if not isinstance(fallback, dict):
        return [
            f"{label}: stale-snapshot/generation-mismatch steps require deterministic_fallback"
        ]
    return _validate_required_fields(
        record=fallback,
        required_fields=REQUIRED_FALLBACK_FIELDS,
        list_fields=set(),
        label=f"{label}.deterministic_fallback",
    )


def _validate_no_unrelated_mutation(
    *,
    record: dict[str, Any],
    label: str,
    diff_harness_ids: set[str],
    step_diff_harness_refs: Any,
    transition_id: Any,
    diff_harness_transition_ids: dict[str, set[str]],
    required: bool,
) -> list[str]:
    expectation = record.get("no_unrelated_mutation")
    if not isinstance(expectation, dict):
        if not required:
            return []
        return [
            f"{label}: blocked/invalid/fallback steps require no_unrelated_mutation expectations"
        ]

    failures = _validate_required_fields(
        record=expectation,
        required_fields=REQUIRED_NO_UNRELATED_MUTATION_FIELDS,
        list_fields=set(),
        label=f"{label}.no_unrelated_mutation",
    )
    harness_id = expectation.get("diff_harness_id")
    if isinstance(harness_id, str) and harness_id.strip() and harness_id not in diff_harness_ids:
        failures.append(
            f"{label}.no_unrelated_mutation: diff_harness_id references unknown diff harness id: {harness_id}"
        )
    if isinstance(harness_id, str) and harness_id.strip():
        if not isinstance(step_diff_harness_refs, list) or harness_id not in {
            value
            for value in step_diff_harness_refs
            if isinstance(value, str) and value.strip()
        }:
            failures.append(
                f"{label}.no_unrelated_mutation: diff_harness_id must be listed in step diff_harness_ids: {harness_id}"
            )
        if (
            isinstance(transition_id, str)
            and transition_id.strip()
            and transition_id not in diff_harness_transition_ids.get(harness_id, set())
        ):
            failures.append(
                f"{label}.no_unrelated_mutation: diff_harness_id must cover transition_id {transition_id}: {harness_id}"
            )
    return failures


def _validate_appstate_transition_sequences(
    *,
    transition_sequences_doc: Any,
    transition_sequences_path: Path,
    transition_ids: dict[str, dict[str, Any]],
    action_ids: set[str],
    action_transition_ids: dict[str, str],
    action_coverage_by_action: dict[str, dict[str, Any]],
    event_ids: set[str],
    event_transition_ids: dict[str, str],
    event_coverage_by_event_id: dict[str, dict[str, Any]],
    invariant_ids: set[str],
    invariant_transition_ids: dict[str, set[str]],
    diff_harness_ids: set[str],
    diff_harness_transition_ids: dict[str, set[str]],
    generation_domain_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    if not isinstance(transition_sequences_doc, dict):
        failures.append(f"{transition_sequences_path}: top-level value must be an object")
        return failures

    records = transition_sequences_doc.get("scenarios")
    if not isinstance(records, list) or not records:
        failures.append(f"{transition_sequences_path}: scenarios must be a non-empty list")
        records = []

    scenario_ids: set[str] = set()
    covered_flows: set[str] = set()
    for scenario_index, record in enumerate(records):
        scenario_label = f"transition_sequence[{scenario_index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_SEQUENCE_FIELDS,
                list_fields=set(),
                label=scenario_label,
            )
        )
        if not isinstance(record, dict):
            continue

        scenario_id = record.get("scenario_id")
        if isinstance(scenario_id, str) and scenario_id.strip():
            if scenario_id in scenario_ids:
                failures.append(f"{scenario_label}: duplicate scenario_id: {scenario_id}")
            scenario_ids.add(scenario_id)

        category = record.get("category")
        if isinstance(category, str) and category.strip():
            if category not in REQUIRED_SEQUENCE_CATEGORIES:
                failures.append(f"{scenario_label}: unknown category: {category}")

        flow = record.get("flow")
        if isinstance(flow, str) and flow.strip():
            covered_flows.add(flow)
            if flow not in REQUIRED_SEQUENCE_FLOWS:
                failures.append(f"{scenario_label}: unknown flow: {flow}")

        failures.extend(
            _validate_status_field(
                record=record,
                label=scenario_label,
                field="coverage_status",
                valid_statuses=VALID_SEQUENCE_COVERAGE_STATUSES,
            )
        )

        steps = record.get("steps")
        if not isinstance(steps, list) or not steps:
            failures.append(f"{scenario_label}: steps must be a non-empty list")
            continue

        ordinals: set[int] = set()
        previous_ordinal = 0
        for step_index, step in enumerate(steps):
            label = f"{scenario_label}.step[{step_index}]"
            failures.extend(
                _validate_required_fields(
                    record=step,
                    required_fields=REQUIRED_SEQUENCE_STEP_FIELDS,
                    list_fields=SEQUENCE_LIST_FIELDS
                    - {
                        "action_coverage_refs",
                        "event_coverage_refs",
                        "generation_domain_expectations",
                    },
                    label=label,
                )
            )
            if not isinstance(step, dict):
                continue

            ordinal = step.get("ordinal")
            if not isinstance(ordinal, int) or ordinal < 1:
                failures.append(f"{label}: ordinal must be a positive integer")
            else:
                if ordinal in ordinals:
                    failures.append(f"{label}: duplicate ordinal: {ordinal}")
                if ordinal <= previous_ordinal:
                    failures.append(f"{label}: ordinal must be greater than previous step ordinal")
                ordinals.add(ordinal)
                previous_ordinal = ordinal

            transition_id = step.get("transition_id")
            if isinstance(transition_id, str) and transition_id.strip():
                if transition_id not in transition_ids:
                    failures.append(
                        f"{label}: transition_id references unknown transition id: {transition_id}"
                    )

            failures.extend(
                _validate_step_stimulus(
                    stimulus=step.get("stimulus"),
                    label=label,
                    transition_id=transition_id,
                    action_ids=action_ids,
                    action_transition_ids=action_transition_ids,
                    event_ids=event_ids,
                    event_transition_ids=event_transition_ids,
                )
            )
            stimulus = step.get("stimulus")
            if not isinstance(stimulus, dict):
                stimulus = {}
            failures.extend(
                _validate_step_coverage_refs(
                    value=step.get("action_coverage_refs"),
                    valid_ids=action_ids,
                    coverage_records_by_id=action_coverage_by_action,
                    label=label,
                    field="action_coverage_refs",
                    reference_label="action coverage record",
                    stimulus_id=stimulus.get("action_id"),
                    transition_id=transition_id,
                    step_invariant_ids=step.get("invariant_ids"),
                    step_diff_harness_ids=step.get("diff_harness_ids"),
                    step_generation_domain_expectations=step.get(
                        "generation_domain_expectations"
                    ),
                )
            )
            failures.extend(
                _validate_step_coverage_refs(
                    value=step.get("event_coverage_refs"),
                    valid_ids=event_ids,
                    coverage_records_by_id=event_coverage_by_event_id,
                    label=label,
                    field="event_coverage_refs",
                    reference_label="event coverage record",
                    stimulus_id=stimulus.get("event_id"),
                    transition_id=transition_id,
                    step_invariant_ids=step.get("invariant_ids"),
                    step_diff_harness_ids=step.get("diff_harness_ids"),
                    step_generation_domain_expectations=step.get(
                        "generation_domain_expectations"
                    ),
                )
            )
            failures.extend(
                _validate_step_reference_list(
                    value=step.get("invariant_ids"),
                    valid_ids=invariant_ids,
                    label=label,
                    field="invariant_ids",
                    reference_label="invariant id",
                )
            )
            failures.extend(
                _validate_step_reference_list(
                    value=step.get("diff_harness_ids"),
                    valid_ids=diff_harness_ids,
                    label=label,
                    field="diff_harness_ids",
                    reference_label="diff harness id",
                )
            )
            failures.extend(
                _validate_after_step_contract_evaluation(step=step, label=label)
            )
            failures.extend(
                _validate_step_invariant_transition_alignment(
                    invariant_refs=step.get("invariant_ids"),
                    transition_id=transition_id,
                    invariant_transition_ids=invariant_transition_ids,
                    label=label,
                )
            )
            failures.extend(
                _validate_step_diff_harness_transition_alignment(
                    diff_harness_refs=step.get("diff_harness_ids"),
                    transition_id=transition_id,
                    diff_harness_transition_ids=diff_harness_transition_ids,
                    label=label,
                )
            )
            failures.extend(
                _validate_generation_expectations(
                    value=step.get("generation_domain_expectations"),
                    label=label,
                    generation_domain_ids=generation_domain_ids,
                )
            )

            expected_result = step.get("expected_result")
            if (
                isinstance(expected_result, str)
                and expected_result.strip()
                and expected_result not in {"allowed", "blocked", "fallback", "invalid"}
            ):
                failures.append(f"{label}: unknown expected_result: {expected_result}")

            if _requires_deterministic_fallback(step):
                failures.extend(
                    _validate_deterministic_fallback(record=step, label=label)
                )

            no_unrelated_required = _requires_no_unrelated_mutation(step)
            if no_unrelated_required or step.get("no_unrelated_mutation") is not None:
                failures.extend(
                    _validate_no_unrelated_mutation(
                        record=step,
                        label=label,
                        diff_harness_ids=diff_harness_ids,
                        step_diff_harness_refs=step.get("diff_harness_ids"),
                        transition_id=transition_id,
                        diff_harness_transition_ids=diff_harness_transition_ids,
                        required=no_unrelated_required,
                    )
                )

    missing_flows = sorted(REQUIRED_SEQUENCE_FLOWS - covered_flows)
    if missing_flows:
        failures.append(
            "transition sequences missing required flow(s): " + ", ".join(missing_flows)
        )

    return failures



def _normalize_transition_sequence_records(records: list[Any]) -> dict[str, dict[str, Any]]:
    normalized: dict[str, dict[str, Any]] = {}
    for record in records:
        if not isinstance(record, dict):
            continue
        scenario_id = record.get("scenario_id")
        if not isinstance(scenario_id, str) or not scenario_id.strip():
            continue
        steps: list[dict[str, Any]] = []
        for step in record.get("steps", []):
            if not isinstance(step, dict):
                continue
            stimulus = step.get("stimulus")
            if not isinstance(stimulus, dict):
                stimulus = {}
            steps.append(
                {
                    "ordinal": step.get("ordinal"),
                    "step_id": step.get("step_id"),
                    "transition_id": step.get("transition_id"),
                    "stimulus_action_id": stimulus.get("action_id"),
                    "stimulus_event_id": stimulus.get("event_id"),
                    "action_coverage_refs": step.get("action_coverage_refs"),
                    "event_coverage_refs": step.get("event_coverage_refs"),
                    "expected_result": step.get("expected_result"),
                    "invariant_ids": step.get("invariant_ids"),
                    "diff_harness_ids": step.get("diff_harness_ids"),
                    "generation_domain_expectations": step.get(
                        "generation_domain_expectations"
                    ),
                    "no_unrelated_mutation": step.get("no_unrelated_mutation"),
                    "precondition": step.get("precondition"),
                    "deterministic_fallback": step.get("deterministic_fallback"),
                }
            )
        normalized[scenario_id] = {
            "scenario_id": scenario_id,
            "category": record.get("category"),
            "flow": record.get("flow"),
            "description": record.get("description"),
            "coverage_status": record.get("coverage_status"),
            "steps": steps,
        }
    return normalized


def _validate_runtime_transition_sequence_registry(
    *,
    runtime_records: list[dict[str, Any]],
    runtime_path: Path,
    transition_sequence_records: list[Any],
    runtime_transition_ids: set[str],
    action_ids: set[str],
    action_transition_ids: dict[str, str],
    action_coverage_by_action: dict[str, dict[str, Any]],
    event_ids: set[str],
    event_transition_ids: dict[str, str],
    event_coverage_by_event_id: dict[str, dict[str, Any]],
    runtime_invariant_ids: set[str],
    runtime_invariant_transition_ids: dict[str, set[str]],
    runtime_diff_harness_ids: set[str],
    runtime_diff_harness_transition_ids: dict[str, set[str]],
    runtime_generation_domain_ids: set[str],
) -> list[str]:
    failures: list[str] = []
    expected_sequences = _normalize_transition_sequence_records(
        transition_sequence_records
    )
    expected_ids = set(expected_sequences)
    covered_ids: set[str] = set()

    for index, record in enumerate(runtime_records):
        label = f"runtime_transition_sequence[{index}]"
        runtime_id = record.get("scenario_id")
        if not isinstance(runtime_id, str) or not runtime_id.strip():
            failures.append(f"{label}: scenario_id must be a non-empty string")
            continue
        if runtime_id in covered_ids:
            failures.append(
                f"{label}: duplicate runtime transition sequence scenario_id: {runtime_id}"
            )
        covered_ids.add(runtime_id)

        expected_record = expected_sequences.get(runtime_id)
        if expected_record is None:
            failures.append(
                f"{label}: scenario_id does not match a transition sequence: {runtime_id}"
            )
        else:
            for field in (
                "category",
                "flow",
                "description",
                "coverage_status",
                "steps",
            ):
                if record.get(field) != expected_record.get(field):
                    failures.append(
                        f"{label}: runtime {field} does not match transition "
                        f"sequence {runtime_id}: {record.get(field)}"
                    )

        for field in ("category", "flow", "description", "coverage_status"):
            value = record.get(field)
            if not isinstance(value, str) or not value.strip():
                failures.append(f"{label}: {field} must be a non-empty string")
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="coverage_status",
                valid_statuses=VALID_SEQUENCE_COVERAGE_STATUSES,
            )
        )

        steps = record.get("steps")
        if not isinstance(steps, list) or not steps:
            failures.append(f"{label}: steps must be non-empty")
            continue

        previous_ordinal = 0
        step_ids: set[str] = set()
        ordinals: set[int] = set()
        for step_index, step in enumerate(steps):
            step_label = f"{label}.step[{step_index}]"
            if not isinstance(step, dict):
                failures.append(f"{step_label}: record must be an object")
                continue

            ordinal = step.get("ordinal")
            if not isinstance(ordinal, int) or ordinal < 1:
                failures.append(f"{step_label}: ordinal must be a positive integer")
            else:
                if ordinal in ordinals:
                    failures.append(f"{step_label}: duplicate ordinal: {ordinal}")
                if ordinal <= previous_ordinal:
                    failures.append(
                        f"{step_label}: ordinal must be greater than previous step ordinal"
                    )
                ordinals.add(ordinal)
                previous_ordinal = ordinal

            step_id = step.get("step_id")
            if not isinstance(step_id, str) or not step_id.strip():
                failures.append(f"{step_label}: step_id must be a non-empty string")
            else:
                if step_id in step_ids:
                    failures.append(f"{step_label}: duplicate step_id: {step_id}")
                step_ids.add(step_id)

            transition_id = step.get("transition_id")
            if not isinstance(transition_id, str) or not transition_id.strip():
                failures.append(
                    f"{step_label}: transition_id must be a non-empty string"
                )
            elif transition_id not in runtime_transition_ids:
                failures.append(
                    f"{step_label}: transition_id does not match runtime transition "
                    f"registry: {transition_id}"
                )

            action_id = step.get("stimulus_action_id")
            event_id = step.get("stimulus_event_id")
            if action_id is None and event_id is None:
                failures.append(
                    f"{step_label}: stimulus must include action_id or event_id"
                )
            if action_id is not None:
                if not isinstance(action_id, str) or not action_id.strip():
                    failures.append(
                        f"{step_label}: stimulus_action_id must be a non-empty string"
                    )
                elif action_id not in action_ids:
                    failures.append(
                        f"{step_label}: stimulus_action_id references unknown action: "
                        f"{action_id}"
                    )
                elif isinstance(transition_id, str) and transition_id.strip():
                    action_transition_id = action_transition_ids.get(action_id)
                    if action_transition_id != transition_id:
                        failures.append(
                            f"{step_label}: stimulus_action_id {action_id} maps to "
                            f"transition_id {action_transition_id}, not "
                            f"step.transition_id {transition_id}"
                        )
            if event_id is not None:
                if not isinstance(event_id, str) or not event_id.strip():
                    failures.append(
                        f"{step_label}: stimulus_event_id must be a non-empty string"
                    )
                elif event_id not in event_ids:
                    failures.append(
                        f"{step_label}: stimulus_event_id references unknown event id: "
                        f"{event_id}"
                    )
                elif isinstance(transition_id, str) and transition_id.strip():
                    event_transition_id = event_transition_ids.get(event_id)
                    if event_transition_id != transition_id:
                        failures.append(
                            f"{step_label}: stimulus_event_id {event_id} maps to "
                            f"transition_id {event_transition_id}, not "
                            f"step.transition_id {transition_id}"
                        )

            failures.extend(
                _validate_step_coverage_refs(
                    value=step.get("action_coverage_refs"),
                    valid_ids=action_ids,
                    coverage_records_by_id=action_coverage_by_action,
                    label=step_label,
                    field="action_coverage_refs",
                    reference_label="action coverage record",
                    stimulus_id=action_id,
                    transition_id=transition_id,
                    step_invariant_ids=step.get("invariant_ids"),
                    step_diff_harness_ids=step.get("diff_harness_ids"),
                    step_generation_domain_expectations=step.get(
                        "generation_domain_expectations"
                    ),
                )
            )
            failures.extend(
                _validate_step_coverage_refs(
                    value=step.get("event_coverage_refs"),
                    valid_ids=event_ids,
                    coverage_records_by_id=event_coverage_by_event_id,
                    label=step_label,
                    field="event_coverage_refs",
                    reference_label="event coverage record",
                    stimulus_id=event_id,
                    transition_id=transition_id,
                    step_invariant_ids=step.get("invariant_ids"),
                    step_diff_harness_ids=step.get("diff_harness_ids"),
                    step_generation_domain_expectations=step.get(
                        "generation_domain_expectations"
                    ),
                )
            )

            for field, valid_ids, reference_label in (
                ("invariant_ids", runtime_invariant_ids, "runtime invariant id"),
                ("diff_harness_ids", runtime_diff_harness_ids, "runtime diff harness id"),
            ):
                values = step.get(field)
                failures.extend(
                    _validate_step_reference_list(
                        value=values,
                        valid_ids=valid_ids,
                        label=step_label,
                        field=field,
                        reference_label=reference_label,
                    )
                )
            failures.extend(
                _validate_after_step_contract_evaluation(step=step, label=step_label)
            )
            failures.extend(
                _validate_step_invariant_transition_alignment(
                    invariant_refs=step.get("invariant_ids"),
                    transition_id=transition_id,
                    invariant_transition_ids=runtime_invariant_transition_ids,
                    label=step_label,
                )
            )
            failures.extend(
                _validate_step_diff_harness_transition_alignment(
                    diff_harness_refs=step.get("diff_harness_ids"),
                    transition_id=transition_id,
                    diff_harness_transition_ids=runtime_diff_harness_transition_ids,
                    label=step_label,
                )
            )

            failures.extend(
                _validate_generation_expectations(
                    value=step.get("generation_domain_expectations"),
                    label=step_label,
                    generation_domain_ids=runtime_generation_domain_ids,
                )
            )

            expected_result = step.get("expected_result")
            if not isinstance(expected_result, str) or not expected_result.strip():
                failures.append(
                    f"{step_label}: expected_result must be a non-empty string"
                )
            elif expected_result not in {"allowed", "blocked", "fallback", "invalid"}:
                failures.append(
                    f"{step_label}: unknown expected_result: {expected_result}"
                )

            precondition = step.get("precondition")
            if precondition is not None and (
                not isinstance(precondition, str) or not precondition.strip()
            ):
                failures.append(f"{step_label}: precondition must be non-empty")

            if isinstance(step, dict) and _requires_deterministic_fallback(step):
                failures.extend(
                    _validate_deterministic_fallback(record=step, label=step_label)
                )
            fallback = step.get("deterministic_fallback")
            if fallback is not None and not isinstance(fallback, dict):
                failures.append(
                    f"{step_label}: deterministic_fallback must be an object"
                )

            no_unrelated_required = _requires_no_unrelated_mutation(step)
            if no_unrelated_required or step.get("no_unrelated_mutation") is not None:
                failures.extend(
                    _validate_no_unrelated_mutation(
                        record=step,
                        label=step_label,
                        diff_harness_ids=runtime_diff_harness_ids,
                        step_diff_harness_refs=step.get("diff_harness_ids"),
                        transition_id=transition_id,
                        diff_harness_transition_ids=runtime_diff_harness_transition_ids,
                        required=no_unrelated_required,
                    )
                )
            no_unrelated = step.get("no_unrelated_mutation")
            if no_unrelated is not None and not isinstance(no_unrelated, dict):
                failures.append(f"{step_label}: no_unrelated_mutation must be an object")

    missing_ids = sorted(expected_ids - covered_ids)
    if missing_ids:
        failures.append(
            f"{runtime_path}: runtime transition sequence registry missing "
            "scenario id(s): " + ", ".join(missing_ids)
        )

    return failures

def validate_contract(
    transitions_path: Path,
    action_coverage_path: Path,
    actions_header_path: Path,
    event_coverage_path: Path = DEFAULT_EVENT_COVERAGE,
    owner_fields_path: Path = DEFAULT_OWNER_FIELDS,
    dispatch_surfaces_path: Path = DEFAULT_DISPATCH_SURFACES,
    invariants_path: Path = DEFAULT_INVARIANTS,
    generation_domains_path: Path = DEFAULT_GENERATION_DOMAINS,
    diff_harness_path: Path = DEFAULT_DIFF_HARNESS,
    transition_sequences_path: Path = DEFAULT_TRANSITION_SEQUENCES,
    action_runtime_path: Path = DEFAULT_ACTION_RUNTIME,
    *,
    repository_root: Path = REPO_ROOT,
) -> list[str]:
    failures: list[str] = []
    transitions_doc, transition_load_failures = _load_json(transitions_path)
    action_coverage_doc, action_coverage_load_failures = _load_json(action_coverage_path)
    event_coverage_doc, event_coverage_load_failures = _load_json(event_coverage_path)
    owner_fields_doc, owner_fields_load_failures = _load_json(owner_fields_path)
    dispatch_surfaces_doc, dispatch_surfaces_load_failures = _load_json(
        dispatch_surfaces_path
    )
    invariants_doc, invariants_load_failures = _load_json(invariants_path)
    generation_domains_doc, generation_domains_load_failures = _load_json(
        generation_domains_path
    )
    diff_harness_doc, diff_harness_load_failures = _load_json(diff_harness_path)
    transition_sequences_doc, transition_sequences_load_failures = _load_json(
        transition_sequences_path
    )
    enum_actions, enum_failures = _parse_ytnova_actions(actions_header_path)
    runtime_action_records, runtime_action_failures = _parse_runtime_action_transitions(
        action_runtime_path
    )
    runtime_action_coverage_records, runtime_action_coverage_failures = (
        _parse_runtime_action_coverage_registry(action_runtime_path)
    )
    runtime_event_coverage_records, runtime_event_coverage_failures = (
        _parse_runtime_event_coverage_registry(action_runtime_path)
    )
    runtime_transition_records, runtime_transition_failures = (
        _parse_runtime_transition_registry(action_runtime_path)
    )
    runtime_owner_field_records, runtime_owner_field_failures = (
        _parse_runtime_owner_field_registry(action_runtime_path)
    )
    runtime_dispatch_surface_records, runtime_dispatch_surface_failures = (
        _parse_runtime_dispatch_surface_registry(action_runtime_path)
    )
    runtime_invariant_records, runtime_invariant_failures = (
        _parse_runtime_invariant_registry(action_runtime_path)
    )
    runtime_generation_domain_records, runtime_generation_domain_failures = (
        _parse_runtime_generation_domain_registry(action_runtime_path)
    )
    runtime_diff_harness_records, runtime_diff_harness_failures = (
        _parse_runtime_diff_harness_registry(action_runtime_path)
    )
    runtime_transition_sequence_records, runtime_transition_sequence_failures = (
        _parse_runtime_transition_sequence_registry(action_runtime_path)
    )
    failures.extend(transition_load_failures)
    failures.extend(action_coverage_load_failures)
    failures.extend(event_coverage_load_failures)
    failures.extend(owner_fields_load_failures)
    failures.extend(dispatch_surfaces_load_failures)
    failures.extend(invariants_load_failures)
    failures.extend(generation_domains_load_failures)
    failures.extend(diff_harness_load_failures)
    failures.extend(transition_sequences_load_failures)
    failures.extend(enum_failures)
    failures.extend(runtime_action_failures)
    failures.extend(runtime_action_coverage_failures)
    failures.extend(runtime_event_coverage_failures)
    failures.extend(runtime_transition_failures)
    failures.extend(runtime_owner_field_failures)
    failures.extend(runtime_dispatch_surface_failures)
    failures.extend(runtime_invariant_failures)
    failures.extend(runtime_generation_domain_failures)
    failures.extend(runtime_diff_harness_failures)
    failures.extend(runtime_transition_sequence_failures)
    if failures:
        return failures

    for doc, path, runtime_records in (
        (transitions_doc, transitions_path, runtime_transition_records),
        (action_coverage_doc, action_coverage_path, runtime_action_coverage_records),
        (event_coverage_doc, event_coverage_path, runtime_event_coverage_records),
        (owner_fields_doc, owner_fields_path, runtime_owner_field_records),
        (dispatch_surfaces_doc, dispatch_surfaces_path, runtime_dispatch_surface_records),
        (invariants_doc, invariants_path, runtime_invariant_records),
        (generation_domains_doc, generation_domains_path, runtime_generation_domain_records),
        (diff_harness_doc, diff_harness_path, runtime_diff_harness_records),
        (transition_sequences_doc, transition_sequences_path, runtime_transition_sequence_records),
    ):
        failures.extend(
            _validate_runtime_backed_registry_notes(
                doc=doc,
                path=path,
                runtime_records=runtime_records,
            )
        )
    if failures:
        return failures

    runtime_invariant_ids = {
        record["invariant_id"] for record in runtime_invariant_records
    }
    runtime_invariant_transition_ids = _invariant_transition_ids_by_invariant(
        runtime_invariant_records
    )
    runtime_invariant_protected_fields = _invariant_protected_fields_by_invariant(
        runtime_invariant_records
    )
    runtime_invariant_protected_fields_by_surface = (
        _invariant_protected_fields_by_dispatch_surface(
            runtime_invariant_records
        )
    )
    runtime_invariant_protected_fields_by_transition = (
        _invariant_protected_fields_by_transition(runtime_invariant_records)
    )
    if isinstance(invariants_doc, dict) and isinstance(
        invariants_doc.get("invariants"), list
    ):
        invariant_records = invariants_doc["invariants"]
    else:
        invariant_records = []
    invariant_ids = _collect_string_ids(
        invariants_doc,
        collection_key="invariants",
        id_field="invariant_id",
    )
    invariant_transition_ids = _invariant_transition_ids_by_invariant(
        invariant_records
    )
    invariant_protected_fields = _invariant_protected_fields_by_invariant(
        invariant_records
    )
    invariant_protected_fields_by_surface = (
        _invariant_protected_fields_by_dispatch_surface(invariant_records)
    )
    invariant_protected_fields_by_transition = (
        _invariant_protected_fields_by_transition(invariant_records)
    )

    registered_owner_fields, owner_field_failures = _validate_owner_fields(
        owner_fields_doc=owner_fields_doc,
        owner_fields_path=owner_fields_path,
        runtime_invariant_ids=runtime_invariant_ids,
        invariant_protected_fields=invariant_protected_fields,
    )
    failures.extend(owner_field_failures)
    if isinstance(owner_fields_doc, dict) and isinstance(
        owner_fields_doc.get("owner_fields"), list
    ):
        owner_field_records = owner_fields_doc["owner_fields"]
    else:
        owner_field_records = []
    failures.extend(
        _validate_runtime_owner_field_registry(
            runtime_records=runtime_owner_field_records,
            runtime_path=action_runtime_path,
            owner_field_records=owner_field_records,
            runtime_invariant_ids=runtime_invariant_ids,
            runtime_invariant_protected_fields=runtime_invariant_protected_fields,
        )
    )

    if not isinstance(transitions_doc, dict):
        failures.append(f"{transitions_path}: top-level value must be an object")
        transitions = []
    else:
        transitions = transitions_doc.get("transitions")
        if not isinstance(transitions, list) or not transitions:
            failures.append(f"{transitions_path}: transitions must be a non-empty list")
            transitions = []

    transition_ids: dict[str, dict[str, Any]] = {}
    categories: set[str] = set()
    for index, record in enumerate(transitions):
        label = f"transition[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_TRANSITION_FIELDS,
                list_fields=LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="boundary_status",
                valid_statuses=VALID_BOUNDARY_STATUSES,
            )
        )
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )
        failures.extend(
            _validate_transition_write_set_has_invariant_coverage(
                record=record,
                protected_fields_by_transition=invariant_protected_fields_by_transition,
                label=label,
            )
        )
        transition_id = record.get("id")
        if isinstance(transition_id, str) and transition_id.strip():
            if transition_id in transition_ids:
                failures.append(f"{label}: duplicate id: {transition_id}")
            transition_ids[transition_id] = record
        category = record.get("category")
        if isinstance(category, str) and category.strip():
            categories.add(category)

    missing_categories = sorted(REQUIRED_TRANSITION_CATEGORIES - categories)
    if missing_categories:
        failures.append(
            "transition matrix missing required category/categories: "
            + ", ".join(missing_categories)
        )
    failures.extend(
        _validate_runtime_transition_registry(
            runtime_records=runtime_transition_records,
            runtime_path=action_runtime_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
            runtime_invariant_protected_fields_by_transition=(
                runtime_invariant_protected_fields_by_transition
            ),
        )
    )

    generation_owner_fields, generation_domain_failures = _validate_generation_domains(
        generation_domains_doc=generation_domains_doc,
        generation_domains_path=generation_domains_path,
        transition_ids=transition_ids,
        registered_owner_fields=registered_owner_fields,
    )
    failures.extend(generation_domain_failures)
    if isinstance(generation_domains_doc, dict) and isinstance(
        generation_domains_doc.get("generation_domains"), list
    ):
        generation_domain_records = generation_domains_doc["generation_domains"]
    else:
        generation_domain_records = []
    generation_domain_owner_fields = {
        record["domain_id"]: record["generation_owner_field"]
        for record in generation_domain_records
        if isinstance(record, dict)
        and isinstance(record.get("domain_id"), str)
        and record["domain_id"].strip()
        and isinstance(record.get("generation_owner_field"), str)
        and record["generation_owner_field"].strip()
    }
    runtime_generation_domain_owner_fields = {
        record["domain_id"]: record["generation_owner_field"]
        for record in runtime_generation_domain_records
        if isinstance(record, dict)
        and isinstance(record.get("domain_id"), str)
        and record["domain_id"].strip()
        and isinstance(record.get("generation_owner_field"), str)
        and record["generation_owner_field"].strip()
    }
    failures.extend(
        _validate_runtime_generation_domain_registry(
            runtime_records=runtime_generation_domain_records,
            runtime_path=action_runtime_path,
            generation_domain_records=generation_domain_records,
            runtime_owner_fields={
                record["field"] for record in runtime_owner_field_records
            },
            runtime_transition_ids={
                record["id"] for record in runtime_transition_records
            },
        )
    )
    failures.extend(
        _validate_transition_generation_effects(
            transitions=transitions,
            generation_owner_fields=generation_owner_fields,
        )
    )
    failures.extend(
        _validate_generation_write_coverage(
            transitions=transitions,
            generation_domain_records=generation_domain_records,
            label_prefix="transition",
        )
    )
    failures.extend(
        _validate_generation_write_coverage(
            transitions=runtime_transition_records,
            generation_domain_records=runtime_generation_domain_records,
            label_prefix="runtime_transition",
        )
    )

    if isinstance(diff_harness_doc, dict) and isinstance(
        diff_harness_doc.get("diff_harness_checks"), list
    ):
        diff_harness_records = diff_harness_doc["diff_harness_checks"]
    else:
        diff_harness_records = []
    diff_harness_ids = _collect_string_ids(
        diff_harness_doc,
        collection_key="diff_harness_checks",
        id_field="harness_id",
    )
    diff_harness_transition_ids = _diff_harness_transition_ids_by_harness(
        diff_harness_records
    )
    diff_harness_owner_field_refs_by_harness = _diff_harness_string_refs_by_harness(
        diff_harness_records,
        "owner_field_refs",
    )
    diff_harness_invariant_ids_by_harness = _diff_harness_string_refs_by_harness(
        diff_harness_records,
        "invariant_ids",
    )
    diff_harness_generation_domain_ids_by_harness = (
        _diff_harness_string_refs_by_harness(
            diff_harness_records,
            "generation_domain_ids",
        )
    )
    runtime_diff_harness_transition_ids = _diff_harness_transition_ids_by_harness(
        runtime_diff_harness_records
    )
    runtime_diff_harness_owner_field_refs_by_harness = (
        _diff_harness_string_refs_by_harness(
            runtime_diff_harness_records,
            "owner_field_refs",
        )
    )
    runtime_diff_harness_invariant_ids_by_harness = (
        _diff_harness_string_refs_by_harness(
            runtime_diff_harness_records,
            "invariant_ids",
        )
    )
    runtime_diff_harness_generation_domain_ids_by_harness = (
        _diff_harness_string_refs_by_harness(
            runtime_diff_harness_records,
            "generation_domain_ids",
        )
    )
    if not isinstance(action_coverage_doc, dict):
        failures.append(f"{action_coverage_path}: top-level value must be an object")
        action_records = []
    else:
        action_records = action_coverage_doc.get("actions")
        if not isinstance(action_records, list) or not action_records:
            failures.append(f"{action_coverage_path}: actions must be a non-empty list")
            action_records = []
    if isinstance(dispatch_surfaces_doc, dict) and isinstance(
        dispatch_surfaces_doc.get("dispatch_surfaces"), list
    ):
        dispatch_surface_records = dispatch_surfaces_doc["dispatch_surfaces"]
    else:
        dispatch_surface_records = []
    expected_actions = set(enum_actions)
    covered_actions: set[str] = set()
    action_coverage_by_action: dict[str, dict[str, Any]] = {}
    for index, record in enumerate(action_records):
        label = f"action[{index}]"
        failures.extend(
            _validate_required_fields(
                record=record,
                required_fields=REQUIRED_ACTION_FIELDS,
                list_fields=ACTION_LIST_FIELDS,
                label=label,
            )
        )
        if not isinstance(record, dict):
            continue
        failures.extend(
            _validate_status_field(
                record=record,
                label=label,
                field="boundary_status",
                valid_statuses=VALID_BOUNDARY_STATUSES,
            )
        )
        failures.extend(
            _validate_registered_write_set(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
            )
        )
        failures.extend(
            _validate_owner_field_coverage_refs(
                record=record,
                registered_fields=registered_owner_fields,
                label=label,
                registry_label="owner-field registry",
            )
        )
        failures.extend(
            _validate_transition_sequence_refs(
                record=record,
                transition_sequence_records=(
                    transition_sequences_doc["scenarios"]
                    if isinstance(transition_sequences_doc, dict)
                    and isinstance(transition_sequences_doc.get("scenarios"), list)
                    else []
                ),
                label=label,
            )[0]
        )
        failures.extend(
            _validate_dispatch_surface_refs(
                record=record,
                dispatch_surface_records=dispatch_surface_records,
                label=label,
            )
        )
        failures.extend(
            _validate_generation_domain_refs(
                generation_domain_refs=record.get("generation_domain_refs"),
                generation_domain_records=generation_domain_records,
                transition_id=record.get("transition_id"),
                label=label,
            )
        )
        failures.extend(
            _validate_record_invariant_refs(
                invariant_refs=record.get("invariant_refs"),
                invariant_ids=invariant_ids,
                invariant_transition_ids=invariant_transition_ids,
                invariant_protected_fields=invariant_protected_fields,
                transition_id=record.get("transition_id"),
                declared_write_set=record.get("declared_write_set"),
                label=label,
            )
        )
        failures.extend(
            _validate_coverage_diff_harness_refs(
                record=record,
                diff_harness_ids=diff_harness_ids,
                diff_harness_transition_ids=diff_harness_transition_ids,
                diff_harness_owner_field_refs=diff_harness_owner_field_refs_by_harness,
                diff_harness_invariant_ids=diff_harness_invariant_ids_by_harness,
                diff_harness_generation_domain_ids=(
                    diff_harness_generation_domain_ids_by_harness
                ),
                label=label,
            )
        )

        action = record.get("action")
        if isinstance(action, str) and action.strip():
            if action in covered_actions:
                failures.append(f"{label}: duplicate action: {action}")
            else:
                action_coverage_by_action[action] = record
            covered_actions.add(action)
            if action not in expected_actions:
                failures.append(f"{label}: unknown YtreeNovaAction enum member: {action}")

        transition_id = record.get("transition_id")
        transition_record = None
        if isinstance(transition_id, str) and transition_id.strip():
            transition_record = transition_ids.get(transition_id)
            if transition_record is None:
                failures.append(
                    f"{label}: transition_id does not match a transition id: {transition_id}"
                )

        category = record.get("category")
        if (
            isinstance(category, str)
            and category.strip()
            and transition_record is not None
            and category != transition_record.get("category")
        ):
            failures.append(
                f"{label}: category does not match transition {transition_id}: {category}"
            )
        owner = record.get("owner")
        if (
            isinstance(owner, str)
            and owner.strip()
            and transition_record is not None
            and owner != transition_record.get("owner")
        ):
            failures.append(
                f"{label}: owner does not match transition {transition_id}: {owner}"
            )

    missing_actions = sorted(expected_actions - covered_actions)
    if missing_actions:
        failures.append(
            "action coverage missing YtreeNovaAction enum member(s): "
            + ", ".join(missing_actions)
        )
    failures.extend(
        _validate_runtime_action_lookup(
            runtime_records=runtime_action_records,
            runtime_path=action_runtime_path,
            enum_actions=enum_actions,
            action_coverage_by_action=action_coverage_by_action,
            transition_ids=transition_ids,
            runtime_transition_ids={record["id"] for record in runtime_transition_records},
        )
    )
    failures.extend(
        _validate_runtime_action_coverage_registry(
            runtime_records=runtime_action_coverage_records,
            runtime_path=action_runtime_path,
            enum_actions=enum_actions,
            action_coverage_by_action=action_coverage_by_action,
            transition_ids=transition_ids,
            transition_sequence_records=(
                transition_sequences_doc["scenarios"]
                if isinstance(transition_sequences_doc, dict)
                and isinstance(transition_sequences_doc.get("scenarios"), list)
                else []
            ),
            runtime_transition_ids={
                record["id"] for record in runtime_transition_records
            },
            runtime_transition_sequence_records=runtime_transition_sequence_records,
            runtime_action_transitions=runtime_action_records,
            runtime_dispatch_surface_records=runtime_dispatch_surface_records,
            runtime_generation_domain_records=runtime_generation_domain_records,
            registered_owner_fields=registered_owner_fields,
            runtime_invariant_ids=runtime_invariant_ids,
            runtime_invariant_transition_ids=runtime_invariant_transition_ids,
            runtime_invariant_protected_fields=runtime_invariant_protected_fields,
            runtime_diff_harness_ids={
                record["harness_id"] for record in runtime_diff_harness_records
            },
            runtime_diff_harness_transition_ids=runtime_diff_harness_transition_ids,
            runtime_diff_harness_owner_field_refs=(
                runtime_diff_harness_owner_field_refs_by_harness
            ),
            runtime_diff_harness_invariant_ids=(
                runtime_diff_harness_invariant_ids_by_harness
            ),
            runtime_diff_harness_generation_domain_ids=(
                runtime_diff_harness_generation_domain_ids_by_harness
            ),
        )
    )
    failures.extend(
        _validate_runtime_action_callsites(
            action_records=action_records,
            dispatch_surface_records=dispatch_surface_records,
            repository_root=repository_root,
            action_runtime_path=action_runtime_path,
        )
    )

    failures.extend(
        _validate_event_coverage(
            event_coverage_doc=event_coverage_doc,
            event_coverage_path=event_coverage_path,
            transition_ids=transition_ids,
            transition_sequence_records=(
                transition_sequences_doc["scenarios"]
                if isinstance(transition_sequences_doc, dict)
                and isinstance(transition_sequences_doc.get("scenarios"), list)
                else []
            ),
            dispatch_surface_records=dispatch_surface_records,
            generation_domain_records=generation_domain_records,
            registered_owner_fields=registered_owner_fields,
            invariant_ids=invariant_ids,
            invariant_transition_ids=invariant_transition_ids,
            invariant_protected_fields=invariant_protected_fields,
            diff_harness_ids=diff_harness_ids,
            diff_harness_transition_ids=diff_harness_transition_ids,
            diff_harness_owner_field_refs=diff_harness_owner_field_refs_by_harness,
            diff_harness_invariant_ids=diff_harness_invariant_ids_by_harness,
            diff_harness_generation_domain_ids=(
                diff_harness_generation_domain_ids_by_harness
            ),
        )
    )
    failures.extend(
        _validate_runtime_event_coverage_registry(
            runtime_records=runtime_event_coverage_records,
            runtime_path=action_runtime_path,
            event_coverage_doc=event_coverage_doc,
            transition_ids=transition_ids,
            runtime_transition_sequence_records=runtime_transition_sequence_records,
            runtime_dispatch_surface_records=runtime_dispatch_surface_records,
            runtime_generation_domain_records=runtime_generation_domain_records,
            registered_owner_fields=registered_owner_fields,
            runtime_transition_ids={record["id"] for record in runtime_transition_records},
            runtime_invariant_ids=runtime_invariant_ids,
            runtime_invariant_transition_ids=runtime_invariant_transition_ids,
            runtime_invariant_protected_fields=runtime_invariant_protected_fields,
            runtime_diff_harness_ids={
                record["harness_id"] for record in runtime_diff_harness_records
            },
            runtime_diff_harness_transition_ids=runtime_diff_harness_transition_ids,
            runtime_diff_harness_owner_field_refs=(
                runtime_diff_harness_owner_field_refs_by_harness
            ),
            runtime_diff_harness_invariant_ids=(
                runtime_diff_harness_invariant_ids_by_harness
            ),
            runtime_diff_harness_generation_domain_ids=(
                runtime_diff_harness_generation_domain_ids_by_harness
            ),
        )
    )
    failures.extend(
        _validate_runtime_event_callsites(
            runtime_records=runtime_event_coverage_records,
            runtime_dispatch_surface_records=runtime_dispatch_surface_records,
            repository_root=repository_root,
            action_runtime_path=action_runtime_path,
        )
    )

    failures.extend(
        _validate_dispatch_surfaces(
            dispatch_surfaces_doc=dispatch_surfaces_doc,
            dispatch_surfaces_path=dispatch_surfaces_path,
            transition_ids=transition_ids,
            runtime_dispatch_surface_ids={
                record["surface_id"] for record in runtime_dispatch_surface_records
            },
            registered_owner_fields=registered_owner_fields,
            invariant_protected_fields_by_surface=invariant_protected_fields_by_surface,
            transition_sequence_records=(
                transition_sequences_doc["scenarios"]
                if isinstance(transition_sequences_doc, dict)
                and isinstance(transition_sequences_doc.get("scenarios"), list)
                else []
            ),
            diff_harness_owner_field_refs=diff_harness_owner_field_refs_by_harness,
            invariant_protected_fields=invariant_protected_fields,
            repository_root=repository_root,
        )
    )
    failures.extend(
        _validate_runtime_dispatch_surface_registry(
            runtime_records=runtime_dispatch_surface_records,
            runtime_path=action_runtime_path,
            dispatch_surface_records=dispatch_surface_records,
            runtime_transition_records={
                record["id"]: record for record in runtime_transition_records
            },
            runtime_transition_ids={record["id"] for record in runtime_transition_records},
            runtime_invariant_protected_fields_by_surface=(
                runtime_invariant_protected_fields_by_surface
            ),
            runtime_transition_sequence_records=runtime_transition_sequence_records,
            runtime_diff_harness_owner_field_refs=(
                runtime_diff_harness_owner_field_refs_by_harness
            ),
            runtime_invariant_protected_fields=runtime_invariant_protected_fields,
        )
    )
    failures.extend(
        _validate_runtime_dispatch_surface_callsites(
            runtime_records=runtime_dispatch_surface_records,
            repository_root=repository_root,
            action_runtime_path=action_runtime_path,
        )
    )
    dispatch_surface_ids = _collect_string_ids(
        dispatch_surfaces_doc,
        collection_key="dispatch_surfaces",
        id_field="surface_id",
    )
    runtime_dispatch_surface_ids = {
        record["surface_id"] for record in runtime_dispatch_surface_records
    }
    if isinstance(diff_harness_doc, dict) and isinstance(
        diff_harness_doc.get("diff_harness_checks"), list
    ):
        diff_harness_records = diff_harness_doc["diff_harness_checks"]
    else:
        diff_harness_records = []
    diff_harness_owner_field_refs = _diff_harness_owner_field_refs(
        diff_harness_records
    )
    runtime_diff_harness_owner_field_refs = _diff_harness_owner_field_refs(
        runtime_diff_harness_records
    )
    failures.extend(
        _validate_invariants(
            invariants_doc=invariants_doc,
            invariants_path=invariants_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
            dispatch_surface_ids=dispatch_surface_ids,
            diff_harness_owner_field_refs=diff_harness_owner_field_refs,
        )
    )
    if isinstance(invariants_doc, dict) and isinstance(
        invariants_doc.get("invariants"), list
    ):
        invariant_records = invariants_doc["invariants"]
    else:
        invariant_records = []
    failures.extend(
        _validate_runtime_invariant_registry(
            runtime_records=runtime_invariant_records,
            runtime_path=action_runtime_path,
            invariant_records=invariant_records,
            runtime_transition_ids={record["id"] for record in runtime_transition_records},
            dispatch_surface_ids=runtime_dispatch_surface_ids,
            runtime_diff_harness_owner_field_refs=(
                runtime_diff_harness_owner_field_refs
            ),
        )
    )
    generation_domain_ids = _collect_string_ids(
        generation_domains_doc,
        collection_key="generation_domains",
        id_field="domain_id",
    )
    diff_harness_ids = _collect_string_ids(
        diff_harness_doc,
        collection_key="diff_harness_checks",
        id_field="harness_id",
    )
    failures.extend(
        _validate_appstate_diff_harness(
            diff_harness_doc=diff_harness_doc,
            diff_harness_path=diff_harness_path,
            transition_ids=transition_ids,
            registered_owner_fields=registered_owner_fields,
            invariant_ids=invariant_ids,
            invariant_transition_ids=_invariant_transition_ids_by_invariant(
                invariant_records
            ),
            invariant_protected_fields=_invariant_protected_fields_by_invariant(
                invariant_records
            ),
            generation_domain_ids=generation_domain_ids,
        )
    )
    failures.extend(
        _validate_generation_diff_harness_coverage(
            generation_domain_records=generation_domain_records,
            diff_harness_records=diff_harness_records,
            label_prefix="generation_domain",
        )
    )
    failures.extend(
        _validate_runtime_diff_harness_registry(
            runtime_records=runtime_diff_harness_records,
            runtime_path=action_runtime_path,
            diff_harness_records=diff_harness_records,
            runtime_transition_ids={
                record["id"] for record in runtime_transition_records
            },
            runtime_owner_fields={
                record["field"] for record in runtime_owner_field_records
            },
            runtime_invariant_ids={
                record["invariant_id"] for record in runtime_invariant_records
            },
            runtime_invariant_transition_ids=_invariant_transition_ids_by_invariant(
                runtime_invariant_records
            ),
            runtime_invariant_protected_fields=_invariant_protected_fields_by_invariant(
                runtime_invariant_records
            ),
            runtime_generation_domain_ids={
                record["domain_id"] for record in runtime_generation_domain_records
            },
        )
    )
    failures.extend(
        _validate_generation_diff_harness_coverage(
            generation_domain_records=runtime_generation_domain_records,
            diff_harness_records=runtime_diff_harness_records,
            label_prefix="runtime_generation_domain",
        )
    )
    failures.extend(
        _validate_diff_harness_write_coverage(
            transitions=transitions,
            diff_harness_records=diff_harness_records,
            label_prefix="transition",
        )
    )
    failures.extend(
        _validate_diff_harness_write_coverage(
            transitions=runtime_transition_records,
            diff_harness_records=runtime_diff_harness_records,
            label_prefix="runtime_transition",
        )
    )
    action_ids = _collect_string_ids(
        action_coverage_doc,
        collection_key="actions",
        id_field="action",
    )
    action_transition_ids = _collect_transition_ids_by_string_id(
        action_coverage_doc,
        collection_key="actions",
        id_field="action",
    )
    if isinstance(action_coverage_doc, dict) and isinstance(
        action_coverage_doc.get("actions"), list
    ):
        action_coverage_by_action = {
            record["action"]: record
            for record in action_coverage_doc["actions"]
            if isinstance(record, dict)
            and isinstance(record.get("action"), str)
            and record["action"].strip()
        }
    else:
        action_coverage_by_action = {}
    event_ids = _collect_string_ids(
        event_coverage_doc,
        collection_key="events",
        id_field="event_id",
    )
    event_transition_ids = _collect_transition_ids_by_string_id(
        event_coverage_doc,
        collection_key="events",
        id_field="event_id",
    )
    if isinstance(event_coverage_doc, dict) and isinstance(
        event_coverage_doc.get("events"), list
    ):
        event_coverage_by_event_id = {
            record["event_id"]: record
            for record in event_coverage_doc["events"]
            if isinstance(record, dict)
            and isinstance(record.get("event_id"), str)
            and record["event_id"].strip()
        }
    else:
        event_coverage_by_event_id = {}
    failures.extend(
        _validate_appstate_transition_sequences(
            transition_sequences_doc=transition_sequences_doc,
            transition_sequences_path=transition_sequences_path,
            transition_ids=transition_ids,
            action_ids=action_ids,
            action_transition_ids=action_transition_ids,
            action_coverage_by_action=action_coverage_by_action,
            event_ids=event_ids,
            event_transition_ids=event_transition_ids,
            event_coverage_by_event_id=event_coverage_by_event_id,
            invariant_ids=invariant_ids,
            invariant_transition_ids=_invariant_transition_ids_by_invariant(
                invariant_records
            ),
            diff_harness_ids=diff_harness_ids,
            diff_harness_transition_ids=_diff_harness_transition_ids_by_harness(
                diff_harness_records
            ),
            generation_domain_ids=generation_domain_ids,
        )
    )
    if isinstance(transition_sequences_doc, dict) and isinstance(
        transition_sequences_doc.get("scenarios"), list
    ):
        transition_sequence_records = transition_sequences_doc["scenarios"]
    else:
        transition_sequence_records = []
    failures.extend(
        _validate_runtime_transition_sequence_registry(
            runtime_records=runtime_transition_sequence_records,
            runtime_path=action_runtime_path,
            transition_sequence_records=transition_sequence_records,
            runtime_transition_ids={
                record["id"] for record in runtime_transition_records
            },
            action_ids=action_ids,
            action_transition_ids=action_transition_ids,
            action_coverage_by_action=action_coverage_by_action,
            event_ids=event_ids,
            event_transition_ids=event_transition_ids,
            event_coverage_by_event_id=event_coverage_by_event_id,
            runtime_invariant_ids={
                record["invariant_id"] for record in runtime_invariant_records
            },
            runtime_invariant_transition_ids=_invariant_transition_ids_by_invariant(
                runtime_invariant_records
            ),
            runtime_diff_harness_ids={
                record["harness_id"] for record in runtime_diff_harness_records
            },
            runtime_diff_harness_transition_ids=_diff_harness_transition_ids_by_harness(
                runtime_diff_harness_records
            ),
            runtime_generation_domain_ids={
                record["domain_id"] for record in runtime_generation_domain_records
            },
        )
    )

    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--transitions", type=Path, default=DEFAULT_TRANSITIONS)
    parser.add_argument("--action-coverage", type=Path, default=DEFAULT_ACTION_COVERAGE)
    parser.add_argument("--event-coverage", type=Path, default=DEFAULT_EVENT_COVERAGE)
    parser.add_argument("--owner-fields", type=Path, default=DEFAULT_OWNER_FIELDS)
    parser.add_argument(
        "--dispatch-surfaces", type=Path, default=DEFAULT_DISPATCH_SURFACES
    )
    parser.add_argument("--invariants", type=Path, default=DEFAULT_INVARIANTS)
    parser.add_argument(
        "--generation-domains", type=Path, default=DEFAULT_GENERATION_DOMAINS
    )
    parser.add_argument("--diff-harness", type=Path, default=DEFAULT_DIFF_HARNESS)
    parser.add_argument(
        "--transition-sequences", type=Path, default=DEFAULT_TRANSITION_SEQUENCES
    )
    parser.add_argument("--actions-header", type=Path, default=DEFAULT_ACTION_HEADER)
    parser.add_argument("--action-runtime", type=Path, default=DEFAULT_ACTION_RUNTIME)
    args = parser.parse_args()

    failures = validate_contract(
        args.transitions,
        args.action_coverage,
        args.actions_header,
        args.event_coverage,
        args.owner_fields,
        args.dispatch_surfaces,
        args.invariants,
        args.generation_domains,
        args.diff_harness,
        args.transition_sequences,
        args.action_runtime,
    )
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        print(f"FAIL: AppState contract guard failed ({len(failures)} issue(s))")
        return 1
    print("PASS: AppState contract guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
