"""Behavioural command-strip contracts.

Command labels, packing, and footer rows are presentation details.  These tests
prove that each active surface exposes usable actions and transitions cleanly.
"""
from __future__ import annotations

import shlex
from pathlib import Path

from helpers_ui import screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())


def _root(tmp_path):
    root = tmp_path / "command_strip_visibility"
    root.mkdir()
    (root / "dir1").mkdir()
    (root / "file1.txt").write_text("seed\n", encoding="utf-8")
    return root


def _spawn(root, cols=120):
    return YtreeNovaTUI(YTNOVA_BIN, cwd=str(root), dimensions=(24, cols))


def _capture_filediff(root):
    log = root / "filediff_args.log"
    helper = root / ".capture_filediff.sh"
    helper.write_text(f"#!/bin/sh\nprintf '%s\\n' \"$@\" > {shlex.quote(str(log))}\n", encoding="utf-8")
    helper.chmod(0o755)
    (root / ".ytnova").write_text(f"[GLOBAL]\nFILEDIFF={helper}\n", encoding="utf-8")
    return log


def test_directory_and_file_surfaces_accept_their_actions_at_narrow_width(tmp_path):
    tui = _spawn(_root(tmp_path), cols=48)
    try:
        assert tui.wait_for_content("file1.txt", timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if any("FILE" in line and "file view" in line for line in lines)
            else False,
            timeout=1.5,
        ), screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ESC,
            lambda lines: lines
            if any("DIR" in line and "dir view" in line for line in lines)
            else False,
            timeout=1.5,
        ), screen_text(tui)
    finally:
        tui.quit()


def test_volume_and_applications_choosers_open_and_cancel_without_stale_modal(tmp_path):
    tui = _spawn(_root(tmp_path), cols=80)
    try:
        assert tui.wait_for_content("file1.txt", timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change("k", timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=1.5), screen_text(tui)
        assert tui.wait_for_content("file1.txt", timeout=1.0), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.F9, timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=1.5), screen_text(tui)
        assert tui.wait_for_content("file1.txt", timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_f2_destination_picker_selects_a_logged_volume(tmp_path):
    root = _root(tmp_path)
    destination = root / "destination"
    destination.mkdir()
    tui = _spawn(root)
    try:
        assert tui.wait_for_content("file1.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.send_and_wait_for_screen_change(Keys.COPY, timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.F2, timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.CTRL_U + str(destination) + Keys.ENTER, timeout=1.5), screen_text(tui)
        assert tui.wait_for_content("destination", timeout=1.5), screen_text(tui)
    finally:
        tui.quit()


def test_compare_history_returns_to_an_usable_target_prompt(tmp_path):
    root = _root(tmp_path)
    target = root / "target.txt"
    target.write_text("target\n", encoding="utf-8")
    log = _capture_filediff(root)
    tui = _spawn(root)
    try:
        tui.send_keystroke(Keys.ENTER)
        assert tui.send_and_wait_for_screen_change("J", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.CTRL_U + str(target) + Keys.ENTER)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_condition(lambda _lines: log.exists(), timeout=2.0), screen_text(tui)
        assert tui.send_and_wait_for_screen_change("J", timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.UP, timeout=1.5), screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=1.5), screen_text(tui)
    finally:
        tui.quit()
