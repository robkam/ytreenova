# Archive Virtual Filesystem Parity

## Mission
Complete **Archive Virtual Filesystem Parity**: archives must behave as filesystem-like volumes according to runtime-probed capabilities.

## Selected coherent family
Runtime archive capability model and truthful UI/help projection. This is the highest-value prerequisite because the acceptance target requires every archive mutation path, footer, stats line, and contextual help to agree on the operations actually available from the opened archive and installed libarchive support.

## In-scope inventory
- Source of truth: `docs/ROADMAP.md` Task 13 acceptance/capability/move/directory/help contracts.
- Runtime archive support and mutations: `src/fs/archive_read.c`, `src/fs/archive_write.c`, `include/ytnova_fs.h`, `include/ytnova_cmd.h`.
- Copy/move and archive/directory call paths: `src/cmd/copy.c`, `src/cmd/move.c`, `src/cmd/mkdir.c`, `src/cmd/delete.c`, `src/cmd/rename.c`, `src/cmd/rmdir.c`.
- UI projection/dispatch: `src/ui/display.c`, `src/ui/stats.c`, `src/ui/ctrl_file_ops.c`, `src/ui/ctrl_file.c`, `src/ui/ctrl_dir.c`.
- Generated-help source and projections: `etc/help/f1.en.md`, `etc/help/f1.de.md`, generated help assets.
- Tests: `tests/test_archive_write_parity.py`, `tests/test_fileops_integrity.py`, archive/UI/help contract tests and focused TUI regressions.
- Safety seams: `Archive_ValidateInternalPath()`, archive rewrite finalization, collision and subtree/self-target checks, cross-archive source-removal ordering.

## Initial observations
- Existing archive copy/move/rewrite and canonical-path guards are present from earlier history.
- `docs/ROADMAP.md` remains Not Started; no matching audit handoff exists.
- Archive F1 text and static footer definitions currently present mutation actions without an apparent runtime capability projection. This is incompatible with the capability contract and requires exact call-path inspection before implementation.

## Closure status
- Inventory: active; no item reconciled yet.
- Deferred families: recursive directory transfer and cross-archive failure ordering remain in this mission but will be deferred from the capability/UI family only if their runtime contracts are already demonstrably complete or require materially different mutation validation.

## Capability-model progress
- Addressed: `Statistic` now records archive capability flags and archive loading probes the opened archive with libarchive reader/writer format and filter support rather than a filename extension.
- Validation: `make -j"$(nproc)"` passed; `pytest -q tests/test_archive_write_parity.py` passed (9 tests).
- Known gap: the capability flags are not yet enforced by mutation dispatch or projected into footer, stats, and contextual help; this remains the active family.
- Environment note: `make clean` cannot complete because a pre-existing root-owned `build/locale/de/LC_MESSAGES/ytnova.mo` cannot be removed by the current user. Incremental `make` passed.

## Recursive directory transfer progress
- Addressed: archive tree extraction, recursive archive insertion, canonical member validation, and preflight collision detection were added to the archive layer; archive-source directory copy/move dispatch performs destination replacement before source deletion.
- Addressed: directory Pathcopy is routed through the directory controller and archive mutation guards now cover directory add/delete/rename entry paths.
- Validation: `make -j"$(nproc)"` passed; `source .venv/bin/activate && pytest -q tests/test_archive_write_parity.py` passed (`9 passed`).
- Still required: durable layout-resilient TUI coverage for archive directory copy/pathcopy/move and capability UI projection; do not mark the roadmap complete yet.

## Reconciled progress
- Addressed: runtime archive capabilities are probed from libarchive, enforced in archive mutators and file/directory dispatch, filtered from archive footer/F1 labels, and projected in stats.
- Addressed: archive directory Copy, Pathcopy, and Move recursively transfer to filesystem destinations; Move deletes the source only after destination transfer succeeds. Archive tree deletion canonicalizes virtual absolute paths before rewrite.
- Addressed: canonical internal paths, traversal rejection, self/subtree rejection, and destination collision preflight are implemented in archive transfer layers.
- Addressed: authored archive help and generated projections state runtime-dependent capability availability and expected writable formats.
- Validation: `make -j"$(nproc)"`; `pytest -q tests/test_archive_ui.py::test_archive_directory_copy_recursively_preserves_source tests/test_archive_ui.py::test_archive_directory_move_to_filesystem_removes_source tests/test_archive_write_parity.py tests/test_help_source_schema.py` (`21 passed`); `pytest -q tests/test_archive_ui.py::test_archive_directory_pathcopy_recursively_preserves_source` (`1 passed`).
- Remaining/unproven: direct archive-to-archive recursive directory transfer, read-only capability UI rejection test, and injected cross-archive source-side deletion failure preservation. These acceptance surfaces must be covered before roadmap closure.
- Cross-archive runtime probe: a linked `ExtractArchiveTree` → `Archive_AddTree` → `Archive_DeleteTree` sequence returned `0`; source became empty and destination retained `keep.txt` plus `copied_bundle/nested/item.txt`. This proves destination replacement precedes source deletion for the recursive archive primitives.
- Cross-archive failure probe: a destination canonical collision made `Archive_AddTree` fail; the operation returned `0` from the expected-failure harness and the source still contained `bundle/item.txt`. This proves destination failure does not alter the source.


## Completed verification family
Durable acceptance coverage for archive-to-archive directory transfers and failure-preserving moves.

## Remaining inventory
- `docs/ROADMAP.md`: addressed; completion restored after durable proof of every listed acceptance criterion.
- `tests/test_archive_write_parity.py`: addressed; logged archive destination Copy, PathCopy, and Move verify recursive members, source retention/removal, and collision preservation.
- `tests/test_archive_ui.py`: addressed; read-only archive Move is invoked and displays the unsupported directory-transfer rejection while the footer remains truthful.
- `tests/test_archive_write_parity.py`: addressed; a permission-injected source rewrite failure after destination write proves both archives retain the transferred directory.
- `src/ui/ctrl_dir.c`: addressed; archive directory dispatch rejects unavailable transfer capability before root-directory handling can mask the capability error.
- Existing archive transfer/capability runtime paths: intentionally unchanged; the new durable coverage did not expose further defects.

## Closure reconciliation
- Addressed: runtime capabilities, operation/dispatch guards, footer/help/stat projection, filesystem and archive destination directory transfers, canonical collision/traversal/subtree guards, and generated help.
- Addressed: archive-to-archive Copy, PathCopy, and Move now have recursive durable coverage; collision leaves both archives intact; read-only invocation produces a clear rejection; and injected source deletion failure retains duplicate source/destination data.
- Validation: `make -j"$(nproc)"` and `make qa-code-quality` passed; `source .venv/bin/activate && pytest -q tests/test_archive_write_parity.py::test_archive_directory_transfer_matrix_vfs_to_vfs tests/test_archive_write_parity.py::test_archive_directory_transfer_rejects_destination_collision tests/test_archive_write_parity.py::test_archive_directory_move_preserves_source_when_source_delete_fails tests/test_archive_ui.py::test_read_only_archive_hides_mutations_and_rejects_move` passed (`6 passed`); `pytest -q tests/test_archive_backend.py` passed (`3 passed`).
- Deferred: none.

## CI remediation
- Root cause: capability filtering can remove navigation commands, but
  `RenderFooterNavRow()` continued packing and inspecting the original command
  count, reading uninitialized command slots.
- Addressed: `RenderFooterNavRow()` now uses the filtered count returned by
  `ResolveFooterCommandList()`.
- Validation: `make -j"$(nproc)"`, `clang-tidy src/ui/display.c -p .`, and
  `source .venv/bin/activate && pytest -q tests/test_archive_ui.py::test_read_only_archive_hides_mutations_and_rejects_move` passed. The canonical local
  `make qa-clang` could not run because its mandatory clean step hit the known
  root-owned `build/locale/de/LC_MESSAGES/ytnova.mo` artifact; it removed the
  normal build outputs before failing, then the incremental build restored them.

## Static analyzer follow-up
- Root cause: cppcheck reported narrow const-correctness and variable-scope
  findings in the new archive transfer paths after the footer read was fixed.
- Addressed: archive mutator inputs and read-only rewrite contexts are const,
  archive traversal entries are read-only, and temporary path buffers now have
  their smallest valid scope.
- Validation: `make -j"$(nproc)"`; `source .venv/bin/activate && pytest -q
  tests/test_archive_write_parity.py
  tests/test_archive_ui.py::test_read_only_archive_hides_mutations_and_rejects_move`
  passed (`15 passed`). The detached focused cppcheck matrix for
  `src/ui/ctrl_dir.c` passed (`0`).

## Dual-panel footer isolation
- Selected defect family: active-panel archive footer/help context leaks from
  global `ViewContext.view_mode` when split panels hold filesystem and archive
  volumes.
- In-scope inventory: `docs/ROADMAP.md` status; footer context, command
  presentation, capability filtering, and integrated-help selection in
  `src/ui/display.c`; active panel volume state via
  `ctx->active->vol->vol_stats.log_mode`; focused PTY coverage in
  `tests/test_archive_ui.py`.
- Closure status: active. The regression must prove F8/Tab changes the footer
  from archive to filesystem context and back without changing either panel's
  volume.

## Dual-panel footer isolation closure
- Addressed: footer command sets, footer command presentations, capability
  filtering, and generated F1 context now derive archive state from the active
  panel volume rather than global view mode.
- Addressed: `tests/test_archive_ui.py` creates split filesystem/archive
  volumes and switches focus in both directions, asserting the corresponding
  filesystem and archive footer headers without layout coordinates.
- Validation: the regression first failed with an `ARCHIVE` footer after
  switching to the filesystem panel. `make -j"$(nproc)"`; `pytest -q
  tests/test_archive_ui.py` (`24 passed`); and `pytest -q
  tests/test_help_text_contract.py` (`7 passed`) now pass.
- Closure reconciliation: all Task 13 acceptance surfaces remain addressed;
  the dual-panel footer/help projection was the only reopened surface.

## Filesystem-to-archive directory transfer
- Reopened family: filesystem directory Copy, PathCopy, and Move into logged
  archive destinations; collision must preserve the filesystem source.
- Inventory: `HandleDirCopyMove`, archive destination routing in
  `archive_transfer.c`, `Archive_AddTree` collision semantics, filesystem
  source deletion ordering, and focused PTY regression coverage.
- Addressed: destination archive routing now runs before filesystem-only
  source/destination guards, resolves the logged target volume, and calls
  `Archive_AddTree` with the canonical archive-relative destination.
- Addressed: filesystem source removal is performed only after `Archive_AddTree`
  succeeds; collision failure leaves the filesystem source unchanged.
- Addressed: `tests/test_archive_ui.py` covers filesystem-directory Copy,
  PathCopy, and Move into a logged archive with nested contents, plus a Move
  collision preserving both the source and existing archive member.
- Validation: `make -j"$(nproc)"`; `source .venv/bin/activate && pytest -q
  tests/test_archive_ui.py::test_filesystem_directory_transfer_to_logged_archive
  tests/test_archive_ui.py::test_filesystem_directory_move_to_logged_archive_collision_preserves_source`
  (`4 passed`).
- Closure status: active; ROADMAP remains In Progress and no commit or push has
  been made pending the requested wider Task 13 reconciliation.

## Static analyzer reconciliation
- The reported `src/ui/display.c:1649 [knownConditionTrueFalse]` does not
  reproduce on base `b5dbe707`: that line is the unrelated filter fallback
  there. It was introduced by the archive-footer help refactor on the PR head.
- Addressed: simplified the logically redundant global-file help branch in
  `src/ui/display.c`; focused cppcheck on that file now exits zero with no
  `knownConditionTrueFalse` diagnostic.
- Validation: `cppcheck --enable=all --inconclusive --force --std=c99 -I
  include src/ui/display.c` (`0`); `make -j"$(nproc)"`; focused archive UI
  selection, transfer, and collision cases (`5 passed`). No push was made.
- CI remediation: CodeQL identified a TOCTOU race in filesystem-source removal.
  Directory removal now uses parent-directory descriptors with `openat`,
  `O_NOFOLLOW`, and `unlinkat`, so archive Move cannot follow a swapped
  filesystem path after archive insertion.
- Validation: `make -j"$(nproc)"`; focused filesystem-directory archive
  transfer and collision regressions (`4 passed`).

## Corrective audit inventory (active)

- **Manual reproducer / archive directory Delete:** `DeleteDirectory()` calls `Archive_DeleteEntry()` for logged archive directories; `Archive_DeleteTree()` exists but is bypassed. Add a nested-directory PTY red regression, then route archive directory deletion through the tree mutator. **Active.**
- **Archive mutation matrix:** archive file Delete (`Archive_DeleteEntry`), directory Delete (`Archive_DeleteTree`), file/directory Rename (`Archive_RenameEntry` and directory dispatch), and Copy/PathCopy/Move endpoint routing (`copy.c`, `move.c`, `ctrl_dir.c`, `archive_transfer.c`) require a call-path/guard review. Capability and canonical/collision/self/subtree guards are included; move ordering must retain source on destination or later source deletion failure. **Active audit.**
- **Capabilities / projections:** runtime probe, mutator rejection, dispatch rejection, footer command filtering, stats, and contextual F1 in `archive_read.c`, `archive_write.c`, `ctrl_file.c`, `ctrl_dir.c`, `display.c`, `stats.c`, and authored/generated help. **Active audit.**
- **Dual-panel F8/Tab / F1 matrix:** filesystem/archive file and directory active states; archive root/subdirectory; writable/read-only states; footer, dispatch, and help must follow `ctx->active->vol->vol_stats.log_mode` only. Existing focused UI regressions are included for confirmation. **Active audit.**
- **Tests and durable artifacts:** `tests/test_archive_ui.py`, `tests/test_archive_write_parity.py`, `tests/test_archive_backend.py`, help tests, and requested `scripts/bugrec.sh`. **Active audit.**
- **Tracker and handoff:** `docs/ROADMAP.md` remains In Progress until the inventory is reconciled; this handoff is the live record. **Active.**

## Corrective audit reconciliation

- **Manual archive directory Delete:** addressed. `DeleteDirectory()` now uses recursive `Archive_DeleteTree()` for archive directories, removes the matching in-memory subtree only after rewrite success, and decrements archive directory stats by the deleted subtree size. The nested-directory PTY regression was red before the route changed and is green after.
- **Archive mutation matrix:** addressed/unchanged after call-path audit. File Delete retains `Archive_DeleteEntry`; directory Delete now uses `Archive_DeleteTree`; Rename routes through canonical archive rename; Copy/PathCopy/Move endpoint transfer, preflight, capability guards, and destination-before-source removal remain in dedicated transfer layers and already have focused coverage. No additional root cause found.
- **Capabilities / projections:** addressed. Active-panel transition now synchronizes the legacy compatibility `ctx->view_mode` from the newly active volume, preventing dispatch from stale archive mode. Runtime help now receives all planned archive command labels and omits unavailable commands rather than merely retaining unused override metadata. Footer/stats/probe/mutator guards are intentionally unchanged after audit.
- **Dual-panel F8/Tab / F1:** addressed for the found boundary defects: filesystem footer and Makedir dispatch work after Tab from an archive panel; read-only archive F1 omits unavailable directory mutation descriptions. Existing active-volume footer transition coverage remains valid. No inactive-panel footer leak found.
- **Tests and durable artifacts:** addressed. `tests/test_archive_ui.py` now protects recursive Delete, filesystem dispatch after Tab, and read-only F1 filtering. Requested `scripts/bugrec.sh` is included unchanged.
- **Tracker and handoff:** at this earlier corrective-audit checkpoint,
  `docs/ROADMAP.md` remained In Progress. The final closure below supersedes
  that interim state after the added regressions and replacement CI run.

## Archive mutation refresh, progress, and root projection inventory (active)

- **Progress lifecycle:** archive directory Copy, PathCopy, and Move through
  `FilesystemDirectoryTransferToArchive`, `ArchiveDirectoryTransfer`,
  `Archive_AddTree`, `Archive_AddTreeRecursive`, `Archive_AddFile`, and
  `Archive_Rewrite`; callback cadence, an immediate pre-blocking render, footer
  restoration, and writable/read-only rejection are included. **Active audit.**
- **Destination refresh/rebind:** successful filesystem-to-archive recursive
  mutations must reload the destination `Volume` and rebind every panel that
  displays it, while the source panel refresh and Move selection semantics stay
  correct. Logged inactive and active archive destinations and Tab/F8 switching
  are included. **Active audit.**
- **Archive-root projection:** `ReadTreeFromArchive`, `InsertArchiveDirEntry`,
  and `MinimizeArchiveTree` must preserve the archive container as the tree root.
  Root files, one top-level folder, multiple top-level folders, nested
  sole-child chains, and explicit versus implicit directory entries are
  included; member paths remain canonical without changing filesystem tree
  minimization. **Active audit.**
- **Focused proof:** add red-first, layout-resilient regressions for immediate
  visibility of a recursively copied directory, visible pre-blocking progress,
  and a single top-level archive directory remaining below the archive root.
  Existing archive backend/write/UI suites and compatibility seams are included
  for focused confirmation. **Active.**
- **Tracker/handoff:** the roadmap status remains In Progress and this live
  inventory must reconcile every item as addressed, intentionally unchanged
  with reason, or separately deferred/blocked before amend/push. **Active.**

## Archive mutation refresh, progress, and root projection reconciliation

- **Progress lifecycle — addressed:** recursive directory Copy, PathCopy, and
  Move now render `ARCHIVE COPY`/`ARCHIVE MOVE` plus the spinner before entering
  blocking archive work, advance the existing progress surface from archive
  callbacks, and finish it on every post-start exit. Capability rejection stays
  before progress startup. File Delete, Makedir, Rename, and directory Delete are
  intentionally unchanged because their existing mutation paths already draw a
  pre-operation spinner and were covered by the earlier audit.
- **Destination/source refresh and dual-panel rebind — addressed:** successful
  filesystem-to-archive and archive-to-archive directory mutations reload the
  already logged archive volume, preserve its filter/sort and panel viewport
  anchors, rebuild file projections, and rebind every panel displaying it.
  Archive Move reloads a separately mutated source volume as well. Filesystem
  source removal remains after destination success and refresh; archive source
  deletion remains after destination success, so a later failure retains the
  destination duplicate. Focused tests cover Copy, PathCopy, and Move plus an
  inactive archive panel followed by Tab.
- **Archive-root projection — addressed:** archive tree minimization was removed
  from `ReadTreeFromArchive`, retaining the archive container root and each
  member-directory level for root files, one or multiple top-level directories,
  nested sole-child chains, and explicit or implicit directory entries.
  Filesystem tree minimization and canonical archive member paths are
  intentionally unchanged.
- **Adjacent capability/path contracts — intentionally unchanged:** runtime
  capability discovery, footer/F1 filtering, operation-layer rejection,
  traversal/collision/self/subtree checks, and file mutation routing have no new
  root cause in this family and retain the earlier focused coverage. The only
  compatibility correction is `RefreshTreeSafe` consulting the panel volume's
  archive mode rather than global view mode.
- **Focused proof — addressed:** red-first PTY failures covered the sole-folder
  root collapse, stale logged destination for all three directory transfer
  commands, and absent pre-blocking progress. Archive-to-archive destination
  visibility and inactive-panel refresh are also explicit. Build, focused
  cppcheck, 49 archive tests, module-boundary/AppState guards, and the refreshed
  test-contract baseline are green. The first amended CI run exposed three
  legacy tests that assumed the removed archive-root collapse; those tests now
  navigate deliberately to the member directory and their exact focused run is
  green (3 passed), without weakening their footer, split-panel, or viewer
  return contracts.
- **Tracker/docs — addressed:** `docs/ROADMAP.md` is restored to Completed after
  the amended required checks passed. No authored help contract changed, so
  help assets were deliberately not regenerated.

## Final closure

- The first corrective CI run identified three compatibility tests that still
  assumed a collapsed archive root. They now select the archive member
  directory explicitly; focused validation is 3 passed and the replacement CI
  run is green across every required check.
- The bounded archive mutation, capability, F8/Tab/F1, refresh, progress, and
  root-projection inventories are reconciled with no blocked or separately
  deferred defect family. Existing file-mutation spinner paths and authored help
  are the only deliberately unchanged adjacent surfaces, for the reasons above.
- `docs/ROADMAP.md` is restored to Completed now that the corrective audit,
  concrete regressions, local proof, and required PR checks are reconciled.
  This final tracker-only amend must itself retain green required checks before
  review or merge. `scripts/bugrec.sh` remains included in the amended commit.

## Archive rewrite cadence and batching inventory (reconciled locally)

- **New recording evidence:**
  `ytnova-20260907-005000-790902-zLDa8n.cast` renders `ARCHIVE COPY`
  immediately at 26.6 seconds but emits no spinner update for about 58 seconds;
  refresh, recursive Delete, and container-root projection behave correctly.
- **Collision preflight:** `ArchiveTreeDestinationAvailable` recursively opens
  and scans the complete archive once for every filesystem member. This is an
  O(source members × archive members) silent preflight and is the first long
  pause in the recording. **Address in the archive-write owner boundary.**
- **Directory insertion:** `Archive_AddTreeRecursive` calls `Archive_AddFile`
  per member, rewriting and replacing the complete archive each time. Replace
  this with one collision-aware rewrite followed by recursive append to the
  same writer, preserving atomic replacement and all-or-nothing source
  retention. **Address in the same family.**
- **Progress cadence:** `process_rewrite_loop` reports only every 50 headers and
  reports nothing while copying a large entry or writing a new entry. Emit
  progress from header and bounded data-copy units; rate-limit terminal redraws
  in the UI layer so callback density cannot dominate transfer time. **Address.**
- **Compatibility seams:** preserve single-file replace semantics, Makedir's
  source-less virtual directory insertion, recursive collision rejection,
  Rename data streaming, archive format/metadata preservation, cancellation,
  destination-before-source deletion, logged-volume reload, and dual-panel
  rebind. **Audit with focused backend/write/UI suites.**
- **Red proof:** the focused backend callback contract returned zero progress
  events while rewriting a one-entry archive before implementation. It must
  report progress during the rewrite and preserve the old member plus the
  recursively added tree. **Red observed.**
- **Tracker:** status returned to In Progress while this concrete recording
  regression is active. Do not restore Completed until focused validation and
  replacement required checks reconcile this inventory.

## Archive rewrite cadence and batching reconciliation (local)

- **Recording regression — addressed:** recursive archive insertion now performs
  one collision-aware rewrite and one recursive append instead of scanning and
  rewriting the archive once per source member. Existing members and the added
  tree are written to one temporary archive before the original is replaced.
- **Progress and cancellation — addressed:** rewrite headers, copied archive
  data, new-entry headers, and new-entry data emit progress callbacks. UI
  rendering is limited to once per second while cancellation polling remains at
  callback cadence. Directory Copy/PathCopy/Move retains its immediate progress
  render, and file/directory Rename now renders immediately and passes its live
  context through streamed-data callbacks.
- **Atomicity and compatibility — addressed:** a destination collision aborts
  the temporary rewrite and leaves the original archive byte-for-byte unchanged;
  the source-less directory path used by archive Makedir remains supported; and
  Rename streams retained entry data through the same progress callback. The
  `Archive_AddFile` source input is const-correct across its public declaration
  and callers.
- **Mutation ordering, refresh, projection, and capability contracts —
  intentionally unchanged:** transfer controllers still refresh/rebind logged
  volumes only after successful destination replacement; Move still removes its
  source only after destination success; archive-root projection remains
  container-rooted; and capability/footer/F1 guards retain the previously green
  implementation. The complete focused archive suite covers these adjacent
  surfaces and exposed no new failure.
- **Tracker — intentionally still In Progress:** local reconciliation is green,
  but Completed must not be restored until the amended SHA has replacement green
  required checks. `scripts/bugrec.sh` remains present in the commit.
- **Validation:** strict red proof for the callback regression was `0` progress
  callbacks before implementation. `make clean && make -j"$(nproc)"` passed
  after quarantining the pre-existing root-owned build tree at
  `/home/rob/ytreenova-build-root-owned-stale-20260907`; `pytest -q
  tests/test_archive_backend.py tests/test_archive_ui.py
  tests/test_archive_write_parity.py
  tests/test_archive_exit_ui.py::test_log_command_on_current_volume_reloads_tree_state`
  passed (`52 passed`); `make qa-fileops-integrity` passed (`42 passed`);
  `make qa-code-quality` passed; the module-boundary, AppState, and contract
  resilience test files passed (`4`, `756`, `9`, and `8` tests respectively).
  The first full `make qa-cppcheck` run found only the new archive source
  const-correctness issue; after correction, focused cppcheck over all changed C
  paths passed. Full local QA and a second full cppcheck sweep are deliberately
  left to required PR CI because focused reconciliation is complete.
