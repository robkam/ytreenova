import pytest
from pathlib import Path
import re
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


VERTICAL_CHARS = {"x", "|"}
BORDER_MIN_Y = 2
BORDER_MAX_Y = 31


@pytest.fixture
def f7_preview_sandbox(tmp_path):
    """
    Build a dedicated tree for F7 preview assertions.

    Layout goal:
    - Root contains exactly one directory so one DOWN reliably selects it.
    - Preview directory contains one extremely long filename to test clipping.
    - File content contains a stable marker so we can assert we are in preview.
    """
    root = tmp_path / "f7_preview_root"
    root.mkdir()

    preview_dir = root / "preview_dir"
    preview_dir.mkdir()

    long_name = (
        "very_long_filename_for_f7_preview_boundary_clipping_validation_"
        "1234567890abcdefghijklmnopqrstuvwxyz.txt"
    )
    marker = "F7_PREVIEW_MARKER_LINE_001"

    (preview_dir / long_name).write_text(
        marker + "\n" + "line_2_for_preview\n",
        encoding="utf-8",
    )
    (preview_dir / "z_secondary_file.txt").write_text(
        "secondary_file_content\n",
        encoding="utf-8",
    )

    return {
        "root": root,
        "long_name": long_name,
        "long_name_token": long_name[:24],
        "marker": marker,
    }


def _screen_text(lines):
    return "\n".join(lines)




def _launch_preview(ytnova_binary, sandbox_info):
    """
    Enter F7 preview from tree mode using project-standard keystrokes.
    """
    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(sandbox_info["root"]))

    assert tui.wait_for_content("preview_dir", timeout=2.0)
    tui.child.send(Keys.EXPAND_ALL)
    assert tui.send_and_wait_for_condition(
        Keys.DOWN,
        lambda current: current
        if sandbox_info["long_name_token"] in _screen_text(current)
        else False,
        timeout=2.0,
    ), "Preview directory did not finish loading its fixture file."
    lines = tui.send_and_wait_for_condition(
        Keys.F7,
        lambda current: current
        if sandbox_info["marker"] in _screen_text(current)
        else False,
        timeout=2.0,
    )
    if not lines:
        lines = tui.get_screen_dump()
    screen = _screen_text(lines)
    if sandbox_info["marker"] not in screen:
        tui.quit()
        pytest.fail(
            "Failed to enter stable F7 preview state.\n"
            f"Expected marker '{sandbox_info['marker']}' not found.\n"
            f"Screen:\n{screen}"
        )

    return tui






def test_f7_blocks_split_and_tab_but_allows_copy_prompt(
    f7_preview_sandbox, ytnova_binary
):
    """
    Required test 3:
    Preview must still block split-mode entry, but common file actions such as
    Copy should remain usable without leaving preview.
    """
    tui = _launch_preview(ytnova_binary, f7_preview_sandbox)
    preview_lines = tui.get_screen_dump()
    preview_screen = _screen_text(preview_lines)

    tui.send_keystroke(Keys.TAB, wait=0.25)

    tab_lines = tui.get_screen_dump()
    tab_screen = _screen_text(tab_lines)
    if tab_lines[1:] != preview_lines[1:]:
        tui.quit()
        pytest.fail(
            "Tab should remain a no-op while F7 preview is active.\n"
            f"Before:\n{preview_screen}\n\nAfter:\n{tab_screen}"
        )

    tui.send_keystroke(Keys.F8, wait=0.25)

    lines = tui.get_screen_dump()
    screen = _screen_text(lines)
    upper = screen.upper()

    if f7_preview_sandbox["marker"] not in screen:
        tui.quit()
        pytest.fail(
            "Preview content marker disappeared after blocked split input.\n"
            f"Screen:\n{screen}"
        )

    if "SPLIT" in upper and "SCREEN" in upper:
        tui.quit()
        pytest.fail(
            "F8 should remain blocked while F7 preview is active.\n"
            f"Screen:\n{screen}"
        )

    tui.send_keystroke(Keys.COPY, wait=0.25)
    copy_screen = _screen_text(tui.get_screen_dump())
    if "COPY:" not in copy_screen.upper():
        tui.quit()
        pytest.fail(
            "F7 preview should allow Copy without forcing an exit first.\n"
            f"Screen:\n{copy_screen}"
        )

    tui.send_keystroke(Keys.ESC, wait=0.25)
    restored_screen = _screen_text(tui.get_screen_dump())
    if f7_preview_sandbox["marker"] not in restored_screen:
        tui.quit()
        pytest.fail(
            "Cancelling Copy from F7 preview should return to the preview surface.\n"
            f"Screen:\n{restored_screen}"
        )

    tui.quit()


def test_f7_preview_action_filter_keeps_tagged_workflow_and_blocks_panel_switch():
    source = Path("src/ui/ctrl_file.c").read_text(encoding="utf-8")
    match = re.search(
        r"static YtreeNovaAction FilterPreviewAction\(YtreeNovaAction action\) \{"
        r"(?P<body>.*?)\n\}",
        source,
        re.S,
    )
    assert match is not None
    filter_body = match.group("body")

    required_actions = (
        "ACTION_CMD_C",
        "ACTION_FILTER",
        "ACTION_TAG_ALL",
        "ACTION_CMD_TAGGED_S",
        "ACTION_CMD_TAGGED_V",
        "ACTION_COMPARE_FILE",
        "ACTION_CMD_M",
        "ACTION_CMD_R",
    )

    for action in required_actions:
        assert action in filter_body, action

    forbidden_actions = ("ACTION_SWITCH_PANEL", "ACTION_MOVE_SIBLING_NEXT")
    for action in forbidden_actions:
        assert action not in filter_body, action


def test_f7_preview_search_highlight_contract_uses_tagged_matches():
    preview_source = Path("src/ui/view_preview.c").read_text(encoding="utf-8")
    tagged_view_source = Path("src/ui/tagged_view.c").read_text(encoding="utf-8")

    assert "ctx->global_search_term[0] != '\\0'" in preview_source
    assert "strcasestr(ptr, ctx->global_search_term)" in preview_source
    assert "if (fe->tagged && fe->matching)" in tagged_view_source
    assert "if (!(fe->tagged && fe->matching))" in tagged_view_source
