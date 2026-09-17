# Trust and Safety

## Quick answer
No file manager is zero-risk.

`ytnova` is built to reduce data-loss risk, but you should still use backups for important data and test in a non-critical directory first.

## What ytnova does to reduce risk
1. It asks before destructive actions.
   Delete asks for confirmation, and overwrite asks what to do next.
2. It blocks obvious self-target mistakes.
   Copy/move/rename paths are checked so a file is not written onto itself.
3. It tries to avoid software-caused corruption and loss during write operations.
   Move uses rename first, and cross-device move only deletes the source after copy succeeds.
   Copy removes partially written destination files when write/cancel errors happen.
   Archive edit operations rewrite to a temporary file and replace the original only on success.
4. It treats file paths as literal paths in command templates.
   Filenames with spaces or special characters are passed as one argument.
5. It uses unique temporary names and cleans them up.
   Archive preview/view temp paths are generated safely and removed after use.
6. It keeps checks automated.
   CI and local QA tests and safety guards must all be green before changes are merged and before release.

## Evidence in this repository
- Delete confirmation flow: [`src/ui/ctrl_file_ops.c`](../src/ui/ctrl_file_ops.c)
- Overwrite prompt flow: [`src/ui/key_engine.c`](../src/ui/key_engine.c)
- Self-target guards: [`src/cmd/copy.c`](../src/cmd/copy.c), [`src/cmd/move.c`](../src/cmd/move.c), [`src/cmd/rename.c`](../src/cmd/rename.c)
- Move/copy failure handling: [`src/cmd/move.c`](../src/cmd/move.c), [`src/cmd/copy.c`](../src/cmd/copy.c)
- Archive rewrite safety flow: [`src/fs/archive_write.c`](../src/fs/archive_write.c)
- Safe command/path quoting: [`src/util/path_utils.c`](../src/util/path_utils.c)
- Security path-handling tests: [`tests/test_security_shell_paths.py`](../tests/test_security_shell_paths.py)
- Archive write parity tests: [`tests/test_archive_write_parity.py`](../tests/test_archive_write_parity.py)
- Temp file/dir handling: [`src/util/path_utils.c`](../src/util/path_utils.c), [`src/ui/interactions.c`](../src/ui/interactions.c), [`src/ui/view_preview.c`](../src/ui/view_preview.c)
- Temp handling tests: [`tests/test_security_tempfiles.py`](../tests/test_security_tempfiles.py)
- QA gates: [`Makefile`](../Makefile), [`AUDIT.md`](AUDIT.md)

## Limits
- The current release is `1.0.0-beta`. See [CHANGELOG.md](CHANGELOG.md).
- There is no promise of zero risk.
- `ytnova` does not try to override user intent for destructive operations.
  If you choose to delete, overwrite, or move the wrong target, that is still your risk.
- During beta, most changes are minor UI/UX and bug-fix work.
  Core file-operation paths are treated as high-stability and kept under strict QA gates.

## Reporting
- Wrong behavior and non-security bugs: [GitHub Issues](https://github.com/robkam/ytreenova/issues)
- Security issues: private reporting via [SECURITY.md](../SECURITY.md)
