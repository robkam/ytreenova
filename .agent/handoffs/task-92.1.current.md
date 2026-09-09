# Footer Command Completeness

## Mission
Complete **Footer Command Completeness Before Contextual F1** by repairing every audited footer mismatch and proving capability-aligned command availability across all six footer surfaces.

## Behavioral contract
Every visible footer command works through the active mode's dispatch path, every supported command is visible, unsupported commands are absent, and archive mutation visibility follows the same capability flags enforced by dispatch.

## In-scope inventory
- Source tracker: `docs/ROADMAP.md`; retain the completed footer audit, restore the read-only signpost contract, record the final six-surface inventory, and close the repair acceptance criteria.
- Footer registries: `src/ui/display.c` filesystem and archive directory/file arrays plus preview and navigation arrays.
- Capability projection: `ActiveFooterVolumeIsArchive()`, archive applicability/capability resolution, and `ResolveFooterCommandList()`.
- Packing surfaces: directory, file, preview command rows, and preview navigation row must all consume the resolved command count.
- Archive directory parity: both archive-root navigation variants must advertise `Invert`, whose shared directory dispatch has no archive mutation requirement.
- Archive preview parity: exclude archive-unsupported `Attributes`, `Edit`, `Newfile`, `Execute`, and `Archive` while preserving browse, copy-out, tagging, output, and view actions.
- Read-only archive presentation: directory, file, and preview footers use the fixed-width `READONLY` signpost when no mutation capability is available; writable archives retain `COMMANDS`.
- Dispatch and compatibility seams: `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/ctrl_file_ops.c`, preview action filtering, and archive mutation guards remain the authority checked against footer visibility.
- Runtime matrix: filesystem directory/file/preview; writable archive directory/file/preview; read-only archive directory/file/preview; tagged and untagged archive file/preview states; `Global` and `Showall` file-list states.
- Regression coverage: focused PTY assertions use semantic footer tokens rather than fixed coordinates or full screen grids, and directly exercise representative advertised actions.
- Help projections: authored English/German contextual `F1`, generated runtime help, authored man help, and generated man/usage projections state that plain `I` inverts only matching visible entries in the active list or selected directory; `^I` remains unavailable because terminals report it as `Tab`.
- Existing committed work in the open delivery: help projection synchronization, archive entry-column alignment, progress overlay styling, their tests, and generated contract baseline remain included and unchanged except where reconciliation requires an amendment.

## Reconciliation
- Addressed: the tracker retains the runtime audit, records the final six-surface inventory and read-only signpost contract, and marks the footer repair complete.
- Addressed: all directory, file, preview, and preview-navigation packing paths consume capability-resolved counts.
- Addressed: both archive-directory registries advertise `Invert`; plain `I` dispatch is exercised in writable and read-only archives.
- Addressed: archive file and preview footers share applicability filtering for filesystem-only actions while capability filtering retains supported read/copy-out actions and removes unsupported mutations.
- Addressed: writable archives use `COMMANDS`; CRC-CPIO provides deterministic readable, non-writable directory/file/preview coverage using `READONLY`.
- Addressed: filesystem, writable archive, read-only archive, tagged/untagged, `Global`, and `Showall` footer states are covered by semantic PTY assertions at 240 columns.
- Addressed: contextual `F1` and man/usage projections name plain `I` and its selected-directory or active-result-set boundary; the archive-directory F1 wording is exercised on the runtime surface.
- Intentionally unchanged: dispatch semantics, key mappings, archive backend capabilities, and filesystem directory/file baseline sets; the repair projects their established behavior without changing it.
- Addressed: previously committed help synchronization, archive entry-column alignment, long-name clipping, and picker-style progress rendering remain included and retain focused coverage.
- Deferred/blocked: none.

## Validation
- Red proof before runtime changes: `source .venv/bin/activate && pytest -q tests/test_footer_command_inventory.py` — 4 failed, 1 passed.
- `source .venv/bin/activate && make clean && make -j"$(nproc)"` — passed with pre-existing compiler warnings only.
- `source .venv/bin/activate && pytest -q tests/test_footer_command_inventory.py` — 5 passed; this includes direct writable/read-only footer captures, `Invert` dispatch, and archive-directory F1 wording.
- `source .venv/bin/activate && pytest -q tests/test_archive_ui.py::test_archive_zero_truncates_long_names_and_aligns_size_columns` — 1 passed.
- `source .venv/bin/activate && pytest -q tests/test_archive_ui.py::test_long_operation_progress_uses_picker_modal_style` — 1 passed.
- `source .venv/bin/activate && pytest -q tests/test_help_source_schema.py` — 10 passed.
- `source .venv/bin/activate && make qa-code-quality` — passed.
- `git diff --check` — passed.
- Local `make qa-all` intentionally not run: repository policy assigns the full pre-merge gate to PR CI for this scoped change.

## Residual risk
- Required full-QA CI and reviewer approval remain outstanding on the amended PR head.

## Delivery
- Branch: `docs/footer-command-inventory`
- PR: https://github.com/robkam/ytreenova/pull/549
- Commit: amend the existing outcome commit so the open delivery remains one coherent unit.
