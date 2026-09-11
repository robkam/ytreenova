# Enhanced Keyboard Input with Legacy Fallback

## Mission and contract
Complete **Enhanced Keyboard Input with Legacy Fallback** by negotiating kitty keyboard protocol support without disturbing user input, preserving a truthful legacy path, and projecting the effective capability through dispatch, footers, configuration, help, and lifecycle boundaries.

When the active terminal path confirms kitty keyboard disambiguation, every existing Ctrl chord is decoded through the shared input boundary, `C-m` moves tagged files, and `C-i` also inverts tags; otherwise ytnova preserves legacy bytes and bindings, advertises only legacy keys, and shows at most one configurable explanatory modal per process.

## Coherent work family
The terminal-input capability boundary is one batch: parser/negotiation, `ViewContext` state, curses/external-command lifecycle, shared key decoding and polling, effective command/footer projection, profile configuration, PTY regressions, and authored/generated user documentation. These surfaces share the same runtime owner, protocol risk, and focused PTY validation path.

## Reconciled inventory
- **Addressed — source of truth:** `docs/ROADMAP.md` defines the completed enhanced-input contract, updates the configurable-keymap dependency, and removes the superseded investigation idea.
- **Addressed — architecture/specification:** `docs/ARCHITECTURE.md` assigns negotiation, parsing, queued input, decoding, and cleanup to `terminal_input`; `docs/SPECIFICATION.md` defines enhanced identities and legacy aliases.
- **Addressed — dedicated runtime boundary:** `src/ui/terminal_input.c` and `include/terminal_input.h` own bounded push/query/device-attributes negotiation, order-independent reply recognition, response classification, pending-byte preservation, CSI-u decoding, one-key polling preservation, suspend/resume, and shutdown.
- **Addressed — state:** `ViewContext` stores capability, session/kitty active states, preserved probe bytes, preserved polled key identity, and once-per-process notice state. Zero initialization remains deterministic and shared across panels.
- **Addressed — startup/config:** `LEGACY_INPUT_WARNING` is a validated boolean in `src/cmd/profile.c`, `include/config.h`, `etc/ytnova.conf`, and the generated default profile. Negotiation occurs after raw-mode terminal and profile initialization.
- **Addressed — shared input path:** `WGetch`, prompts, menus, dialogs, viewers, controllers, nonblocking key polling, and escape normalization consume the terminal-input boundary. All alphabetic Ctrl events normalize to established control bytes except distinct internal `C-i`/`C-m` identities.
- **Addressed — dispatch:** `GetKeyAction` maps enhanced `C-i` to Invert Tags and enhanced `C-m` to tagged move while retaining `i`/`I` and legacy `C-n`; all other Ctrl mappings retain their established contextual behavior.
- **Addressed — footer projection:** all directory/file/archive/preview footer registries expose Invert’s enhanced alias and the resolver selects `m`/`^m` plus `i`/`^i` for kitty capability or `m`/`^n` plus ordinary `i` for legacy capability. Localized labels remain authoritative.
- **Addressed — external lifecycle:** every `endwin()` runtime site in core shutdown, pipe, tagged file operations, print, and system interaction paths pops before handoff and re-probes after restoration. Direct shell launches that retain curses mode use the same suspend/resume boundary without duplicate nested negotiation.
- **Addressed — shutdown:** ordinary quit and handled `SIGINT` converge on `ShutdownCurses()` and pop enhanced input before curses teardown; no shutdown resume occurs.
- **Addressed — PTY harness and regressions:** `tests/tui_harness.py` emulates enhanced, legacy, and changing terminal capability and counts probes/pops. `tests/terminal_input_driver.c` and `tests/test_terminal_input_protocol.py` prove negotiation success, unsupported/malformed/truncated fallback, DA/status response order, exact interleaved and pre-queued input preservation, all alphabetic Ctrl events, unsupported-modifier preservation, polling preservation, enhanced/legacy dispatch and footers, configurable one-time notice, enhanced and legacy re-probing, capability loss after handoff, and ordinary/signal shutdown pop.
- **Addressed — adjacent tests:** footer inventory, help schema/text contracts, profile-template sync, and configuration robustness pass with generated projections.
- **Addressed — authored/generated docs:** README, FAQ, English/German F1 sources, and English man source explain support and effective bindings. `make help-assets` regenerated runtime F1, manpage, and USAGE projections; `make profile-template` regenerated the packaged default profile.
- **Intentionally unchanged — command catalogs:** `etc/ytnova.commands`, English/German presentation presets, and their generated headers retain `Ctrl+N` as the portable legacy catalog binding. Capability-aware runtime footer resolution advertises `^M` only after negotiation; changing catalog/keymap ownership belongs to the separately defined configurable-keymap contract.
- **Intentionally unchanged — backend:** ncurses remains the rendering/input backend; enhanced input is a negotiated layer over its shared reader.
- **Deferred/blocked:** none.

## Footer and documentation correction
- **Addressed — legacy footer projection:** tagged Move now uses key-prefix rendering (`M/^N move`) and legacy Invert renders as `Invert`, rather than exposing a redundant key prefix.
- **Addressed — enhanced footer projection:** tagged Move uses key-prefix rendering (`M/^M move`) after successful negotiation.
- **Addressed — archive footer projection:** archive file mode advertises its supported numeric range as `1..0 file view`.
- **Addressed — kitty enablement guidance:** README, FAQ, English/German runtime F1, and the authored man source identify `keyboard_protocol kitty` in `~/.config/kitty/kitty.conf`, restart requirement, and pass-through requirement for multiplexers/remote sessions. Generated runtime help is synchronized.
- **Addressed — regressions:** PTY assertions cover enhanced and legacy Move/Invert text; archive inventory covers `1..0 file view`; source-schema coverage requires actionable kitty configuration in each canonical English surface.

## Validation evidence
- Red compatibility proof: `pytest -q tests/test_terminal_input_protocol.py` initially produced five setup errors because the terminal-input module/header did not exist.
- Red lifecycle proof: `pytest -q tests/test_terminal_input_protocol.py::test_legacy_session_reprobes_after_external_handoff` failed because legacy sessions were not marked suspendable and were not re-probed.
- Red capability-change proof: `pytest -q tests/test_terminal_input_protocol.py::test_external_handoff_reports_a_new_legacy_fallback_once` failed because fallback notice handling existed only in startup.
- Red terminal-stack proof: `pytest -q tests/test_terminal_input_protocol.py::test_queued_user_input_preempts_negotiation_without_delay_or_loss` failed because the queued-input fast path emitted an unmatched kitty pop despite never pushing protocol state.
- `source .venv/bin/activate && make clean && make -j"$(nproc)"` — passed; existing unrelated warnings remain.
- `source .venv/bin/activate && pytest -q tests/test_terminal_input_protocol.py` — 17 passed.
- `source .venv/bin/activate && pytest -q tests/test_footer_command_inventory.py` — 5 passed.
- `source .venv/bin/activate && pytest -q tests/test_help_source_schema.py` — 10 passed.
- `source .venv/bin/activate && pytest -q tests/test_help_text_contract.py` — 7 passed.
- `source .venv/bin/activate && pytest -q tests/test_profile_template_sync.py` — 3 passed.
- `source .venv/bin/activate && pytest -q tests/test_config_history_robustness.py` — 4 passed.
- `source .venv/bin/activate && make qa-help-assets` — passed.
- `source .venv/bin/activate && make qa-profile-template` — passed.
- First PR full-QA classification: the clean-code guard rejected unnamed protocol literals and a session-scoped build fixture; the PTY harness's global zero send delay removed established user-input pacing and cascaded into split-panel, file-mutation, full-pytest, and coverage failures. These were narrow in-scope implementation/test defects rather than unrelated regressions.
- Root-cause remediation: protocol constants are named, the compiled driver fixture is function-scoped, normal pexpect pacing is preserved for user keystrokes, and only synthetic terminal negotiation replies temporarily bypass send delay. Semantic footer normalization now recognizes capability-decorated key tokens rather than brittle raw presentation substrings.
- Follow-up CI classification: composite `CC` values now tokenize correctly in the protocol driver; the raw startup-exit test disables the unrelated configurable fallback modal; and the print workflow enters file mode through the established semantic footer predicate instead of assuming two distinct repaints. The generic controller now exercises legacy fallback deterministically and dismisses its documented notice, preventing a capability response delayed beyond the bounded runtime probe from leaking its trailing `c` as a Copy command. Full pytest and coverage reproduced the same controller race.
- `source .venv/bin/activate && make clean && make -j"$(nproc)"` — passed after remediation; existing unrelated warnings remain.
- `source .venv/bin/activate && pytest -q tests/test_terminal_input_protocol.py` — 17 passed after remediation.
- `source .venv/bin/activate && make qa-clean-code` — passed after remediation.
- `source .venv/bin/activate && make qa-split-panel-gates` — 6 passed after remediation.
- `source .venv/bin/activate && pytest -q tests/test_runtime_exit_paths.py::test_startup_log_missing_path_exits_without_segv tests/test_terminal_input_protocol.py` — 18 passed after remediation.
- `source .venv/bin/activate && pytest -q tests/test_print_feature.py::test_stale_output_commands_conf_does_not_abort_startup` — passed after semantic wait remediation.
- `source .venv/bin/activate && pytest -q tests/test_destination_prompt.py::test_file_copy_missing_destination_yes_creates_directory_and_copies` — passed through the deterministic legacy controller path.
- `source .venv/bin/activate && make qa-test-contract-resilience` — passed after regenerating the line-sensitive exception registry.
- `source .venv/bin/activate && make clean && make -j"$(nproc)" && pytest -q tests/test_terminal_input_protocol.py` — passed after distinguishing skipped probes from attempted negotiation; 17 protocol tests passed.
- `source .venv/bin/activate && make qa-clean-code qa-test-contract-resilience` — passed after reconciling the line-sensitive exception registry.
- `source .venv/bin/activate && pytest -q tests/test_terminal_input_protocol.py tests/test_footer_command_inventory.py tests/test_help_source_schema.py tests/test_help_text_contract.py` — 40 passed after footer and documentation correction.
- `source .venv/bin/activate && make qa-help-assets && make qa-test-contract-resilience && make qa-clean-code` — passed after generated help and contract-registry reconciliation.
- Local `make qa-all` intentionally unrun: repository policy assigns the full gate to PR CI; focused build, PTY, footer, config, and generated-help checks cover the changed surfaces locally.

## Final sweep
The roadmap entry, architecture/specification, every `endwin()` and direct runtime-launch seam, every raw curses input call, footer registries/resolver, profile surfaces, authored/generated docs, command presentation catalogs, PTY tests, and live inventory have been reconciled. No unmigrated, unfixed, deferred, or unaccounted in-scope family remains.

## Help-source contract correction
- **Addressed — source-boundary documentation:** `docs/ARCHITECTURE.md`, `docs/SPECIFICATION.md`, and `docs/TRANSLATORS.md` now describe the actual independent authored-source contract: F1 requires contextual content while `man.en.md` uses direct reference subsections. This removes the stale claim that F1 is generated from the man source.
- **Addressed — clean man-source schema:** `scripts/generate_help_assets.py` now recognizes `####` reference subsections directly after contextual/link content for man sources. `man.en.md` and `man.de.md` no longer contain the parser-only `### Long form` delimiter. F1 retains its independently optional long-form section.
- **Addressed — regressions and projections:** generator/schema regressions prove direct man reference subsection parsing; generated manpage and USAGE projections remain clean. No compatible legacy marker is retained for generated man sources, preventing its reintroduction.
- **Addressed — generated-manpage coverage:** the navigation reference is now projected in the generated key-binding section, so the authored Kitty protocol explanation, configuration line, legacy fallback bindings, and terminal-path condition appear in `etc/ytnova.1.md` and `docs/USAGE.md`. The archive-only `0` command description no longer directs users to `F6`; the global `F6` entry documents stats independently.
- **Addressed — independent help-source schemas:** all `man.*.md` sources now contain only metadata and direct `####` reference sections; they contain no F1 headings, explainer links, or discarded long-form delimiters. All `f1.*.md` sources now retain their complete runtime text inside `### Contextual F1`; no F1 long-form section is silently dropped. The generator separately parses the two schemas, renders man topics only from reference sections, and rejects the obsolete F1 delimiter.
- **Addressed — audience separation and protocol guidance:** man sources no longer instruct users how to navigate the F1 popup, while runtime F1 carries no man-source material. The generated manpage and USAGE project one dedicated Enhanced Keyboard Input section in plain language; F1 English and German include the same optional Kitty protocol heading, binding outcomes, configuration line, legacy fallback, and pass-through condition.
- **Addressed — roadmap follow-on:** `docs/ROADMAP.md` now records the separate optional Kitty protocol investigation, including terminal-native preview/UI possibilities and a narrow replaceable capability boundary that preserves portable fallback and future Rust/notcurses/backend choices.

## Final code-auditor gate
- **Initial verdict: FAIL (high).** The queued-input compatibility path skipped the kitty push/query but sent a stack pop, risking corruption of inherited terminal protocol state.
- **Resolution:** negotiation now reports whether a push was attempted, fallback cleanup pops only attempted negotiation, and the PTY regression proves the queued-input path emits neither probe nor pop while preserving the queued key.
- **Final verdict: PASS.** Parser bounds and ordering, capability-aware dispatch/footer projection, suspension/resumption and shutdown coverage, memory and terminal-state safety, documentation projections, and the reconciled inventory contain no remaining credible in-scope defect.
- **Residual risk:** kitty-protocol behavior is terminal-dependent; bounded enhanced, legacy, malformed, truncated, queued-input, lifecycle, and capability-change PTY cases cover the focused risk, with full-QA CI required on the amended head.
