import shutil
import tarfile

from helpers_ui import footer_lines as _footer_lines
from helpers_ui import footer_text as _footer_text
from helpers_ui import screen_text as _screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _send_left_arrow(tui, wait=0.4):
    tui.send_keystroke(Keys.LEFT, wait=wait)


def _create_archive(root, archive_name="sample.tar"):
    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")
    (archive_source / "nested").mkdir()
    (archive_source / "nested" / "nested.txt").write_text("nested", encoding="utf-8")

    archive_path = root / archive_name
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)
    return archive_path


def _assert_margin_plus_marker(screen_rows, dir_name, expect_slash=False):
    candidates = [
        line
        for line in screen_rows
        if dir_name in line and "Path:" not in line and "CURRENT DIR" not in line
        and "ATTRIBUTES" not in line
    ]
    row = next((line for line in candidates if "tq" in line or "mq" in line), "")
    if not row and candidates:
        row = candidates[0]
    assert row, f"Expected a tree row for {dir_name!r}.\n" + "\n".join(screen_rows)
    name_col = row.find(dir_name)
    plus_col = row.find("+")
    assert plus_col >= 0 and plus_col < name_col, (
        "Unlogged marker must render in tree status margin, not in name text.\n"
        f"row={row!r}"
    )
    assert f"{dir_name}+" not in row, (
        "Tree row must not append '+' suffixes to directory names.\n"
        f"row={row!r}"
    )
    if expect_slash:
        assert f"{dir_name}/" in row, (
            "Unlogged directory with subdirs should show '/' suffix.\n"
            f"row={row!r}"
        )
    else:
        assert f"{dir_name}/" not in row, (
            "Unlogged directory without subdirs should not show '/' suffix.\n"
            f"row={row!r}"
        )


def _has_tree_row_for_dir(screen_rows, dir_name):
    for line in screen_rows:
        if (
            dir_name in line
            and "Path:" not in line
            and "CURRENT DIR" not in line
            and "CURRENT FILE" not in line
            and ("mq" in line or "tq" in line)
        ):
            return True
    return False


def _footer_has_key_tokens(footer, *tokens):
    footer = footer.lower()
    return all(f"({token.lower()})" in footer for token in tokens)


def _footer_key_token_index(footer_line, token):
    return footer_line.lower().index(f"({token.lower()})")


def _assert_footer_segments_in_order(line, *segments):
    start = 0
    for segment in segments:
        pos = line.find(segment, start)
        assert pos >= 0, (
            f"Expected footer segment {segment!r} in order.\n"
            f"Line: {line!r}"
        )
        start = pos + len(segment)


def _graceful_quit(tui):
    tui.send_keystroke("q", wait=0)
    assert tui.wait_for_exit(timeout=3.0), "ytnova did not complete orderly quit"
    tui.child.close()


def _open_config_and_wait_for_effect(tui, effect, description):
    assert tui.send_and_wait_for_screen_change("\x1b[21~", timeout=2.0), (
        "F10 did not open the configuration command strip."
    )
    result = tui.send_and_wait_for_condition(
        Keys.ENTER,
        lambda lines: lines if effect() else False,
        timeout=3.0,
    )
    assert result, f"F10 configuration edit did not complete {description}."


def _cell_style_for_text(tui, needle, *, exclude_substrings=()):
    screen_rows = tui.get_screen_dump()

    for y, line in enumerate(screen_rows):
        if needle not in line:
            continue
        if any(excluded in line for excluded in exclude_substrings):
            continue
        x = line.index(needle)
        cell = tui.screen.buffer[y][x]
        return cell.fg, cell.bg, cell.bold, cell.reverse

    raise AssertionError(
        f"Could not find screen cell for {needle!r}.\nScreen:\n{_screen_text(tui)}"
    )


def _wait_for_style_change(
    tui, needle, before_style, *, exclude_substrings=(), timeout=2.0
):
    changed_style = tui.wait_for_condition(
        lambda _: (
            current_style
            if (
                current_style := _cell_style_for_text(
                    tui, needle, exclude_substrings=exclude_substrings
                )
            )
            != before_style
            else False
        ),
        timeout=timeout,
        description=f"style change for {needle!r}",
    )
    return changed_style or before_style


def _write_test_theme_catalog(root):
    theme_dir = root / ".config" / "ytnova"
    theme_dir.mkdir(parents=True)
    (theme_dir / "themes.conf").write_text(
        """
[theme quiet-blue]
background = blue
box_lines = cyan
tree_lines = +white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
footer = white
selection = black on white
dialog = white
picker = black on cyan
help = white
help_link = cyan
help_link_selection = yellow
info = +white
warning = black on yellow
error = +white on red
search_hit = black on yellow

[theme bash-black]
background = black
box_lines = grey
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
footer = white
selection = black on white
dialog = white
picker = black on grey
help = white
help_link = cyan
help_link_selection = yellow
info = white on blue
warning = black on yellow
error = white on red
search_hit = black on yellow
""".strip()
        + "\n",
        encoding="utf-8",
    )


def _write_theme_switch_editor(root):
    editor = root / "switch_theme_editor.sh"
    editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        "sed -i 's/^THEME=.*/THEME=bash-black/' \"$f\"\n",
        encoding="utf-8",
    )
    editor.chmod(0o755)
    return editor


def _write_smallwindowskip_toggle_editor(root):
    editor = root / "toggle_smallwindowskip.sh"
    editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        "if grep -q '^SMALLWINDOWSKIP=' \"$f\"; then\n"
        "  sed -i 's/^SMALLWINDOWSKIP=.*/SMALLWINDOWSKIP=1/' \"$f\"\n"
        "else\n"
        "  printf '\\nSMALLWINDOWSKIP=1\\n' >> \"$f\"\n"
        "fi\n",
        encoding="utf-8",
    )
    editor.chmod(0o755)
    return editor


def test_archive_left_at_root_collapses_once_then_noop(tmp_path, ytnova_binary):
    """At archive root: first LEFT collapses children, second LEFT is a no-op."""
    root = tmp_path / "archive_exit_root"
    root.mkdir()

    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")
    _create_archive(root, "Absolutely MAD.tar")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    # Enter file view and log selected archive file.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.9)
    assert tui.wait_for_content("ARCHIVE", timeout=3.0), _screen_text(tui)

    before_left = "\n".join(tui.get_screen_dump())
    assert "nested" in before_left, (
        "Precondition failed: archive root should show nested child row before LEFT.\n"
        f"Screen:\n{before_left}"
    )

    # First LEFT at archive root collapses one level but does not exit archive mode.
    _send_left_arrow(tui, wait=0.8)

    after_first_left = "\n".join(tui.get_screen_dump())
    assert "ARCHIVE" in after_first_left, (
        "First LEFT at archive root must not exit archive mode.\n"
        f"Screen:\n{after_first_left}"
    )
    assert "nested" not in after_first_left, (
        "First LEFT at archive root should collapse child rows.\n"
        f"Screen:\n{after_first_left}"
    )

    # Second LEFT at already-collapsed archive root is a no-op.
    _send_left_arrow(tui, wait=0.8)
    after_second_left = "\n".join(tui.get_screen_dump())
    assert "ARCHIVE" in after_second_left, (
        "Second LEFT at collapsed archive root must stay in archive mode.\n"
        f"Screen:\n{after_second_left}"
    )
    assert "nested" not in after_second_left, (
        "Second LEFT at collapsed archive root should remain collapsed.\n"
        f"Screen:\n{after_second_left}"
    )

    tui.quit()


def test_minus_on_leaf_unlogs_directory_state(tmp_path, ytnova_binary):
    root = tmp_path / "minus_leaf_unlog"
    root.mkdir()
    leaf = root / "leaf_dir"
    leaf.mkdir()
    (leaf / "leaf_file.txt").write_text("leaf", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke("-", wait=0.4)
    tui.send_keystroke(Keys.ENTER, wait=0.4)

    footer = _footer_text(tui)
    screen = "\n".join(tui.get_screen_dump())
    assert not _footer_has_key_tokens(footer, "H", "I", "J"), (
        "Leaf directory should be unlogged after '-' and not enter file mode.\n"
        f"Footer:\n{footer}\n\nScreen:\n{screen}"
    )
    assert "leaf_file.txt" in screen, (
        "Enter on unlogged leaf should relog/reveal one level while staying in tree mode.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()


def test_archive_left_non_root_does_not_exit_immediately(tmp_path, ytnova_binary):
    root = tmp_path / "a_left"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")
    archive_path = _create_archive(root, "l.tar")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.9)
    assert tui.wait_for_content("ARCHIVE", timeout=3.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke("*", wait=0.6)
    steps = 0
    while "inside_dir/nested" not in tui.get_screen_dump()[0]:
        if steps >= 12:
            raise AssertionError(
                "Could not select inside_dir/nested in the archive tree.\n"
                f"{_screen_text(tui)}"
            )
        tui.send_keystroke(Keys.DOWN, wait=0.25)
        steps += 1

    _send_left_arrow(tui, wait=0.6)

    screen = "\n".join(tui.get_screen_dump())
    assert "ARCHIVE" in screen, (
        "LEFT from archive non-root must not exit archive mode immediately.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()


def test_fs_left_at_root_collapses_once_then_noop(tmp_path, ytnova_binary):
    root = tmp_path / "fs_left_root_collapse_once"
    root.mkdir()
    (root / "child_a").mkdir()
    (root / "child_b").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        before_left = "\n".join(tui.get_screen_dump())
        assert "child_a" in before_left or "child_b" in before_left, (
            "Precondition failed: filesystem root should show child rows before LEFT.\n"
            f"Screen:\n{before_left}"
        )

        _send_left_arrow(tui, wait=0.6)
        after_first_left = "\n".join(tui.get_screen_dump())
        assert "child_a" not in after_first_left and "child_b" not in after_first_left, (
            "First LEFT at filesystem root should collapse child rows.\n"
            f"Screen:\n{after_first_left}"
        )

        _send_left_arrow(tui, wait=0.6)
        after_second_left = "\n".join(tui.get_screen_dump())
        assert "child_a" not in after_second_left and "child_b" not in after_second_left, (
            "Second LEFT at collapsed filesystem root should keep children collapsed.\n"
            f"Screen:\n{after_second_left}"
        )
        assert "Unlogged" in after_second_left, (
            "Second LEFT at collapsed filesystem root should keep root unlogged.\n"
            f"Screen:\n{after_second_left}"
        )
    finally:
        tui.quit()


def test_fs_root_left_then_right_does_not_restore_deep_state(tmp_path, ytnova_binary):
    root = tmp_path / "fs_root_left_right_reset"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    (root / "alpha" / "child" / "grand").mkdir(parents=True)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # alpha
        tui.send_keystroke(Keys.RIGHT, wait=0.4)  # expand alpha
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # child
        tui.send_keystroke(Keys.RIGHT, wait=0.4)  # expand child
        tui.send_keystroke(Keys.DOWN, wait=0.2)   # grand

        before_reset = "\n".join(tui.get_screen_dump())
        assert "grand" in before_reset, (
            "Precondition failed: expected deep expansion before reset.\n"
            f"Screen:\n{before_reset}"
        )

        tui.send_keystroke(Keys.UP, wait=0.2)
        tui.send_keystroke(Keys.UP, wait=0.2)
        tui.send_keystroke(Keys.UP, wait=0.2)     # root
        assert tui.send_and_wait_for_screen_change(Keys.LEFT, timeout=1.5)
        after_lines = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines if any("alpha" in line for line in lines) else False,
            timeout=2.0,
        )
        assert after_lines, "Root re-expand did not reveal the immediate child."
        after_reexpand = "\n".join(after_lines)
        tree_and_footer = "\n".join(after_lines[1:])
        assert "alpha" in after_reexpand, (
            "Root re-expand should show immediate child directories.\n"
            f"Screen:\n{after_reexpand}"
        )
        assert " mqchild" not in tree_and_footer and " mqgrand" not in tree_and_footer, (
            "Root LEFT reset must discard prior ad-hoc deep expansion; RIGHT must"
            " not restore it.\n"
            f"Screen:\n{after_reexpand}"
        )
    finally:
        tui.quit()


def test_archive_root_unlogged_right_does_not_show_permission_denied(
    tmp_path, ytnova_binary
):
    root = tmp_path / "archive_root_unlogged_right"
    root.mkdir()
    _create_archive(root, "roundtrip.tar")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke(Keys.LOG, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.9)
        assert tui.wait_for_content("ARCHIVE", timeout=3.0), _screen_text(tui)

        _send_left_arrow(tui, wait=0.8)  # reset/unlog archive root
        tui.send_keystroke(Keys.RIGHT, wait=0.9)

        after = _screen_text(tui)
        assert "Permission Denied" not in after, (
            "Right at unlogged archive root should relog archive context, not "
            "raise a filesystem permission error.\n"
            f"{after}"
        )
        assert "ARCHIVE" in after, (
            "Right at unlogged archive root should remain in archive mode.\n"
            f"{after}"
        )
    finally:
        tui.quit()


def test_archive_root_backslash_exits_to_parent_file_focus(tmp_path, ytnova_binary):
    root = tmp_path / "a_bs"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")
    archive_path = _create_archive(root, "b.tar")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.9)
    assert tui.wait_for_content("ARCHIVE", timeout=3.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke("\\", wait=0.8)

    screen = "\n".join(tui.get_screen_dump())
    assert "ARCHIVE" not in screen, (
        "Backslash at archive root must exit archive context.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()


def test_archive_non_root_backslash_jumps_to_archive_root(tmp_path, ytnova_binary):
    root = tmp_path / "archive_non_root_bs_noop"
    root.mkdir()
    _create_archive(root, "noop.tar")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), _screen_text(tui)

    tui.send_keystroke("*", wait=0.6)
    steps = 0
    while "inside_dir/nested" not in tui.get_screen_dump()[0]:
        if steps >= 10:
            raise AssertionError(
                "Could not select inside_dir/nested in the archive tree.\n"
                f"{_screen_text(tui)}"
            )
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        steps += 1
    before = _screen_text(tui)
    tui.send_keystroke("\\", wait=0.6)
    after = _screen_text(tui)
    assert "ARCHIVE" in after, "Backslash at archive non-root must not exit archive mode."
    assert "inside_dir/nested" not in tui.get_screen_dump()[0], (
        "Backslash at archive non-root must jump to archive root."
    )
    assert "inside_dir" in after, "Archive root context should remain visible after jump."
    assert "\a" not in before + after, "No bell expected for archive-root jump action."

    tui.quit()


def test_archive_file_backslash_is_silent_noop(tmp_path, ytnova_binary):
    root = tmp_path / "archive_file_bs_noop"
    root.mkdir()
    _create_archive(root, "archive_file_noop.tar")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.send_keystroke(Keys.DOWN, wait=0.2)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.8)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), _screen_text(tui)

    assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    before = _screen_text(tui)
    before_footer = _footer_text(tui)
    assert _footer_has_key_tokens(
        before_footer, "H", "I", "J"
    ), "Expected archive file window footer."

    tui.send_keystroke("\\", wait=0.4)
    after = _screen_text(tui)
    after_footer = _footer_text(tui)
    assert "ARCHIVE" in after, "Backslash in archive file window must stay in archive context."
    assert _footer_has_key_tokens(
        after_footer, "H", "I", "J"
    ), "Backslash in archive file window must be a no-op."
    assert after_footer == before_footer, "Backslash in archive file window should not move context."
    assert "\a" not in before + after, "No bell expected for no-op backslash action."

    tui.quit()


def test_backslash_in_fs_dir_and_file_windows_is_silent_noop(tmp_path, ytnova_binary):
    root = tmp_path / "fs_backslash_noop"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha", encoding="utf-8")
    (root / "beta.txt").write_text("beta", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    dir_before = _screen_text(tui)
    dir_before_footer = _footer_text(tui)
    tui.send_keystroke("\\", wait=0.4)
    dir_after = _screen_text(tui)
    dir_after_footer = _footer_text(tui)
    assert "ARCHIVE" not in dir_after, "Normal filesystem dir window must remain non-archive."
    assert dir_after_footer == dir_before_footer, "Backslash in fs dir window should be a no-op."
    assert "\a" not in dir_before + dir_after, "No bell expected for fs dir backslash no-op."

    tui.send_keystroke(Keys.ENTER, wait=0.4)
    file_before = _screen_text(tui)
    file_before_footer = _footer_text(tui)
    assert _footer_has_key_tokens(
        file_before_footer, "H", "I", "J"
    ), "Expected normal filesystem file window."

    tui.send_keystroke("\\", wait=0.4)
    file_after = _screen_text(tui)
    file_after_footer = _footer_text(tui)
    assert _footer_has_key_tokens(
        file_after_footer, "H", "I", "J"
    ), "Backslash in fs file window must be a no-op."
    assert file_after_footer == file_before_footer, "Backslash in fs file window should not move context."
    assert "\a" not in file_before + file_after, "No bell expected for fs file backslash no-op."

    tui.quit()


def test_unlogged_tree_shows_plus_marker_and_plus_relogs(tmp_path, ytnova_binary):
    root = tmp_path / "unlogged_plus_marker"
    root.mkdir()
    node = root / "node"
    node.mkdir()
    (node / "child.txt").write_text("payload", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke("-", wait=0.4)
    screen_after_unlog = _screen_text(tui)
    rows_after_unlog = tui.get_screen_dump()
    _assert_margin_plus_marker(rows_after_unlog, "node")

    tui.send_keystroke("+", wait=0.5)
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    footer = _footer_text(tui)
    assert _footer_has_key_tokens(footer, "H", "I", "J"), (
        "'+' should relog directory so Enter opens file mode.\n"
        f"Footer:\n{footer}\n\nScreen:\n{_screen_text(tui)}"
    )

    tui.quit()


def test_enter_on_unlogged_dir_relogs_and_reveals_first_level_only(
    tmp_path, ytnova_binary
):
    root = tmp_path / "unlogged_enter_relogs_first_level"
    root.mkdir()
    node = root / "node"
    node.mkdir()
    (node / "child_a").mkdir()
    (node / "child_b").mkdir()
    (node / "child_a" / "nested.txt").write_text("payload", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("-", wait=0.4)
        before = _screen_text(tui)
        assert "child_a" not in before, (
            "Precondition failed: collapsed unlogged directory should hide "
            "first-level children before Enter.\n"
            f"{before}"
        )

        after_lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if any("child_a" in line for line in lines)
            and any("child_b" in line for line in lines)
            else False,
            timeout=2.0,
        )
        assert after_lines, "Enter on the unlogged directory did not relog children."
        after = "\n".join(after_lines)
        footer = _footer_text(tui).lower()

        assert "child_a" in after and "child_b" in after, (
            "Enter on unlogged dir should relog and reveal immediate "
            "subdirectories.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "child_a")
        _assert_margin_plus_marker(rows, "child_b")
        assert not _footer_has_key_tokens(footer, "H", "I", "J"), (
            "Enter on unlogged dir should stay in directory window, not switch "
            "to file window.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_unlogged_directory_with_subdirs_shows_slash_suffix(
    tmp_path, ytnova_binary
):
    root = tmp_path / "unlogged_slash_suffix"
    root.mkdir()
    top = root / "top"
    top.mkdir()
    (top / "child").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("+", wait=0.5)
        tui.send_keystroke("-", wait=0.4)
        tui.send_keystroke("-", wait=0.4)
        _assert_margin_plus_marker(tui.get_screen_dump(), "top", expect_slash=True)
    finally:
        tui.quit()


def test_unlogged_placeholder_with_subdirs_shows_slash_suffix(
    tmp_path, ytnova_binary
):
    root = tmp_path / "unlogged_placeholder_slash_suffix"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    top = root / "top"
    top.mkdir()
    (top / "child").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.4)
        _assert_margin_plus_marker(tui.get_screen_dump(), "top", expect_slash=True)
    finally:
        tui.quit()


def test_enter_on_placeholder_dir_logs_and_reveals_first_level_only(
    tmp_path, ytnova_binary
):
    root = tmp_path / "placeholder_enter_reveals_first_level"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "ui").mkdir()
    (src / "cmd" / "main.c").write_text("int main(void){return 0;}\n",
                                         encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before = _screen_text(tui)
        assert "cmd+" not in before and "ui+" not in before, (
            "Precondition failed: placeholder directory should hide first-level "
            "children before Enter.\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "cmd" in after and "ui" in after, (
            "Enter on placeholder dir should reveal immediate subdirectories.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "cmd")
        _assert_margin_plus_marker(rows, "ui")
        assert not _footer_has_key_tokens(footer, "H", "I", "J"), (
            "Enter on placeholder dir should stay in directory window, not "
            "switch to file window.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_root_left_resets_tree_and_right_relogs_to_profile_depth(
    tmp_path, ytnova_binary
):
    root = tmp_path / "root_minus_right_profile_depth"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    src = root / "src"
    src.mkdir()
    cmd = src / "cmd"
    cmd.mkdir()
    deep = cmd / "deeper"
    deep.mkdir()
    (deep / "leaf.txt").write_text("payload", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        assert tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines if any("cmd" in line for line in lines) else False,
            timeout=2.0,
        )
        tui.send_keystroke(Keys.DOWN, wait=0.2)
        expanded_lines = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines if any("deeper" in line for line in lines) else False,
            timeout=2.0,
        )
        assert expanded_lines, "Could not expand the fixture tree to the deep node."
        expanded = "\n".join(expanded_lines)
        assert "deeper" in expanded, (
            "Precondition failed: expected deep expansion before root reset.\n"
            f"{expanded}"
        )

        tui.send_keystroke(Keys.HOME, wait=0.3)
        steps = 0
        while str(root) not in tui.get_screen_dump()[0]:
            if steps >= 8:
                raise AssertionError(
                    "Could not return selection to the fixture root.\n"
                    f"{_screen_text(tui)}"
                )
            tui.send_keystroke(Keys.LEFT, wait=0.3)
            steps += 1

        at_root_before = _screen_text(tui)
        assert str(root) in tui.get_screen_dump()[0], (
            "Precondition failed: expected selection at root before root-left reset.\n"
            f"{at_root_before}"
        )

        tui.send_keystroke(Keys.LEFT, wait=0.7)
        after_left = _screen_text(tui)
        assert "deeper" not in after_left, (
            "Left collapse at root should reset/release expanded descendant state.\n"
            f"{after_left}"
        )

        after_right_lines = tui.send_and_wait_for_condition(
            Keys.RIGHT,
            lambda lines: lines if any("src" in line for line in lines) else False,
            timeout=2.0,
        )
        assert after_right_lines, "Right on the reset root did not relog its child."
        after_right = "\n".join(after_right_lines)
        assert "src/" in after_right, (
            "Right on reset root should relog to configured TREEDEPTH and show "
            "first-level directory placeholders.\n"
            f"{after_right}"
        )
        assert "deeper" not in after_right, (
            "Right after root reset must not restore previous deep expansion state.\n"
            f"{after_right}"
        )
    finally:
        tui.quit()


def test_enter_on_placeholder_dir_is_consistent_with_smallwindowskip_one(
    tmp_path, ytnova_binary
):
    root = tmp_path / "placeholder_enter_smallwindowskip_one"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=1\nSMALLWINDOWSKIP=1\n", encoding="utf-8"
    )

    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "ui").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before = _screen_text(tui)
        assert "cmd+" not in before and "ui+" not in before, (
            "Precondition failed: placeholder directory should hide first-level "
            "children before Enter (SMALLWINDOWSKIP=1).\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "cmd" in after and "ui" in after, (
            "Enter behavior should match standard mode for placeholder dirs "
            "when SMALLWINDOWSKIP=1.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "cmd")
        _assert_margin_plus_marker(rows, "ui")
        assert not _footer_has_key_tokens(footer, "H", "I", "J"), (
            "Enter on placeholder dir should remain in directory view when "
            "SMALLWINDOWSKIP=1.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_enter_on_placeholder_dir_is_consistent_with_smallwindowskip_zero(
    tmp_path, ytnova_binary
):
    root = tmp_path / "placeholder_enter_smallwindowskip_zero"
    root.mkdir()
    (root / ".ytnova").write_text(
        "[GLOBAL]\nTREEDEPTH=1\nSMALLWINDOWSKIP=0\n", encoding="utf-8"
    )

    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "ui").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before = _screen_text(tui)
        assert "cmd+" not in before and "ui+" not in before, (
            "Precondition failed: placeholder directory should hide first-level "
            "children before Enter (SMALLWINDOWSKIP=0).\n"
            f"{before}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.6)
        after = _screen_text(tui)
        footer = _footer_text(tui).lower()

        assert "cmd" in after and "ui" in after, (
            "Enter behavior should match standard mode for placeholder dirs "
            "when SMALLWINDOWSKIP=0.\n"
            f"{after}"
        )
        rows = tui.get_screen_dump()
        _assert_margin_plus_marker(rows, "cmd")
        _assert_margin_plus_marker(rows, "ui")
        assert not _footer_has_key_tokens(footer, "H", "I", "J"), (
            "Enter on placeholder dir should remain in directory view when "
            "SMALLWINDOWSKIP=0.\n"
            f"Footer:\n{footer}\n\nScreen:\n{after}"
        )
    finally:
        tui.quit()


def test_smallwindowskip_negative_value_falls_back_to_default_profile(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_negative_value"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")
    (root / ".ytnova").write_text(
        "[GLOBAL]\nSMALLWINDOWSKIP=-1\n", encoding="utf-8"
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        footer = _footer_text(tui).lower()
        footer_lines = [line.strip().lower() for line in _footer_lines(tui)]
        assert (
            footer_lines
            and footer_lines[0].startswith("file")
        ), (
            "SMALLWINDOWSKIP=-1 should invalidate the startup profile and "
            "fall back to the built-in default profile (SMALLWINDOWSKIP=1), "
            "which bypasses the intermediate directory view on Enter.\n"
            f"Footer:\n{footer}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_smallwindowskip_trailing_junk_value_falls_back_to_default_profile(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_trailing_junk_value"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")
    (root / ".ytnova").write_text(
        "[GLOBAL]\nSMALLWINDOWSKIP=1junk\n", encoding="utf-8"
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        footer = _footer_text(tui).lower()
        footer_lines = [line.strip().lower() for line in _footer_lines(tui)]
        assert (
            footer_lines
            and footer_lines[0].startswith("file")
        ), (
            "SMALLWINDOWSKIP=1junk should invalidate the startup profile and "
            "fall back to the built-in default profile (SMALLWINDOWSKIP=1), "
            "which bypasses the intermediate directory view on Enter.\n"
            f"Footer:\n{footer}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_smallwindowskip_config_edit_applies_immediately_in_session(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_live_apply"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    toggle_editor = _write_smallwindowskip_toggle_editor(root)

    (root / ".ytnova").write_text(
        "[GLOBAL]\n"
        "SMALLWINDOWSKIP=0\n"
        f"EDITOR={toggle_editor}\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)  # select target dir

        # Baseline (SMALLWINDOWSKIP=0): two ENTER presses stay in file views.
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert "file" in _footer_text(tui).lower(), (
            "Precondition failed: SMALLWINDOWSKIP=0 should still be in file view "
            "after two ENTER presses.\n"
            f"{_screen_text(tui)}"
        )
        tui.send_keystroke(Keys.ENTER, wait=0.5)  # back to dir

        xdg_profile = root / ".config" / "ytnova" / "ytnova.conf"
        # Edit config via F10 and let the configured editor switch value to 1.
        _open_config_and_wait_for_effect(
            tui,
            lambda: xdg_profile.exists()
            and "SMALLWINDOWSKIP=1" in xdg_profile.read_text(encoding="utf-8"),
            "the XDG profile update",
        )
        assert "SMALLWINDOWSKIP=1" in xdg_profile.read_text(encoding="utf-8"), (
            "Config edit flow should migrate the edited session into the preferred "
            "XDG profile file."
        )

        # After live apply: two ENTER presses should return to dir mode.
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        footer_after = _footer_text(tui).lower()
        footer_after_lines = [line.strip().lower() for line in _footer_lines(tui)]
        assert footer_after_lines and footer_after_lines[0].startswith("dir"), (
            "SMALLWINDOWSKIP change from config edit did not apply in-session.\n"
            f"Footer:\n{footer_after}\n\nScreen:\n{_screen_text(tui)}"
        )
        assert "tree" in footer_after, (
            "Directory footer should be restored after returning with "
            "SMALLWINDOWSKIP=1.\n"
            f"Footer:\n{footer_after}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_smallwindowskip_config_edit_uses_startup_selected_profile_path(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_custom_profile"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    toggle_editor = _write_smallwindowskip_toggle_editor(root)
    custom_profile = root / "custom.conf"
    custom_profile.write_text(
        "[GLOBAL]\n"
        "SMALLWINDOWSKIP=0\n"
        f"EDITOR={toggle_editor}\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=["-p", str(custom_profile)],
    )

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        _open_config_and_wait_for_effect(
            tui,
            lambda: "SMALLWINDOWSKIP=1"
            in custom_profile.read_text(encoding="utf-8"),
            "the selected profile update",
        )

        assert "SMALLWINDOWSKIP=1" in custom_profile.read_text(encoding="utf-8"), (
            "F10 config edit must target the startup-selected -p profile path.\n"
            f"Profile contents:\n{custom_profile.read_text(encoding='utf-8')}"
        )
        assert not (root / ".ytnova").exists(), (
            "F10 config edit must not fall back to ~/.ytnova when startup used -p."
        )

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        footer_after = _footer_text(tui).lower()
        footer_after_lines = [line.strip().lower() for line in _footer_lines(tui)]
        assert footer_after_lines and footer_after_lines[0].startswith("dir"), (
            "Reload after editing the startup-selected profile must apply "
            "SMALLWINDOWSKIP in-session.\n"
            f"Footer:\n{footer_after}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f10_reload_repaints_theme_from_tree_focus(tmp_path, ytnova_binary):
    root = tmp_path / "f10_reload_tree_repaint"
    root.mkdir()
    _write_test_theme_catalog(root)
    editor = _write_theme_switch_editor(root)
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    profile_path = root / ".ytnova"
    profile_path.write_text(
        "[GLOBAL]\n"
        "THEME=quiet-blue\n"
        "SMALLWINDOWSKIP=1\n"
        f"EDITOR={editor}\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before_style = _cell_style_for_text(tui, "Path:")

        updated_profile = root / ".config" / "ytnova" / "ytnova.conf"
        _open_config_and_wait_for_effect(
            tui,
            lambda: updated_profile.exists()
            and "THEME=bash-black" in updated_profile.read_text(encoding="utf-8"),
            "the selected theme update",
        )

        after_style = _wait_for_style_change(tui, "Path:", before_style)
        assert after_style != before_style, (
            "F10 reload from tree focus must repaint the visible path header after "
            "theme changes.\n"
            f"Before={before_style} After={after_style}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_f10_reload_repaints_theme_from_file_focus(tmp_path, ytnova_binary):
    root = tmp_path / "f10_reload_file_repaint"
    root.mkdir()
    _write_test_theme_catalog(root)
    editor = _write_theme_switch_editor(root)
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    profile_path = root / ".ytnova"
    profile_path.write_text(
        "[GLOBAL]\n"
        "THEME=quiet-blue\n"
        "SMALLWINDOWSKIP=1\n"
        f"EDITOR={editor}\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        before_style = _cell_style_for_text(tui, "Path:")

        updated_profile = root / ".config" / "ytnova" / "ytnova.conf"
        _open_config_and_wait_for_effect(
            tui,
            lambda: updated_profile.exists()
            and "THEME=bash-black" in updated_profile.read_text(encoding="utf-8"),
            "the selected theme update",
        )

        after_style = _wait_for_style_change(tui, "Path:", before_style)
        assert after_style != before_style, (
            "F10 reload from file focus must repaint the visible path header after "
            "theme changes.\n"
            f"Before={before_style} After={after_style}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_missing_profile_f10_unchanged_edit_creates_profile(tmp_path, ytnova_binary):
    root = tmp_path / "missing_profile_f10_unchanged_edit"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    editor_capture = root / "f10_default_buffer_snapshot.txt"
    unchanged_editor = root / "unchanged_profile_editor.sh"
    unchanged_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    unchanged_editor.chmod(0o755)

    profile_path = root / ".config" / "ytnova" / "ytnova.conf"
    assert not profile_path.exists()

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"EDITOR": str(unchanged_editor)},
    )

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        tui.wait_for_condition(
            lambda _: editor_capture.exists(),
            timeout=2.0,
            description="editor_capture creation",
        )
        assert editor_capture.exists(), (
            "F10 -> Enter on a missing profile must open an editable default profile buffer."
        )
        profile_text = editor_capture.read_text(encoding="utf-8")
        assert "# YtreeNova Defaults" in profile_text, (
            "Missing-profile F10 bootstrap should keep the commented starter "
            "profile header, not a stripped key dump."
        )
        assert "[GLOBAL]" in profile_text, "Profile buffer should include [GLOBAL]."
        assert "\n[MENU]\n" not in profile_text and "\n[DIRMAP]\n" not in profile_text, (
            "Missing-profile F10 bootstrap should no longer keep legacy command "
            "customization sections inside ytnova.conf."
        )
        assert f"EDITOR={unchanged_editor}" in profile_text, (
            "Missing-profile F10 bootstrap should seed from the active in-memory "
            "runtime profile so the current EDITOR survives into the created file."
        )
        assert profile_path.exists(), (
            "A successful F10 -> Enter edit of a missing profile must keep the starter profile."
        )
        assert profile_path.read_text(encoding="utf-8") == profile_text, (
            "An unchanged missing-profile edit must persist the starter profile verbatim."
        )
        assert ".jpg,.gif,.bmp,.tif,.ppm,.xpm=xv -" in profile_text, (
            "Runtime-seeded profiles should keep grouped VIEWER extension lists so "
            "the starter config stays tidy."
        )
        assert ".1,.2,.3,.4,.5,.6,.7,.8,.n=nroff -man - | less" in profile_text, (
            "Runtime-seeded profiles should keep grouped VIEWER manpage handlers "
            "instead of exploding every extension onto its own line."
        )
    finally:
        tui.quit()


def test_missing_themes_f10_unchanged_edit_keeps_starter_file(tmp_path, ytnova_binary):
    root = tmp_path / "missing_themes_f10_unchanged_edit"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    editor_capture = root / "f10_default_themes_snapshot.txt"
    unchanged_editor = root / "unchanged_themes_editor.sh"
    unchanged_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    unchanged_editor.chmod(0o755)

    themes_path = root / ".config" / "ytnova" / "themes.conf"

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"EDITOR": str(unchanged_editor)},
    )

    try:
        themes_path.unlink(missing_ok=True)
        assert not themes_path.exists()

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        tui.wait_for_condition(
            lambda _: editor_capture.exists(),
            timeout=2.0,
            description="editor_capture creation",
        )
        assert editor_capture.exists(), (
            "F10 -> Themes on a missing themes file must open an editable default themes buffer."
        )
        assert "[theme quiet-blue]" in editor_capture.read_text(encoding="utf-8"), (
            "Default themes buffer should include the compiled starter catalog."
        )
        assert themes_path.exists(), (
            "A successful F10 -> Themes edit of a missing file must keep the starter themes file."
        )
        assert themes_path.read_text(encoding="utf-8") == editor_capture.read_text(
            encoding="utf-8"
        ), (
            "An unchanged missing-themes edit must persist the starter themes catalog verbatim."
        )
    finally:
        tui.quit()


def test_missing_commands_f10_unchanged_edit_keeps_starter_file(
    tmp_path, ytnova_binary
):
    root = tmp_path / "missing_commands_f10_unchanged_edit"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    editor_capture = root / "f10_default_commands_snapshot.txt"
    unchanged_editor = root / "unchanged_commands_editor.sh"
    unchanged_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    unchanged_editor.chmod(0o755)

    commands_path = root / ".config" / "ytnova" / "commands.conf"

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"EDITOR": str(unchanged_editor)},
    )

    try:
        commands_path.unlink(missing_ok=True)
        assert not commands_path.exists()

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke("m", wait=0.2)

        tui.wait_for_condition(
            lambda _: editor_capture.exists(),
            timeout=2.0,
            description="editor_capture creation",
        )
        assert editor_capture.exists(), (
            "F10 -> Commands on a missing commands file must open an editable default commands buffer."
        )
        commands_text = editor_capture.read_text(encoding="utf-8")
        assert (
            "[DIR]" in commands_text
            and "[ARCHIVE_DIR]" in commands_text
            and "[FILE]" in commands_text
            and "[ARCHIVE_FILE]" in commands_text
        ), (
            "Default commands buffer should include the canonical commands.conf sections."
        )
        assert "#   preset = de" in commands_text, (
            "Default commands buffer should advertise the optional packaged preset selector."
        )
        assert "binding | shown | label | action | command" in commands_text, (
            "Default commands buffer should include the canonical per-section commands.conf columns."
        )
        assert "A | A | Attributes | ACTION_CMD_A |" in commands_text, (
            "Default commands buffer should include live built-in command rows."
        )
        assert commands_path.exists(), (
            "A successful F10 -> Commands edit of a missing file must keep the starter commands file."
        )
        assert commands_path.read_text(encoding="utf-8") == commands_text, (
            "An unchanged missing-commands edit must persist the starter commands catalog verbatim."
        )
    finally:
        tui.quit()


def test_legacy_six_column_commands_file_does_not_abort_startup(
    tmp_path, ytnova_binary
):
    root = tmp_path / "legacy_commands_startup"
    root.mkdir()
    work = root / "work"
    work.mkdir()
    (work / "file0.txt").write_text("x", encoding="utf-8")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "commands.conf").write_text(
        "dir | A | A | Attributes | ACTION_CMD_A |\n"
        "file | X | X | Execute | ACTION_CMD_X |\n",
        encoding="utf-8",
    )

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(work),
        env_extra={"HOME": str(root)},
    )

    try:
        assert tui.child.isalive(), "Legacy six-column commands.conf must not abort startup."
        screen_text = "\n".join(tui.get_screen_dump())
        assert "LoadCommands failed" not in screen_text
    finally:
        tui.quit()


def test_f10_themes_edits_active_home_dotfile_fallback(tmp_path, ytnova_binary):
    root = tmp_path / "f10_themes_home_fallback"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    edited_path_capture = root / "edited_themes_path.txt"
    editor_capture = root / "edited_themes_buffer.txt"
    touch_editor = root / "touch_fallback_themes_editor.sh"
    touch_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"printf '%s\\n' \"$f\" > \"{edited_path_capture}\"\n"
        "printf '\\n# edited by f10 themes\\n' >> \"$f\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    touch_editor.chmod(0o755)

    fallback_themes_path = root / ".ytnova.themes"
    fallback_themes_path.write_text(
        """
[theme quiet-blue]
background = blue
box_lines = cyan
tree_lines = +white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
footer = white
selection = black on white
dialog = white
picker = black on cyan
help = white
help_link = cyan
help_link_selection = yellow
info = +white
warning = black on yellow
error = +white on red
search_hit = black on yellow
""".strip()
        + "\n",
        encoding="utf-8",
    )
    xdg_themes_path = root / ".config" / "ytnova" / "themes.conf"
    xdg_themes_path.unlink(missing_ok=True)

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"EDITOR": str(touch_editor)},
    )

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke("t", wait=0.2)

        tui.wait_for_condition(
            lambda _: edited_path_capture.exists(),
            timeout=2.0,
            description="edited_path_capture creation",
        )

        assert edited_path_capture.exists(), "F10 -> Themes must invoke the editor."
        assert edited_path_capture.read_text(encoding="utf-8").strip() == str(
            fallback_themes_path
        ), (
            "F10 -> Themes must edit the active home-dotfile fallback theme file, "
            "not silently switch to XDG."
        )
        assert not xdg_themes_path.exists(), (
            "Editing an active home-dotfile fallback themes file must not create "
            "a parallel XDG themes authority."
        )
        assert "# edited by f10 themes" in fallback_themes_path.read_text(
            encoding="utf-8"
        ), "The active fallback themes file should receive the edit."
        assert editor_capture.exists(), "The editor should capture the edited buffer."
    finally:
        tui.quit()


def test_legacy_profile_f10_migrates_to_xdg_profile(tmp_path, ytnova_binary):
    root = tmp_path / "legacy_profile_f10_migrates_to_xdg"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    edited_path_capture = root / "f10_migrated_profile_path.txt"
    editor_capture = root / "f10_migrated_profile_snapshot.txt"
    migrate_editor = root / "migrate_profile_editor.sh"
    migrate_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"printf '%s\\n' \"$f\" > \"{edited_path_capture}\"\n"
        "printf '\\nSMALLWINDOWSKIP=1\\n' >> \"$f\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    migrate_editor.chmod(0o755)

    legacy_profile_path = root / ".ytnova"
    legacy_profile_path.write_text(
        "[GLOBAL]\n"
        "TREEDEPTH=7\n"
        f"EDITOR={migrate_editor}\n",
        encoding="utf-8",
    )
    profile_path = root / ".config" / "ytnova" / "ytnova.conf"
    assert not profile_path.exists()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        tui.wait_for_condition(
            lambda _: edited_path_capture.exists(),
            timeout=2.0,
            description="edited_path_capture creation",
        )

        assert edited_path_capture.exists(), "F10 -> Enter must invoke the editor."
        assert edited_path_capture.read_text(encoding="utf-8").strip() == str(
            profile_path
        ), "F10 config edit should migrate legacy ~/.ytnova editing to the XDG profile path."
        assert profile_path.exists(), "F10 config edit should create the XDG profile during migration."
        profile_text = profile_path.read_text(encoding="utf-8")
        assert "TREEDEPTH=7" in profile_text, (
            "Migrated XDG profile should preserve existing legacy profile settings.\n"
            f"Profile contents:\n{profile_text}"
        )
        assert f"EDITOR={migrate_editor}" in profile_text, (
            "Migrated XDG profile should be seeded from the active legacy profile, "
            "not from the packaged default template."
        )
        assert "SMALLWINDOWSKIP=1" in profile_text, (
            "Edits written through F10 should persist into the migrated XDG profile."
        )
    finally:
        tui.quit()


def test_removed_legacy_profile_f10_recreates_xdg_not_dotfile(
    tmp_path, ytnova_binary
):
    root = tmp_path / "removed_legacy_profile_f10_recreates_xdg"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    edited_path_capture = root / "f10_removed_legacy_profile_path.txt"
    save_editor = root / "removed_legacy_profile_editor.sh"
    save_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        f"printf '%s\\n' \"$f\" > \"{edited_path_capture}\"\n"
        "printf '\\nSMALLWINDOWSKIP=1\\n' >> \"$f\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    save_editor.chmod(0o755)

    legacy_profile_path = root / ".ytnova"
    legacy_profile_path.write_text(
        "[GLOBAL]\n"
        "TREEDEPTH=7\n"
        f"EDITOR={save_editor}\n",
        encoding="utf-8",
    )
    profile_path = root / ".config" / "ytnova" / "ytnova.conf"
    assert not profile_path.exists()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        legacy_profile_path.unlink()
        assert not legacy_profile_path.exists()

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        tui.wait_for_condition(
            lambda _: edited_path_capture.exists(),
            timeout=2.0,
            description="edited_path_capture creation",
        )

        assert edited_path_capture.exists(), "F10 -> Enter must invoke the editor."
        assert edited_path_capture.read_text(encoding="utf-8").strip() == str(
            profile_path
        ), "F10 config edit should recreate the preferred XDG profile, not ~/.ytnova."
        assert profile_path.exists(), "Saving after legacy-profile removal should create the XDG profile."
        assert not legacy_profile_path.exists(), (
            "F10 config edit should not recreate ~/.ytnova when the preferred "
            "XDG profile path is available."
        )
        profile_text = profile_path.read_text(encoding="utf-8")
        assert "# YtreeNova Defaults" in profile_text, (
            "Recreated XDG profiles should keep the commented starter template, "
            "not rewrite the file as a stripped key dump."
        )
        assert "\n[MENU]\n" not in profile_text and "\n[DIRMAP]\n" not in profile_text, (
            "Recreated XDG profiles should not restore legacy command "
            "customization sections into ytnova.conf."
        )
        assert "TREEDEPTH=7" in profile_text, (
            "When the legacy profile has been removed, F10 should seed the new "
            "XDG profile from the active in-memory runtime profile.\n"
            f"Profile contents:\n{profile_text}"
        )
        assert f"EDITOR={save_editor}" in profile_text, (
            "The recreated XDG profile should preserve the active in-memory "
            "EDITOR setting after the legacy file has been removed."
        )
    finally:
        tui.quit()


def test_default_history_save_uses_xdg_state_home(tmp_path, ytnova_binary):
    root = tmp_path / "default_history_uses_xdg_state_home"
    root.mkdir()
    (root / "file0.txt").write_text("x", encoding="utf-8")
    (root / ".ytnova-hst").write_text("0:0:legacy-history-entry\n", encoding="utf-8")
    state_home = root / ".statehome"
    history_path = state_home / "ytnova" / "ytnova.hst"

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"XDG_STATE_HOME": str(state_home)},
    )

    try:
        pass
    finally:
        _graceful_quit(tui)

    assert history_path.exists(), (
        "Default history should persist under $XDG_STATE_HOME/ytnova/ytnova.hst."
    )
    assert "legacy-history-entry" in history_path.read_text(encoding="utf-8")
    assert not (state_home / ".ytnova-hst").exists(), (
        "Default history save should not write the legacy filename under "
        "$XDG_STATE_HOME."
    )


def test_default_history_save_uses_local_state_fallback(tmp_path, ytnova_binary):
    root = tmp_path / "default_history_uses_local_state"
    root.mkdir()
    (root / "file0.txt").write_text("x", encoding="utf-8")
    (root / ".ytnova-hst").write_text("0:0:legacy-history-entry\n", encoding="utf-8")
    history_path = root / ".local" / "state" / "ytnova" / "ytnova.hst"

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        pass
    finally:
        _graceful_quit(tui)

    assert history_path.exists(), (
        "When XDG_STATE_HOME is unset, history should persist under "
        "~/.local/state/ytnova/ytnova.hst."
    )
    assert "legacy-history-entry" in history_path.read_text(encoding="utf-8")


def test_custom_history_path_from_h_overrides_default_state_path(
    tmp_path, ytnova_binary
):
    root = tmp_path / "custom_history_path_override"
    root.mkdir()
    (root / "file0.txt").write_text("x", encoding="utf-8")
    state_home = root / ".statehome"
    custom_history = root / "custom-history.hst"
    default_history = state_home / "ytnova" / "ytnova.hst"
    custom_history.write_text("0:0:custom-history-entry\n", encoding="utf-8")
    before_mtime = custom_history.stat().st_mtime_ns

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        args=["-h", str(custom_history)],
        env_extra={"XDG_STATE_HOME": str(state_home)},
    )

    _graceful_quit(tui)

    assert custom_history.exists(), (
        "An explicit -h history path must remain authoritative for quit-time saves."
    )
    assert "custom-history-entry" in custom_history.read_text(encoding="utf-8")
    assert custom_history.stat().st_mtime_ns > before_mtime, (
        "An explicit -h history file should be rewritten on quit rather than "
        "falling back to the default state path."
    )
    assert not default_history.exists(), (
        "An explicit -h history path must not also create the default XDG state file."
    )


def test_legacy_history_is_loaded_and_migrated_to_state_path(tmp_path, ytnova_binary):
    root = tmp_path / "legacy_history_migrates_to_state"
    root.mkdir()
    (root / "file0.txt").write_text("x", encoding="utf-8")
    legacy_history = root / ".ytnova-hst"
    state_history = root / ".local" / "state" / "ytnova" / "ytnova.hst"
    legacy_history.write_text("0:0:legacy-history-entry\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        pass
    finally:
        _graceful_quit(tui)

    assert state_history.exists(), (
        "Legacy history should migrate into the XDG state history path on quit."
    )
    assert "legacy-history-entry" in state_history.read_text(encoding="utf-8")


def test_missing_profile_f10_save_creates_profile(tmp_path, ytnova_binary):
    root = tmp_path / "missing_profile_f10_save"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")

    editor_capture = root / "f10_saved_buffer_snapshot.txt"
    save_editor = root / "save_profile_editor.sh"
    save_editor.write_text(
        "#!/bin/sh\n"
        "f=\"$1\"\n"
        "printf '\\nSMALLWINDOWSKIP=1\\n' >> \"$f\"\n"
        f"cp \"$f\" \"{editor_capture}\"\n"
        "exit 0\n",
        encoding="utf-8",
    )
    save_editor.chmod(0o755)

    profile_path = root / ".config" / "ytnova" / "ytnova.conf"
    assert not profile_path.exists()

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        env_extra={"EDITOR": str(save_editor)},
    )

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        tui.send_keystroke("\x1b[21~", wait=0.2)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        tui.wait_for_condition(
            lambda _: editor_capture.exists(),
            timeout=2.0,
            description="editor_capture creation",
        )
        assert editor_capture.exists(), (
            "F10 -> Enter on a missing profile must open an editable default profile buffer."
        )
        assert profile_path.exists(), "Saving the F10 -> Enter config edit must create a profile."
        assert "SMALLWINDOWSKIP=1" in profile_path.read_text(encoding="utf-8"), (
            "Saved F10 -> Enter missing-profile edit must persist into the profile."
        )
    finally:
        tui.quit()


def test_smallwindowskip_zero_enter_chain_is_small_then_big_then_tree(
    tmp_path, ytnova_binary
):
    root = tmp_path / "smallwindowskip_zero_enter_chain"
    root.mkdir()
    target = root / "target"
    target.mkdir()
    (target / "file0.txt").write_text("x", encoding="utf-8")
    (target / "file1.txt").write_text("x", encoding="utf-8")
    (root / ".ytnova").write_text("[GLOBAL]\nSMALLWINDOWSKIP=0\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)

        before_rows = tui.get_screen_dump()
        assert _has_tree_row_for_dir(before_rows, "target"), (
            "Precondition failed: tree row for selected directory was not visible.\n"
            f"{_screen_text(tui)}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        first_rows = tui.get_screen_dump()
        assert "file" in _footer_text(tui).lower(), (
            "First ENTER should move from tree to file mode.\n"
            f"{_screen_text(tui)}"
        )
        assert _has_tree_row_for_dir(first_rows, "target"), (
            "SMALLWINDOWSKIP=0 first ENTER must keep tree pane visible (small file window).\n"
            f"{_screen_text(tui)}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        second_rows = tui.get_screen_dump()
        assert "file" in _footer_text(tui).lower(), (
            "Second ENTER should remain in file mode (big window).\n"
            f"{_screen_text(tui)}"
        )
        assert not _has_tree_row_for_dir(second_rows, "target"), (
            "Second ENTER must switch to big file window and hide tree pane.\n"
            f"{_screen_text(tui)}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        footer_third = _footer_text(tui).lower()
        footer_third_lines = [line.strip().lower() for line in _footer_lines(tui)]
        assert footer_third_lines and footer_third_lines[0].startswith("dir"), (
            "Third ENTER should return to tree mode.\n"
            f"Footer:\n{footer_third}\n\nScreen:\n{_screen_text(tui)}"
        )
        assert "tree" in footer_third, (
            "Tree footer should be visible after returning from big file window.\n"
            f"Footer:\n{footer_third}\n\nScreen:\n{_screen_text(tui)}"
        )
    finally:
        tui.quit()


def test_logged_empty_vs_unlogged_labels(tmp_path, ytnova_binary):
    root = tmp_path / "empty_vs_unlogged_labels"
    root.mkdir()
    empty = root / "emptydir"
    empty.mkdir()
    (root / "probe.txt").write_text("probe", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    root_screen = tui.get_screen_dump()
    root_file_row = next((line for line in root_screen if "probe.txt" in line), "")
    assert root_file_row, "Expected root file row containing probe.txt."
    first_filename_col = root_file_row.find("probe.txt")
    assert first_filename_col >= 0, "Could not determine first filename column."

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke("+", wait=0.4)
    logged_lines = tui.get_screen_dump()
    logged_row = next((line for line in logged_lines if "No files" in line), "")
    assert logged_row, (
        "Logged empty directory should show 'No files' text.\n"
        f"Screen:\n{_screen_text(tui)}"
    )
    logged_col = logged_row.find("No files")
    assert logged_col == first_filename_col, (
        "Logged empty label must align with first filename column.\n"
        f"expected={first_filename_col}, actual={logged_col}\n"
        f"row={logged_row!r}"
    )

    tui.send_keystroke("-", wait=0.4)
    unlogged_lines = tui.get_screen_dump()
    unlogged_row = next((line for line in unlogged_lines if "Unlogged" in line), "")
    assert unlogged_row, (
        "Unlogged directory should show 'Unlogged' text."
    )
    unlogged_col = unlogged_row.find("Unlogged")
    assert unlogged_col == first_filename_col, (
        "Unlogged label must align with first filename column.\n"
        f"expected={first_filename_col}, actual={unlogged_col}\n"
        f"row={unlogged_row!r}"
    )

    tui.quit()


def test_small_window_tagged_symlink_and_empty_labels_share_name_column(
    tmp_path, ytnova_binary
):
    root = tmp_path / "small_window_symlink_alignment"
    root.mkdir()
    target = root / "check_xml_integrit"
    target.write_text("payload", encoding="utf-8")
    (root / "current").symlink_to(target.name)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        initial_lines = tui.get_screen_dump()
        file_row = next(
            (line for line in initial_lines if "check_xml_integrit" in line), ""
        )
        symlink_row = next((line for line in initial_lines if "@current" in line), "")
        assert file_row, "Expected untagged file row."
        assert symlink_row, "Expected untagged symlink row with '@' prefix."

        name_col = file_row.find("check_xml_integrit")
        symlink_col = symlink_row.find("@current")
        assert name_col >= 0 and symlink_col >= 0
        assert symlink_col == name_col, (
            "Untagged symlink label must start at the same file-name column.\n"
            f"name_col={name_col}, symlink_col={symlink_col}\n"
            f"file_row={file_row!r}\nsymlink_row={symlink_row!r}"
        )

        tui.send_keystroke(Keys.ENTER, wait=0.4)
        tui.send_keystroke("\x14", wait=0.5)  # Ctrl+T (tag all)

        tagged_lines = tui.get_screen_dump()
        tagged_file_row = next(
            (line for line in tagged_lines if "* check_xml_integrit" in line), ""
        )
        tagged_symlink_row = next(
            (line for line in tagged_lines if "* @current" in line), ""
        )
        assert tagged_file_row, "Expected tagged file row with '* ' prefix."
        assert tagged_symlink_row, "Expected tagged symlink row with '* @' prefix."

        tagged_name_col = tagged_file_row.find("check_xml_integrit")
        tagged_symlink_col = tagged_symlink_row.find("@current")
        assert tagged_name_col == name_col, (
            "Tagged file label must preserve the filename start column.\n"
            f"expected={name_col}, actual={tagged_name_col}\n"
            f"row={tagged_file_row!r}"
        )
        assert tagged_symlink_col == name_col, (
            "Tagged symlink label must preserve the filename start column.\n"
            f"expected={name_col}, actual={tagged_symlink_col}\n"
            f"row={tagged_symlink_row!r}"
        )
    finally:
        tui.quit()


def test_placeholder_dir_shows_unlogged_not_no_files(tmp_path, ytnova_binary):
    root = tmp_path / "placeholder_shows_unlogged"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    src = root / "src"
    src.mkdir()
    (src / "cmd").mkdir()
    (src / "cmd" / "main.c").write_text("int main(void){return 0;}\n",
                                         encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.4)
        screen = _screen_text(tui)
        assert "Unlogged" in screen, (
            "Unscanned placeholder directory should display Unlogged in file view.\n"
            f"{screen}"
        )
        assert "No files" not in screen, (
            "Placeholder/unlogged directory must not display No files.\n"
            f"{screen}"
        )
    finally:
        tui.quit()


def test_volume_menu_enter_on_current_volume_preserves_existing_state(
    tmp_path, ytnova_binary
):
    root = tmp_path / "volume_menu_preserve_current_state"
    root.mkdir()
    active_dir = root / "active_dir"
    stale_dir = root / "vol_old_name_dir"
    active_dir.mkdir()
    stale_dir.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before_rename = _screen_text(tui)
        assert "vol_old_name_dir" in before_rename, (
            "Precondition failed: expected original tree entry before rename.\n"
            f"{before_rename}"
        )

        stale_dir.rename(root / "vol_new_name_dir")

        before_relog = _screen_text(tui)
        assert "vol_old_name_dir" in before_relog, (
            "Tree should remain stale before explicit relog when focused on sibling.\n"
            f"{before_relog}"
        )

        tui.send_keystroke("K", wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.9)

        after_select = _screen_text(tui)
        assert "vol_old_name_dir" in after_select, (
            "Selecting the current volume in Volume Menu should preserve its "
            "existing in-memory state (no relog).\n"
            f"{after_select}"
        )
        assert "vol_new_name_dir" not in after_select, (
            "Volume Menu selection must not implicitly refresh/reload current "
            "volume state.\n"
            f"{after_select}"
        )
    finally:
        tui.quit()


def test_log_command_on_current_volume_reloads_tree_state(
    tmp_path, ytnova_binary
):
    root = tmp_path / "log_current_volume_refresh"
    root.mkdir()
    active_dir = root / "active_dir"
    stale_dir = root / "vol_old_name_dir"
    active_dir.mkdir()
    stale_dir.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    try:
        tui.send_keystroke(Keys.DOWN, wait=0.3)
        before_rename = _screen_text(tui)
        assert "vol_old_name_dir" in before_rename, (
            "Precondition failed: expected original tree entry before rename.\n"
            f"{before_rename}"
        )

        stale_dir.rename(root / "vol_new_name_dir")

        before_log = _screen_text(tui)
        assert "vol_old_name_dir" in before_log, (
            "Tree should remain stale before explicit relog command.\n"
            f"{before_log}"
        )

        tui.send_keystroke(Keys.LOG, wait=0.1)
        tui.send_keystroke(Keys.CTRL_U + str(root), wait=0)
        after_log_lines = tui.send_and_wait_for_condition(
            Keys.ENTER,
            lambda lines: lines
            if any("vol_new_name_dir" in line for line in lines)
            else False,
            timeout=3.0,
        )
        assert after_log_lines, "Logging the current volume did not refresh its tree."
        after_log = "\n".join(after_log_lines)
        assert "vol_new_name_dir" in after_log, (
            "Logging the current volume should refresh the tree from disk.\n"
            f"{after_log}"
        )
        assert "vol_old_name_dir" not in after_log, (
            "Relog should replace stale directory entries with current disk state.\n"
            f"{after_log}"
        )
        header = tui.get_screen_dump()[0]
        assert str(root) in header and str(root / "active_dir") not in header, (
            "Explicit relog should reanchor selection at volume root.\n"
            f"Header: {header!r}\n\nScreen:\n{after_log}"
        )
    finally:
        tui.quit()


def test_depth_limited_placeholder_plus_loads_leaf_files(tmp_path, ytnova_binary):
    root = tmp_path / "depth_limited_placeholder"
    root.mkdir()
    docs = root / "docs"
    docs.mkdir()
    ai = docs / "ai"
    ai.mkdir()
    (ai / "AGENT_PROMPT_TEMPLATE.md").write_text("prompt", encoding="utf-8")
    (ai / "DEBUGGING.md").write_text("debug", encoding="utf-8")
    (ai / "WORKFLOW.md").write_text("workflow", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    # root -> docs -> expand docs -> select docs/ai
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.4)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    header = tui.get_screen_dump()[0]
    assert "docs/ai" in header, (
        "Expected docs/ai to be selected before expansion.\n"
        f"Header: {header!r}\n\nScreen:\n{_screen_text(tui)}"
    )

    tui.send_keystroke("+", wait=0.6)
    screen = _screen_text(tui)
    assert "AGENT_PROMPT_TEMPLATE.md" in screen, (
        "Depth-limited placeholder should load visible files after '+'.\n"
        f"Screen:\n{screen}"
    )
    assert "DEBUGGING.md" in screen, (
        "Leaf directory should no longer remain an empty placeholder.\n"
        f"Screen:\n{screen}"
    )

    tui.quit()


def test_archive_dir_compare_and_exit_actions_are_available(tmp_path, ytnova_binary):
    root = tmp_path / "archive_dir_footer_compare"
    root.mkdir()

    archive_source = root / "_archive_src"
    archive_source.mkdir()
    (archive_source / "inside.txt").write_text("inside", encoding="utf-8")

    archive_path = root / "aa_dir_footer_test.tar"
    with tarfile.open(archive_path, "w") as tf:
        tf.add(archive_source, arcname="inside_dir")
    shutil.rmtree(archive_source)

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))

    # Enter file view first, then log the selected archive file.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke(Keys.LOG, wait=0.3)
    tui.send_keystroke(Keys.ENTER, wait=0.6)
    assert tui.wait_for_content("ARCHIVE", timeout=2.0), (
        "Expected archive mode after logging into tar file."
    )

    tui.send_keystroke("J", wait=0.3)
    assert tui.wait_for_content("COMPARE TARGET", timeout=1.0)
    tui.send_keystroke(Keys.ESC, wait=0.2)
    tui.send_keystroke("\\", wait=0.4)
    assert tui.wait_for_content(archive_path.name, timeout=1.0)

    tui.quit()
