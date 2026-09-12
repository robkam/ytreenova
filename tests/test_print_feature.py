import os
import tempfile

from ytnova_control import YtreeNovaController
from ytnova_keys import Keys


def _binary_path() -> str:
    return os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "build",
        "ytnova",
    )


def _spawn_controller(cwd: str) -> YtreeNovaController:
    ytnova = YtreeNovaController(_binary_path(), cwd)
    ytnova.wait_for_startup()
    return ytnova


def _enter_file_mode(ytnova: YtreeNovaController) -> None:
    ytnova.child.send(Keys.ENTER)
    ytnova.wait_for_refresh()
    ytnova.wait_for_refresh()


def _wait_for_screen_text(ytnova: YtreeNovaController, text: str, timeout: float = 2.0):
    lines = ytnova.wait_for_text(text, timeout=timeout)
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _assert_directory_frame_restored(lines):
    dump = "\n".join(lines)
    assert any(line.startswith("l") and "FILTER" in line for line in lines), dump
    assert sum(1 for line in lines if line.startswith("x")) >= 8, dump
    assert any(line.startswith("m") and "j" in line for line in lines), dump


def _open_output_flow(ytnova: YtreeNovaController, key: str = "o"):
    lines = ytnova.send_and_wait_for_condition(
        key,
        lambda screen: screen
        if any("Output to:" in line and "Hardcopy" in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _choose_output_route(ytnova: YtreeNovaController, route_key: str, prompt_text: str):
    lines = ytnova.send_and_wait_for_condition(
        route_key,
        lambda screen: screen
        if any(prompt_text in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def _cycle_output_format(ytnova: YtreeNovaController, prompt_text: str):
    lines = ytnova.send_and_wait_for_condition(
        Keys.F3,
        lambda screen: screen
        if any(prompt_text in line for line in screen)
        else False,
        timeout=2.0,
    )
    assert lines, "\n".join(ytnova.get_screen_dump())
    return lines


def test_output_flow_uses_o_key_and_explicit_file_and_hardcopy_prompts():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("explicit-output-prompts\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            _open_output_flow(ytnova, "o")

            _choose_output_route(ytnova, "F", "Output file [Raw]")

            output_path = os.path.join(td, "prompt_check.txt")
            ytnova.input_text(output_path)
            assert os.path.exists(output_path)

            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "H", "Printer command:")
        finally:
            ytnova.quit()


def test_stale_output_commands_conf_does_not_abort_startup():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("stale-output-commands-conf\n")

        config_dir = os.path.join(td, ".config", "ytnova")
        os.makedirs(config_dir, exist_ok=True)
        with open(os.path.join(config_dir, "commands.conf"), "w", encoding="utf-8") as handle:
            handle.write(
                "[DIR]\n"
                "A | A | Attributes | ACTION_CMD_A |\n"
                "C | C | Copy | ACTION_CMD_C |\n"
                "D | D | Delete | ACTION_CMD_D |\n"
                "F | F | Filter | ACTION_FILTER |\n"
                "G | G | Global | ACTION_CMD_G |\n"
                "I | I | Invert | ACTION_INVERT |\n"
                "J | J | Compare | ACTION_COMPARE_DIR |\n"
                "L | L | Log | ACTION_LOG |\n"
                "M | M | Makedir | ACTION_CMD_M |\n"
                "N | N | Newfile | ACTION_CMD_MKFILE |\n"
                "O | O | Only tagged | ACTION_TOGGLE_TAGGED_MODE |\n"
                "P | P | Pipe | ACTION_CMD_P |\n"
                "Q | Q | Quit | ACTION_QUIT |\n"
                "R | R | Rename | ACTION_CMD_R |\n"
                "S | S | Showall | ACTION_CMD_S |\n"
                "T | T | Tag | ACTION_TAG |\n"
                "U | U | Untag | ACTION_UNTAG |\n"
                "V | V | Movedir | ACTION_CMD_V |\n"
                "W | W | Write | ACTION_CMD_PRINT |\n"
                "X | X | Execute | ACTION_CMD_X |\n"
                "Z | Z | Archive | ACTION_CMD_I |\n"
                "/ | / | Jump | ACTION_LIST_JUMP |\n"
                "` | ` | Dotfiles | ACTION_TOGGLE_HIDDEN |\n"
                "\n"
                "[FILE]\n"
                "A | A | Attributes | ACTION_CMD_A |\n"
                "C | C | Copy | ACTION_CMD_C |\n"
                "Ctrl+C | ^C | Copy tagged | ACTION_CMD_TAGGED_C |\n"
                "D | D | Delete | ACTION_CMD_D |\n"
                "E | E | Edit | ACTION_CMD_E |\n"
                "F | F | Filter | ACTION_FILTER |\n"
                "H | H | Hex | ACTION_CMD_H |\n"
                "I | I | Invert | ACTION_INVERT |\n"
                "J | J | Compare | ACTION_COMPARE_FILE |\n"
                "L | L | Log | ACTION_LOG |\n"
                "M | M | Move | ACTION_CMD_M |\n"
                "Ctrl+N | ^N | Move tagged | ACTION_CMD_TAGGED_M |\n"
                "N | N | Newfile | ACTION_CMD_MKFILE |\n"
                "O | O | Only tagged | ACTION_TOGGLE_TAGGED_MODE |\n"
                "P | P | Pipe | ACTION_CMD_P |\n"
                "Q | Q | Quit | ACTION_QUIT |\n"
                "R | R | Rename | ACTION_CMD_R |\n"
                "S | S | Sort | ACTION_CMD_S |\n"
                "W | W | Write | ACTION_CMD_PRINT |\n"
                "X | X | Execute | ACTION_CMD_X |\n"
                "Y | Y | Pathcopy | ACTION_CMD_Y |\n"
                "Z | Z | Archive | ACTION_CMD_I |\n"
                "/ | / | Jump | ACTION_LIST_JUMP |\n"
                "` | ` | Dotfiles | ACTION_TOGGLE_HIDDEN |\n"
            )

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            route_screen = "\n".join(_open_output_flow(ytnova, "o"))
            assert "Output to:" in route_screen, route_screen
            assert "Write" not in route_screen, route_screen
            assert "Only tagged" not in route_screen, route_screen
        finally:
            ytnova.quit()


def test_output_plain_path_defaults_to_file_destination():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("plain-path-default\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            out_path = os.path.join(td, "plain_destination.txt")
            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "F", "Output file [Raw]")
            ytnova.input_text(out_path)

            assert os.path.exists(out_path)
            with open(out_path, "r", encoding="utf-8") as handle:
                assert "plain-path-default" in handle.read()

            alias_out_path = os.path.join(td, "legacy_alias_destination.txt")
            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "H", "Printer command:")
            ytnova.input_text(f">{alias_out_path}")

            assert os.path.exists(alias_out_path)
            with open(alias_out_path, "r", encoding="utf-8") as handle:
                assert "plain-path-default" in handle.read()
        finally:
            ytnova.quit()


def test_framed_output_uses_multiline_fence_around_file_content():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("hello\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            output_path = os.path.join(td, "framed_output.txt")
            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "F", "Output file [Raw]")
            _cycle_output_format(ytnova, "Frame separator")
            ytnova.child.send(Keys.ENTER)
            _wait_for_screen_text(ytnova, "Output file [Framed]")
            ytnova.input_text(output_path)

            with open(output_path, "r", encoding="utf-8") as handle:
                assert handle.read() == "source.txt\n```\n\nhello\n\n```\n\n"
        finally:
            ytnova.quit()


def test_directory_output_framed_default_separator_writes_listing_without_crash():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("directory-output-source\n")

        ytnova = _spawn_controller(td)
        try:
            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "F", "Output file [Raw]")
            _cycle_output_format(ytnova, "Frame separator")
            ytnova.child.send(Keys.ENTER)
            _wait_for_screen_text(ytnova, "Output file [Framed]")

            output_path = os.path.join(td, "directory_listing.txt")
            ytnova.input_text(output_path)
            ytnova._read_output(timeout=0.5)
            _assert_directory_frame_restored(ytnova.get_screen_dump())

            assert ytnova.child.isalive(), "directory-tree output crashed"
            assert os.path.exists(output_path)
            with open(output_path, "r", encoding="utf-8") as handle:
                output_text = handle.read()
            assert "source.txt" in output_text
        finally:
            ytnova.quit()


def test_tagged_output_shortcut_reuses_output_flow():
    with tempfile.TemporaryDirectory() as td:
        with open(os.path.join(td, "alpha.txt"), "w", encoding="utf-8") as handle:
            handle.write("alpha-tagged-output\n")
        with open(os.path.join(td, "beta.txt"), "w", encoding="utf-8") as handle:
            handle.write("beta-tagged-output\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)
            ytnova.child.send("t")
            ytnova.wait_for_refresh()
            ytnova.child.send(Keys.DOWN)
            ytnova.wait_for_refresh()
            ytnova.child.send("t")
            ytnova.wait_for_refresh()

            output_path = os.path.join(td, "tagged_output.txt")
            _open_output_flow(ytnova, Keys.CTRL_O)
            _choose_output_route(ytnova, "F", "Output file [Raw]")
            ytnova.input_text(output_path)

            assert os.path.exists(output_path)
            with open(output_path, "r", encoding="utf-8") as handle:
                output_text = handle.read()
            assert "alpha-tagged-output" in output_text
            assert "beta-tagged-output" in output_text
        finally:
            ytnova.quit()


def test_preview_output_refreshes_file_list_with_new_destination():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("preview-output\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)
            ytnova.child.send(Keys.F7)
            _wait_for_screen_text(ytnova, "exit preview")

            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "F", "Output file [Raw]")
            ytnova.input_text("preview_export.txt")

            assert os.path.exists(os.path.join(td, "preview_export.txt"))
            _wait_for_screen_text(ytnova, "preview_export.txt", timeout=3.0)
        finally:
            ytnova.quit()


def test_output_hardcopy_failure_shows_error_without_crash():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("command-failure\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "H", "Printer command:")
            ytnova.input_text("false")

            _wait_for_screen_text(ytnova, "execution of command", timeout=5.0)
            assert ytnova.child.isalive(), "ytnova crashed after hardcopy failure"
        finally:
            ytnova.quit()


def test_page_break_prompt_still_uses_distinct_separator_prompt():
    with tempfile.TemporaryDirectory() as td:
        source_path = os.path.join(td, "source.txt")
        with open(source_path, "w", encoding="utf-8") as handle:
            handle.write("page-break-prompt\n")

        ytnova = _spawn_controller(td)
        try:
            _enter_file_mode(ytnova)

            _open_output_flow(ytnova, "o")
            _choose_output_route(ytnova, "F", "Output file [Raw]")
            _cycle_output_format(ytnova, "Frame separator")
            ytnova.child.send(Keys.ENTER)
            _wait_for_screen_text(ytnova, "Output file [Framed]")

            _cycle_output_format(ytnova, "Page break separator")

            ytnova.input_text("---SEP---")
            _wait_for_screen_text(ytnova, "Output file [Page break]")

            output_path = os.path.join(td, "page_break_output.txt")
            ytnova.input_text(output_path)
            assert os.path.exists(output_path)
        finally:
            ytnova.quit()
