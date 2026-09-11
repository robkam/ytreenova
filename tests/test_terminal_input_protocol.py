import os
import shlex
import signal
import string
import subprocess

import pexpect
import pytest

from helpers_ui import assert_file_tag_state, footer_lines
from tui_harness import YtreeNovaTUI


PROBE_REQUEST = b"\x1b[>1u\x1b[?u\x1b[c"
POP_REQUEST = b"\x1b[<u"


@pytest.fixture
def terminal_input_driver(tmp_path_factory):
    output = tmp_path_factory.mktemp("terminal-input") / "terminal-input-driver"
    subprocess.run(
        [
            *shlex.split(os.environ.get("CC", "cc")),
            "-std=c99",
            "-D_GNU_SOURCE",
            "-Iinclude",
            "tests/terminal_input_driver.c",
            "src/ui/terminal_input.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(output),
        ],
        check=True,
    )
    return output


def _spawn(driver, mode, key_count):
    env = {"TERM": "xterm", "LC_ALL": "C.UTF-8"}
    return pexpect.spawn(
        str(driver),
        [mode, str(key_count)],
        env=env,
        encoding=None,
        timeout=3,
        dimensions=(24, 80),
    )


def _finish_probe(child, expected):
    child.expect_exact(expected)
    child.expect(pexpect.EOF)
    child.close()
    assert child.exitstatus == 0


def test_probe_confirms_requested_flag_and_preserves_interleaved_input(
    terminal_input_driver,
):
    child = _spawn(terminal_input_driver, "probe", 2)
    child.expect_exact(PROBE_REQUEST)
    child.send(b"x\x1b[?1uY\x1b[?1;2c")
    _finish_probe(child, b"CAP=2 ACTIVE=1 KEYS=120,89")


def test_queued_user_input_preempts_negotiation_without_delay_or_loss(
    terminal_input_driver,
):
    child = _spawn(terminal_input_driver, "queued-probe", 0)
    child.expect(pexpect.EOF)
    assert PROBE_REQUEST not in child.before
    assert POP_REQUEST not in child.before
    assert b"CAP=1 KEY=120" in child.before


def test_probe_falls_back_when_requested_flag_is_unsupported(terminal_input_driver):
    child = _spawn(terminal_input_driver, "probe", 1)
    child.expect_exact(PROBE_REQUEST)
    child.send(b"\x1b[?0uZ\x1b[?1;2c")
    child.expect_exact(POP_REQUEST)
    _finish_probe(child, b"CAP=1 ACTIVE=0 KEYS=90")


@pytest.mark.parametrize(
    ("response", "expected_keys"),
    [
        (b"\x1b[?xu\x1b[?1;2c", b"27,91,63,120,117"),
        (b"\x1b[?1", b"27,91,63,49"),
    ],
)
def test_probe_falls_back_without_losing_malformed_or_truncated_input(
    terminal_input_driver, response, expected_keys
):
    key_count = expected_keys.count(b",") + 1
    child = _spawn(terminal_input_driver, "probe", key_count)
    child.expect_exact(PROBE_REQUEST)
    child.send(response)
    _finish_probe(child, b"CAP=1 ACTIVE=0 KEYS=" + expected_keys)


def test_legacy_session_reprobes_after_external_handoff(terminal_input_driver):
    child = _spawn(terminal_input_driver, "lifecycle", 0)
    child.expect_exact(PROBE_REQUEST)
    child.send(b"\x1b[?0u\x1b[?1;2c")
    child.expect_exact(b"SUSPENDED CAP=1 RESULT=1")
    child.expect_exact(PROBE_REQUEST)
    child.send(b"\x1b[?1;2c\x1b[?1u")
    _finish_probe(child, b"RESUMED CAP=2 ACTIVE=1")


def test_every_control_letter_uses_the_shared_enhanced_decoder(terminal_input_driver):
    child = _spawn(terminal_input_driver, "decode", 26)
    child.expect_exact(b"READY\n")
    encoded = b"".join(
        f"\x1b[{ord(letter)};5u".encode("ascii") for letter in string.ascii_lowercase
    )
    child.send(encoded)

    expected = list(range(1, 27))
    expected[12] = 0x110002
    child.expect_exact(("KEYS=" + ",".join(map(str, expected))).encode("ascii"))
    child.expect(pexpect.EOF)
    assert child.exitstatus == 0


def test_polled_control_sequence_preserves_tab_identity(
    terminal_input_driver,
):
    child = _spawn(terminal_input_driver, "unread", 0)
    child.expect_exact(b"READY\n")
    child.send(b"\x1b[105;5u")
    child.expect_exact(b"KEYS=9,9")
    child.expect(pexpect.EOF)


def test_unsupported_modifier_sequence_is_preserved_without_ctrl_dispatch(
    terminal_input_driver,
):
    child = _spawn(terminal_input_driver, "decode", 8)
    child.expect_exact(b"READY\n")
    child.send(b"\x1b[109;7u")
    child.expect_exact(b"KEYS=27,91,49,48,57,59,55,117")
    child.expect(pexpect.EOF)


def _footer_text(tui):
    return " ".join(footer_lines(tui)).lower()


def test_protocol_bindings_dispatch_and_match_the_footer(ytnova_binary, tmp_path):
    root = tmp_path / "enhanced-bindings"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")
    (root / "two.txt").write_text("two", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), dimensions=(40, 240)
    )
    try:
        assert tui.keyboard_probe_count == 1
        assert_file_tag_state(tui, "one.txt", False)
        assert_file_tag_state(tui, "two.txt", False)

        tui.send_keystroke("i")
        assert_file_tag_state(tui, "one.txt", True)
        assert_file_tag_state(tui, "two.txt", True)

        assert tui.send_and_wait_for_screen_change("\r", timeout=2.0)
        footer = _footer_text(tui)
        assert "m/^m move" in footer
        tui.send_keystroke("\x1b[109;5u")
        assert tui.wait_for_text("MOVE: TAGGED FILES AS:", timeout=2.0)
    finally:
        tui.quit()


def test_legacy_mode_keeps_portable_bindings_out_of_the_enhanced_footer(
    ytnova_binary, tmp_path
):
    root = tmp_path / "legacy-bindings"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        dimensions=(40, 240),
        keyboard_protocol="legacy",
    )
    try:
        assert tui.send_and_wait_for_screen_change("\r", timeout=2.0)
        footer = _footer_text(tui)
        assert "m/^n move" in footer
        assert "m/^m" not in footer
        assert "invert" in footer
    finally:
        tui.quit()


def test_legacy_fallback_is_silent(ytnova_binary, tmp_path):
    root = tmp_path / "legacy-notice"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        keyboard_protocol="legacy",
    )
    try:
        tui.send_keystroke("\r")
        assert tui.wait_for_text("one.txt", timeout=2.0)
    finally:
        tui.quit()


def test_external_handoff_pops_and_renegotiates_enhanced_input(
    ytnova_binary, tmp_path
):
    root = tmp_path / "external-handoff"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")
    (root / ".ytnova").write_text(
        "[GLOBAL]\nEDITOR=true\n", encoding="utf-8"
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.send_and_wait_for_screen_change("\r", timeout=2.0)
        tui.send_keystroke("e", wait=0)
        assert tui.wait_for_keyboard_probe_count(2, timeout=3.0)
        assert tui.keyboard_pop_count >= 1
        assert tui.wait_for_text("one.txt", timeout=2.0)
    finally:
        tui.quit()


def test_external_handoff_silently_uses_legacy_fallback(
    ytnova_binary, tmp_path
):
    root = tmp_path / "external-fallback"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")
    (root / ".ytnova").write_text(
        "[GLOBAL]\nEDITOR=true\n", encoding="utf-8"
    )

    tui = YtreeNovaTUI(
        executable=ytnova_binary,
        cwd=str(root),
        keyboard_protocol=("enhanced", "legacy"),
    )
    try:
        assert tui.send_and_wait_for_screen_change("\r", timeout=2.0)
        tui.send_keystroke("e", wait=0)
        assert tui.wait_for_keyboard_probe_count(2, timeout=3.0)
        assert tui.wait_for_text("one.txt", timeout=2.0)

    finally:
        tui.quit()


def test_orderly_shutdown_pops_enhanced_input(ytnova_binary, tmp_path):
    root = tmp_path / "shutdown-pop"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")
    (root / ".ytnova").write_text(
        "[GLOBAL]\nCONFIRMQUIT=0\n", encoding="utf-8"
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    tui.send_keystroke("q", wait=0)
    assert tui.wait_for_exit(timeout=3.0)
    assert tui.keyboard_pop_count >= 1


def test_handled_interrupt_pops_enhanced_input(ytnova_binary, tmp_path):
    root = tmp_path / "interrupt-pop"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    assert tui.wait_for_text("one.txt", timeout=2.0)
    tui.child.kill(signal.SIGINT)
    assert tui.wait_for_exit(timeout=3.0)
    assert tui.keyboard_pop_count >= 1
