# Prompt Bureaucracy Audit

This checklist is reusable internal guidance for auditing prompt necessity, redundant follow-up prompts, and remediation-family boundaries.

## Source of truth

- the roadmap prompt-bureaucracy family
- `docs/SPECIFICATION.md` §§ 4.6 and 10
- `docs/ARCHITECTURE.md` command-surface and runtime-help ownership boundaries

## Coverage inventory

| Coverage surface | Runtime/help contexts | Primary owner modules | Evidence anchors |
| --- | --- | --- | --- |
| Filesystem directory/file | `main.dir`, `main.file` | `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/interactions.c` | `etc/help/f1.en.md`, `tests/test_destination_prompt.py` |
| Archive directory/file | `main.archive-dir`, `main.archive-file` | `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c`, `src/ui/interactions.c` | `tests/test_archive_ui.py`, `tests/test_print_feature.py` |
| Aggregate views | `main.showall`, `main.global` | `src/ui/ctrl_file.c`, `src/ui/ctrl_file_ops.c` | `etc/help/f1.en.md`, `tests/test_filtering.py` |
| Split surfaces | `overlay.f8-dir`, `overlay.f8-file` | `src/ui/split_transition.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` | `tests/test_panel_isolation.py`, `tests/test_destination_prompt.py` |
| Preview overlay | `overlay.f7-dir`, `overlay.f7-file` | `src/ui/view_preview.c`, `src/ui/ctrl_file.c`, `src/ui/print_controller.c` | `tests/test_f7_preview.py`, `tests/test_print_feature.py` |
| Tagged workflows | tagged delete, tagged copy/move, tagged output | `src/ui/file_tags.c`, `src/ui/ctrl_file_ops.c`, `src/ui/print_controller.c` | `tests/test_vi_keys_mode.py`, `tests/test_core.py`, `tests/test_print_feature.py` |
| Compare / chooser prompts | compare target, history, F2 browse, applications, volumes | `src/ui/compare_request.c`, `src/ui/history_dialog.c`, `src/ui/f2_picker.c`, `src/ui/application_menu.c`, `src/ui/volume_menu.c` | `tests/test_compare_actions.py`, `tests/test_f2_vols.py` |
| Output / execute / archive prompts | output destination, execute, archive-create | `src/ui/print_controller.c`, `src/ui/interactions.c`, `src/ui/input_line.c` | `tests/test_print_feature.py`, `tests/test_help_text_contract.py` |

## Classification rules used in this audit

- **Required input:** the operator must supply data that the runtime cannot infer safely.
- **Separate user decision:** a prompt stays explicit when it captures a meaningfully different choice than the prompt before it.
- **Real safety confirmation:** a prompt is legitimate only when it gates destructive or conflict-resolving behavior with concrete source/target context.
- **Prompt-local aid:** `F1`, history, `F2` browse, completion, and similar same-layer helpers do not count as bureaucracy when they return to the same pending prompt.
- **Unnecessary bureaucracy:** a chooser, confirmation, or policy prompt is redundant once the user's intent is already clear and no new meaning or safety boundary has appeared.

## Prompt-bureaucracy matrix

| Flow family | Key(s) / entry path | Current prompt chain | Required vs explicit vs safety vs bureaucracy classification | Manual repro | Likely owner files/modules | Likely tests/docs to update | Audit status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Filter tagged-scope family | `F` on file-like surfaces | `F -> FILTER prompt [Tab tagged-only toggle] -> Enter -> filtered list` | Filter text is required input; tagged-only scope is a prompt-local explicit toggle on the same prompt; no follow-up bureaucracy remains. | File/Archive-File/Showall/Global/F7/Split-File: `F`, then `Tab` | `src/ui/interactions.c`, `src/ui/ctrl_file_ops.c` | `tests/test_filtering.py`, `etc/help/f1.en.md` | Addressed baseline |
| Compare target family | `J` on dir-like surfaces | `J -> COMPARE TARGET [scope/basis/tag toggles] -> Enter -> result` | Compare target is required input; scope/basis/tagged-result stay explicit on one prompt; the removed extra compare chooser was unnecessary bureaucracy. | Directory/Archive-Dir/Split-Dir: `J`, then `F3/F4/F5` | `src/ui/compare_request.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` | `tests/test_compare_actions.py`, compare help topics | Addressed baseline |
| Attribute date family | `A` then date-capable field | `A -> ATTRIBUTES chooser -> date prompt [F3 scope toggle] -> result` | Attribute field choice and value entry are separate decisions; date scope stays explicit as a prompt-local toggle; the removed intermediate date-scope chooser was unnecessary bureaucracy. | File/Dir/Showall/Global/F7/Split: `A`, then `D`/date-capable field | `src/ui/attr_actions.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` | attribute tests, prompt help | Addressed baseline |
| Copy/move target family | `C`, `M`, `V`, `Y`, `C-k`, `C-n` | `key -> name/pattern prompt -> destination prompt -> conflict/create-dir prompts only when needed` | Name/pattern and destination are separate explicit decisions; overwrite/create-directory prompts are real safety prompts; blanket post-target approvals are bureaucracy and are already removed. | Main/archive/split surfaces: start copy or move, enter new name, then destination | `src/ui/interactions.c`, `src/ui/ctrl_dir.c`, `src/ui/ctrl_file.c` | `tests/test_destination_prompt.py`, `docs/SPECIFICATION.md` | Addressed documented exception |
| Tagged delete family | `C-d` or VI `D` with tags | `key -> batch delete confirmation -> result` | Batch delete confirmation is a real safety prompt; a second `confirm each file` policy prompt would be bureaucracy and is no longer present. | Tag files, then `C-d`; VI mode file view: `D` | `src/ui/file_tags.c`, `src/ui/ctrl_file_ops.c` | `tests/test_vi_keys_mode.py`, delete help/spec text | Addressed baseline |
| Output export family | `O`, `C-o`, `C-w` | `O -> route chooser -> destination prompt [F3 format, separator if needed] -> result` | Route and format remain explicit where they carry different meaning; separator prompting is conditional on framed/page-break output; the removed standalone format chooser was unnecessary bureaucracy. | File/Dir/Archive/Showall/Global/F7/Split: `O`; tagged output: `C-o`/`C-w` | `src/ui/print_controller.c`, `src/cmd/print_ops.c` | `tests/test_print_feature.py`, `tests/test_help_text_contract.py`, output help topics | Addressed baseline |
| Tagged overwrite policy family | tagged copy/move with multiple overwrite conflicts | `key -> name/pattern prompt -> first concrete overwrite prompt -> remaining conflicts as needed -> result` | Name/pattern and destination remain explicit; per-conflict overwrite prompts are real safety prompts; the old pre-conflict yes/no policy prompt was unnecessary bureaucracy and is now removed so the first conflict carries the first overwrite decision. | Tag two files, trigger copy/move into conflicting destination, answer the first overwrite conflict directly | `src/ui/ctrl_file_ops.c` | `tests/test_core.py`, destination/overwrite help/spec text | Addressed baseline |
| Execute / archive-create / chooser aids | `X`, `Z`, prompt `Up`, prompt `F2`, `K`, `F9` | direct prompt or chooser entry with same-layer aids | Command input or archive target is required input; history/F2 are prompt-local aids; chooser entry itself is the explicit decision; no extra policy/approval prompts were found in the audited common path. | `X`, `Z`, prompt `Up`, prompt `F2`, `K`, `F9` | `src/ui/interactions.c`, `src/ui/input_line.c`, `src/ui/history_dialog.c`, `src/ui/f2_picker.c`, `src/ui/volume_menu.c`, `src/ui/application_menu.c` | help topics, chooser tests | Intentionally unchanged |

## Ranked offender families

| Rank | Family | User-cost / frequency / common-path rationale | Boundary and validation path | Status |
| --- | --- | --- | --- | --- |
| 1 | Tagged overwrite policy family | Tagged copy/move now opens the first concrete overwrite prompt directly, so the first overwrite decision carries real conflict context instead of a policy pre-prompt. | `src/ui/ctrl_file_ops.c`; focused overwrite regression in `tests/test_core.py` | Addressed |
| 2 | Filter tagged-scope family | High-frequency filter workflow previously paid an avoidable follow-up choice for tagged-only scope. | `src/ui/interactions.c`, `src/ui/ctrl_file_ops.c`; filtering regressions | Addressed |
| 3 | Compare target family | Compare previously used extra chooser bureaucracy on a frequent dir-like workflow. | `src/ui/compare_request.c`; compare prompt regressions and help topics | Addressed |
| 4 | Output export family | Output/export previously required a redundant format chooser and misplaced separator timing on a common command path. | `src/ui/print_controller.c`; print/help regressions | Addressed |
| 5 | Attribute date family | Date edits previously spent an avoidable extra chooser step before value entry. | `src/ui/attr_actions.c`; attribute/date regressions | Addressed |
| 6 | Tagged delete family | Batch delete historically risked a redundant `confirm each file` policy prompt after the batch approval. | `src/ui/file_tags.c`; tagged delete regressions | Addressed |
| 7 | Copy/move blanket confirmation family | Copy/move needed explicit exception documentation so prompt compression did not erase meaning or reintroduce routine post-target approvals. | `src/ui/interactions.c`, destination tests/spec | Addressed |

## Remediation-family boundaries

1. **Filter tagged-scope compression** — owner boundary `src/ui/interactions.c` / `src/ui/ctrl_file_ops.c`; validation path `tests/test_filtering.py`; addressed.
2. **Compare prompt compression** — owner boundary `src/ui/compare_request.c`; validation path `tests/test_compare_actions.py`; addressed.
3. **Attribute date prompt compression** — owner boundary `src/ui/attr_actions.c`; validation path attribute/date regressions; addressed.
4. **Copy/move explicit multi-input exception** — owner boundary `src/ui/interactions.c` plus spec/help surfaces; validation path `tests/test_destination_prompt.py`; addressed.
5. **Tagged delete confirmation cleanup** — owner boundary `src/ui/file_tags.c`; validation path `tests/test_vi_keys_mode.py`; addressed.
6. **Output/export prompt compression** — owner boundary `src/ui/print_controller.c`; validation path `tests/test_print_feature.py` and help-topic regressions; addressed.
7. **Tagged overwrite policy cleanup** — owner boundary `src/ui/ctrl_file_ops.c`; validation path `tests/test_core.py`; addressed.

## Remaining remediation batches

- No separate prompt-bureaucracy remediation batches remain open in this audit snapshot.
- Chooser aids, execute, archive-create, history, and volume/application menus are intentionally unchanged because the audit found direct prompt entry or same-layer aids rather than unnecessary approval/policy prompts.
