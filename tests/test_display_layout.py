import pytest
import re
from itertools import repeat
import pexpect
import io
from helpers_stats import current_file_from_stats as _current_file_from_stats
from helpers_stats import detect_stats_split_x as _detect_stats_split_x
from helpers_ui import footer_lines as _footer_lines
from helpers_ui import footer_text as _footer_text
from helpers_ui import drive_action_until
from ytnova_keys import Keys
from tui_harness import YtreeNovaTUI


def _select_tree_header_marker(tui, marker, timeout=3.0):
    lines = drive_action_until(
        tui,
        Keys.DOWN,
        lambda dump: dump if marker in next(iter(dump), "") else False,
        max_actions=128,
        timeout=timeout,
    )
    if lines:
        return lines
    screen = "\n".join(tui.get_screen_dump())
    pytest.fail(f"Could not select {marker!r}.\n{screen}")

def get_clean_screen(yt):
    try:
        yt.child.expect(pexpect.TIMEOUT, timeout=0.2)
    except:
        pass
    raw = (yt.child.before if isinstance(yt.child.before, str) else "") + \
          (yt.child.after if isinstance(yt.child.after, str) else "")
    clean = re.sub(r'\x1B(?:\[[0-9;]*[a-zA-Z]|\(B|\[\?[0-9]*[a-zA-Z]|\**|[=>])?', '', raw)
    return clean

def sync_state(yt):
    yt.child.expect(r'20\d{2}')





def _file_index(name):
    if not name:
        return None
    m = re.search(r"a(\d{3})\.txt", name)
    if not m:
        return None
    return int(m.group(1))


def _move_to_file_index(tui, split_x, target_idx, key, timeout=3.0):
    selected = drive_action_until(
        tui,
        key,
        lambda lines: (
            lines
            if _file_index(_current_file_from_stats(lines, split_x)) == target_idx
            else False
        ),
        max_actions=128,
        timeout=timeout,
    )
    if selected is not False:
        return target_idx
    pytest.fail(f"Could not select file index {target_idx}.")














def test_file_detail_rows_do_not_wrap_attributes_into_next_line(ytnova_binary, tmp_path):
    """
    Regression guard:
    In narrow layouts, file-detail modes must clip row content instead of
    wrapping attributes/dates into the next visual line.
    """
    d = tmp_path / "file_detail_clip"
    d.mkdir()
    for i in range(12):
        (d / f"f{i:03d}.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(32, 84)
    tui.screen.resize(32, 84)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.ENTER, wait=0.5)

    lines = tui.get_screen_dump()
    stats_split_x = _detect_stats_split_x(lines)
    assert stats_split_x is not None, "Could not detect file/stats split border"

    # Rotate into a detail-heavy mode that shows dates on file rows.
    for _ in repeat(None, 6):
        screen = "\n".join(tui.get_screen_dump())
        if re.search(r"\d{4}-\d{2}-\d{2}", screen):
            break
        tui.send_and_wait_for_screen_change("\x06", timeout=1.0)  # C-f

    dump = tui.get_screen_dump()
    candidate_lines = [line[:stats_split_x] for line in dump[2:24]]
    date_no_name = [
        line for line in candidate_lines
        if re.search(r"\d{4}-\d{2}-\d{2}", line) and ".txt" not in line
    ]
    assert not date_no_name, (
        "Detail attributes/dates wrapped into continuation lines in narrow file view.\n"
        + "\n".join(candidate_lines)
    )

    tui.quit()




def test_global_repeat_key_is_noop_in_global_view(ytnova_binary, tmp_path):
    root = tmp_path / "global_toggle_repeat"
    root.mkdir()
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke("g", wait=0.5)
    tui.send_keystroke("g", wait=0.5)
    tui.send_keystroke("\\", wait=0.5)
    assert tui.wait_for_text(root.name, timeout=1.0)

    tui.quit()


def test_showall_repeat_key_sorts_in_showall_view(ytnova_binary, tmp_path):
    root = tmp_path / "showall_toggle_repeat"
    root.mkdir()
    (root / "a.txt").write_text("a", encoding="utf-8")
    (root / "b.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    tui.send_keystroke(Keys.ESC, wait=0.5)
    tui.send_keystroke("\\", wait=0.5)
    assert tui.wait_for_text(root.name, timeout=1.0)

    tui.quit()


def test_mutating_action_repeat_is_not_undo(ytnova_binary, tmp_path):
    root = tmp_path / "mkdir_repeat_action"
    root.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        for name in ("first_dir", "second_dir"):
            created = root / name
            assert tui.send_and_wait_for_screen_change("M", timeout=2.0)
            assert tui.send_and_wait_for_condition(
                name + "\r",
                lambda lines: lines if created.is_dir() else False,
                timeout=2.0,
            ), f"mkdir action did not create {name}"
    finally:
        tui.quit()

    assert (root / "first_dir").is_dir(), (
        "First mkdir action should persist after repeating the key."
    )
    assert (root / "second_dir").is_dir(), (
        "Second mkdir keypress must execute another create action, not undo."
    )


def test_showall_repeat_stays_in_showall_context(ytnova_binary, tmp_path):
    root = tmp_path / "showall_repeat_start_dir"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only.txt").write_text("a", encoding="utf-8")
    (beta / "beta_only.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    # Move from root to alpha in tree mode and remember this start context.
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    tui.send_keystroke("f", wait=0.2)
    tui.send_keystroke("beta_only.txt\r", wait=0.5)

    # Repeat S in showall: should sort, not leave showall mode.
    tui.send_keystroke(Keys.SHOWALL, wait=0.5)
    tui.send_keystroke(Keys.ESC, wait=0.5)
    screen = "\n".join(tui.get_screen_dump())
    assert "beta_only.txt" in screen

    tui.quit()


def test_global_repeat_stays_in_global_context(ytnova_binary, tmp_path):
    root = tmp_path / "global_repeat_start_dir"
    root.mkdir()
    alpha = root / "alpha"
    beta = root / "beta"
    alpha.mkdir()
    beta.mkdir()
    (alpha / "alpha_only.txt").write_text("a", encoding="utf-8")
    (beta / "beta_only.txt").write_text("b", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    # Move from root to alpha in tree mode and remember this start context.
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.send_keystroke("g", wait=0.5)
    tui.send_keystroke("f", wait=0.2)
    tui.send_keystroke("beta_only.txt\r", wait=0.5)

    # Repeat G in global-all-volumes mode: should be a no-op.
    tui.send_keystroke("g", wait=0.5)
    screen = "\n".join(tui.get_screen_dump())
    assert "beta_only.txt" in screen

    tui.quit()


@pytest.mark.parametrize("mode_key", [Keys.SHOWALL, "g"])
def test_backslash_to_dir_in_showall_and_global(ytnova_binary, tmp_path, mode_key):
    """
    REGRESSION:
    In Show All / Global file list modes, '\\' exits the mode and re-anchors
    the tree/file cursors to the selected file inside its owner directory.
    """
    root = tmp_path / "to_dir_mode"
    owner_dir = root / "owner_dir"
    other_dir = root / "other_dir"
    owner_dir.mkdir(parents=True)
    other_dir.mkdir(parents=True)

    target_name = "jump_target.txt"
    (owner_dir / target_name).write_text("x", encoding="utf-8")
    (other_dir / "other_file.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(mode_key, wait=0.6)

    # Select the target file deterministically via filter.
    tui.send_keystroke("f", wait=0.2)
    tui.send_keystroke(f"{target_name}\r", wait=0.6)
    screen = "\n".join(tui.get_screen_dump())
    assert target_name in screen, "Target file should be selected in global file list"

    lines = tui.send_and_wait_for_condition(
        "\\",
        lambda current: current
        if "DIR" in "\n".join(current)
        and owner_dir.name in "\n".join(current)
        and target_name in "\n".join(current)
        else False,
        timeout=2.0,
    )
    assert lines, "Expected to return to the selected file's directory"
    screen = "\n".join(lines)
    split_x = _detect_stats_split_x(lines)
    jumped_current = _current_file_from_stats(lines, split_x)
    assert "DIR" in screen, "Expected to return to tree mode after '\\'"
    assert owner_dir.name in screen, "Expected header/tree context to include owner directory"
    assert target_name in screen, "Expected file cursor to land on selected file in owner directory"
    assert (
        jumped_current == target_name
    ), f"Expected CURRENT FILE to stay on selected target after '\\' (got {jumped_current})"

    tui.quit()


def test_sort_prompt_cancels_to_file_view(ytnova_binary, tmp_path):
    """
    REGRESSION:
    Sort prompt must fully occupy the footer area in file mode. The previous
    file footer row must not bleed through above SORT/COMMANDS lines.
    """
    d = tmp_path / "sort_footer_bleed"
    d.mkdir()
    (d / "b.txt").write_text("x", encoding="utf-8")
    (d / "a.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(d.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    # Enter file mode and open sort prompt.
    tui.send_keystroke(Keys.ENTER, wait=0.4)
    tui.send_keystroke("s", wait=0.4)

    tui.send_keystroke(Keys.ESC, wait=0.2)
    assert tui.wait_for_text("a.txt", timeout=1.0)
    tui.quit()


@pytest.mark.parametrize(
    "action_key,new_name",
    [
        ("c", "dir_copy_out"),
        ("v", "dir_move_out"),
    ],
)
def test_dir_copy_move_keeps_full_frame_after_command(
    ytnova_binary, tmp_path, action_key, new_name
):
    root = tmp_path / "dir_ops_frame"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    # Select src_dir (first child of logged root in this fixture).
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    screen = "\n".join(tui.get_screen_dump())
    assert "src_dir" in screen

    tui.child.send(action_key)
    tui.child.expect("COPY:|MOVE:")
    tui.child.send("\x15")
    tui.child.send(f"{new_name}\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    raw_output = io.StringIO()
    tui.child.logfile_read = raw_output
    tui.child.send(".\r")
    out_dir = root / new_name
    payload = out_dir / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: payload if payload.exists() else False,
        timeout=2.0,
    )
    assert created, "\n".join(tui.get_screen_dump())
    command_output = raw_output.getvalue()
    tui.child.logfile_read = None
    assert "\x1b[?1049l" not in command_output, command_output
    assert "\x1b[?1049h" not in command_output, command_output
    assert "\x1b[H\x1b[2J" not in command_output, command_output
    assert payload.exists()
    if action_key == "v":
        assert not src.exists()

    if action_key == "c":
        post = "\n".join(tui.wait_for_text(new_name, timeout=2.0))
        assert new_name in post, (
            "Directory copy did not keep the new directory visible in-session.\n"
            f"{post}"
        )

    tui.quit()


def test_dir_copy_to_missing_destination_decline_reopens_prompt_without_frame_corruption(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_copy_missing_dest_no"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("./new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("N")

    tui.send_keystroke("", wait=0.2)

    assert not (root / "new_parent").exists()
    assert (root / "src_dir" / "nested" / "payload.txt").exists()

    tui.quit()


def test_dir_copy_to_missing_destination_create_yes_copies_and_restores_frame(
    ytnova_binary, tmp_path
):
    root = tmp_path / "dir_copy_missing_dest_yes"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("./new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy did not complete after confirming create"

    tui.quit()


def test_dir_copy_prompt_shows_source_and_as_target(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_prompt_as"
    root.mkdir()
    src = root / "src_dir"
    src.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.child.send("c")
    tui.child.expect(r"COPY:\s+src_dir\s+AS:", timeout=2.0)
    tui.child.send(Keys.ESC)
    tui.send_keystroke("", wait=0.2)
    tui.quit()


def test_file_move_prompt_shows_source_and_as_target(ytnova_binary, tmp_path):
    root = tmp_path / "file_move_prompt_as"
    root.mkdir()
    (root / "src.txt").write_text("payload\n", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text(root.name, timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.ENTER, wait=0.3)
    tui.child.send("m")
    tui.child.expect(r"MOVE:\s+src\.txt\s+AS:", timeout=2.0)
    tui.child.send(Keys.ESC)
    tui.send_keystroke("", wait=0.2)
    tui.quit()


def test_dir_copy_refreshes_destination_branch_without_relog(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_cross_branch_refresh"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    source_bucket = root / "source_bucket"
    target_bucket = root / "target_bucket"
    source_bucket.mkdir()
    target_bucket.mkdir()
    src = source_bucket / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text("source_bucket", timeout=2.0), "\n".join(tui.get_screen_dump())

    # root -> source_bucket -> src_dir
    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.6)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("../target_bucket/new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "target_bucket" / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy did not complete"

    # Move to target_bucket and verify the copied branch is visible immediately.
    _select_tree_header_marker(tui, "target_bucket")

    tui.send_keystroke(Keys.ENTER, wait=0.6)
    after_target_enter = "\n".join(tui.get_screen_dump())
    assert "new_parent" in after_target_enter, (
        "Created destination directory is not visible in-session after copy "
        "(requires relog today).\n"
        f"{after_target_enter}"
    )

    _select_tree_header_marker(tui, "new_parent")

    tui.send_keystroke(Keys.ENTER, wait=0.6)
    after_new_parent_enter = "\n".join(tui.get_screen_dump())
    assert "copied_src" in after_new_parent_enter, (
        "Copied subtree is not visible immediately after destination creation.\n"
        f"{after_new_parent_enter}"
    )

    tui.quit()


def test_dir_copy_delete_created_destination_updates_in_session(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_delete_created_destination"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    source_bucket = root / "source_bucket"
    target_bucket = root / "target_bucket"
    source_bucket.mkdir()
    target_bucket.mkdir()
    src = source_bucket / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text("source_bucket", timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.6)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send("../target_bucket/new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "target_bucket" / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy did not complete"

    # Navigate to created destination directory and delete it.
    _select_tree_header_marker(tui, "target_bucket")
    tui.send_keystroke(Keys.ENTER, wait=0.6)

    _select_tree_header_marker(tui, "new_parent")

    tui.child.send(Keys.DELETE)
    tui.child.expect(r"(Delete this directory|PRUNE)", timeout=2.0)
    tui.child.send("Y")
    tui.send_keystroke("", wait=0.8)

    assert not (root / "target_bucket" / "new_parent").exists(), (
        "Deleting the created destination directory had no filesystem effect."
    )
    after_delete = "\n".join(tui.get_screen_dump())
    assert "new_parent" not in after_delete, (
        "Created destination directory still rendered after in-session delete.\n"
        f"{after_delete}"
    )

    tui.quit()


def test_dir_copy_absolute_destination_refreshes_without_relog(ytnova_binary, tmp_path):
    root = tmp_path / "dir_copy_absolute_destination_refresh"
    root.mkdir()
    (root / ".ytnova").write_text("[GLOBAL]\nTREEDEPTH=1\n", encoding="utf-8")
    source_bucket = root / "source_bucket"
    target_bucket = root / "target_bucket"
    source_bucket.mkdir()
    target_bucket.mkdir()
    src = source_bucket / "src_dir"
    src.mkdir()
    (src / "nested").mkdir()
    (src / "nested" / "payload.txt").write_text("x", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.child.setwinsize(36, 140)
    tui.screen.resize(36, 140)
    assert tui.wait_for_text("source_bucket", timeout=2.0), "\n".join(tui.get_screen_dump())

    tui.send_keystroke(Keys.DOWN, wait=0.3)
    tui.send_keystroke(Keys.RIGHT, wait=0.6)
    tui.send_keystroke(Keys.DOWN, wait=0.3)

    tui.child.send("c")
    tui.child.expect("COPY:")
    tui.child.send("\x15")
    tui.child.send("copied_src\r")
    tui.child.expect("To Directory")
    tui.child.send("\x15")
    tui.child.send(f"{target_bucket}/new_parent\r")
    tui.child.expect("Create missing directory\\?", timeout=2.0)
    tui.child.send("Y")

    copied = root / "target_bucket" / "new_parent" / "copied_src" / "nested" / "payload.txt"
    created = tui.wait_for_condition(
        lambda _screen: copied if copied.exists() else False,
        timeout=2.0,
    )
    assert created, "Directory copy to absolute destination did not complete"

    _select_tree_header_marker(tui, "target_bucket")
    tui.send_keystroke(Keys.ENTER, wait=0.6)
    after_target_enter = "\n".join(tui.get_screen_dump())
    assert "new_parent" in after_target_enter, (
        "Absolute-path destination not visible in-session after copy.\n"
        f"{after_target_enter}"
    )

    tui.quit()
