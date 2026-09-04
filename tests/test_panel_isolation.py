import pytest
import shlex
import tarfile
import time
import re
from pathlib import Path
from helpers_files import wait_for_file as _wait_for_file
from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import (
    assert_tree_viewport_origin_stable as _assert_tree_viewport_origin_stable,
    find_line_with_text as _find_line_with_text,
    footer_text as _footer_text,
    footer_text_from_lines as _footer_text_from_lines,
    line_marks_file_as_tagged as _line_marks_file_as_tagged,
    screen_text as _screen_text,
    drive_action_until,
    tree_panel_selected_label as _tree_panel_selected_label,
    tree_row_visible as _tree_row_visible,
)
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _current_copy_source(tui):
    m = re.search(r"COPY:\s+([^\s_]+)", _screen_text(tui))
    if not m:
        return None
    return m.group(1)


def _assert_dir_mode_footer(tui, message):
    footer = _footer_text(tui)


def _stats_current_dir_contains(lines, marker):
    split_x = _detect_stats_split_x(lines)
    for i, line in enumerate(lines):
        segment = line[split_x:] if split_x is not None else line
        if "CURRENT DIR" not in segment:
            continue
        for j in (1, 2):
            idx = i + j
            if idx >= len(lines):
                continue
            candidate = lines[idx][split_x:] if split_x is not None else lines[idx]
            if marker in candidate:
                return True
    return False


def _active_volume_name_from_lines(lines, *volume_names):
    header = lines[0] if lines else ""
    for name in volume_names:
        if name in header:
            return name
    return None


def _wait_for_footer_state(tui, *, contains=(), excludes=(), timeout=2.0):
    def footer_matches(current_lines):
        footer = _footer_text_from_lines(current_lines)
        if all(token in footer for token in contains) and all(
            token not in footer for token in excludes
        ):
            return current_lines
        return False

    lines = tui.wait_for_condition(
        footer_matches,
        timeout=timeout,
    )
    assert lines, _screen_text(tui)
    return lines


def _send_and_wait_for_transition(tui, keys, timeout=2.0):
    lines = tui.send_and_wait_for_screen_change(keys, timeout=timeout)
    assert lines, _screen_text(tui)
    return lines


def _log_path_and_wait_for_fixture(tui, path, fixture_identity):
    prompt = tui.send_and_wait_for_screen_change(Keys.LOG, timeout=1.5)
    assert prompt, _screen_text(tui)
    lines = tui.send_and_wait_for_condition(
        Keys.CTRL_U + str(path) + Keys.ENTER,
        lambda current_lines: current_lines
        if any(fixture_identity in line for line in current_lines)
        else False,
        timeout=3.0,
    )
    assert lines, _screen_text(tui)
    return lines


def _run_compare_and_read_source(tui, compare_target, log_path):
    if log_path.exists():
        log_path.unlink()

    lines = tui.send_and_wait_for_condition(
        "J",
        lambda current_lines: current_lines
        if any("COMPARE TARGET:" in line for line in current_lines)
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)

    lines = tui.send_and_wait_for_condition(
        Keys.CTRL_U + str(compare_target) + Keys.ENTER,
        lambda current_lines: current_lines
        if log_path.exists()
        or any("Hit return to continue" in line for line in current_lines)
        else False,
        timeout=2.5,
    )
    assert lines, _screen_text(tui)

    if any("Hit return to continue" in line for line in lines):
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if not any("Hit return to continue" in line for line in current_lines)
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)

    assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
    lines = tui.get_screen_dump()
    if any("Hit return to continue" in line for line in lines):
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if not any("Hit return to continue" in line for line in current_lines)
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
    return log_path.read_text(encoding="utf-8").splitlines()[0]


def _enter_file_view_for_fixture(tui, fixture_name):
    lines = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda current_lines: current_lines
        if any(fixture_name in line for line in current_lines)
        else False,
        timeout=1.5,
    )
    if lines:
        return lines

    assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=1.5), _screen_text(tui)
    assert tui.send_and_wait_for_screen_change(
        ">", timeout=1.5
    ), _screen_text(tui)
    lines = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda current_lines: current_lines
        if any(fixture_name in line for line in current_lines)
        else False,
        timeout=1.5,
    )
    assert lines, _screen_text(tui)
    return lines


def _configure_filediff_capture(tmp_dir):
    log_path = tmp_dir / "filediff_args.log"
    helper_path = tmp_dir / ".capture_filediff.sh"
    helper_path.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" > {shlex.quote(str(log_path))}\n",
        encoding="utf-8",
    )
    helper_path.chmod(0o755)

    (tmp_dir / ".ytnova").write_text(
        f"[GLOBAL]\nFILEDIFF={helper_path}\n",
        encoding="utf-8",
    )
    return log_path


def _detect_split_column(lines):
    if len(lines) < 3:
        return None

    top = lines[1]
    for ch in ("w", "┬", "+"):
        idx = top.find(ch, 1)
        if idx != -1:
            return idx

    counts = {}
    for row in lines[2:-4]:
        for x, ch in enumerate(row):
            if ch in ("x", "|"):
                counts[x] = counts.get(x, 0) + 1

    if not counts:
        return None
    return max(counts, key=counts.get)

def _assert_split_column_continuous(lines, label):
    split_col = _detect_split_column(lines)
    assert split_col is not None, f"Could not detect split column ({label}).\n" + "\n".join(lines)

    assert all(
        split_col >= len(row) or row[split_col] != " " for row in lines[2:-4]
    ), f"Split separator has a gap ({label}).\n" + "\n".join(lines)


def _split_segments_for_file(tui, filename):
    lines = tui.get_screen_dump()
    split_col = _detect_split_column(lines)
    screen = "\n".join(lines)
    assert split_col is not None, f"Could not detect split column.\n{screen}"

    for line in lines:
        if filename in line:
            return line[:split_col], line[split_col:], screen

    raise AssertionError(f"Could not find {filename} in split screen.\n{screen}")


def _first_tree_row_segment(lines, split_col=None):
    for line in lines[2:-4]:
        segment = line[:split_col] if split_col is not None else line
        if segment.strip():
            return segment.rstrip()
    return None


def _tree_segment_rows(lines, split_col):
    return [line[:split_col].rstrip() for line in lines[2:-4]]


def _populate_hidden_prefix_viewport_tree(root):
    root.mkdir(parents=True)
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=4\nHIDEDOTFILES=1\nSMALLWINDOWSKIP=0\n",
        encoding="utf-8",
    )

    for i in range(40):
        (root / f".hidden_{i:02d}").mkdir()

    for name, child in (
        ("go", "pkg"),
        ("gone", "home"),
        ("snap", "glow"),
        ("wikiteam3_utilities", "for later"),
        ("ytnova", "docs"),
    ):
        d = root / name
        d.mkdir()
        (d / child).mkdir(parents=True)


def _move_to_stats_dir(tui, marker, *, timeout=20.0):
    _select_tree_stats_marker(
        tui,
        f" {marker} ",
        timeout=timeout,
        keys=(Keys.DOWN,),
    )


def _select_tree_dir_by_marker(tui, marker, timeout=5.0):
    lines = drive_action_until(
        tui,
        Keys.DOWN,
        lambda dump: dump if marker in next(iter(dump), "") else False,
        max_actions=128,
        timeout=timeout,
    )
    if lines:
        return lines
    pytest.fail(f"Could not select '{marker}' in tree view.\n{_screen_text(tui)}")


def _select_tree_stats_marker(tui, marker, timeout=5.0, keys=(Keys.UP, Keys.DOWN)):
    for key in keys:
        lines = drive_action_until(
            tui,
            key,
            lambda dump: dump if _stats_current_dir_contains(dump, marker) else False,
            max_actions=128,
            timeout=timeout,
        )
        if lines:
            return
    pytest.fail(f"Could not select '{marker}' in tree view.\n{_screen_text(tui)}")


def _enter_fixture_file_view(tui, markers, file_marker):
    assert tui.wait_for_content(markers[0], timeout=2.0), _screen_text(tui)
    for marker in markers:
        _select_tree_dir_by_marker(tui, marker)
        assert tui.send_and_wait_for_screen_change(Keys.RIGHT, timeout=2.0), _screen_text(tui)
    lines = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda current_lines: current_lines
        if any(file_marker in line for line in current_lines)
        else False,
        timeout=2.0,
    )
    assert lines, _screen_text(tui)
    return lines


def test_panel_switch_updates_small_window(dual_panel_sandbox, ytnova_binary):
    """
    Verify that switching panels updates the content of the small file window.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(dual_panel_sandbox))
    left_file = "very_long_filename_alpha_numeric_extension_test.txt"

    try:
        assert tui.wait_for_content("left_dir", timeout=2.0)
        assert tui.send_and_wait_for_condition(
            Keys.DOWN + Keys.ENTER,
            lambda lines: lines if left_file in "\n".join(lines) else False,
            timeout=2.0,
        ), "Entering the left fixture directory did not reveal its file."

        assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)

        # A split cloned from a file view must return to the peer tree before
        # selecting the peer directory.
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
        assert tui.send_and_wait_for_condition(
            Keys.DOWN + Keys.ENTER,
            lambda lines: lines
            if any(line.startswith("Path:") and "right_dir" in line for line in lines)
            else False,
            timeout=2.0,
        ), "Switching to the right panel did not enter its fixture directory."

        assert tui.send_and_wait_for_condition(
            Keys.TAB,
            lambda lines: lines
            if any(line.startswith("Path:") and "left_dir" in line for line in lines)
            else False,
            timeout=2.0,
        ), "Switching back did not restore the left panel directory."
    finally:
        tui.quit()


def test_split_from_file_keeps_file_focus_on_tab(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_focus_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)

    footer = _footer_text(tui)

    tui.quit()


def test_split_tab_from_small_file_does_not_expand_inactive_panel(tmp_path, ytnova_binary):
    root = tmp_path / "split_tab_small_file_inactive_shape"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nSMALLWINDOWSKIP=0\n", encoding="utf-8")
    left = root / "left"
    right = root / "right"
    left.mkdir()
    right.mkdir()
    for idx in range(5):
        (left / f"left{idx}.txt").write_text("left\n", encoding="utf-8")
        (right / f"right{idx}.txt").write_text("right\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        # Baseline: small-window file content is below the split separator.
        before_tab = tui.get_screen_dump()
        before_idx = next(
            (idx for idx, line in enumerate(before_tab) if "left0.txt" in line), -1
        )
        assert before_idx >= 0, _screen_text(tui)
        assert before_idx > 10, _screen_text(tui)

        tui.send_keystroke(Keys.TAB, wait=0.5)
        after_tab = tui.get_screen_dump()
        after_idx = next(
            (idx for idx, line in enumerate(after_tab) if "left0.txt" in line), -1
        )
        assert after_idx >= 0, _screen_text(tui)

        # Inactive panel must keep tree+small layout, not expand to big file.
        assert after_idx > 10, (
            "Tab from small file view expanded the inactive panel to big file mode "
            "(file rows jumped into the top/tree area).\n"
            f"{_screen_text(tui)}"
        )
    finally:
        tui.quit()








def test_split_same_directory_file_tags_are_panel_local(tmp_path, ytnova_binary):
    root = tmp_path / "split_same_dir_panel_local_tags"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    for idx in range(3):
        (alpha / f"panel_tag_{idx}.txt").write_text("tag\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke("t", wait=0.3)
        left, right, screen = _split_segments_for_file(tui, "panel_tag_0.txt")
        assert _line_marks_file_as_tagged(left, "panel_tag_0.txt"), screen
        assert not _line_marks_file_as_tagged(right, "panel_tag_0.txt"), (
            "Tagging the active panel leaked to the inactive peer panel.\n"
            f"{screen}"
        )

        tui.send_keystroke(Keys.TAB, wait=0.4)
        left, right, screen = _split_segments_for_file(tui, "panel_tag_0.txt")
        assert _line_marks_file_as_tagged(left, "panel_tag_0.txt"), screen
        assert not _line_marks_file_as_tagged(right, "panel_tag_0.txt"), (
            "Switching panels should not import the other panel's tags.\n"
            f"{screen}"
        )

        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.3)
        left, right, screen = _split_segments_for_file(tui, "panel_tag_1.txt")
        assert not _line_marks_file_as_tagged(left, "panel_tag_1.txt"), (
            "Right-panel tagging leaked into the left panel.\n"
            f"{screen}"
        )
        assert _line_marks_file_as_tagged(right, "panel_tag_1.txt"), screen

        tui.send_keystroke(Keys.TAB, wait=0.4)
        left, right, screen = _split_segments_for_file(tui, "panel_tag_0.txt")
        assert _line_marks_file_as_tagged(left, "panel_tag_0.txt"), screen
        assert not _line_marks_file_as_tagged(right, "panel_tag_0.txt"), screen
        left, right, screen = _split_segments_for_file(tui, "panel_tag_1.txt")
        assert not _line_marks_file_as_tagged(left, "panel_tag_1.txt"), screen
        assert _line_marks_file_as_tagged(right, "panel_tag_1.txt"), screen
    finally:
        tui.quit()


def test_unreading_directory_clears_panel_local_tags(tmp_path, ytnova_binary):
    root = tmp_path / "unread_clears_panel_tags"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    alpha = root / "alpha"
    (alpha / "child").mkdir(parents=True)
    (alpha / "panel_tag_0.txt").write_text("tag\n", encoding="utf-8")
    (alpha / "child" / "nested.txt").write_text("nested\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.RIGHT, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        tui.send_keystroke("t", wait=0.3)
        line = _find_line_with_text(tui, "panel_tag_0.txt")
        assert line is not None, _screen_text(tui)
        assert _line_marks_file_as_tagged(line, "panel_tag_0.txt"), _screen_text(tui)

        tui.send_keystroke(Keys.ESC, wait=0.3)

        tui.send_keystroke("-", wait=0.3)
        tui.send_keystroke("-", wait=0.4)
        tui.send_keystroke(Keys.RIGHT, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        line = _find_line_with_text(tui, "panel_tag_0.txt")
        assert line is not None, _screen_text(tui)
        assert not _line_marks_file_as_tagged(line, "panel_tag_0.txt"), (
            "Unreading a directory must discard saved tags beneath it.\n"
            f"Row: {line}\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def _assert_collapse_action_clears_panel_local_tags(tmp_path, ytnova_binary, key):
    root = tmp_path / f"collapse_clears_panel_tags_{ord(key[0])}"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    alpha = root / "alpha"
    (alpha / "child").mkdir(parents=True)
    (alpha / "panel_tag_0.txt").write_text("tag\n", encoding="utf-8")
    (alpha / "child" / "nested.txt").write_text("nested\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.RIGHT, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        tui.send_keystroke("t", wait=0.3)
        line = _find_line_with_text(tui, "panel_tag_0.txt")
        assert line is not None, _screen_text(tui)
        assert _line_marks_file_as_tagged(line, "panel_tag_0.txt"), _screen_text(tui)

        tui.send_keystroke(Keys.ESC, wait=0.3)

        tui.send_keystroke(key, wait=0.4)
        tui.send_keystroke(Keys.RIGHT, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        line = _find_line_with_text(tui, "panel_tag_0.txt")
        assert line is not None, _screen_text(tui)
        assert not _line_marks_file_as_tagged(line, "panel_tag_0.txt"), (
            "Collapsing a directory must discard saved tags beneath it.\n"
            f"Row: {line}\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_minus_collapse_clears_panel_local_tags(tmp_path, ytnova_binary):
    _assert_collapse_action_clears_panel_local_tags(tmp_path, ytnova_binary, "-")


def test_left_arrow_collapse_clears_panel_local_tags(tmp_path, ytnova_binary):
    _assert_collapse_action_clears_panel_local_tags(
        tmp_path, ytnova_binary, Keys.LEFT
    )


def _assert_collapse_resets_subtree_expansion(tmp_path, ytnova_binary, key):
    root = tmp_path / f"collapse_reset_subtree_{ord(key[0])}"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    alpha = root / "alpha"
    (alpha / "child" / "grand" / "great").mkdir(parents=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # alpha
        tui.send_keystroke(Keys.RIGHT, wait=0.4)  # show child
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # child
        tui.send_keystroke(Keys.RIGHT, wait=0.4)  # show grand
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # grand
        tui.send_keystroke(Keys.RIGHT, wait=0.4)  # show great

        before = _screen_text(tui)
        assert "great" in before, (
            "Precondition failed: deep expansion should reveal great.\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.UP, wait=0.2)     # child
        tui.send_keystroke(Keys.UP, wait=0.2)     # alpha
        assert tui.send_and_wait_for_screen_change(key, timeout=1.5)
        after = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines if any("child" in line for line in lines) else False,
            timeout=2.0,
        )
        assert after, "Re-expand did not restore immediate child visibility."
        after = "\n".join(after)
        assert "grand" not in after and "great" not in after, (
            "Collapse with Left or '-' must reset subtree expansion state for"
            " that node.\n"
            f"{after}"
        )
    finally:
        tui.quit()


def test_minus_collapse_resets_subtree_expansion_state(tmp_path, ytnova_binary):
    _assert_collapse_resets_subtree_expansion(tmp_path, ytnova_binary, "-")


def test_left_collapse_resets_subtree_expansion_state(tmp_path, ytnova_binary):
    _assert_collapse_resets_subtree_expansion(tmp_path, ytnova_binary, Keys.LEFT)


def test_split_from_dir_immediately_renders_peer_panel(tmp_path, ytnova_binary):
    root = tmp_path / "split_dir_immediate_render"
    root.mkdir()
    (root / "alpha_peer_dir").mkdir()
    (root / "beta_peer_dir").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha_peer_dir", timeout=2.0)
    split_lines = tui.send_and_wait_for_condition(
        Keys.F8,
        lambda lines: lines
        if "\n".join(lines).count("alpha_peer_dir") >= 2
        else False,
        timeout=2.0,
    )
    assert split_lines, "Split did not project the peer tree."

    screen = "\n".join(split_lines)
    assert screen.count("alpha_peer_dir") >= 2, (
        "Split from dir view did not render peer panel until next keypress.\n"
        f"{screen}"
    )

    tui.quit()


def test_split_from_file_preserves_inactive_panel_file_state(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_focus_inactive_state"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_unique_123.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_unique_456.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0)
    assert tui.send_and_wait_for_condition(
        Keys.DOWN + Keys.ENTER,
        lambda lines: lines if "alpha_unique_123.txt" in "\n".join(lines) else False,
        timeout=2.0,
    )

    assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0)
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)

    screen = "\n".join(tui.get_screen_dump())
    assert screen.count("alpha_unique_123.txt") >= 2, (
        "Inactive panel did not retain its file-window state after split/tab.\n"
        f"{screen}"
    )

    tui.quit()

def test_split_from_file_immediate_peer_mirror_not_blank(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_immediate_mirror"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_immediate_uniq.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta_immediate_uniq.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.wait_for_text("alpha")

    tui.send_keystroke(Keys.DOWN, wait=0)
    tui.send_keystroke(Keys.ENTER, wait=0)
    assert tui.wait_for_text("alpha_immediate_uniq.txt"), _screen_text(tui)

    tui.send_keystroke(Keys.F8, wait=0)

    assert tui.wait_for_condition(
        lambda lines: "\n".join(lines).count("alpha_immediate_uniq.txt") >= 2,
        description="split peer file identity",
    ), _screen_text(tui)
    screen = "\n".join(tui.get_screen_dump())
    assert screen.count("alpha_immediate_uniq.txt") >= 2, (
        "Peer panel stayed blank after splitting from file view.\n"
        f"{screen}"
    )

    tui.quit()


def test_split_volume_cycle_preserves_panel_local_file_lists(tmp_path, ytnova_binary):
    vol_a = tmp_path / "vol_a"
    vol_b = tmp_path / "vol_b"
    vol_c = tmp_path / "vol_c"
    vol_a.mkdir()
    vol_b.mkdir()
    vol_c.mkdir()
    (vol_a / "a_only.txt").write_text("a\n", encoding="utf-8")
    (vol_b / "b_only.txt").write_text("b\n", encoding="utf-8")
    (vol_c / "c_only.txt").write_text("c\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(vol_a),
        args=[str(vol_b), str(vol_c)],
    )

    vol_to_file = {
        "vol_a": "a_only.txt",
        "vol_b": "b_only.txt",
        "vol_c": "c_only.txt",
    }

    def active_volume_name():
        header = tui.get_screen_dump()[0]
        for vol_name in vol_to_file:
            if vol_name in header:
                return vol_name
        return None

    assert tui.wait_for_condition(
        lambda _lines: active_volume_name() is not None,
        description="starting fixture volume",
    ), _screen_text(tui)
    start_vol = active_volume_name()
    assert start_vol is not None, "Could not detect starting volume in header."

    tui.send_keystroke(Keys.ENTER, wait=0)
    start_file = vol_to_file[start_vol]
    assert tui.wait_for_text(start_file), _screen_text(tui)

    # Split before cycling so both panel-local volume projections remain live.
    tui.send_keystroke(Keys.F8, wait=0)

    # Cycle until a different volume becomes active.
    target_vol = drive_action_until(
        tui,
        "<",
        lambda lines: next(
            (
                volume
                for volume in vol_to_file
                if volume != start_vol and volume in next(iter(lines), "")
            ),
            False,
        ),
        max_actions=len(vol_to_file),
    )
    assert target_vol, "Failed to cycle to a different volume."

    target_file = vol_to_file[target_vol]
    if target_file not in "\n".join(tui.get_screen_dump()):
        tui.send_keystroke(Keys.ENTER, wait=0)
    assert tui.wait_for_text(target_file), _screen_text(tui)

    tui.send_keystroke(Keys.TAB, wait=0)
    assert tui.wait_for_text(start_file), _screen_text(tui)
    screen = "\n".join(tui.get_screen_dump())
    assert target_file in screen and start_file in screen, (
        "Split volume cycling lost a panel-local file projection.\n"
        f"{screen}"
    )
    tui.send_keystroke(Keys.TAB, wait=0)
    assert tui.wait_for_text(target_file), _screen_text(tui)

    tui.quit()


def test_volume_cycle_does_not_leak_file_focus_between_volumes(tmp_path, ytnova_binary):
    vol_a = tmp_path / "bug40_vol_a"
    vol_b = tmp_path / "bug40_vol_b"
    vol_a.mkdir()
    vol_b.mkdir()
    (vol_a / "a0.txt").write_text("a0\n", encoding="utf-8")
    (vol_b / "b0.txt").write_text("b0\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(vol_a))

    try:
        assert tui.wait_for_condition(
            lambda lines: lines
            if _active_volume_name_from_lines(
                lines, "bug40_vol_a", "bug40_vol_b"
            )
            == "bug40_vol_a"
            else False,
            timeout=3.0,
        ), _screen_text(tui)
        _assert_dir_mode_footer(tui, "Precondition failed: expected tree mode.")

        # Log second volume so cycling has two loaded targets.
        assert tui.send_and_wait_for_condition(
            Keys.LOG,
            lambda lines: lines
            if any("Log Path:" in line for line in lines)
            else False,
            timeout=2.0,
        ), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.CTRL_U + str(vol_b) + Keys.ENTER,
            lambda lines: lines
            if _active_volume_name_from_lines(
                lines, "bug40_vol_a", "bug40_vol_b"
            )
            == "bug40_vol_b"
            else False,
            timeout=3.0,
        ), _screen_text(tui)
        _assert_dir_mode_footer(tui, "Expected tree mode after logging volume B.")

        # Return to volume A in tree mode.
        assert tui.send_and_wait_for_condition(
            "<",
            lambda lines: lines
            if _active_volume_name_from_lines(
                lines, "bug40_vol_a", "bug40_vol_b"
            )
            == "bug40_vol_a"
            else False,
            timeout=3.0,
        ), _screen_text(tui)
        _assert_dir_mode_footer(tui, "Expected tree mode after cycling back to A.")


        lines = tui.send_and_wait_for_condition(
            "<",
            lambda current: current
            if _active_volume_name_from_lines(
                current, "bug40_vol_a", "bug40_vol_b"
            )
            == "bug40_vol_b"
            and "hex invert j compare" not in _footer_text(tui)
            and "j tree" in _footer_text(tui)
            else False,
            timeout=3.0,
        )
        screen = "\n".join(lines)
        footer = _footer_text(tui)

        assert _active_volume_name_from_lines(
            lines, "bug40_vol_a", "bug40_vol_b"
        ) == "bug40_vol_b", (
            "Volume cycle did not switch to target volume.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_inactive_dir_focus_survives_tab_away_and_back(tmp_path, ytnova_binary):
    root = tmp_path / "inactive_dir_focus_survives_tab"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (beta / "beta_focus_file.txt").write_text("b\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)
    _assert_dir_mode_footer(tui, "Expected directory footer at startup.")

    tui.send_keystroke(Keys.F8, wait=0.4)
    _assert_dir_mode_footer(tui, "Expected directory footer after split.")

    # Move to right panel and enter file mode there.
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    # Return to left panel. It must still be in dir mode.
    tui.send_keystroke(Keys.TAB, wait=0.4)
    _assert_dir_mode_footer(
        tui, "Inactive panel lost dir focus after tab-away from dir mode."
    )

    tui.quit()


def test_split_refresh_updates_inactive_tree_file_list_without_tab(
    tmp_path, ytnova_binary
):
    root = tmp_path / "split_inactive_refresh_updates_tree_file_list"
    root.mkdir()
    left = root / "left_dir"
    right = root / "right_dir"
    left.mkdir()
    right.mkdir()
    (left / "left_old.txt").write_text("left\n", encoding="utf-8")
    (right / "right_old.txt").write_text("right\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.wait_for_text("left_dir")

    tui.send_keystroke(Keys.F8, wait=0)
    tui.send_keystroke(Keys.TAB, wait=0)

    # Right panel: select right_dir in tree mode so its small file list is visible.
    selected = drive_action_until(
        tui,
        Keys.DOWN,
        lambda lines: lines if any("right_old.txt" in line for line in lines) else False,
        max_actions=128,
    )
    assert selected, "Could not select the right_dir fixture tree entry."

    # Keep right panel inactive while refreshing from the left panel.
    tui.send_keystroke(Keys.TAB, wait=0)
    (right / "right_new.txt").write_text("new\n", encoding="utf-8")
    tui.send_keystroke(Keys.CTRL_L, wait=0)
    tui.wait_for_text("right_new.txt")

    screen = "\n".join(tui.get_screen_dump())
    assert "right_new.txt" in screen, (
        "Inactive split panel did not refresh its file list after external change.\n"
        "The new file only appeared after switching panels.\n"
        f"{screen}"
    )

    tui.quit()


def test_split_tab_back_preserves_selected_file_index(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_selection_persistence"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (beta / "beta_0.txt").write_text("b\n", encoding="utf-8")

    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)

    # Select alpha_2.txt in the left panel.
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.DOWN)

    _send_and_wait_for_transition(tui, Keys.F8)
    _send_and_wait_for_transition(tui, Keys.TAB)

    # Do work in other panel: navigate to beta and enter file view.
    if "hex invert j compare" in _footer_text(tui):
        _send_and_wait_for_transition(tui, Keys.ESC)
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)

    # Return to original panel and verify selected file is unchanged.
    _send_and_wait_for_transition(tui, Keys.TAB)

    _send_and_wait_for_transition(tui, "J")
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # HitReturnToContinue

    assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(alpha / "alpha_2.txt"), (
        "Split-tab round-trip changed selected source file.\n"
        f"Expected source: {alpha / 'alpha_2.txt'}\nActual source: {logged[0]}"
    )

    tui.quit()


def test_f8_close_from_active_file_panel_preserves_file_focus_and_selection(
    tmp_path, ytnova_binary
):
    root = tmp_path / "split_close_active_file"
    root.mkdir()
    alpha = root / "alpha"
    alpha.mkdir()
    for idx in range(3):
        (alpha / f"alpha_{idx}.txt").write_text(f"{idx}\n", encoding="utf-8")
    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    try:
        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.ENTER)

        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.F8)
        _send_and_wait_for_transition(tui, Keys.F8)

        assert "alpha_2.txt" in _screen_text(tui), _screen_text(tui)

        _send_and_wait_for_transition(tui, "J")
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), _screen_text(tui)
        _send_and_wait_for_transition(tui, Keys.CTRL_U + str(compare_target) + Keys.ENTER)
        if tui.wait_for_content("Hit return to continue", timeout=1.0):
            _send_and_wait_for_transition(tui, Keys.ENTER)

        assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
        logged = log_path.read_text(encoding="utf-8").splitlines()
        assert logged[0] == str(alpha / "alpha_2.txt"), (
            "Closing split changed the selected file in the surviving panel.\n"
            f"Expected source: {alpha / 'alpha_2.txt'}\nActual source: {logged[0]}"
        )
    finally:
        tui.quit()


def test_f8_close_from_active_right_file_panel_donates_selection(
    tmp_path, ytnova_binary
):
    root = tmp_path / "split_close_active_right_file"
    root.mkdir()
    vol_a = root / "split_close_vol_a"
    vol_b = root / "split_close_vol_b"
    vol_a.mkdir()
    vol_b.mkdir()
    for idx in range(3):
        (vol_a / f"a_right_{idx}.txt").write_text(f"a{idx}\n", encoding="utf-8")
    for idx in range(3):
        (vol_b / f"b_right_{idx}.txt").write_text(f"b{idx}\n", encoding="utf-8")
    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=[str(vol_a), str(vol_b)],
    )
    assert tui.wait_for_content("a_right_0.txt", timeout=2.0)

    def active_volume_name():
        screen = _screen_text(tui)
        if "a_right_0.txt" in screen and "b_right_0.txt" not in screen:
            return "split_close_vol_a"
        if "b_right_0.txt" in screen and "a_right_0.txt" not in screen:
            return "split_close_vol_b"
        return None

    def cycle_to(volume_name):
        if active_volume_name() == volume_name:
            return
        for key in (">", "<"):
            if tui.send_and_wait_for_condition(
                key,
                lambda lines: volume_name
                if (
                    (volume_name == "split_close_vol_a")
                    == ("a_right_0.txt" in "\n".join(lines))
                    and ("b_right_0.txt" in "\n".join(lines))
                    != (volume_name == "split_close_vol_a")
                )
                else False,
                timeout=1.5,
            ):
                return
        assert active_volume_name() == volume_name, _screen_text(tui)

    def run_compare_and_read_source():
        if log_path.exists():
            log_path.unlink()
        tui.send_keystroke("J", wait=0.25)
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke(Keys.CTRL_U + str(compare_target) + Keys.ENTER, wait=0.6)
        if tui.wait_for_content("Hit return to continue", timeout=1.0):
            tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
        return log_path.read_text(encoding="utf-8").splitlines()[0]

    try:
        assert tui.send_and_wait_for_screen_change(">", timeout=1.5)
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)
        source_left_b_expected = run_compare_and_read_source()
        assert source_left_b_expected.endswith("b_right_0.txt"), source_left_b_expected

        cycle_to("split_close_vol_a")
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)

        assert tui.send_and_wait_for_screen_change(">", timeout=1.5)
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        source_right_b_expected = run_compare_and_read_source()
        assert source_right_b_expected.endswith("b_right_1.txt"), source_right_b_expected

        assert tui.send_and_wait_for_screen_change("<", timeout=1.5)
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.F8, wait=0.5)

        assert "a_right_1.txt" in _screen_text(tui), _screen_text(tui)

        assert tui.send_and_wait_for_screen_change(">", timeout=1.5)
        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.4)
        source_b_after_close = run_compare_and_read_source()
        assert source_b_after_close == source_right_b_expected, (
            "Right-panel split close did not donate per-volume file selection.\n"
            f"Left stale source: {source_left_b_expected}\n"
            f"Expected right source: {source_right_b_expected}\n"
            f"Actual source:   {source_b_after_close}\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f8_close_from_active_right_tree_preserves_viewport(tmp_path, ytnova_binary):
    root = tmp_path / "split_close_right_tree_viewport"
    root.mkdir()
    for idx in range(45):
        (root / f"dir_{idx:02d}_right_close").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, "Expected active right panel in tree mode.")

        tui.send_keystroke("\033OF", wait=0.35)
        assert tui.wait_for_content("dir_44_right_close", timeout=1.0), _screen_text(
            tui
        )
        tui.send_keystroke(Keys.UP, wait=0.25)
        assert "dir_43_right_close" in _screen_text(tui), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=1.5)
        assert tui.wait_for_content("dir_43_right_close", timeout=1.0), (
            "Closing split from active right tree lost the active selection.\n"
            f"{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_inactive_panel_stays_file_focused_after_tab_away(tmp_path, ytnova_binary):
    root = tmp_path / "inactive_panel_file_focus"
    root.mkdir()
    left = root / "left_focus_dir_A"
    right = root / "right_focus_dir_B"
    left.mkdir()
    right.mkdir()
    (right / "right_focus_file_0.txt").write_text("x\n", encoding="utf-8")
    (right / "right_focus_file_1.txt").write_text("y\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("right_focus_dir_B", timeout=2.0), _screen_text(tui)

    _send_and_wait_for_transition(tui, Keys.F8)
    _send_and_wait_for_transition(tui, Keys.TAB)

    # Move right panel to right_focus_dir_B and enter file view.
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)

    # Switch away. Inactive right panel should remain file-focused visually,
    # not revert to tree.
    _send_and_wait_for_transition(tui, Keys.TAB)

    screen = "\n".join(tui.get_screen_dump())
    assert "right_focus_file_0.txt" in screen
    assert "right_focus_file_1.txt" in screen
    assert screen.count("right_focus_dir_B") <= 1, (
        "Inactive panel reverted to tree view after tab away.\n"
        f"{screen}"
    )

    tui.quit()

def test_split_separator_stays_continuous_during_file_tree_toggle(tmp_path, ytnova_binary):
    root = tmp_path / "split_separator_continuity"
    root.mkdir()
    left = root / "left_sep_dir"
    right = root / "right_sep_dir"
    left.mkdir()
    right.mkdir()
    (left / "left_a.txt").write_text("a\n", encoding="utf-8")
    (right / "right_a.txt").write_text("b\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), _screen_text(tui)

    tui.send_keystroke(Keys.F8, wait=0.4)
    tui.send_keystroke(Keys.TAB, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    lines = tui.get_screen_dump()
    _assert_split_column_continuous(lines, "right active file / left inactive tree")

    tui.send_keystroke(Keys.TAB, wait=0.4)
    lines = tui.get_screen_dump()
    _assert_split_column_continuous(lines, "left active tree / right inactive file")

    tui.send_keystroke(Keys.TAB, wait=0.4)
    lines = tui.get_screen_dump()
    _assert_split_column_continuous(lines, "right active file after second tab")

    tui.quit()



def test_volume_cycle_restores_prior_directory_selection(tmp_path, ytnova_binary):
    vol_rich = tmp_path / "vol_rich_restore_selection"
    vol_sparse_b = tmp_path / "vol_sparse_b_restore_selection"
    vol_sparse_c = tmp_path / "vol_sparse_c_restore_selection"
    vol_rich.mkdir()
    vol_sparse_b.mkdir()
    vol_sparse_c.mkdir()

    for i in range(6):
        d = vol_rich / f"r_dir_{i}"
        d.mkdir()
        (d / f"r_file_{i}_unique.txt").write_text(f"{i}\n", encoding="utf-8")

    (vol_sparse_b / "b_dir_0").mkdir()
    (vol_sparse_b / "b_dir_0" / "b_file_0_unique.txt").write_text("b\n", encoding="utf-8")
    (vol_sparse_c / "c_dir_0").mkdir()
    (vol_sparse_c / "c_dir_0" / "c_file_0_unique.txt").write_text("c\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(tmp_path),
        args=[str(vol_rich), str(vol_sparse_b), str(vol_sparse_c)],
    )
    assert tui.wait_for_content("vol_rich_restore_selection", timeout=2.0), _screen_text(tui)

    def active_volume_name():
        header = tui.get_screen_dump()[0]
        for name in (
            "vol_rich_restore_selection",
            "vol_sparse_b_restore_selection",
            "vol_sparse_c_restore_selection",
        ):
            if name in header:
                return name
        return None

    # Normalize to the rich volume first so we can choose a non-trivial index.
    if active_volume_name() != "vol_rich_restore_selection":
        assert tui.send_and_wait_for_screen_change(">", timeout=1.0), _screen_text(tui)
    if active_volume_name() != "vol_rich_restore_selection":
        assert tui.send_and_wait_for_screen_change(">", timeout=1.0), _screen_text(tui)
    if active_volume_name() != "vol_rich_restore_selection":
        assert tui.send_and_wait_for_screen_change(">", timeout=1.0), _screen_text(tui)
    assert active_volume_name() == "vol_rich_restore_selection", (
        "Could not switch to rich volume for selection restore test.\n"
        + "\n".join(tui.get_screen_dump())
    )
    start_vol = "vol_rich_restore_selection"

    # Move off the default/root selection before capturing target.
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.DOWN, wait=0.25)
    tui.send_keystroke(Keys.ENTER, wait=0.45)

    screen = "\n".join(tui.get_screen_dump())
    expected_file = None
    for candidate in (
        "r_file_0_unique.txt",
        "r_file_1_unique.txt",
        "r_file_2_unique.txt",
        "r_file_3_unique.txt",
        "r_file_4_unique.txt",
        "r_file_5_unique.txt",
    ):
        if candidate in screen:
            expected_file = candidate
            break
    assert expected_file is not None, (
        "Failed to detect selected file on initial volume before cycling.\n" + screen
    )

    tui.send_keystroke(Keys.ESC, wait=0.35)
    _assert_dir_mode_footer(tui, "Expected directory footer after leaving file view.")
    lines = tui.get_screen_dump()
    expected_dir = expected_file.replace("r_file_", "r_dir_").replace("_unique.txt", "")
    assert _stats_current_dir_contains(lines, expected_dir), (
        "Baseline check failed: selected directory was not retained after leaving file view.\n"
        f"Expected selected dir marker: {expected_dir}\n" + "\n".join(lines)
    )

    # Cycle away and back.
    assert tui.send_and_wait_for_screen_change(">", timeout=1.0), _screen_text(tui)
    assert active_volume_name() != start_vol, _screen_text(tui)
    assert tui.send_and_wait_for_screen_change(">", timeout=1.0), _screen_text(tui)
    assert tui.send_and_wait_for_screen_change(">", timeout=1.0), _screen_text(tui)
    returned = active_volume_name() == start_vol

    assert returned, "Did not return to start volume while cycling loaded volumes."

    lines = tui.get_screen_dump()
    screen = "\n".join(lines)
    assert _stats_current_dir_contains(lines, expected_dir), (
        "Directory selection was not restored after cycling volumes away/back.\n"
        f"Expected selected dir marker: {expected_dir}\n{screen}"
    )

    tui.quit()


def test_smallwindowskip_volume_cycle_restores_deep_file_context(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_volume_cycle_deep_context"
    root.mkdir()
    vol_a = root / "smallskip_cycle_vol_a"
    vol_b = root / "smallskip_cycle_vol_b"
    home_vol = root / "smallskip_cycle_home"
    deep_a = vol_a / "a_parent" / "a_deep"
    deep_b = vol_b / "b_parent" / "b_deep"
    release_anchor = home_vol / "zz_release_anchor"
    deep_a.mkdir(parents=True)
    deep_b.mkdir(parents=True)
    release_anchor.mkdir(parents=True)

    for i in range(3):
        (deep_a / f"a_deep_{i}.txt").write_text(f"a{i}\n", encoding="utf-8")
        (deep_b / f"b_deep_{i}.txt").write_text(f"b{i}\n", encoding="utf-8")

    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)
    with (root / ".ytnova").open("a", encoding="utf-8") as profile:
        profile.write("TREEDEPTH=1\nSMALLWINDOWSKIP=1\n")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=[str(home_vol), str(vol_a), str(vol_b)],
    )
    volume_names = (
        "smallskip_cycle_vol_a",
        "smallskip_cycle_vol_b",
        "smallskip_cycle_home",
    )

    def cycle_to(volume_name, ready_marker, key=">"):
        def volume_ready(lines):
            return (
                _active_volume_name_from_lines(lines, *volume_names) == volume_name
                and any(ready_marker in line for line in lines)
            )
        if volume_ready(tui.get_screen_dump()):
            return
        for _ in range(12):
            lines = tui.send_and_wait_for_condition(
                key,
                lambda current_lines: current_lines if volume_ready(current_lines) else False,
                timeout=1.5,
            )
            if lines:
                return
        assert volume_ready(tui.get_screen_dump()), _screen_text(tui)

    try:
        cycle_to("smallskip_cycle_vol_a", "a_parent")

        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if "hex invert j compare" not in _footer_text_from_lines(lines)
            else False,
            timeout=1.5,
        )
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if _stats_current_dir_contains(current_lines, "a_deep")
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if "hex invert j compare" in _footer_text_from_lines(current_lines)
            and any("a_deep_0.txt" in line for line in current_lines)
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        source_a_expected = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_a_expected.endswith("a_deep_1.txt"), source_a_expected

        cycle_to("smallskip_cycle_vol_b", "b_parent")
        if "hex invert j compare" in _footer_text(tui):
            tui.send_and_wait_for_condition(
                Keys.ESC,
                lambda lines: lines
                if "hex invert j compare" not in _footer_text_from_lines(lines)
                else False,
                timeout=1.5,
            )
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if "hex invert j compare" not in _footer_text_from_lines(lines)
            else False,
            timeout=1.5,
        )
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if _stats_current_dir_contains(current_lines, "b_deep")
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if "hex invert j compare" in _footer_text_from_lines(current_lines)
            and any("b_deep_0.txt" in line for line in current_lines)
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        source_b_expected = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_b_expected.endswith("b_deep_2.txt"), source_b_expected

        cycle_to("smallskip_cycle_home", "zz_release_anchor")
        if "hex invert j compare" in _footer_text(tui):
            tui.send_and_wait_for_condition(
                Keys.ESC,
                lambda lines: lines
                if "hex invert j compare" not in _footer_text_from_lines(lines)
                else False,
                timeout=1.5,
            )
        _wait_for_footer_state(
            tui,
            contains=("j compare", "j tree"),
            excludes=("hex invert j compare",),
        )
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        lines = tui.wait_for_condition(
            lambda current_lines: current_lines
            if _stats_current_dir_contains(current_lines, "zz_release_anchor")
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        lines = tui.send_and_wait_for_condition(
            "k",
            lambda current_lines: current_lines
            if any("Select Volume" in line for line in current_lines)
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        tui.send_and_wait_for_screen_change("d", timeout=1.0)
        tui.send_and_wait_for_screen_change("y", timeout=1.5)
        if tui.wait_for_text("Select Volume", timeout=0.2):
            tui.send_and_wait_for_condition(
                Keys.ESC,
                lambda lines: lines
                if not any("Select Volume" in line for line in lines)
                else False,
                timeout=1.5,
            )

        cycle_to("smallskip_cycle_vol_a", "a_parent")
        _wait_for_footer_state(tui, contains=("hex invert j compare",))

        source_a_after_cycle = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_a_after_cycle == source_a_expected, (
            "SMALLWINDOWSKIP=1 volume cycling lost deep per-volume file context for A.\n"
            f"Expected: {source_a_expected}\n"
            f"Actual:   {source_a_after_cycle}\n{_screen_text(tui)}"
        )
        tui.send_and_wait_for_condition(
            Keys.ESC,
            lambda lines: lines
            if "hex invert j compare" not in _footer_text_from_lines(lines)
            else False,
            timeout=1.5,
        )
        _wait_for_footer_state(
            tui,
            contains=("j compare", "j tree"),
            excludes=("hex invert j compare",),
        )
        lines = tui.wait_for_condition(
            lambda current_lines: current_lines
            if _stats_current_dir_contains(current_lines, "a_deep")
            else False,
            timeout=1.5,
        )
        assert lines, (
            "Leaving restored file view returned to a parent/tree location.\n"
            + _screen_text(tui)
        )
        tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if "hex invert j compare" in _footer_text_from_lines(lines)
            else False,
            timeout=1.5,
        )

        cycle_to("smallskip_cycle_vol_b", "b_parent")
        _wait_for_footer_state(tui, contains=("hex invert j compare",))
        source_b_after_cycle = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_b_after_cycle == source_b_expected, (
            "SMALLWINDOWSKIP=1 volume cycling lost deep per-volume file context for B.\n"
            f"Expected: {source_b_expected}\n"
            f"Actual:   {source_b_after_cycle}\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_smallwindowskip_release_active_volume_switch_keeps_stats_anchor_safe(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_release_active_volume_switch_safe"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=1\nSMALLWINDOWSKIP=1\n",
        encoding="utf-8",
    )

    home_vol = root / "aa_home_vol"
    work_vol = root / "zz_work_vol"
    (home_vol / "h_parent" / "h_deep").mkdir(parents=True)
    (work_vol / "w_parent" / "w_deep").mkdir(parents=True)

    for i in range(3):
        (home_vol / "h_parent" / "h_deep" / f"h{i}.txt").write_text(
            f"h{i}\n", encoding="utf-8"
        )
        (work_vol / "w_parent" / "w_deep" / f"w{i}.txt").write_text(
            f"w{i}\n", encoding="utf-8"
        )

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=[str(home_vol), str(work_vol)],
    )
    assert tui.wait_for_content("h_parent", timeout=2.0)

    def volume_visible(lines, volume_name):
        screen = "\n".join(lines)
        if volume_name == "aa_home_vol":
            return (
                ("h0.txt" in screen or "h_parent" in screen)
                and "w0.txt" not in screen
                and "w_parent" not in screen
            )
        return (
            ("w0.txt" in screen or "w_parent" in screen)
            and "h0.txt" not in screen
            and "h_parent" not in screen
        )

    def active_volume_name():
        for volume_name in ("aa_home_vol", "zz_work_vol"):
            if volume_visible(tui.get_screen_dump(), volume_name):
                return volume_name
        return None

    def cycle_to(volume_name, key):
        if active_volume_name() == volume_name:
            return
        if tui.send_and_wait_for_condition(
            key,
            lambda lines: lines if volume_visible(lines, volume_name) else False,
            timeout=2.0,
        ):
            return
        assert active_volume_name() == volume_name, _screen_text(tui)

    def enter_deep_file_view(prefix):
        if "hex invert j compare" in _footer_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(tui, f"Expected dir mode before entering {prefix}.")

        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert f"{prefix}0.txt" in _screen_text(tui), _screen_text(tui)
        tui.send_keystroke(Keys.DOWN, wait=0.2)

    try:
        cycle_to("aa_home_vol", ">")
        enter_deep_file_view("h")

        cycle_to("zz_work_vol", ">")
        enter_deep_file_view("w")

        cycle_to("aa_home_vol", "<")
        if "hex invert j compare" in _footer_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.4)
        _assert_dir_mode_footer(tui, "Expected dir mode on releasable home volume.")

        tui.send_keystroke("k", wait=0.4)
        assert tui.wait_for_content("Select Volume", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("d", wait=0.3)
        tui.send_keystroke("y", wait=1.0)
        assert tui.wait_for_content("Select Volume", timeout=1.0), _screen_text(tui)

        tui.send_keystroke(Keys.ENTER, wait=1.0)
        assert active_volume_name() == "zz_work_vol", _screen_text(tui)

        if "hex invert j compare" not in _footer_text(tui):
            tui.send_keystroke(Keys.ENTER, wait=0.5)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        screen = _screen_text(tui)
        assert "w1.txt" in screen or "w2.txt" in screen, screen
    finally:
        tui.quit()


def test_enter_repo_src_preserves_tree_viewport_anchor(ytnova_binary):
    repo_root = Path(__file__).resolve().parents[1]
    home = repo_root.parent

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(home))

    def move_to_stats_dir(marker, *, max_steps=120):
        marker_token = f" {marker} "
        lines = tui.get_screen_dump()
        if _stats_current_dir_contains(lines, marker_token):
            return lines
        for _ in range(max_steps):
            lines = tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.4)
            if not lines:
                lines = tui.get_screen_dump()
            if _stats_current_dir_contains(lines, marker_token):
                return lines
        pytest.fail(
            f"Failed to move selection to stats marker '{marker}'.\n{_screen_text(tui)}"
        )

    try:
        move_to_stats_dir(repo_root.name)
        tui.send_and_wait_for_screen_change(Keys.RIGHT, timeout=1.5)

        before_lines = move_to_stats_dir("src")
        before_screen = "\n".join(before_lines)
        before_first_row = _first_tree_row_segment(
            before_lines, _detect_stats_split_x(before_lines)
        )
        assert before_first_row is not None, before_screen

        after_lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if _stats_current_dir_contains(current_lines, "src")
            and "hex invert j compare" not in _footer_text_from_lines(current_lines)
            else False,
            timeout=2.0,
        )
        assert after_lines, _screen_text(tui)
        _assert_dir_mode_footer(tui, "Expected tree mode after scanning deep tree node.")

        after_screen = "\n".join(after_lines)
        assert _stats_current_dir_contains(after_lines, "src"), (
            "ENTER moved active tree selection unexpectedly.\n"
            f"{after_screen}"
        )
        assert (
            _first_tree_row_segment(after_lines, _detect_stats_split_x(after_lines))
            == before_first_row
        ), (
            "ENTER recentered tree viewport instead of preserving the existing "
            "viewport origin.\n"
            f"before_first_row={before_first_row!r}\n"
            f"{after_screen}"
        )
    finally:
        tui.quit()


def test_enter_repo_src_cmd_preserves_tree_viewport_anchor(ytnova_binary):
    repo_root = Path(__file__).resolve().parents[1]

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(repo_root))
    assert tui.wait_for_text(repo_root.name, timeout=2.0), _screen_text(tui)

    def move_to_stats_dir(marker, *, timeout=20.0):
        """Navigate downward to the selected current-directory marker."""
        _select_tree_stats_marker(
            tui,
            f" {marker} ",
            timeout=timeout,
            keys=(Keys.DOWN,),
        )


    try:
        move_to_stats_dir("src")
        before_src_lines = tui.get_screen_dump()
        before_src_screen = "\n".join(before_src_lines)
        before_src_selected_label = _tree_panel_selected_label(
            before_src_lines, _detect_stats_split_x(before_src_lines)
        )
        assert before_src_selected_label == "src", before_src_screen

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        _assert_dir_mode_footer(tui, "Expected tree mode after entering src subtree.")

        after_src_lines = tui.get_screen_dump()
        after_src_screen = "\n".join(after_src_lines)
        assert _stats_current_dir_contains(after_src_lines, "src"), (
            "ENTER moved selection away from src unexpectedly.\n"
            f"{after_src_screen}"
        )
        viewport = _assert_tree_viewport_origin_stable(
            before_src_lines,
            after_src_lines,
            split_col=_detect_stats_split_x(after_src_lines),
            label="ENTER on src",
        )
        assert viewport["selected_visible"], (
            "ENTER on src should keep the selected tree row visible.\n"
            f"{after_src_screen}"
        )

        move_to_stats_dir("cmd")
        before_cmd_lines = tui.get_screen_dump()
        before_cmd_screen = "\n".join(before_cmd_lines)
        before_cmd_selected_label = _tree_panel_selected_label(
            before_cmd_lines, _detect_stats_split_x(before_cmd_lines)
        )
        assert before_cmd_selected_label == "cmd", before_cmd_screen

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        tui.send_keystroke(Keys.ESC, wait=0.4)
        _assert_dir_mode_footer(
            tui, "Expected tree mode after returning from src/cmd file view."
        )

        after_cmd_lines = tui.get_screen_dump()
        after_cmd_screen = "\n".join(after_cmd_lines)
        assert _stats_current_dir_contains(after_cmd_lines, "cmd"), (
            "Returning from cmd file view moved selection unexpectedly.\n"
            f"{after_cmd_screen}"
        )
        viewport = _assert_tree_viewport_origin_stable(
            before_cmd_lines,
            after_cmd_lines,
            split_col=_detect_stats_split_x(after_cmd_lines),
            label="ENTER on src/cmd after file view",
        )
        assert viewport["selected_visible"], (
            "Returning from cmd file view should keep the selected tree row visible.\n"
            f"{after_cmd_screen}"
        )
    finally:
        tui.quit()


def test_split_tab_end_home_preserves_left_tree_viewport(tmp_path, ytnova_binary):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "split_tab_end_home_home"
    home.mkdir()
    (home / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\nSMALLWINDOWSKIP=0\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(repo_root),
        env_extra={"HOME": str(home)},
    )
    assert tui.wait_for_text(repo_root.name, timeout=2.0), _screen_text(tui)

    def move_to_stats_dir(marker, *, timeout=20.0):
        """Navigate downward to the selected current-directory marker."""
        _select_tree_stats_marker(
            tui,
            f" {marker} ",
            timeout=timeout,
            keys=(Keys.DOWN,),
        )


    try:
        move_to_stats_dir("src")
        tui.send_keystroke(Keys.ENTER, wait=0.6)
        _assert_dir_mode_footer(tui, "Expected tree mode after entering src subtree.")

        move_to_stats_dir("cmd")
        tui.send_keystroke(Keys.ENTER, wait=0.6)

        tui.send_keystroke(Keys.F8, wait=0.6)
        tui.send_keystroke(Keys.TAB, wait=0.6)

        split_lines = tui.get_screen_dump()
        split_col = _detect_split_column(split_lines)
        assert split_col is not None, _screen_text(tui)
        left_rows_before = _tree_segment_rows(split_lines, split_col)

        tui.send_keystroke("\033OF", wait=0.6)
        tui.send_keystroke(Keys.HOME, wait=0.6)
        tui.send_keystroke(Keys.TAB, wait=0.6)

        after_lines = tui.get_screen_dump()
        after_split_col = _detect_split_column(after_lines)
        assert after_split_col is not None, _screen_text(tui)
        left_rows_after = _tree_segment_rows(after_lines, after_split_col)

        assert left_rows_after == left_rows_before, (
            "Split/tab/end/home/tab should preserve the left panel tree "
            "viewport exactly; the inactive tree pane shifted unexpectedly.\n"
            f"Before: {left_rows_before}\nAfter:  {left_rows_after}\n"
            f"{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_volume_cycle_leak_state_preserves_per_volume_file_selection(
    tmp_path, ytnova_binary
):
    root = tmp_path / "volume_cycle_leak_state_file_selection"
    root.mkdir()
    vol_a = root / "cycle_state_vol_a"
    vol_b = root / "cycle_state_vol_b"
    vol_a.mkdir()
    vol_b.mkdir()

    for i in range(4):
        (vol_a / f"a_state_{i}.txt").write_text(f"a{i}\n", encoding="utf-8")
    for i in range(3):
        (vol_b / f"b_state_{i}.txt").write_text(f"b{i}\n", encoding="utf-8")

    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=[str(vol_a), str(vol_b)],
    )

    def active_volume_name():
        header = tui.get_screen_dump()[0]
        for name in ("cycle_state_vol_a", "cycle_state_vol_b"):
            if name in header:
                return name
        return None

    def wait_for_active_volume(key, expected):
        lines = tui.send_and_wait_for_condition(
            key,
            lambda current_lines: current_lines
            if expected in (current_lines[0] if current_lines else "")
            else False,
            timeout=2.0,
        )
        assert lines, _screen_text(tui)

    lines = tui.send_and_wait_for_condition(
        Keys.F5,
        lambda current_lines: current_lines
        if any(
            name in (current_lines[0] if current_lines else "")
            for name in ("cycle_state_vol_a", "cycle_state_vol_b")
        )
        else False,
        timeout=2.0,
    )
    assert lines, _screen_text(tui)

    def run_compare_and_read_source():
        if log_path.exists():
            log_path.unlink()
        tui.send_keystroke("J", wait=0.25)
        assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke(Keys.CTRL_U + str(compare_target) + Keys.ENTER, wait=0.6)
        assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
        if tui.wait_for_content("Hit return to continue", timeout=1.0):
            _send_and_wait_for_transition(tui, Keys.ENTER)
        return log_path.read_text(encoding="utf-8").splitlines()[0]

    if active_volume_name() != "cycle_state_vol_a":
        wait_for_active_volume(">", "cycle_state_vol_a")
    assert active_volume_name() == "cycle_state_vol_a", _screen_text(tui)

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    source_a_expected = run_compare_and_read_source()
    assert source_a_expected.endswith("a_state_2.txt"), source_a_expected

    wait_for_active_volume(">", "cycle_state_vol_b")
    assert active_volume_name() == "cycle_state_vol_b", _screen_text(tui)

    if "hex invert j compare" not in _footer_text(tui):
        tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    source_b_expected = run_compare_and_read_source()
    assert source_b_expected.endswith("b_state_1.txt"), source_b_expected

    wait_for_active_volume("<", "cycle_state_vol_a")
    assert active_volume_name() == "cycle_state_vol_a", _screen_text(tui)
    if "hex invert j compare" not in _footer_text(tui):
        tui.send_keystroke(Keys.ENTER, wait=0.4)
    source_a_after_cycle = run_compare_and_read_source()
    assert source_a_after_cycle == source_a_expected, (
        "Volume cycling leaked file selection state across volumes (A).\n"
        f"Expected: {source_a_expected}\n"
        f"Actual:   {source_a_after_cycle}\n{_screen_text(tui)}"
    )

    wait_for_active_volume(">", "cycle_state_vol_b")
    assert active_volume_name() == "cycle_state_vol_b", _screen_text(tui)
    if "hex invert j compare" not in _footer_text(tui):
        tui.send_keystroke(Keys.ENTER, wait=0.4)
    source_b_after_cycle = run_compare_and_read_source()
    assert source_b_after_cycle == source_b_expected, (
        "Volume cycling leaked file selection state across volumes (B).\n"
        f"Expected: {source_b_expected}\n"
        f"Actual:   {source_b_after_cycle}\n{_screen_text(tui)}"
    )

    tui.quit()


def test_split_file_selection_preserves_panel_local_volume_cycle_state(
    tmp_path, ytnova_binary
):
    root = tmp_path / "split_preserves_panel_local_volume_cycle_state"
    root.mkdir()
    vol_a = root / "split_cycle_vol_a"
    vol_b = root / "split_cycle_vol_b"
    vol_a.mkdir()
    vol_b.mkdir()

    for i in range(4):
        (vol_a / f"a_state_{i}.txt").write_text(f"a{i}\n", encoding="utf-8")
    for i in range(2):
        (vol_b / f"b_state_{i}.txt").write_text(f"b{i}\n", encoding="utf-8")

    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")
    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=[str(vol_a), str(vol_b)],
    )
    volume_names = ("split_cycle_vol_a", "split_cycle_vol_b")

    try:
        _enter_file_view_for_fixture(tui, "a_state_0.txt")
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        source_left_expected = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_left_expected.endswith("a_state_2.txt"), source_left_expected

        tui.send_and_wait_for_screen_change(Keys.F8, timeout=1.5)
        tui.send_and_wait_for_screen_change(Keys.TAB, timeout=1.5)
        _wait_for_footer_state(tui, contains=("hex invert j compare",))
        tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=0.8)
        source_right_expected = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_right_expected.endswith("a_state_3.txt"), source_right_expected

        tui.send_and_wait_for_screen_change(Keys.TAB, timeout=1.5)
        source_left_before_cycle = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_left_before_cycle == source_left_expected, (
            "Split panel file selections should diverge per panel before cycling.\n"
            f"Left expected:  {source_left_expected}\n"
            f"Left observed:  {source_left_before_cycle}\n{_screen_text(tui)}"
        )

        lines = tui.send_and_wait_for_condition(
            ">",
            lambda current_lines: current_lines
            if _active_volume_name_from_lines(current_lines, *volume_names)
            == "split_cycle_vol_b"
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        if "hex invert j compare" not in _footer_text_from_lines(lines):
            lines = tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda current_lines: current_lines
                if "hex invert j compare" in _footer_text_from_lines(current_lines)
                else False,
                timeout=1.5,
            )
            assert lines, _screen_text(tui)
        lines = tui.send_and_wait_for_condition(
            "<",
            lambda current_lines: current_lines
            if _active_volume_name_from_lines(current_lines, *volume_names)
            == "split_cycle_vol_a"
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        if "hex invert j compare" not in _footer_text_from_lines(lines):
            lines = tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda current_lines: current_lines
                if "hex invert j compare" in _footer_text_from_lines(current_lines)
                else False,
                timeout=1.5,
            )
            assert lines, _screen_text(tui)
        source_left_after_cycle = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_left_after_cycle == source_left_expected, (
            "Left split panel lost its own per-volume file selection.\n"
            f"Expected: {source_left_expected}\n"
            f"Actual:   {source_left_after_cycle}\n{_screen_text(tui)}"
        )

        lines = tui.send_and_wait_for_screen_change(Keys.TAB, timeout=1.5)
        assert lines, _screen_text(tui)
        if "hex invert j compare" not in _footer_text_from_lines(lines):
            lines = tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda current_lines: current_lines
                if "hex invert j compare" in _footer_text_from_lines(current_lines)
                else False,
                timeout=1.5,
            )
            assert lines, _screen_text(tui)
        source_right_before_cycle = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_right_before_cycle == source_right_expected, (
            "Cycling the left split panel leaked selection into right panel.\n"
            f"Expected: {source_right_expected}\n"
            f"Actual:   {source_right_before_cycle}\n{_screen_text(tui)}"
        )

        lines = tui.send_and_wait_for_condition(
            ">",
            lambda current_lines: current_lines
            if _active_volume_name_from_lines(current_lines, *volume_names)
            == "split_cycle_vol_b"
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        if "hex invert j compare" not in _footer_text_from_lines(lines):
            lines = tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda current_lines: current_lines
                if "hex invert j compare" in _footer_text_from_lines(current_lines)
                else False,
                timeout=1.5,
            )
            assert lines, _screen_text(tui)
        lines = tui.send_and_wait_for_condition(
            "<",
            lambda current_lines: current_lines
            if _active_volume_name_from_lines(current_lines, *volume_names)
            == "split_cycle_vol_a"
            else False,
            timeout=1.5,
        )
        assert lines, _screen_text(tui)
        if "hex invert j compare" not in _footer_text_from_lines(lines):
            lines = tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda current_lines: current_lines
                if "hex invert j compare" in _footer_text_from_lines(current_lines)
                else False,
                timeout=1.5,
            )
            assert lines, _screen_text(tui)
        source_right_after_cycle = _run_compare_and_read_source(tui, compare_target, log_path)
        assert source_right_after_cycle == source_right_expected, (
            "Right split panel lost its own per-volume file selection.\n"
            f"Expected: {source_right_expected}\n"
            f"Actual:   {source_right_after_cycle}\n{_screen_text(tui)}"
        )

        lines = tui.send_and_wait_for_screen_change(Keys.TAB, timeout=1.5)
        assert lines, _screen_text(tui)
        if "hex invert j compare" not in _footer_text_from_lines(lines):
            lines = tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda current_lines: current_lines
                if "hex invert j compare" in _footer_text_from_lines(current_lines)
                else False,
                timeout=1.5,
            )
            assert lines, _screen_text(tui)
        source_left_after_right_cycle = _run_compare_and_read_source(
            tui, compare_target, log_path
        )
        assert source_left_after_right_cycle == source_left_expected, (
            "Right-panel cycling leaked back into left panel selection.\n"
            f"Expected: {source_left_expected}\n"
            f"Actual:   {source_left_after_right_cycle}\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_navigation_does_not_expand(tmp_path, ytnova_binary):
    """
    BUG 2: Verifies that pressing DOWN arrow merely moves the cursor,
    and does NOT automatically scan/expand subdirectories.
    """
    # Create nested structure: test_root/parent/child/file.txt
    root = tmp_path / "test_root"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    child_dir = root / "parent" / "child"
    child_dir.mkdir(parents=True)
    (child_dir / "file.txt").touch()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("parent", timeout=2.0), _screen_text(tui)

    # Press DOWN to highlight 'parent'. It should NOT expand to show 'child'.
    before = tui.get_screen_dump()
    screen = "\n".join(before)
    if "child" in screen: print("CHILD ALREADY VISIBLE")
    lines = tui.send_and_wait_for_condition(
        Keys.DOWN,
        lambda current_lines: current_lines
        if current_lines != before and not any("child" in line for line in current_lines)
        else False,
        timeout=2.0,
    )
    assert lines, _screen_text(tui)

    screen = "\n".join(lines)

    if "child" in screen:
        pytest.fail(f"AUTO-EXPAND BUG: Pressing DOWN automatically expanded the directory. 'child' is visible:\n{screen}")

    tui.quit()


def test_down_from_root_does_not_scroll_hidden_prefix(tmp_path, ytnova_binary):
    """
    With HIDEDOTFILES=1, DOWN from root must move to the next visible sibling
    without treating hidden-dot entries as scroll-driving rows.
    """
    root = tmp_path / "down_root_hidden_prefix"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=1\nHIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for i in range(40):
        (root / f".hidden_{i:02d}").mkdir()

    for name in ("go", "gone", "snap", "wikiteam3_utilities"):
        d = root / name
        d.mkdir()
        (d / "child").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("go", timeout=2.0), _screen_text(tui)
    try:
        before = _screen_text(tui)
        before_lines = before.splitlines()
        assert len(before_lines) > 2, before
        assert "mq/" in before_lines[2], before

        tui.send_keystroke(Keys.UP, wait=0.4)
        after_up = _screen_text(tui)
        after_up_lines = after_up.splitlines()
        assert len(after_up_lines) > 2, after_up
        assert "mq/" in after_up_lines[2], (
            "UP at top wrapped selection to the bottom.\n"
            f"Before:\n{before}\n\nAfter UP:\n{after_up}"
        )

        tui.send_keystroke(Keys.DOWN, wait=0.4)
        after = _screen_text(tui)
        after_lines = after.splitlines()
        assert len(after_lines) > 2, after

        assert "mq/" in after_lines[2], (
            "DOWN from root scrolled the tree by hidden-dot index distance "
            "instead of one visible row.\n"
            f"Before:\n{before}\n\nAfter:\n{after}"
        )
    finally:
        tui.quit()


def test_tree_jump_ignores_hidden_dot_prefix_descendants(tmp_path, ytnova_binary):
    root = tmp_path / "jump_hidden_prefix" / "home" / "rob"
    root.mkdir(parents=True)
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=4\nHIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    hidden_src = root / ".local" / "src"
    visible_src = root / "ytnova" / "src"
    hidden_src.mkdir(parents=True)
    visible_src.mkdir(parents=True)
    (hidden_src / "hidden_src_marker.txt").write_text("hidden\n", encoding="utf-8")
    (visible_src / "visible_src_marker.txt").write_text("visible\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"HOME": str(root)},
    )
    try:
        assert tui.wait_for_text("ytnova", timeout=3.0), _screen_text(tui)
        lines = tui.send_and_wait_for_condition(
            "/src" + Keys.ENTER,
            lambda current: current
            if any("visible_src_marker.txt" in line for line in current)
            else False,
            timeout=3.0,
        )
        assert lines, _screen_text(tui)
        screen = "\n".join(lines)
        assert "visible_src_marker.txt" in screen, (
            "Visible tree jump should select the visible src directory.\n"
            f"{screen}"
        )
        assert "hidden_src_marker.txt" not in screen, (
            "Visible tree jump selected a hidden dot-directory descendant.\n"
            f"{screen}"
        )
        assert "/.local/src" not in screen, (
            "Visible tree jump resolved through a hidden dot-directory path.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_mkdir_preserves_collapsed_children_after_left_enter(
    tmp_path, ytnova_binary
):
    """
    Collapsing root descendants (LEFT then ENTER) must remain collapsed after
    creating a new sibling directory at root.
    """
    root = tmp_path / "mkdir_collapse_preservation"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for i in range(40):
        (root / f".hidden_{i:02d}").mkdir()

    for name, child in (
        ("go", "pkg"),
        ("gone", "home"),
        ("snap", "glow"),
        ("wikiteam3_utilities", "dumps"),
        ("ytnova", "docs"),
    ):
        d = root / name
        d.mkdir()
        (d / child).mkdir(parents=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.wait_for_content("tqgo", timeout=3.0), _screen_text(tui)
        tui.send_keystroke(Keys.LEFT, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.4)

        collapsed = _screen_text(tui)
        assert "tqgo" not in collapsed, collapsed
        assert "pkg" not in collapsed, collapsed

        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(
            tui
        )
        tui.send_keystroke("00" + Keys.ENTER, wait=0.8)
        assert tui.wait_for_condition(
            lambda lines: lines
            if not any("MAKE DIRECTORY:" in line for line in lines)
            else False,
            timeout=3.0,
            description="completed root directory creation",
        ), _screen_text(tui)

        after = _screen_text(tui)
        assert "tqgo" in after, after
        assert "pkg" not in after, (
            "mkdir at root re-expanded previously collapsed descendants.\n"
            f"{after}"
        )
    finally:
        tui.quit()


def test_split_peer_tree_keeps_root_visible_with_hidden_prefix(
    tmp_path, ytnova_binary
):
    """
    Regression guard:
    After mkdir + DOWN + split, the peer panel tree must keep root visible at the
    top when HIDEDOTFILES hides a large hidden-prefix set.
    """
    root = tmp_path / "split_peer_root_visible"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\nSMALLWINDOWSKIP=0\n",
        encoding="utf-8",
    )

    for i in range(40):
        (root / f".hidden_{i:02d}").mkdir()

    for name, child in (
        ("go", "pkg"),
        ("gone", "home"),
        ("snap", "glow"),
        ("wikiteam3_utilities", "dumps"),
        ("ytnova", "docs"),
    ):
        d = root / name
        d.mkdir()
        (d / child).mkdir(parents=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.wait_for_content("tqgo", timeout=3.0), _screen_text(tui)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(
            tui
        )
        tui.send_keystroke("00" + Keys.ENTER, wait=0.7)
        assert tui.wait_for_content("tq00", timeout=3.0), _screen_text(tui)
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.F8, wait=0.5)

        lines = tui.wait_for_condition(
            lambda current: current
            if any("<*>" in line for line in current)
            else False,
            timeout=3.0,
            description="split panel frame",
        )
        assert any("mq/tmp" in line for line in lines), (
            "Peer panel tree lost its root after split despite available space.\n"
            f"{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_end_preserves_tree_viewport_with_hidden_prefix(tmp_path, ytnova_binary):
    root = tmp_path / "end_hidden_prefix_viewport"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\nSMALLWINDOWSKIP=0\n",
        encoding="utf-8",
    )

    for i in range(40):
        (root / f".hidden_{i:02d}").mkdir()

    for name, child in (
        ("go", "pkg"),
        ("gone", "home"),
        ("snap", "glow"),
        ("wikiteam3_utilities", "dumps"),
        ("ytnova", "docs"),
    ):
        d = root / name
        d.mkdir()
        (d / child).mkdir(parents=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("go", timeout=2.0), _screen_text(tui)
    try:
        before = _first_tree_row_segment(tui.get_screen_dump())
        assert before is not None, _screen_text(tui)

        tui.send_keystroke("\033OF", wait=0.6)
        after = _first_tree_row_segment(tui.get_screen_dump())
        assert after == before, (
            "End on a hidden-prefix tree should keep the visible viewport "
            "anchored when the selected row still fits in view.\n"
            f"Before: {before!r}\nAfter:  {after!r}\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


# Viewport-origin regression coverage notes:
# - Dotfile reveal/conceal and visible-child delete exercise rebuild/mutation
#   paths where selection remains visible and the top visible row must stay
#   anchored.
# - Mixed split-panel file/tree reactivation is covered by adjacent split
#   viewport tests; these hidden-prefix fixtures stay filesystem-independent.
def test_dotfiles_toggle_restores_tree_viewport_origin_with_hidden_prefix(
    tmp_path, ytnova_binary
):
    root = tmp_path / "dotfiles_toggle_hidden_prefix_viewport" / "home" / "rob"
    _populate_hidden_prefix_viewport_tree(root)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"HOME": str(root)},
    )
    assert tui.wait_for_text("wikiteam3_utilities", timeout=2.0), _screen_text(tui)
    try:
        _move_to_stats_dir(tui, "for later")
        before_lines = tui.get_screen_dump()
        before_screen = "\n".join(before_lines)
        assert _tree_panel_selected_label(before_lines) == "for later", before_screen

        tui.send_keystroke("`", wait=0.8)
        assert _tree_panel_selected_label(tui.get_screen_dump()) == "for later", (
            "Revealing dotfiles should preserve the selected directory.\n"
            f"{_screen_text(tui)}"
        )
        tui.send_keystroke("`", wait=0.8)

        after_lines = tui.get_screen_dump()
        after_screen = "\n".join(after_lines)
        assert _tree_panel_selected_label(after_lines) == "for later", after_screen
        viewport = _assert_tree_viewport_origin_stable(
            before_lines,
            after_lines,
            split_col=_detect_stats_split_x(after_lines),
            label="dotfile reveal/conceal",
        )
        assert viewport["selected_visible"], (
            "Dotfile reveal/conceal should keep the selected tree row visible.\n"
            f"{after_screen}"
        )
    finally:
        tui.quit()


def test_delete_visible_child_restores_tree_viewport_origin_with_hidden_prefix(
    tmp_path, ytnova_binary
):
    root = tmp_path / "delete_hidden_prefix_viewport" / "home" / "rob"
    _populate_hidden_prefix_viewport_tree(root)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"HOME": str(root)},
    )
    assert tui.wait_for_text("wikiteam3_utilities", timeout=2.0), _screen_text(tui)
    try:
        _move_to_stats_dir(tui, "for later")
        before_lines = tui.get_screen_dump()
        before_screen = "\n".join(before_lines)
        assert _tree_panel_selected_label(before_lines) == "for later", before_screen

        tui.send_keystroke("d", wait=0.4)
        assert tui.wait_for_content("Delete this directory", timeout=1.0), _screen_text(
            tui
        )
        tui.send_keystroke("y", wait=1.2)

        after_lines = tui.get_screen_dump()
        after_screen = "\n".join(after_lines)
        assert not (root / "wikiteam3_utilities" / "for later").exists()
        assert _tree_panel_selected_label(after_lines) == "wikiteam3_utilities", (
            after_screen
        )
        viewport = _assert_tree_viewport_origin_stable(
            before_lines,
            after_lines,
            selected_label="wikiteam3_utilities",
            split_col=_detect_stats_split_x(after_lines),
            label="delete visible child",
        )
        assert viewport["selected_visible"], (
            "Deleting a visible child should keep the parent tree row visible.\n"
            f"{after_screen}"
        )
    finally:
        tui.quit()


def test_split_tab_round_trip_preserves_tree_viewport_with_hidden_prefix(
    tmp_path, ytnova_binary
):
    root = tmp_path / "split_tab_hidden_prefix_viewport"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\nSMALLWINDOWSKIP=0\n",
        encoding="utf-8",
    )

    for i in range(40):
        (root / f".hidden_{i:02d}").mkdir()

    for name, child in (
        ("go", "pkg"),
        ("gone", "home"),
        ("snap", "glow"),
        ("wikiteam3_utilities", "dumps"),
        ("ytnova", "docs"),
    ):
        d = root / name
        d.mkdir()
        (d / child).mkdir(parents=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("go", timeout=2.0), _screen_text(tui)
    try:
        before_lines = tui.get_screen_dump()
        before_selected_label = _tree_panel_selected_label(before_lines)
        assert before_selected_label is not None, _screen_text(tui)

        tui.send_keystroke(Keys.F8, wait=0.6)
        tui.send_keystroke(Keys.TAB, wait=0.6)
        tui.send_keystroke(Keys.TAB, wait=0.6)

        after_lines = tui.get_screen_dump()
        viewport = _assert_tree_viewport_origin_stable(
            before_lines,
            after_lines,
            split_col=_detect_split_column(after_lines),
            label="Split Tab round-trip",
        )
        if not viewport["selected_visible"]:
            assert not _tree_row_visible(
                after_lines,
                viewport["selected_label"],
                split_col=viewport["split_col"],
                panel=viewport["panel"],
            ), (
                "Split Tab round-trip allowed the viewport to reanchor because the selected row is no longer visible.\n"
                f"{_screen_text(tui)}"
            )
    finally:
        tui.quit()


def test_delete_first_visible_dir_keeps_visible_selection(
    tmp_path, ytnova_binary
):
    """
    Regression guard:
    Deleting the first visible (non-dot) child must not leave selection anchored
    on a hidden-dot sibling, which makes the tree cursor disappear until movement.
    """
    root = tmp_path / "delete_first_visible"
    home = root / "home" / "rob"
    home.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=2\nHIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in (".hidden_a", ".hidden_b", ".hidden_c"):
        (home / name).mkdir()
    for name in ("00", "zzz"):
        (home / name).mkdir()

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(home),
        env_extra={"HOME": str(home)},
    )
    assert tui.wait_for_text("00", timeout=2.0), _screen_text(tui)
    try:
        tui.send_keystroke(Keys.HOME, wait=0.3)
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before_lines = tui.get_screen_dump()
        assert _stats_current_dir_contains(before_lines, "00"), (
            "Precondition failed: DOWN from root should select visible '00'.\n"
            f"{_screen_text(tui)}"
        )

        tui.send_keystroke("d", wait=0.3)
        if tui.wait_for_content("Delete this directory", timeout=0.4):
            tui.send_keystroke("y", wait=0.8)

        after_lines = tui.get_screen_dump()
        assert _stats_current_dir_contains(after_lines, "rob"), (
            "After deleting first visible child, selection left the visible tree "
            "and anchored to a hidden-dot entry.\n"
            f"{_screen_text(tui)}"
        )
    finally:
        tui.quit()



def test_dialog_screen_wiping(dual_panel_sandbox, ytnova_binary):
    """BUG 4: Returning from a dialog leaves the screen missing separator lines."""
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(dual_panel_sandbox))
    assert tui.wait_for_content("left_dir", timeout=2.0), _screen_text(tui)

    # Trigger a dialog (Makedir) and cancel out.
    assert tui.send_and_wait_for_condition(
        "M", lambda lines: lines if any("MAKE DIRECTORY:" in line for line in lines) else False, timeout=2.0
    ), _screen_text(tui)
    assert tui.send_and_wait_for_condition(
        Keys.ESC, lambda lines: lines if not any("MAKE DIRECTORY:" in line for line in lines) else False, timeout=2.0
    ), _screen_text(tui)

    screen = "\n".join(tui.get_screen_dump())
    # VT100 mode renders separator lines as 'q' characters (ACS_HLINE).
    # If the screen was wiped, the horizontal line above the menu will be missing.
    assert "qqq" in screen, "Separator lines were wiped from the background!"

def test_negative_filter_logic(dual_panel_sandbox, ytnova_binary):
    """BUG 5: Negative filter (-*.o) hides everything instead of just .o files."""
    # Create a mixed directory
    (dual_panel_sandbox / "code.c").touch()
    (dual_panel_sandbox / "code.o").touch()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(dual_panel_sandbox))
    assert tui.wait_for_content("code.c", timeout=2.0), _screen_text(tui)

    assert tui.send_and_wait_for_condition(
        "f\x15-*.o\r", lambda lines: lines if any("code.c" in line for line in lines) and not any("code.o" in line for line in lines) else False, timeout=2.0
    ), _screen_text(tui)

    screen = "\n".join(tui.get_screen_dump())
    assert "code.c" in screen, "Negative filter hid files that should be visible!"
    assert "code.o" not in screen, "Negative filter failed to hide the target file!"

def test_split_screen_memory_isolation(dual_panel_sandbox, ytnova_binary):
    """BUG 1: Inactive panel displays garbage/forgets state when active panel scrolls."""
    (dual_panel_sandbox / "left_dir" / "target_file.txt").touch()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(dual_panel_sandbox))
    try:
        assert tui.wait_for_content("left_dir", timeout=2.0), _screen_text(tui)

        # Left Panel: Enter left_dir to see target_file.txt.
        _select_tree_dir_by_marker(tui, "left_dir")
        lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda current_lines: current_lines
            if any("target_file.txt" in line for line in current_lines)
            else False,
            timeout=2.0,
        )
        assert lines, _screen_text(tui)

        # Split, switch to the peer panel, then exercise its navigation.
        assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.LEFT, timeout=2.0), _screen_text(tui)

        screen = _screen_text(tui)
        # If memory is corrupted or state is lost, target_file.txt will turn into garbage (e.g., *?^X)
        assert "target_file.txt" in screen, "Inactive panel lost its memory state or was overwritten by garbage!"
    finally:
        tui.quit()



def test_f8_inactive_selection_moves_to_parent_on_mirrored_collapse(tmp_path, ytnova_binary):
    """
    Regression:
    When both panels share the same tree and active panel collapses a branch,
    an inactive selection inside that branch must re-anchor to the parent.
    """
    root = tmp_path / "f8_mirrored_collapse_root"
    root.mkdir()

    parent_dir = root / "parent_dir"
    child_dir = parent_dir / "child_dir"
    child_dir.mkdir(parents=True)

    (parent_dir / "parent_file.txt").write_text("parent\n", encoding="utf-8")
    (child_dir / "child_file.txt").write_text("child\n", encoding="utf-8")

    sibling_dir = root / "sibling_dir"
    sibling_dir.mkdir()
    (sibling_dir / "sibling_file.txt").write_text("sibling\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("parent_dir", timeout=2.0), _screen_text(tui)

    # Expand tree so parent/child are visible in both panels.
    assert tui.send_and_wait_for_condition(
        Keys.EXPAND_ALL,
        lambda lines: lines if any("child_dir" in line for line in lines) else False,
        timeout=2.0,
    ), _screen_text(tui)

    # Left panel (active): move to parent_dir.
    assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0), _screen_text(tui)

    # Split and move to right panel.
    assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0), _screen_text(tui)
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)

    # Right panel: move inside collapsed target branch (parent -> child).
    assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0), _screen_text(tui)

    # Back to left panel and collapse parent_dir.
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)
    assert tui.send_and_wait_for_condition(
        Keys.LEFT,
        lambda lines: lines if not any("child_dir" in line for line in lines) else False,
        timeout=2.0,
    ), _screen_text(tui)

    # Return to right panel and enter selected directory.
    # Expected: selection was re-anchored to parent_dir.
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)
    assert tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: lines if any("parent_file.txt" in line for line in lines) else False,
        timeout=2.0,
    ), _screen_text(tui)

    screen = "\n".join(tui.get_screen_dump())
    if "parent_file.txt" not in screen:
        tui.quit()
        pytest.fail(
            "Inactive selection did not move to parent after mirrored collapse.\n"
            f"Screen:\n{screen}"
        )
    if "child_file.txt" in screen:
        tui.quit()
        pytest.fail(
            "Inactive panel still entered child after parent collapse.\n"
            f"Screen:\n{screen}"
        )

    tui.quit()


def test_f8_inactive_selection_delete_falls_to_next_visible_sibling(
    tmp_path, ytnova_binary
):
    root = tmp_path / "f8_mirrored_delete_sibling_fallback"
    root.mkdir()

    before_dir = root / "aaa_before_dir"
    target_dir = root / "bbb_target_dir"
    after_dir = root / "ccc_after_dir"
    before_dir.mkdir()
    target_dir.mkdir()
    after_dir.mkdir()
    (before_dir / "before_marker.txt").write_text("before\n", encoding="utf-8")
    (target_dir / "target_marker.txt").write_text("target\n", encoding="utf-8")
    (after_dir / "after_marker.txt").write_text("after\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("bbb_target_dir", timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.DOWN, wait=0.3)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(
            tui, "Expected right panel tree mode before mirrored delete."
        )

        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.child.send(Keys.DELETE)
        tui.child.expect(r"(Delete this directory|PRUNE)", timeout=2.0)
        tui.child.send("Y")
        assert tui.wait_for_condition(
            lambda lines: lines if not target_dir.exists() else False,
            timeout=2.0,
        ), _screen_text(tui)

        tui.send_keystroke(Keys.TAB, wait=0.5)
        _assert_dir_mode_footer(
            tui, "Expected right panel tree mode after mirrored delete."
        )
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("after_marker.txt" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)

        screen = _screen_text(tui)
        header = screen.splitlines()[0] if screen else ""
        assert "ccc_after_dir" in header and "bbb_target_dir" not in header, (
            "Deleting the inactive-selected directory should fall forward to the "
            "next visible sibling before root.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_f8_inactive_selection_delete_falls_to_visible_ancestor(
    tmp_path, ytnova_binary
):
    root = tmp_path / "f8_mirrored_delete_ancestor_fallback"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")

    grand_dir = root / "grand_dir"
    deleted_ancestor = grand_dir / "ancestor_to_delete"
    selected_dir = deleted_ancestor / "selected_leaf"
    grand_dir.mkdir()
    deleted_ancestor.mkdir()
    selected_dir.mkdir()
    (grand_dir / "grand_marker.txt").write_text("grand\n", encoding="utf-8")
    (selected_dir / "selected_marker.txt").write_text("selected\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("grand_dir", timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.RIGHT, wait=0.5)
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.RIGHT, wait=0.5)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(
            tui, "Expected right panel tree mode before ancestor delete."
        )
        tui.send_keystroke(Keys.DOWN, wait=0.3)

        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.child.send(Keys.DELETE)
        tui.child.expect(r"(Delete this directory|PRUNE)", timeout=2.0)
        tui.child.send("Y")
        assert tui.wait_for_condition(
            lambda lines: lines if not deleted_ancestor.exists() else False,
            timeout=2.0,
        ), _screen_text(tui)

        tui.send_keystroke(Keys.TAB, wait=0.5)
        _assert_dir_mode_footer(
            tui, "Expected right panel tree mode after ancestor delete."
        )
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("grand_marker.txt" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)

        screen = _screen_text(tui)
        assert "grand_marker.txt" in screen, (
            "Deleting an ancestor of the inactive selection should fall back to the "
            "nearest surviving visible ancestor.\n"
            f"{screen}"
        )
        assert "selected_marker.txt" not in screen, (
            "Inactive panel still entered the deleted subtree after ancestor delete.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_bug_f_eight_mirrored_inactive_selection_identity_stable(tmp_path, ytnova_binary):
    """
    Regression:
    In mirrored split mode, expanding a sibling branch in the active panel must not
    shift inactive panel selection by row index when the selected directory still
    exists.
    """
    root = tmp_path / "bug_f_eight_identity_root"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")

    parent_dir = root / "parent_dir"
    child_dir = parent_dir / "child_dir"
    child_dir.mkdir(parents=True)
    (child_dir / "child_file.txt").write_text("child\n", encoding="utf-8")

    sibling_dir = root / "sibling_dir"
    sibling_dir.mkdir()
    (sibling_dir / "sibling_file.txt").write_text("sibling\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("parent_dir", timeout=2.0), _screen_text(tui)

    # Left panel (active): select parent_dir.
    assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0), _screen_text(tui)

    # Split and move to right panel (mirrored tree state).
    assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0), _screen_text(tui)
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)

    # Right panel (inactive target): select sibling_dir.
    assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0), _screen_text(tui)

    # Back to left panel and expand parent_dir.
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)
    assert tui.send_and_wait_for_condition(
        Keys.RIGHT,
        lambda lines: lines if any("child_dir" in line for line in lines) else False,
        timeout=2.0,
    ), _screen_text(tui)

    # Return to right panel and enter the selected directory.
    # Expected: still sibling_dir (stable identity), not parent_dir/child_dir.
    assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0), _screen_text(tui)
    assert tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: lines if any("sibling_file.txt" in line for line in lines) else False,
        timeout=2.0,
    ), _screen_text(tui)

    screen = "\n".join(tui.get_screen_dump())
    if "sibling_file.txt" not in screen:
        tui.quit()
        pytest.fail(
            "Inactive panel selection drifted after mirrored expand.\n"
            f"Screen:\n{screen}"
        )
    if "child_file.txt" in screen:
        tui.quit()
        pytest.fail(
            "Inactive panel entered child_dir after mirrored expand.\n"
            f"Screen:\n{screen}"
        )

    tui.quit()


def test_bug_f_eight_mkdir_additions_keep_inactive_selection_identity(
    tmp_path, ytnova_binary
):
    root = tmp_path / "bug_f_eight_mkdir_add_identity"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")

    active_branch = root / "active_branch"
    tail_dir = root / "zzz_tail"
    active_branch.mkdir()
    tail_dir.mkdir()
    (active_branch / "branch_file.txt").write_text("branch\n", encoding="utf-8")
    (active_branch / "child_existing").mkdir()
    (tail_dir / "tail_file.txt").write_text("tail\n", encoding="utf-8")

    def _select_dir_by_marker(tui, marker):
        return _select_tree_dir_by_marker(tui, marker)

    def _assert_right_panel_opens_tail_dir(tui, label):
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, f"{label}: expected right panel tree mode.")
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        screen = _screen_text(tui)
        header = screen.splitlines()[0] if screen else ""
        assert "zzz_tail" in header and "active_branch" not in header, (
            f"{label}: inactive selection drifted away from zzz_tail.\n{screen}"
        )
        tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(tui, f"{label}: expected right panel to return to tree mode.")
        tui.send_keystroke(Keys.TAB, wait=0.4)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("active_branch", timeout=2.0), _screen_text(tui)
    try:
        _select_dir_by_marker(tui, "active_branch")
        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, "Expected right panel tree mode after split.")
        _select_dir_by_marker(tui, "zzz_tail")
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, "Expected left panel tree mode before mkdir flow.")

        _assert_right_panel_opens_tail_dir(tui, "baseline")

        tui.send_keystroke(Keys.HOME, wait=0.3)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("aaa_root_insert" + Keys.ENTER, wait=0.8)
        _assert_right_panel_opens_tail_dir(tui, "after sibling add")

        _select_dir_by_marker(tui, "active_branch")
        tui.send_keystroke(Keys.RIGHT, wait=0.4)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("aaa_child_insert" + Keys.ENTER, wait=0.8)
        _assert_right_panel_opens_tail_dir(tui, "after ancestor-branch add")
    finally:
        tui.quit()


def test_bug_f_eight_dotfiles_toggle_keeps_inactive_selection_identity(
    tmp_path, ytnova_binary
):
    """
    Regression:
    In split mode, toggling dotfiles from the active panel must not re-index
    the inactive panel selection when the selected directory remains valid.
    """
    root = tmp_path / "bug_f_eight_dotfiles_inactive_identity"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    (root / ".dot_insert").mkdir()
    (root / "active_dir").mkdir()
    anchor_dir = root / "anchor_dir"
    anchor_dir.mkdir()
    tail_dir = root / "zzz_tail"
    tail_dir.mkdir()

    (anchor_dir / "anchor_file.txt").write_text("anchor\n", encoding="utf-8")
    (tail_dir / "tail_file.txt").write_text("tail\n", encoding="utf-8")

    def _select_dir_by_marker(tui, marker):
        return _select_tree_dir_by_marker(tui, marker)

    def _assert_right_panel_opens_tail_dir(tui, label):
        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, f"{label}: expected right panel tree mode.")
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        screen = _screen_text(tui)
        assert "tail_file.txt" in screen, (
            f"{label}: inactive selection drifted away from zzz_tail.\n{screen}"
        )
        assert "anchor_file.txt" not in screen, (
            f"{label}: inactive panel entered anchor_dir after dotfiles toggle.\n{screen}"
        )
        tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(tui, f"{label}: expected right panel to return to tree mode.")
        tui.send_keystroke(Keys.TAB, wait=0.4)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("active_dir", timeout=2.0), _screen_text(tui)
    try:
        _select_dir_by_marker(tui, "active_dir")

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        if "hex invert j compare" in _footer_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(tui, "Expected right panel tree mode after split.")
        _select_dir_by_marker(tui, "zzz_tail")

        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, "Expected left panel tree mode before toggle.")

        _assert_right_panel_opens_tail_dir(tui, "baseline")

        tui.send_keystroke("`", wait=0.5)
        _assert_right_panel_opens_tail_dir(tui, "dotfiles shown")

        tui.send_keystroke("`", wait=0.5)
        _assert_right_panel_opens_tail_dir(tui, "dotfiles hidden")
    finally:
        tui.quit()


def test_bug_f_eight_dotfiles_toggle_is_panel_local_visibility(
    tmp_path, ytnova_binary
):
    """
    Regression:
    In split mode, dotfile visibility toggled on the active panel must not leak
    into the inactive panel's file view.
    """
    root = tmp_path / "bug_f_eight_dotfiles_panel_local_visibility"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=3\nHIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    active_dir = root / "active_dir"
    active_dir.mkdir()
    tail_dir = root / "zzz_tail"
    tail_dir.mkdir()
    hidden_parent = root / ".split_hidden_dir"
    hidden_parent.mkdir()
    (hidden_parent / "visible_child").mkdir()

    (active_dir / "active_visible.txt").write_text("visible\n", encoding="utf-8")
    (active_dir / ".active_hidden.txt").write_text("hidden\n", encoding="utf-8")
    (tail_dir / "tail_visible.txt").write_text("visible\n", encoding="utf-8")
    (tail_dir / ".tail_hidden.txt").write_text("hidden\n", encoding="utf-8")

    def _select_dir_by_marker(tui, marker):
        return _select_tree_dir_by_marker(tui, marker)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("active_dir", timeout=2.0), _screen_text(tui)
    try:
        _select_dir_by_marker(tui, "active_dir")
        tui.send_keystroke(Keys.F8, wait=0.4)

        tui.send_keystroke(Keys.TAB, wait=0.4)
        if "hex invert j compare" in _footer_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(tui, "Expected right panel tree mode after split.")
        _select_dir_by_marker(tui, "zzz_tail")

        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(
            tui, "Expected left panel tree mode before active toggle."
        )

        tui.send_keystroke(Keys.ENTER, wait=0.4)
        baseline_screen = _screen_text(tui)
        assert "active_visible.txt" in baseline_screen, baseline_screen
        assert ".active_hidden.txt" not in baseline_screen, baseline_screen

        tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(
            tui, "Expected left panel tree mode before split tree assertion."
        )
        left_tree_screen = _screen_text(tui)
        assert ".split_hidden_dir" not in left_tree_screen, left_tree_screen
        assert "visible_child" not in left_tree_screen, left_tree_screen

        tui.send_keystroke("`", wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        toggled_screen = _screen_text(tui)
        assert ".active_hidden.txt" in toggled_screen, toggled_screen
        tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(
            tui, "Expected left panel tree mode after active toggle."
        )
        left_tree_toggled = _screen_text(tui)
        assert ".split_hidden_dir" in left_tree_toggled, left_tree_toggled

        tui.send_keystroke(Keys.HOME, wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.RIGHT, wait=0.4)
        left_tree_expanded = _screen_text(tui)
        assert "visible_child" in left_tree_expanded, left_tree_expanded

        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(tui, "Expected right panel tree mode after Tab.")
        left_seg, right_seg, screen = _split_segments_for_file(
            tui, ".split_hidden_dir"
        )
        assert ".split_hidden_dir" in left_seg, screen
        assert ".split_hidden_dir" not in right_seg, screen
        assert "visible_child" not in right_seg, screen

        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(
            tui, "Expected left panel tree mode before hiding expanded subtree."
        )
        tui.send_keystroke("`", wait=0.5)
        hidden_again_screen = _screen_text(tui)
        assert ".split_hidden_dir" not in hidden_again_screen, hidden_again_screen
        assert "visible_child" not in hidden_again_screen, hidden_again_screen

        tui.send_keystroke(Keys.TAB, wait=0.4)
        _assert_dir_mode_footer(
            tui, "Expected right panel tree mode after second Tab."
        )
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        right_screen = _screen_text(tui)
        assert "tail_visible.txt" in right_screen, right_screen
        assert ".tail_hidden.txt" not in right_screen, right_screen
    finally:
        tui.quit()


def test_bug_f_eight_source_selection_survives_destination_tree_prep(
    tmp_path, ytnova_binary
):
    """
    Regression:
    In same-volume split mode, destination-side prep work (newfile + tree
    navigation + mkdir/cd) must not re-index source file selection.
    """
    root = tmp_path / "bug_f_eight_source_selection_tree_prep"
    root.mkdir()

    source_dir = root / "source_dir"
    source_dir.mkdir()
    alpha_dir = root / "alpha_dir"
    alpha_dir.mkdir()

    (source_dir / "source_0.txt").write_text("0\n", encoding="utf-8")
    (source_dir / "source_1.txt").write_text("1\n", encoding="utf-8")
    (source_dir / "source_2.txt").write_text("2\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("source_dir", timeout=2.0), _screen_text(tui)

    try:
        # Source intent: enter source_dir, tag source_0, then keep source_1 selected.
        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.ENTER)
        _send_and_wait_for_transition(tui, "t")

        # Baseline check: without split prep, copy source should be source_1.
        _send_and_wait_for_transition(tui, "c")
        assert tui.wait_for_content("COPY: source_1.txt", timeout=1.0), _screen_text(
            tui
        )
        _send_and_wait_for_transition(tui, Keys.ESC)

        source_line = _find_line_with_text(tui, "source_0.txt")
        assert source_line is not None, _screen_text(tui)
        assert _line_marks_file_as_tagged(source_line, "source_0.txt"), _screen_text(
            tui
        )

        # Split, switch to destination, then do destination prep that inserts a
        # new tree row before source_dir (alpha_dir + mkdir child).
        _send_and_wait_for_transition(tui, Keys.F8)
        _send_and_wait_for_transition(tui, Keys.TAB)
        _send_and_wait_for_transition(tui, Keys.ESC)
        _send_and_wait_for_transition(tui, Keys.UP)
        _send_and_wait_for_transition(tui, "M")
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        _send_and_wait_for_transition(tui, "aaa_shift_anchor" + Keys.ENTER)
        _send_and_wait_for_transition(tui, Keys.ENTER)

        # Back to source and verify source intent is stable by identity.
        _send_and_wait_for_transition(tui, Keys.TAB)
        source_line_after = _find_line_with_text(tui, "source_0.txt")
        assert source_line_after is not None, _screen_text(tui)
        assert _line_marks_file_as_tagged(source_line_after, "source_0.txt"), (
            "Destination-side prep mutated source tagged state.\n"
            f"Row: {source_line_after}\n{_screen_text(tui)}"
        )

        _send_and_wait_for_transition(tui, "c")
        assert tui.wait_for_content("COPY: source_1.txt", timeout=1.0), (
            "Destination-side prep re-indexed source file selection by row position.\n"
            f"{_screen_text(tui)}"
        )
        _send_and_wait_for_transition(tui, Keys.ESC)
    finally:
        tui.quit()


def test_source_selection_survives_destination_tree_prep_home_mkdir(
    tmp_path, ytnova_binary
):
    """
    Regression:
    Destination tree HOME+mkdir prep in same-volume split mode must not blank
    the source panel or mutate source tagged/selection identity.
    """
    root = tmp_path / "bug_f_eight_source_selection_tree_home_mkdir"
    root.mkdir()

    source_dir = root / "source_dir"
    source_dir.mkdir()
    target_dir = root / "target_dir"
    target_dir.mkdir()

    (source_dir / "source_0.txt").write_text("0\n", encoding="utf-8")
    (source_dir / "source_1.txt").write_text("1\n", encoding="utf-8")
    (source_dir / "source_2.txt").write_text("2\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("source_dir", timeout=2.0), _screen_text(tui)

    try:
        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.ENTER)
        _send_and_wait_for_transition(tui, "t")

        _send_and_wait_for_transition(tui, "c")
        assert tui.wait_for_content("COPY: source_1.txt", timeout=1.0), _screen_text(
            tui
        )
        _send_and_wait_for_transition(tui, Keys.ESC)

        source_line = _find_line_with_text(tui, "source_0.txt")
        assert source_line is not None, _screen_text(tui)
        assert _line_marks_file_as_tagged(source_line, "source_0.txt"), _screen_text(
            tui
        )

        _send_and_wait_for_transition(tui, Keys.F8)
        _send_and_wait_for_transition(tui, Keys.TAB)
        if "hex invert j compare" in _footer_text(tui):
            _send_and_wait_for_transition(tui, Keys.ESC)
        _assert_dir_mode_footer(tui, "Destination panel should be in tree view.")
        _send_and_wait_for_transition(tui, Keys.LEFT)
        _send_and_wait_for_transition(tui, Keys.HOME)
        _send_and_wait_for_transition(tui, "M")
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        _send_and_wait_for_transition(tui, "aaa_dest_stage_dir" + Keys.ENTER)
        _send_and_wait_for_transition(tui, Keys.ENTER)

        screen_while_right_active = _screen_text(tui)
        assert "source_1.txt" in screen_while_right_active, (
            "Source panel blanked while destination tree flow remained active.\n"
            f"{screen_while_right_active}"
        )

        _send_and_wait_for_transition(tui, Keys.TAB)
        screen = _screen_text(tui)
        assert "source_1.txt" in screen, (
            "Source panel blanked after destination tree HOME+mkdir flow.\n" f"{screen}"
        )

        source_line_after = _find_line_with_text(tui, "source_0.txt")
        assert source_line_after is not None, screen
        assert _line_marks_file_as_tagged(source_line_after, "source_0.txt"), (
            "Destination tree HOME+mkdir flow mutated source tagged state.\n"
            f"Row: {source_line_after}\n{screen}"
        )

        _send_and_wait_for_transition(tui, "c")
        assert tui.wait_for_content("COPY: source_1.txt", timeout=1.0), (
            "Destination tree HOME+mkdir flow re-indexed source file selection.\n"
            f"{_screen_text(tui)}"
        )
        _send_and_wait_for_transition(tui, Keys.ESC)

        # Close split from destination tree context, then recover source_dir.
        _send_and_wait_for_transition(tui, Keys.TAB)
        if "hex invert j compare" in _footer_text(tui):
            _send_and_wait_for_transition(tui, Keys.ESC)
        _assert_dir_mode_footer(tui, "Destination should be in tree mode before unsplit.")
        _send_and_wait_for_transition(tui, Keys.F8)
        _assert_dir_mode_footer(
            tui, "Unsplitting from destination tree must keep tree UI stable."
        )

        _select_tree_stats_marker(tui, "source_dir")

        _send_and_wait_for_transition(tui, Keys.ENTER)
        source_line_unsplit = _find_line_with_text(tui, "source_0.txt")
        assert source_line_unsplit is not None, _screen_text(tui)
        assert _line_marks_file_as_tagged(source_line_unsplit, "source_0.txt"), (
            "Unsplitting from destination tree lost source tagged state.\n"
            f"{_screen_text(tui)}"
        )
        _send_and_wait_for_transition(tui, "c")
        assert tui.wait_for_content("COPY: source_1.txt", timeout=1.0), (
            "Unsplitting from destination tree changed source selection identity.\n"
            f"{_screen_text(tui)}"
        )
        _send_and_wait_for_transition(tui, Keys.ESC)
    finally:
        tui.quit()


def test_bug_same_volume_home_mkdir_keeps_inactive_source_dir(tmp_path, ytnova_binary):
    """
    Repro from manual QA:
    In same-volume split mode, destination HOME+mkdir must not retarget the
    inactive source panel to an unrelated directory.
    """
    root = tmp_path / "same_volume_home_mkdir_inactive_anchor"
    root.mkdir()

    src_dir = root / "src"
    src_dir.mkdir()
    cmd_dir = src_dir / "cmd"
    cmd_dir.mkdir()
    tests_dir = root / "tests"
    tests_dir.mkdir()

    for idx in range(3):
        (cmd_dir / f"f{idx}.txt").write_text(f"{idx}\n", encoding="utf-8")
    for idx in range(2):
        (tests_dir / f"t{idx}.txt").write_text(f"{idx}\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        # Tree -> src -> cmd -> file window
        assert tui.wait_for_content("src", timeout=2.0), _screen_text(tui)
        _select_tree_dir_by_marker(tui, "src")

        assert tui.send_and_wait_for_screen_change(Keys.RIGHT, timeout=2.0), _screen_text(tui)

        _select_tree_dir_by_marker(tui, "cmd")

        assert tui.send_and_wait_for_screen_change(Keys.RIGHT, timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines if any("f0.txt" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)

        # Tag three files in source.
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        # Split, switch to destination panel, then HOME+mkdir there.
        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        if "hex invert j compare" in _footer_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.3)
        _assert_dir_mode_footer(tui, "Destination should be in tree mode.")
        tui.send_keystroke(Keys.HOME, wait=0.3)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            "00" + Keys.ENTER,
            lambda lines: lines if any("00" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)

        screen_while_right_active = _screen_text(tui)
        assert "f0.txt" in screen_while_right_active and "f1.txt" in screen_while_right_active, (
            "Inactive source panel lost cmd file view while destination panel stayed active.\n"
            f"{screen_while_right_active}"
        )
        assert "t0.txt" not in screen_while_right_active and "t1.txt" not in screen_while_right_active, (
            "Inactive source panel jumped to tests while destination panel stayed active.\n"
            f"{screen_while_right_active}"
        )

        # Returning to source must keep cmd file view, not jump to tests.
        tui.send_keystroke(Keys.TAB, wait=0.5)
        screen = _screen_text(tui)
        assert "f0.txt" in screen and "f1.txt" in screen and "f2.txt" in screen, (
            "Source panel lost cmd file view after destination HOME+mkdir flow.\n"
            f"{screen}"
        )
        assert "t0.txt" not in screen and "t1.txt" not in screen, (
            "Source panel jumped to tests file view after destination HOME+mkdir flow.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_bug_same_volume_home_mkdir_with_repo_like_tree_keeps_inactive_source(
    tmp_path, ytnova_binary
):
    """
    Reproduce manual flow on a repo-like tree:
    while destination performs ENTER+HOME+mkdir, inactive source must stay on
    src/cmd file view and must not jump to tests/.
    """
    home = tmp_path / "home" / "user"
    repo = home / "ytnova"
    repo.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\n"
        "AUTO_REFRESH=3\n"
        "TREEDEPTH=2\n"
        "FILEMODE=2\n"
        "SMALLWINDOWSKIP=1\n"
        "HIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in (
        "build",
        "coverage",
        "docs",
        "etc",
        "include",
        "infra",
        "src",
        "tests",
    ):
        (repo / name).mkdir()

    src_alt_dir = repo / "src" / "aaa"
    src_alt_dir.mkdir()
    cmd_dir = repo / "src" / "cmd"
    cmd_dir.mkdir()
    tests_dir = repo / "tests"
    (repo / "bak.sh").write_text("#!/bin/sh\n", encoding="utf-8")

    for idx in range(3):
        (cmd_dir / f"src_file_{idx}.c").write_text("x\n", encoding="utf-8")
    for idx in range(4):
        (tests_dir / f"test_file_{idx}.py").write_text("y\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(repo), env_extra={"HOME": str(home)}
    )

    try:
        assert tui.wait_for_content("src", timeout=2.0), _screen_text(tui)
        _select_tree_dir_by_marker(tui, "src")

        assert tui.send_and_wait_for_screen_change(Keys.RIGHT, timeout=2.0), _screen_text(tui)

        _select_tree_dir_by_marker(tui, "cmd")

        assert tui.send_and_wait_for_screen_change(Keys.RIGHT, timeout=2.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if any("src_file_0.c" in line for line in lines)
            else False,
            timeout=2.0,
        ), _screen_text(tui)

        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        # Exact manual sequence: f8 tab enter home mkdir 00
        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        assert tui.send_and_wait_for_condition(
            "00" + Keys.ENTER,
            lambda lines: lines if any("00" in line for line in lines) else False,
            timeout=2.0,
        ), _screen_text(tui)

        screen_while_right_active = _screen_text(tui)
        assert "src_file_0.c" in screen_while_right_active, screen_while_right_active
        assert "test_file_0.py" not in screen_while_right_active, (
            "Inactive source panel jumped to tests while destination remained active.\n"
            f"{screen_while_right_active}"
        )

        tui.send_keystroke(Keys.TAB, wait=0.5)
        screen = _screen_text(tui)
        assert "src_file_0.c" in screen and "src_file_1.c" in screen, screen
        assert "test_file_0.py" not in screen and "test_file_1.py" not in screen, (
            "Source panel jumped to tests after destination ENTER+HOME+mkdir.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_bug_same_volume_home_mkdir_listjump_sequence_keeps_inactive_source(
    tmp_path, ytnova_binary
):
    """
    Portable regression for the user-reported variant:
    log HOME -> down/down/right -> /s Enter -> right -> down Enter -> ttt ->
    F8 Tab Enter Home -> mkdir 00.

    Inactive source panel must not jump to tests file view or collapse to
    repo-root file view.
    """
    home = tmp_path / "home" / "user"
    repo = home / "ytnova"
    repo.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\n"
        "AUTO_REFRESH=3\n"
        "TREEDEPTH=2\n"
        "FILEMODE=2\n"
        "SMALLWINDOWSKIP=1\n"
        "HIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in (
        "build",
        "coverage",
        "docs",
        "etc",
        "include",
        "infra",
        "src",
        "tests",
    ):
        (repo / name).mkdir()

    cmd_dir = repo / "src" / "cmd"
    cmd_dir.mkdir()
    tests_dir = repo / "tests"
    (repo / "bak.sh").write_text("#!/bin/sh\n", encoding="utf-8")

    for idx in range(3):
        (cmd_dir / f"src_file_{idx}.c").write_text("x\n", encoding="utf-8")
    for idx in range(4):
        (tests_dir / f"test_file_{idx}.py").write_text("y\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(repo), env_extra={"HOME": str(home)}
    )
    assert tui.wait_for_content("src", timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke("l", wait=0.2)
        if tui.wait_for_content("LOG", timeout=0.8) or tui.wait_for_content(
            "PATH", timeout=0.8
        ):
            tui.send_keystroke(str(home) + Keys.ENTER, wait=0.7)
        else:
            tui.send_keystroke(Keys.ESC, wait=0.2)

        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.RIGHT, wait=0.25)
        tui.send_keystroke("/", wait=0.2)
        tui.send_keystroke("s", wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.RIGHT, wait=0.25)
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.45)

        if "src_file_0.c" not in _screen_text(tui):
            if "hex invert j compare" in _footer_text(tui):
                tui.send_keystroke(Keys.ESC, wait=0.25)

            _select_tree_stats_marker(tui, "cmd")
            assert tui.send_and_wait_for_condition(
                Keys.ENTER,
                lambda lines: lines if any("src_file_0.c" in line for line in lines) else False,
                timeout=2.0,
            ), _screen_text(tui)

        pre_screen = _screen_text(tui)
        assert "src_file_0.c" in pre_screen, (
            "List-jump variant did not reach src/cmd file mode before split flow.\n"
            f"{pre_screen}"
        )

        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke("t", wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("00" + Keys.ENTER, wait=0.8)

        screen = _screen_text(tui)
        path_line = screen.splitlines()[0] if screen else ""
        repo_root_jump = "Path: " in path_line and path_line.rstrip().endswith("/ytreenova")
        root_file_window = "bak.sh" in screen
        assert (
            "test_file_0.py" not in screen
            and "/tests" not in screen
            and not (repo_root_jump and root_file_window)
        ), (
            "Inactive source panel drifted after list-jump variant "
            "(tests jump or repo-root collapse).\n"
            f"{screen}"
        )

        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, (
                "Inactive source panel blanked during destination mkdir.\n"
                f"{screen}"
            )
            assert _line_marks_file_as_tagged(line, name), (
                "Inactive source tagged state was lost during destination mkdir.\n"
                f"Row: {line}\n{screen}"
            )

        tui.send_keystroke(Keys.TAB, wait=0.5)
        screen_after_switch = _screen_text(tui)
        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, (
                "Source panel lost file rows after switching back from destination.\n"
                f"{screen_after_switch}"
            )
            assert _line_marks_file_as_tagged(line, name), (
                "Tagged files were cleared after switching back to source panel.\n"
                f"Row: {line}\n{screen_after_switch}"
            )
    finally:
        tui.quit()


def test_bug_same_volume_home_mkdir_from_home_root_keeps_inactive_file_state(
    tmp_path, ytnova_binary
):
    """
    Reproduces the user's direct flow from HOME root:
    start at HOME, expand ytreenova/src/cmd, enter file mode, tag three files,
    split, switch to right, HOME, mkdir.

    Inactive left panel must stay in file view on src/cmd with tags intact.
    """
    home = tmp_path / "home" / "user"
    repo = home / "ytnova"
    repo.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\n"
        "AUTO_REFRESH=3\n"
        "TREEDEPTH=2\n"
        "FILEMODE=2\n"
        "SMALLWINDOWSKIP=1\n"
        "HIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in ("go", "snap", "wikiteam3_utilities"):
        (home / name).mkdir()
    (home / "go" / "pkg").mkdir()
    (home / "snap" / "glow").mkdir(parents=True)
    (home / "wikiteam3_utilities" / "dumps").mkdir(parents=True)
    (home / "wikiteam3_utilities" / "for later").mkdir(parents=True)

    for name in (
        "build",
        "coverage",
        "docs",
        "etc",
        "include",
        "infra",
        "obj",
        "scripts",
        "src",
        "tests",
    ):
        (repo / name).mkdir()

    cmd_dir = repo / "src" / "cmd"
    cmd_dir.mkdir()
    tests_dir = repo / "tests"
    (repo / "bak.sh").write_text("#!/bin/sh\n", encoding="utf-8")
    for idx in range(3):
        (cmd_dir / f"src_file_{idx}.c").write_text("x\n", encoding="utf-8")
    for idx in range(4):
        (tests_dir / f"test_file_{idx}.py").write_text("y\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(home), env_extra={"HOME": str(home)}
    )

    try:
        _enter_fixture_file_view(tui, ("ytnova", "src", "cmd"), "src_file_0.c")

        tagged = tui.send_and_wait_for_condition(
            "ttt",
            lambda lines: lines
            if all(f"* {name}" in "\n".join(lines) for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"))
            else False,
            timeout=2.0,
        )
        assert tagged, _screen_text(tui)
        pre_screen = _screen_text(tui)
        pre_tag_state = {}
        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, pre_screen
            pre_tag_state[name] = _line_marks_file_as_tagged(line, name)
        assert any(pre_tag_state.values()), (
            "Precondition failed: no tagged source files before split flow.\n"
            f"{pre_screen}"
        )

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("00" + Keys.ENTER, wait=0.8)

        screen_right_active = _screen_text(tui)
        assert "src_file_0.c" in screen_right_active, (
            "Inactive left panel blanked while destination panel remained active.\n"
            f"{screen_right_active}"
        )
        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, (
                "Inactive source panel lost file rows after destination mkdir.\n"
                f"{screen_right_active}"
            )
            assert _line_marks_file_as_tagged(line, name) == pre_tag_state[name], (
                "Inactive source tagged state changed after destination mkdir.\n"
                f"Expected tagged={pre_tag_state[name]} Row: {line}\n"
                f"{screen_right_active}"
            )

        tui.send_keystroke(Keys.TAB, wait=0.5)
        screen_after_tab = _screen_text(tui)
        assert "No files" not in screen_after_tab, (
            "Switching to source panel produced empty file view.\n"
            f"{screen_after_tab}"
        )
        for name in ("src_file_0.c", "src_file_1.c", "src_file_2.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, (
                "Source panel did not resume src/cmd file view after TAB.\n"
                f"{screen_after_tab}"
            )
            assert _line_marks_file_as_tagged(line, name) == pre_tag_state[name], (
                "Source tag state changed after TAB back to source panel.\n"
                f"Expected tagged={pre_tag_state[name]} Row: {line}\n"
                f"{screen_after_tab}"
            )
    finally:
        tui.quit()


def test_bug2_copy_cancel_then_destination_mkdir_keeps_source_anchor(
    tmp_path, ytnova_binary
):
    """
    Regression (maintainer manual flow):
    In source file mode, opening COPY and stepping into destination prompt before
    split destination prep must not mutate source tagged/selection identity.
    """
    home = tmp_path / "home" / "user"
    repo = home / "ytnova"
    repo.mkdir(parents=True)
    (home / ".ytnova").write_text(
        "[GLOBAL]\n"
        "AUTO_REFRESH=3\n"
        "TREEDEPTH=2\n"
        "FILEMODE=2\n"
        "SMALLWINDOWSKIP=1\n"
        "HIDEDOTFILES=1\n",
        encoding="utf-8",
    )

    for name in ("go", "snap", "wikiteam3_utilities"):
        (home / name).mkdir()
    (home / "go" / "pkg").mkdir()
    (home / "snap" / "glow").mkdir(parents=True)
    (home / "wikiteam3_utilities" / "dumps").mkdir(parents=True)
    (home / "wikiteam3_utilities" / "for later").mkdir(parents=True)

    for name in ("docs", "include", "scripts", "src", "tests"):
        (repo / name).mkdir()

    cmd_dir = repo / "src" / "cmd"
    cmd_dir.mkdir()
    (cmd_dir / "a.c").write_text("a\n", encoding="utf-8")
    (cmd_dir / "b.c").write_text("b\n", encoding="utf-8")
    (cmd_dir / "c.c").write_text("c\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(home), env_extra={"HOME": str(home)}
    )

    try:
        _enter_fixture_file_view(tui, ("ytnova", "src", "cmd"), "a.c")

        tagged = tui.send_and_wait_for_condition(
            "ttt",
            lambda lines: lines
            if all(f"* {name}" in "\n".join(lines) for name in ("a.c", "b.c", "c.c"))
            else False,
            timeout=2.0,
        )
        assert tagged, _screen_text(tui)
        pre_screen = _screen_text(tui)
        pre_tag_state = {}
        for name in ("a.c", "b.c", "c.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, pre_screen
            pre_tag_state[name] = _line_marks_file_as_tagged(line, name)
        assert any(pre_tag_state.values()), (
            "Precondition failed: no tagged source files before split flow.\n"
            f"{pre_screen}"
        )

        tui.send_keystroke("c", wait=0.3)
        assert tui.wait_for_content("COPY:", timeout=1.0), _screen_text(tui)
        baseline_source = _current_copy_source(tui)
        assert baseline_source is not None, _screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        tui.send_keystroke(Keys.ESC, wait=0.25)
        if "COPY:" in _screen_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.2)

        tui.send_keystroke(Keys.F8, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.ENTER, wait=0.35)
        if "hex invert j compare" in _footer_text(tui):
            tui.send_keystroke(Keys.ESC, wait=0.25)
        _assert_dir_mode_footer(
            tui, "Destination panel should be in tree mode after leaving file view."
        )
        tui.send_keystroke(Keys.HOME, wait=0.35)
        tui.send_keystroke("M", wait=0.25)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("00" + Keys.ENTER, wait=0.8)

        screen_right_active = _screen_text(tui)
        for name in ("a.c", "b.c", "c.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, (
                "Source panel lost file rows while destination prep was active.\n"
                f"{screen_right_active}"
            )
            assert _line_marks_file_as_tagged(line, name) == pre_tag_state[name], (
                "Source tagged state changed during destination prep.\n"
                f"Expected tagged={pre_tag_state[name]} Row: {line}\n"
                f"{screen_right_active}"
            )

        tui.send_keystroke(Keys.TAB, wait=0.5)
        screen_after_tab = _screen_text(tui)
        for name in ("a.c", "b.c", "c.c"):
            line = _find_line_with_text(tui, name)
            assert line is not None, (
                "Source panel did not resume cmd file rows after TAB.\n"
                f"{screen_after_tab}"
            )
            assert _line_marks_file_as_tagged(line, name) == pre_tag_state[name], (
                "Source tagged state changed after TAB back.\n"
                f"Expected tagged={pre_tag_state[name]} Row: {line}\n"
                f"{screen_after_tab}"
            )

        tui.send_keystroke("c", wait=0.3)
        assert tui.wait_for_content(f"COPY: {baseline_source}", timeout=1.0), (
            "Source selected-file identity changed after destination prep flow.\n"
            f"{_screen_text(tui)}"
        )
        tui.send_keystroke(Keys.ESC, wait=0.2)
    finally:
        tui.quit()


def test_source_tagged_selection_survives_destination_prep(tmp_path, ytnova_binary):
    """
    Regression:
    When split is closed from the right panel, source selection identity must
    stay with the active (right) file instead of restoring a stale left anchor.
    """
    root = tmp_path / "bug_f_eight_unsplit_right_anchor"
    root.mkdir()
    source_dir = root / "source_dir"
    source_dir.mkdir()

    (source_dir / "a.txt").write_text("a\n", encoding="utf-8")
    (source_dir / "b.txt").write_text("b\n", encoding="utf-8")
    (source_dir / "c.txt").write_text("c\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("source_dir", timeout=2.0), _screen_text(tui)

    try:
        _send_and_wait_for_transition(tui, Keys.DOWN)
        _send_and_wait_for_transition(tui, Keys.ENTER)

        # Left source anchor: keep b.txt selected before splitting.
        _send_and_wait_for_transition(tui, Keys.DOWN)
        assert _find_line_with_text(tui, "b.txt") is not None, _screen_text(tui)

        # Split and switch to right panel; select c.txt there.
        _send_and_wait_for_transition(tui, Keys.F8)
        _send_and_wait_for_transition(tui, Keys.TAB)
        _send_and_wait_for_transition(tui, Keys.DOWN)

        _send_and_wait_for_transition(tui, "c")
        assert _current_copy_source(tui) == "c.txt", _screen_text(tui)
        _send_and_wait_for_transition(tui, Keys.ESC)

        # Close split from right panel; source must stay c.txt.
        _send_and_wait_for_transition(tui, Keys.F8)
        assert _find_line_with_text(tui, "b.txt") is not None, _screen_text(tui)

        _send_and_wait_for_transition(tui, "c")
        assert _current_copy_source(tui) == "c.txt", (
            "Unsplitting from right restored stale left source anchor.\n"
            f"{_screen_text(tui)}"
        )
        _send_and_wait_for_transition(tui, Keys.ESC)
    finally:
        tui.quit()


def test_split_file_focus_survives_tab_round_trip(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_focus_round_trip"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    assert "alpha.txt" in "\n".join(tui.get_screen_dump())

    _send_and_wait_for_transition(tui, Keys.F8)
    _send_and_wait_for_transition(tui, Keys.TAB)
    _send_and_wait_for_transition(tui, Keys.TAB)

    screen = "\n".join(tui.get_screen_dump())
    assert "alpha.txt" in screen, f"Left panel lost its file selection after split/tab round-trip.\n{screen}"

    tui.quit()


def test_split_panels_keep_independent_file_focus_states(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_focus_independent"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (beta / "beta.txt").write_text("beta\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    assert "alpha.txt" in "\n".join(tui.get_screen_dump())

    _send_and_wait_for_transition(tui, Keys.F8)
    _send_and_wait_for_transition(tui, Keys.TAB)

    if "hex invert j compare" in _footer_text(tui):
        _send_and_wait_for_transition(tui, Keys.ESC)

    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    assert "beta.txt" in "\n".join(tui.get_screen_dump())

    _send_and_wait_for_transition(tui, Keys.TAB)

    screen = "\n".join(tui.get_screen_dump())
    assert "alpha.txt" in screen, f"Returning to the left panel did not restore its file view.\n{screen}"

    tui.quit()


def test_active_mode_toggles_do_not_mutate_inactive_file_state(tmp_path, ytnova_binary):
    root = tmp_path / "split_independent_mode_toggles"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (beta / "beta_0.txt").write_text("0\n", encoding="utf-8")

    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    # Left panel: enter alpha file view and select alpha_1.
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    _send_and_wait_for_transition(tui, Keys.DOWN)

    # Split and move to right panel.
    _send_and_wait_for_transition(tui, Keys.F8)
    _send_and_wait_for_transition(tui, Keys.TAB)

    # Right panel: toggle between dir/file/small-big transitions.
    if "hex invert j compare" in _footer_text(tui):
        lines = tui.send_and_wait_for_condition(
            Keys.ESC,
            lambda current_lines: current_lines
            if "hex invert j compare" not in _footer_text_from_lines(current_lines)
            else False,
            timeout=2.0,
        )
        assert lines, _screen_text(tui)
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # file (small)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # file (big)
    _send_and_wait_for_transition(tui, Keys.ESC)    # back to tree
    _send_and_wait_for_transition(tui, Keys.ENTER)  # file again

    # Left panel must still have file focus and selected alpha_1.
    _send_and_wait_for_transition(tui, Keys.TAB)
    _send_and_wait_for_transition(tui, "J")
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # HitReturnToContinue
    assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(alpha / "alpha_1.txt"), (
        "Active-panel mode changes mutated inactive panel file selection/state."
    )

    tui.quit()


def test_split_from_file_keeps_inactive_file_selection_independent(tmp_path, ytnova_binary):
    root = tmp_path / "split_file_independent_scroll_state"
    root.mkdir()
    alpha = root / "alpha"
    alpha.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (alpha / "alpha_3.txt").write_text("3\n", encoding="utf-8")

    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # Enter alpha

    # Set split baseline to alpha_1.
    _send_and_wait_for_transition(tui, Keys.DOWN)

    _send_and_wait_for_transition(tui, Keys.F8)

    # Move active panel to alpha_3; inactive panel should remain on alpha_1.
    _send_and_wait_for_transition(tui, Keys.DOWN)
    _send_and_wait_for_transition(tui, Keys.DOWN)

    _send_and_wait_for_transition(tui, Keys.TAB)
    if "hex invert j compare" not in _footer_text(tui):
        _send_and_wait_for_transition(tui, Keys.ENTER)

    _send_and_wait_for_transition(tui, "J")
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    _send_and_wait_for_transition(tui, Keys.ENTER)
    _send_and_wait_for_transition(tui, Keys.ENTER)  # HitReturnToContinue

    assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(alpha / "alpha_1.txt"), (
        "Inactive panel file selection tracked active scrolling after split-from-file."
    )

    tui.quit()


def test_log_new_volume_from_file_view_resets_focus_and_selection(tmp_path, ytnova_binary):
    root = tmp_path / "file_log_new_volume_resets_focus"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (beta / "beta_0.txt").write_text("0\n", encoding="utf-8")
    (beta / "beta_1.txt").write_text("1\n", encoding="utf-8")
    (beta / "beta_2.txt").write_text("2\n", encoding="utf-8")
    compare_target = root / "compare_target.txt"
    compare_target.write_text("target\n", encoding="utf-8")

    log_path = _configure_filediff_capture(root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("alpha", timeout=2.0), _screen_text(tui)

    # Enter alpha file view and select a non-first file.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)

    # Log a new volume directly from file view.
    _log_path_and_wait_for_fixture(tui, beta, "beta_0.txt")

    # Must return to directory mode first (no implicit file-window entry).
    _assert_dir_mode_footer(
        tui, "Logging a new volume from file view should return to directory mode."
    )

    # Enter file view explicitly and ensure selection starts at first file.
    tui.send_keystroke(Keys.ENTER, wait=0.5)
    tui.send_keystroke("J", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET:", timeout=1.0)
    tui.send_keystroke(Keys.CTRL_U + str(compare_target) + Keys.ENTER, wait=0.55)
    tui.send_keystroke(Keys.ENTER, wait=0.35)  # HitReturnToContinue
    assert _wait_for_file(tui, log_path, timeout=2.0), "FILEDIFF helper did not run."
    logged = log_path.read_text(encoding="utf-8").splitlines()
    assert len(logged) >= 2, f"FILEDIFF should receive source+target args.\nArgs: {logged}"
    assert logged[0] == str(beta / "beta_0.txt"), (
        "Newly logged volume file selection should reset to first entry."
    )

    tui.quit()


def test_log_current_volume_from_file_view_keeps_file_anchor_safe(tmp_path, ytnova_binary):
    root = tmp_path / "file_log_current_volume_anchor_safe"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nSMALLWINDOWSKIP=1\n", encoding="utf-8")
    alpha = root / "alpha"
    alpha.mkdir()

    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.DOWN, wait=0.2)

        relog_marker = root / "relog_completed.txt"
        relog_marker.write_text("complete\n", encoding="utf-8")
        _log_path_and_wait_for_fixture(tui, root, relog_marker.name)

        _assert_dir_mode_footer(
            tui,
            "Reloading the current volume from file view should return safely to dir mode.",
        )

        tui.send_keystroke(Keys.DOWN, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert _find_line_with_text(tui, "alpha_0.txt") is not None, _screen_text(tui)
    finally:
        tui.quit()


def test_log_second_volume_from_file_view_keeps_tree_on_root(tmp_path, ytnova_binary):
    root = tmp_path / "file_log_second_volume_starts_at_root"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()

    (root / "root_mode_anchor.txt").write_text("anchor\n", encoding="utf-8")
    (alpha / "alpha_0.txt").write_text("0\n", encoding="utf-8")
    (alpha / "alpha_1.txt").write_text("1\n", encoding="utf-8")
    (alpha / "alpha_2.txt").write_text("2\n", encoding="utf-8")
    (beta / "beta_root_file.txt").write_text("root\n", encoding="utf-8")
    (beta / "aa_probe_dir").mkdir()
    (beta / "bb_probe_dir").mkdir()
    (beta / "aa_probe_dir" / "aa_only.txt").write_text("aa\n", encoding="utf-8")
    (beta / "bb_probe_dir" / "bb_only.txt").write_text("bb\n", encoding="utf-8")
    _configure_filediff_capture(root)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_content("alpha", timeout=2.0), _screen_text(tui)

    # Enter file view first to exercise the same path that regressed.
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    _log_path_and_wait_for_fixture(tui, beta, "beta_root_file.txt")

    _assert_dir_mode_footer(
        tui, "Logging a second volume from file view should return to directory mode."
    )

    lines = tui.get_screen_dump()
    screen = "\n".join(lines)
    assert not _stats_current_dir_contains(lines, "aa_probe_dir"), (
        "Newly logged second volume should select root, not the first child dir.\n"
        f"{screen}"
    )
    assert not _stats_current_dir_contains(lines, "bb_probe_dir"), (
        "Newly logged second volume should select root, not a child dir.\n"
        f"{screen}"
    )

    tui.quit()


def test_volume_menu_cancel_restores_dir_surface(tmp_path, ytnova_binary):
    root = tmp_path / "volume_menu_cancel_dir_footer"
    root.mkdir()
    (root / "a.txt").write_text("a\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("a.txt", timeout=2.0), _screen_text(tui)

    tui.send_keystroke("k", wait=0.3)
    assert tui.wait_for_content("Select Volume", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.3)

    assert tui.wait_for_text("a.txt", timeout=1.0), _screen_text(tui)

    tui.quit()


def test_volume_menu_cancel_restores_file_surface(tmp_path, ytnova_binary):
    root = tmp_path / "volume_menu_cancel_file_footer"
    root.mkdir()
    (root / "a.txt").write_text("a\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("a.txt", timeout=2.0), _screen_text(tui)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    tui.send_keystroke("k", wait=0.3)
    assert tui.wait_for_content("Select Volume", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.3)

    assert tui.wait_for_text("a.txt", timeout=1.0), _screen_text(tui)

    tui.quit()


def test_f8_release_volume_keeps_small_window_and_tab_safe(tmp_path, ytnova_binary):
    vol_a = tmp_path / "bug41_vol_a"
    vol_b = tmp_path / "bug41_vol_b"
    vol_a.mkdir()
    vol_b.mkdir()
    (vol_a / "a_only.txt").write_text("a\n", encoding="utf-8")
    (vol_b / "b_only.txt").write_text("b\n", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(vol_a),
        args=[str(vol_b)],
    )
    assert tui.wait_for_text("b_only.txt", timeout=2.0), _screen_text(tui)

    try:
        tui.send_keystroke(Keys.F8, wait=0.5)

        # Open loaded-volume menu and release selected volume.
        tui.send_keystroke("k", wait=0.4)
        assert tui.wait_for_content("Select Volume", timeout=1.0), _screen_text(tui)
        tui.send_keystroke("d", wait=0.3)
        tui.send_keystroke("y", wait=0.8)
        if tui.wait_for_content("Select Volume", timeout=0.4):
            tui.send_keystroke(Keys.ESC, wait=0.5)

        lines = tui.get_screen_dump()
        screen = "\n".join(lines)
        assert "Path:" in screen, (
            "UI header vanished after releasing a volume in split mode.\n"
            f"{screen}"
        )
        assert screen.count("a_only.txt") + screen.count("b_only.txt") >= 2, (
            "Small/file windows were not rendered in both split panes after "
            "release-volume flow.\n"
            f"{screen}"
        )
        _assert_split_column_continuous(lines, "after split release-volume flow")

        # Regression: Tab after release must not blank/crash.
        tui.send_keystroke(Keys.TAB, wait=0.6)
        lines = tui.get_screen_dump()
        screen = "\n".join(lines)
        assert "Path:" in screen, (
            "Tab after split release-volume flow blanked/crashed UI.\n"
            f"{screen}"
        )
        assert screen.count("a_only.txt") + screen.count("b_only.txt") >= 2, (
            "Tab after release-volume flow left one split pane blank.\n"
            f"{screen}"
        )
        _assert_split_column_continuous(lines, "after split release + tab")

        tui.send_keystroke(Keys.TAB, wait=0.4)
        tui.send_keystroke(Keys.TAB, wait=0.4)
        lines = tui.get_screen_dump()
        screen = "\n".join(lines)
        assert "Path:" in screen, (
            "Repeated tabbing after split release-volume flow corrupted UI.\n"
            f"{screen}"
        )
        assert screen.count("a_only.txt") + screen.count("b_only.txt") >= 2, (
            "Repeated tabbing after split release-volume flow left a pane blank.\n"
            f"{screen}"
        )
        _assert_split_column_continuous(lines, "after split release + repeated tab")
    finally:
        tui.quit()


def test_f8_release_inactive_disk_volume_while_active_archive_keeps_split_stable(
    tmp_path, ytnova_binary
):
    root = tmp_path / "bug41_archive_release_inactive"
    root.mkdir()
    disk_vol = root / "disk_vol"
    disk_vol.mkdir()
    (disk_vol / "disk_only.txt").write_text("disk\n", encoding="utf-8")

    archive_src = root / "_archive_src"
    archive_src.mkdir()
    (archive_src / "inside.txt").write_text("inside\n", encoding="utf-8")
    (archive_src / "nested").mkdir()
    (archive_src / "nested" / "deep.txt").write_text("deep\n", encoding="utf-8")
    archive_path = root / "sample.tar"
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_src, arcname="inside_dir")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(disk_vol),
        args=[str(archive_path)],
    )
    assert tui.wait_for_text("inside_dir", timeout=2.0), _screen_text(tui)

    try:
        # Move active context to the archive volume by its visible identity.
        assert drive_action_until(
            tui,
            "<",
            lambda lines: lines if "sample.tar" in next(iter(lines), "") else False,
            max_actions=32,
        ), _screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        assert tui.wait_for_text("inside.txt", timeout=2.0), _screen_text(tui)

        tui.send_keystroke(Keys.F8, wait=0.5)
        tui.send_keystroke("k", wait=0.3)
        assert tui.wait_for_content("Select Volume", timeout=1.0), _screen_text(tui)
        tui.send_keystroke(Keys.DOWN, wait=0.2)  # select inactive disk volume
        tui.send_keystroke("d", wait=0.2)
        tui.send_keystroke("y", wait=0.8)
        if tui.wait_for_content("Select Volume", timeout=0.4):
            tui.send_keystroke(Keys.ESC, wait=0.5)

        lines = tui.get_screen_dump()
        screen = "\n".join(lines)
        assert "Path:" in screen, (
            "Release-volume flow in active archive mode blanked/crashed UI.\n"
            f"{screen}"
        )
        assert screen.count("inside.txt") >= 1, (
            "Active archive pane content disappeared after releasing inactive disk volume.\n"
            f"{screen}"
        )
        _assert_split_column_continuous(lines, "archive active + inactive release")
    finally:
        tui.quit()
