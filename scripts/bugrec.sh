#!/usr/bin/env bash
set -euo pipefail

record_root="${YTNOVA_RECORDINGS_DIR:-$HOME/recordings}"
inbox="$record_root/inbox"
mkdir -p "$inbox"

stamp="$(date +%Y%m%d-%H%M%S)"
pid="$$"

# mktemp ensures every run gets a unique, non-overwriting base name.
cast="$(mktemp "$inbox/ytnova-${stamp}-${pid}-XXXXXX.cast")"
base="${cast%.cast}"
log="${base}.debug.log"
meta="${base}.txt"

if (($# == 0)); then
  targets=("$HOME")
else
  targets=("$@")
fi

: >"$log"

cmd_args=(ytnova "${targets[@]}")
cmd="$(printf '%q ' "${cmd_args[@]}")"
cmd="${cmd% } 2> $(printf '%q' "$log")"

{
  echo "created: $(date -Is)"
  echo "cwd: $(pwd)"
  echo "tty: $(tty 2>/dev/null || true)"
  echo "size: $(stty size 2>/dev/null || true)"
  echo "cast: $cast"
  echo "log:  $log"
  echo "meta: $meta"
  echo "targets: ${targets[*]}"
  echo "cmd:  $cmd"
} >"$meta"

echo "Recording to:"
echo "  $cast"
echo "  $log"
echo "  $meta"
echo

asciinema rec --stdin --overwrite -c "$cmd" "$cast"
status=$?

echo
echo "Saved:"
echo "  $cast"
echo "  $log"
echo "  $meta"
exit "$status"
