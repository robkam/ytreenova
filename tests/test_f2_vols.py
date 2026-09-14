import pytest
import os
import re
from helpers_ui import _panel_path_header, _path_label
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

YTNOVA_BIN = os.path.abspath("./build/ytnova")

def get_screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def _destination_prompt_line(tui):
    for line in tui.get_screen_dump():
        if "To Directory:" in line:
            return line
    return ""


def _total_items_count(tui):
    for line in tui.get_screen_dump():
        match = re.search(r"Tot:\s*([0-9,]+)", line)
        if match:
            return int(match.group(1).replace(",", ""))
    return None


def _screen_contains_text(tui, target):
    return any(target in line for line in tui.get_screen_dump())


def _wait_for_text_visibility(tui, target, *, present, timeout=1.5):
    lines = tui.wait_for_condition(
        lambda screen: screen
        if any(target in line for line in screen) == present
        else False,
        timeout=timeout,
        poll_interval=0.05,
    )
    assert lines, get_screen_text(tui)
    return lines


def _send_and_wait_for_text_visibility(tui, keys, target, *, present, timeout=1.5):
    lines = tui.send_and_wait_for_condition(
        keys,
        lambda screen: screen
        if any(target in line for line in screen) == present
        else False,
        timeout=timeout,
        poll_interval=0.05,
    )
    assert lines, get_screen_text(tui)
    return lines


def _row_style_spans(tui, row, span_limit=120):
    spans = []
    buffer = tui.screen.buffer
    cell_index = 0
    while cell_index < span_limit:
        ch = buffer[row][cell_index]
        style = (ch.reverse, ch.fg, ch.bg, ch.bold, ch.underscore)
        start = cell_index
        while (
            cell_index < span_limit
            and (
                buffer[row][cell_index].reverse,
                buffer[row][cell_index].fg,
                buffer[row][cell_index].bg,
                buffer[row][cell_index].bold,
                buffer[row][cell_index].underscore,
            )
            == style
        ):
            cell_index += 1
        spans.append((start, cell_index - 1, style, "".join(buffer[row][i].data for i in range(start, cell_index))))
    return spans


def _span_containing(tui, needle, span_limit=120):
    for row, line in enumerate(tui.get_screen_dump()):
        if needle not in line:
            continue
        for start, end, style, text in _row_style_spans(tui, row, span_limit=span_limit):
            if needle in text:
                return row, start, end, style, text
    raise AssertionError(f"Could not find styled span containing {needle!r}.\n{get_screen_text(tui)}")


def _has_exact_span_text(tui, target, span_limit=120):
    for row, line in enumerate(tui.get_screen_dump()):
        if target not in line:
            continue
        for _, _, _, text in _row_style_spans(tui, row, span_limit=span_limit):
            if text == target:
                return True
    return False


def _left_exact_span_row(tui, target, span_limit=120, max_x=70):
    for row, line in enumerate(tui.get_screen_dump()):
        if target not in line:
            continue
        for start, _, _, text in _row_style_spans(tui, row, span_limit=span_limit):
            if text == target and start < max_x:
                return row
    raise AssertionError(
        f"Could not find left-panel exact span {target!r}.\n{get_screen_text(tui)}"
    )


def _left_row_containing(tui, target, max_x=70):
    for row, line in enumerate(tui.get_screen_dump()):
        col = line.find(target)
        if 0 <= col < max_x:
            return row
    raise AssertionError(
        f"Could not find left-panel row containing {target!r}.\n{get_screen_text(tui)}"
    )


def _move_selection_to_exact_span(tui, target, max_steps=12):
    steps = 0
    while not _has_exact_span_text(tui, target):
        if steps >= max_steps:
            raise AssertionError(
                f"Could not move F2 selection to {target!r}.\n{get_screen_text(tui)}"
            )
        if not _screen_contains_text(tui, target):
            tui.send_keystroke(Keys.RIGHT, wait=0.6)
        else:
            tui.send_keystroke(Keys.DOWN, wait=0.3)
        steps += 1

def test_f2_log_and_cycle_volumes(tmp_path):
    """F2 can log a destination volume and cycle away from and back to it."""
    dir_a = tmp_path / "volume_a"
    dir_a.mkdir()
    (dir_a / "file_a.txt").touch()

    dir_b = tmp_path / "volume_b"
    dir_b.mkdir()
    (dir_b / "file_b.txt").touch()
    (tmp_path / "seed.txt").touch()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(tmp_path))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_condition(
            lambda lines: lines
            if any("file view" in line.lower() for line in lines)
            else False,
            timeout=1.0,
            poll_interval=0.05,
        ), get_screen_text(tui)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        assert tui.wait_for_content("COPY:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_content("To Directory:", timeout=1.0), get_screen_text(tui)
        assert tui.send_and_wait_for_screen_change(Keys.F2)

        tui.send_keystroke(Keys.LOG, wait=0.3)
        assert tui.wait_for_content("Log Path:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.CTRL_U + str(dir_a) + Keys.ENTER, wait=0.3)
        assert tui.wait_for_content("volume_a", timeout=1.0), get_screen_text(tui)

        tui.send_keystroke("<", wait=0.3)
        assert tui.wait_for_condition(
            lambda lines: lines
            if any("seed.txt" in line for line in lines)
            else False,
            timeout=2.0,
            poll_interval=0.05,
        ), get_screen_text(tui)
        tui.send_keystroke(">", wait=0.3)
        assert tui.wait_for_content("volume_a", timeout=2.0), get_screen_text(tui)

    finally:
        tui.quit()


def test_f2_right_expands_then_enters_and_left_collapses_or_returns_parent(tmp_path):
    root = tmp_path / "f2_tree_nav"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / "alpha" / "child" / "grand").mkdir(parents=True)
    (root / "beta").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.3)

        tui.send_keystroke(Keys.F2, wait=0.8)
        _move_selection_to_exact_span(tui, "alpha")
        tui.send_keystroke(Keys.RIGHT, wait=0.6)  # expand alpha
        expanded = get_screen_text(tui)
        assert "child" in expanded, (
            "RIGHT in the F2 tree should expand a collapsed directory.\n"
            f"{expanded}"
        )
        assert "grand" not in expanded, (
            "RIGHT in the F2 tree should respect TREEDEPTH instead of "
            "expanding all descendants at once.\n"
            f"{expanded}"
        )

        tui.send_keystroke(Keys.LEFT, wait=0.6)   # collapse alpha
        collapsed = get_screen_text(tui)
        assert "child" not in collapsed, (
            "LEFT in the F2 tree should collapse an expanded directory.\n"
            f"{collapsed}"
        )

        tui.send_keystroke(Keys.RIGHT, wait=0.6)  # expand alpha again
        tui.send_keystroke(Keys.RIGHT, wait=0.6)  # enter child
        tui.send_keystroke(Keys.LEFT, wait=0.6)   # collapse child
        child_collapsed = get_screen_text(tui)
        assert "grand" not in child_collapsed, (
            "LEFT on an expanded child in the F2 tree should collapse that child first.\n"
            f"{child_collapsed}"
        )

        tui.send_keystroke(Keys.LEFT, wait=0.6)   # parent
        tui.send_keystroke(Keys.ENTER, wait=0.5)

        parent_prompt = _destination_prompt_line(tui)
        assert str(root / "alpha") in parent_prompt, (
            "LEFT in the F2 tree should go back to the parent directory.\n"
            f"{get_screen_text(tui)}"
        )
        assert str(root / "alpha" / "child") not in parent_prompt, (
            "LEFT in the F2 tree should not leave the child path selected.\n"
            f"{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_picker_inherits_and_toggles_dotfile_visibility(tmp_path):
    root = tmp_path / "f2_dotfile_visibility"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / "visible_dest").mkdir()
    (root / ".hidden_dest").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        if not _screen_contains_text(tui, ".hidden_dest"):
            _send_and_wait_for_text_visibility(
                tui, "`", ".hidden_dest", present=True, timeout=1.5
            )

        tui.send_keystroke(Keys.ENTER, wait=0.3)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        assert tui.wait_for_content("COPY:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_content("To Directory:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.F2, wait=0.3)
        assert tui.wait_for_content("cycle", timeout=1.0), get_screen_text(tui)

        f2_commands = next(
            line for line in tui.get_screen_dump() if "cycle" in line and "Log" in line
        )
        assert "dotfiles" in f2_commands and "`" in f2_commands, f2_commands
        assert _screen_contains_text(tui, ".hidden_dest"), get_screen_text(tui)

        _send_and_wait_for_text_visibility(
            tui, "`", ".hidden_dest", present=False, timeout=1.5
        )

        tui.send_keystroke(Keys.ESC, wait=0.3)
        assert tui.wait_for_content("To Directory:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ESC, wait=0.3)
        assert not _screen_contains_text(tui, ".hidden_dest"), get_screen_text(tui)
    finally:
        tui.quit()


def test_f2_picker_hidden_entries_cannot_be_selected_while_hidden(tmp_path):
    root = tmp_path / "f2_hidden_selection"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / ".hidden_dest").mkdir()
    (root / "visible_dest").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        if _screen_contains_text(tui, ".hidden_dest"):
            _send_and_wait_for_text_visibility(
                tui, "`", ".hidden_dest", present=False, timeout=1.5
            )

        tui.send_keystroke(Keys.ENTER, wait=0.3)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        assert tui.wait_for_content("COPY:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_content("To Directory:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.F2, wait=0.3)
        assert tui.wait_for_content("cycle", timeout=1.0), get_screen_text(tui)
        assert not _screen_contains_text(tui, ".hidden_dest"), get_screen_text(tui)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        assert _has_exact_span_text(tui, "visible_dest"), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)

        prompt = tui.wait_for_condition(
            lambda lines: _destination_prompt_line(tui)
            if any("To Directory:" in line for line in lines)
            else False,
            timeout=1.0,
            poll_interval=0.05,
        )
        assert prompt, get_screen_text(tui)
        destination_line = _destination_prompt_line(tui)
        assert str(root / "visible_dest") in destination_line, get_screen_text(tui)
        assert ".hidden_dest" not in destination_line, get_screen_text(tui)
    finally:
        tui.quit()


def test_f2_picker_preserves_visible_selection_index_when_hidden_rows_exist(tmp_path):
    root = tmp_path / "f2_visible_index_selection"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / ".UnixTree").mkdir()
    for name in ("00", "Cline", "Hooks"):
        child = root / name
        child.mkdir()
        (child / "inside.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        if _screen_contains_text(tui, ".UnixTree"):
            _send_and_wait_for_text_visibility(
                tui, "`", ".UnixTree", present=False, timeout=1.5
            )

        tui.send_keystroke(Keys.ENTER, wait=0.3)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        assert tui.wait_for_content("COPY:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_content("To Directory:", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.F2, wait=0.3)
        assert tui.wait_for_content("cycle", timeout=1.0), get_screen_text(tui)
        assert not _screen_contains_text(tui, ".UnixTree"), get_screen_text(tui)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        assert _has_exact_span_text(tui, "Cline"), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        assert tui.wait_for_content(str(root / "Cline"), timeout=1.0), get_screen_text(tui)

        lines = tui.send_and_wait_for_condition(
            Keys.ESC,
            lambda screen: screen
            if (_path_label(_panel_path_header(screen) or "") == "Cline")
            else False,
            timeout=1.5,
        )
        assert lines, get_screen_text(tui)
        current_path = _panel_path_header(lines) or ""
        assert _path_label(current_path) == "Cline", get_screen_text(tui)
        assert _path_label(current_path) != "00", get_screen_text(tui)
        assert ".UnixTree" not in current_path, get_screen_text(tui)
    finally:
        tui.quit()


def test_f2_escape_can_abort_right_expand_scan(tmp_path):
    root = tmp_path / "f2_tree_abort"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=0\n", encoding="utf-8")
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    alpha = root / "alpha"
    alpha.mkdir()
    total_files = 1
    for i in range(300):
        branch = alpha / f"dir_{i:03d}" / "child" / "grand"
        branch.mkdir(parents=True)
        for j in range(25):
            (alpha / f"dir_{i:03d}" / f"f_{j:03d}.txt").write_text(
                "x", encoding="utf-8"
            )
            total_files += 1

    (root / "beta").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert _total_items_count(tui) == 1

        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.3)

        tui.send_keystroke(Keys.F2, wait=0.8)
        _move_selection_to_exact_span(tui, "alpha")
        tui.child.send(Keys.RIGHT)                # start subtree scan
        tui.child.send(Keys.ESC)                  # abort the scan mid-flight
        tui._read_output(timeout=0.8)

        scanned_total = _total_items_count(tui)
        assert scanned_total is not None
        assert scanned_total < total_files, (
            "ESC during F2 RIGHT-expansion should stop the subtree scan before "
            "the full file count is loaded.\n"
            f"expected less than {total_files}, saw {scanned_total}\n"
            f"{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_history_selection_only_covers_the_current_item(tmp_path):
    root = tmp_path / "history_selection_bounds"
    root.mkdir()
    (root / "file0.txt").write_text("x", encoding="utf-8")
    (root / "dest").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke("dest\r", wait=0.8)

        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.UP, wait=0.8)

        _, start, end, _, text = _span_containing(tui, "dest")
        assert text == text.rstrip(), (
            "History selection should stop at the selected item instead of "
            "extending across the rest of the row.\n"
            f"span {start}-{end}: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_selection_stops_before_the_synthetic_expand_suffix(tmp_path):
    root = tmp_path / "f2_selection_bounds"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (root / "alpha" / "child").mkdir(parents=True)
    (root / "beta").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.COPY, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.3)
        tui.send_keystroke(Keys.F2, wait=0.8)
        _move_selection_to_exact_span(tui, "alpha")

        _, _, _, _, text = _span_containing(tui, "alpha")
        assert text == "alpha", (
            "F2 selection should cover only the current item, not the "
            "synthetic expandability suffix.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_picker_down_moves_highlight_before_scrolling(tmp_path):
    root = tmp_path / "f2_picker_selection_motion"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    for i in range(80):
        (root / f"dir_{i:02d}").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        steps = 0
        while "dir_34" not in tui.get_screen_dump()[0]:
            if steps >= 80:
                raise AssertionError(
                    "Could not select dir_34 before opening the F2 picker.\n"
                    f"{get_screen_text(tui)}"
                )
            tui.send_keystroke(Keys.DOWN, wait=0.03)
            steps += 1

        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.COPY, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.F2, wait=0.5)

        before_row = _left_exact_span_row(tui, "dir_34")
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        after_row = _left_exact_span_row(tui, "dir_35")

        assert after_row > before_row, (
            "F2 destination browsing should move the highlighted selection "
            "through the visible rows before it starts scrolling the tree.\n"
            f"before row={before_row}, after row={after_row}\n"
            f"{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_picker_up_at_top_stops_without_wrapping(tmp_path):
    root = tmp_path / "f2_picker_top_boundary"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    for i in range(5):
        (root / f"dir_{i:02d}").mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.COPY, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.F2, wait=0.5)

        root_path = str(root)
        assert _left_exact_span_row(tui, root_path) >= 0, get_screen_text(tui)
        tui.send_keystroke(Keys.HOME, wait=0.1)
        assert _left_exact_span_row(tui, root_path) >= 0, get_screen_text(tui)

        tui.send_keystroke(Keys.UP, wait=0.2)
        assert _left_exact_span_row(tui, root_path) >= 0, (
            "F2 destination browsing should stop at the top item instead of "
            "wrapping to the bottom.\n"
            f"{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f2_picker_keeps_viewport_fixed_until_highlight_hits_edge(tmp_path):
    root = tmp_path / "r"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    for name in ["00", "Cline", "codex-lb", "codex-pooler", "priv"]:
        (root / name).mkdir()

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=2.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.COPY, wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.2)
        tui.send_keystroke(Keys.F2, wait=0.5)

        root_row = _left_row_containing(tui, str(root))
        child_row = _left_row_containing(tui, "00")

        tui.send_keystroke(Keys.DOWN, wait=0.2)
        moved_child_row = _left_row_containing(tui, "00")

        assert moved_child_row > root_row, (
            "F2 destination browsing should keep the viewport fixed until the "
            "highlight reaches the visible edge instead of scrolling the root "
            "selection out of view on the first Down.\n"
            f"root row={root_row}, initial child row={child_row}, "
            f"moved child row={moved_child_row}\n"
            f"{get_screen_text(tui)}"
        )
        assert _left_row_containing(tui, str(root)) == root_row, get_screen_text(tui)
    finally:
        tui.quit()


def test_volume_menu_selection_only_covers_current_item(tmp_path):
    root = tmp_path / "volume_selection_bounds"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.send_keystroke("K", wait=0.6)

        _, _, _, _, text = _span_containing(tui, f"[*] {root}")
        assert text.startswith("[*] "), (
            "Volume-menu selection should start at the current item instead of "
            "including the whole row padding.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
        assert text == text.rstrip(), (
            "Volume-menu selection should stop at the selected item instead of "
            "extending across the rest of the row.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f9_applications_menu_selection_only_covers_current_item(tmp_path):
    root = tmp_path / "applications_menu_bounds"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        tui.send_keystroke(Keys.F9, wait=0.6)

        _, _, _, _, text = _span_containing(tui, "wget fetch preset")
        assert text.startswith("wget fetch preset"), (
            "Applications-menu selection should start at the current item instead "
            "of including row padding.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
        assert text == text.rstrip(), (
            "Applications-menu selection should stop at the selected item instead "
            "of extending across the rest of the row.\n"
            f"selected span: {text!r}\n{get_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f9_applications_menu_navigation_keys_and_edit_action(tmp_path):
    root = tmp_path / "applications_menu_edit_nav"
    root.mkdir()
    (root / "seed.txt").write_text("seed", encoding="utf-8")

    editor_capture = root / "applications_catalog_capture.txt"
    editor = root / "applications_catalog_editor.sh"
    editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "printf '\\n# edited by f9 apps\\n' >> \"$f\"\n",
        encoding="utf-8",
    )
    editor.chmod(0o755)

    tui = YtreeNovaTUI(
        executable=YTNOVA_BIN,
        cwd=str(root),
        env_extra={"EDITOR": str(editor)},
    )

    try:
        assert tui.wait_for_content("seed.txt", timeout=1.5), get_screen_text(tui)
        tui.send_keystroke(Keys.F9, wait=0.6)

        assert _has_exact_span_text(tui, "wget fetch preset"), get_screen_text(tui)
        tui.send_keystroke(Keys.END, wait=0.3)
        assert _has_exact_span_text(tui, "duplicate report"), get_screen_text(tui)
        tui.send_keystroke(Keys.HOME, wait=0.3)
        assert _has_exact_span_text(tui, "wget fetch preset"), get_screen_text(tui)
        tui.send_keystroke(Keys.PGDN, wait=0.3)
        assert _has_exact_span_text(tui, "duplicate report"), get_screen_text(tui)
        tui.send_keystroke(Keys.PGUP, wait=0.3)
        assert _has_exact_span_text(tui, "wget fetch preset"), get_screen_text(tui)

        tui.send_keystroke("e", wait=0.8)
        assert tui.wait_for_condition(
            lambda lines: lines if editor_capture.exists() else False,
            timeout=2.0,
            poll_interval=0.05,
        ), get_screen_text(tui)

        applications_path = next(
            (
                candidate
                for candidate in (
                    root / ".config" / "ytnova" / "applications.conf",
                    root / ".ytnova.applications",
                )
                if candidate.exists()
            ),
            None,
        )
        assert applications_path is not None, (
            "Applications edit should bootstrap the dedicated applications catalog before opening the editor."
        )
        applications_text = applications_path.read_text(encoding="utf-8")
        assert "# edited by f9 apps" in applications_text
        capture_text = editor_capture.read_text(encoding="utf-8")
        assert "# applications.conf starter for YtreeNova F9 application presets." in capture_text
        assert "# label | prompt | command" in capture_text
        assert "open selected item |  | xdg-open {}" in capture_text
        assert "open typed URL | URL | xdg-open {input}" in capture_text
        assert "# open selected media in mpv |  | mpv {}" in capture_text
    finally:
        tui.quit()


def test_f9_applications_menu_launches_entries_and_returns_without_pause(tmp_path):
    root = tmp_path / "applications_menu_execute"
    config_dir = root / ".config" / "ytnova"
    root.mkdir()
    config_dir.mkdir(parents=True)
    (root / "seed.txt").write_text("seed", encoding="utf-8")
    (config_dir / "applications.conf").write_text(
        "# label | prompt | command\n"
        "launch success marker |  | sh -c 'sleep 0.2; printf ok > success.txt'\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(executable=YTNOVA_BIN, cwd=str(root))

    try:
        assert tui.wait_for_content("seed.txt", timeout=1.5), get_screen_text(tui)

        tui.send_keystroke(Keys.F9, wait=0.6)
        assert tui.wait_for_content("launch success marker", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke("\n", wait=0.2)
        assert tui.wait_for_condition(
            lambda lines: lines
            if "launched: launch success marker" in "\n".join(lines).lower()
            and "[Hit return to continue]" not in "\n".join(lines)
            else False,
            timeout=1.5,
            poll_interval=0.05,
        ), get_screen_text(tui)

        tui.send_keystroke(Keys.F9, wait=0.4)
        assert tui.wait_for_content("launch success marker", timeout=1.0), get_screen_text(tui)
        tui.send_keystroke(Keys.ESC, wait=0.2)

        assert tui.wait_for_condition(
            lambda lines: (root / "success.txt").exists()
            and "[Hit return to continue]" not in "\n".join(lines),
            timeout=2.0,
            poll_interval=0.05,
        ), get_screen_text(tui)
    finally:
        tui.quit()
