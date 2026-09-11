import io
import tarfile

import pytest

from helpers_ui import (
    assert_file_tag_state,
    dismiss_archive_unsafe_warnings,
    drive_action_until,
    footer_lines,
)
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _create_tar(path, entries):
    with tarfile.open(path, "w") as archive:
        for name, value in entries.items():
            payload = value.encode("utf-8")
            info = tarfile.TarInfo(name)
            info.size = len(payload)
            archive.addfile(info, io.BytesIO(payload))


def _create_cpio_crc(path, entries):
    def append_entry(output, name, payload, inode):
        name_bytes = name.encode("utf-8") + b"\0"
        fields = (
            inode,
            0o100644,
            0,
            0,
            1,
            0,
            len(payload),
            0,
            0,
            0,
            0,
            len(name_bytes),
            sum(payload),
        )
        output.extend(b"070702" + b"".join(f"{field:08x}".encode() for field in fields))
        output.extend(name_bytes)
        output.extend(b"\0" * (-len(output) % 4))
        output.extend(payload)
        output.extend(b"\0" * (-len(output) % 4))

    output = bytearray()
    for inode, (name, value) in enumerate(entries.items(), start=1):
        append_entry(output, name, value.encode("utf-8"), inode)
    append_entry(output, "TRAILER!!!", b"", len(entries) + 1)
    path.write_bytes(output)


def _open_selected_archive(tui):
    assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=2.0)
    assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=2.0)
    tui.child.send(Keys.ENTER)
    assert dismiss_archive_unsafe_warnings(
        tui, "Skipped unsafe archive member path", "ARCHIVE", Keys.ENTER
    )


def _footer_text(tui):
    return "\n".join(footer_lines(tui)).lower()


def _assert_archive_file_commands(footer, *, read_only):
    assert "1..0 file view" in footer, footer
    for command in ("copy", "filter", "hex", "invert", "output", "pipe", "view", "pathcopy"):
        assert command in footer, footer
    for unsupported in ("attributes", "edit", "newfile", "xecute", "z/^z"):
        assert unsupported not in footer, footer
    if read_only:
        assert "readonly" in footer and "commands" not in footer, footer
        assert "delete" not in footer and "rename" not in footer and "m/^n" not in footer
    else:
        assert "commands" in footer and "readonly" not in footer, footer
        assert "delete" in footer and "rename" in footer and "m/^m" in footer


def _assert_archive_preview_commands(footer, *, read_only):
    assert "preview" in footer, footer
    for command in ("copy", "filter", "invert", "output", "view", "pathcopy"):
        assert command in footer, footer
    for unsupported in ("attributes", "edit", "newfile", "xecute", "z/^z"):
        assert unsupported not in footer, footer
    if read_only:
        assert "readonly" in footer and "commands" not in footer, footer
        assert "delete" not in footer and "rename" not in footer and "m/^n" not in footer
    else:
        assert "commands" in footer and "readonly" not in footer, footer
        assert "delete" in footer and "rename" in footer and "m/^m" in footer


def test_filesystem_directory_file_and_preview_commands(ytnova_binary, tmp_path):
    root = tmp_path / "filesystem_footer_inventory"
    root.mkdir()
    (root / "one.txt").write_text("one", encoding="utf-8")

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), dimensions=(40, 240)
    )
    try:
        directory_footer = _footer_text(tui)
        for command in ("attributes", "copy", "delete", "invert", "newfile", "movedir"):
            assert command in directory_footer, directory_footer

        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=2.0)
        file_footer = _footer_text(tui)
        for command in ("attributes", "edit", "newfile", "xecute", "z/^z"):
            assert command in file_footer, file_footer

        assert tui.send_and_wait_for_screen_change(Keys.F7, timeout=2.0)
        preview_footer = _footer_text(tui)
        for command in ("attributes", "edit", "newfile", "xecute", "z/^z"):
            assert command in preview_footer, preview_footer
        for excluded in ("hex", "log", "volume", "sort"):
            assert excluded not in preview_footer, preview_footer
    finally:
        tui.quit()


def test_writable_archive_footer_commands_and_invert_dispatch(
    ytnova_binary, tmp_path
):
    root = tmp_path / "writable_archive_footer_inventory"
    root.mkdir()
    archive_path = root / "writable.tar"
    _create_tar(
        archive_path,
        {"one.txt": "one", "two.txt": "two", "nested/three.txt": "three"},
    )

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), dimensions=(40, 240)
    )
    try:
        _open_selected_archive(tui)
        directory_footer = _footer_text(tui)
        assert "1..0 dir view" in directory_footer, directory_footer
        for command in ("copy", "delete", "invert", "makedir", "movedir", "pathcopy", "rename"):
            assert command in directory_footer, directory_footer
        assert "commands" in directory_footer and "readonly" not in directory_footer
        assert tui.send_and_wait_for_screen_change(Keys.F1, timeout=2.0)
        assert drive_action_until(
            tui,
            Keys.DOWN,
            lambda lines: lines
            if any(
                "Reverse tags only on filter-matching visible entries" in line
                for line in lines
            )
            else False,
            max_actions=80,
            timeout=0.2,
        )
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)

        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        nested_directory_footer = _footer_text(tui)
        assert "invert" in nested_directory_footer and "root" in nested_directory_footer
        assert tui.send_and_wait_for_screen_change(Keys.HOME, timeout=2.0)

        assert_file_tag_state(tui, "one.txt", False)
        assert_file_tag_state(tui, "two.txt", False)
        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        assert_file_tag_state(tui, "one.txt", True)
        assert_file_tag_state(tui, "two.txt", True)
        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        assert_file_tag_state(tui, "one.txt", False)
        assert_file_tag_state(tui, "two.txt", False)

        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=2.0)
        _assert_archive_file_commands(_footer_text(tui), read_only=False)
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        assert_file_tag_state(tui, "one.txt", True)
        _assert_archive_file_commands(_footer_text(tui), read_only=False)

        assert tui.send_and_wait_for_screen_change(Keys.F7, timeout=2.0)
        _assert_archive_preview_commands(_footer_text(tui), read_only=False)
    finally:
        tui.quit()


@pytest.mark.parametrize("state_key", ["g", "s"])
def test_writable_archive_global_and_showall_use_archive_file_commands(
    ytnova_binary, tmp_path, state_key
):
    root = tmp_path / f"writable_archive_{state_key}_footer_inventory"
    root.mkdir()
    archive_path = root / "writable.tar"
    _create_tar(archive_path, {"one.txt": "one", "two.txt": "two"})

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), dimensions=(40, 240)
    )
    try:
        _open_selected_archive(tui)
        assert tui.send_and_wait_for_screen_change(state_key, timeout=2.0)
        _assert_archive_file_commands(_footer_text(tui), read_only=False)
        assert tui.send_and_wait_for_screen_change(Keys.F7, timeout=2.0)
        _assert_archive_preview_commands(_footer_text(tui), read_only=False)
    finally:
        tui.quit()


def test_read_only_archive_keeps_read_commands_and_marks_all_footer_surfaces(
    ytnova_binary, tmp_path
):
    root = tmp_path / "read_only_archive_footer_inventory"
    root.mkdir()
    archive_path = root / "readonly.cpio"
    _create_cpio_crc(archive_path, {"one.txt": "one", "two.txt": "two"})

    tui = YtreeNovaTUI(
        executable=ytnova_binary, cwd=str(root), dimensions=(40, 240)
    )
    try:
        _open_selected_archive(tui)
        directory_footer = _footer_text(tui)
        for command in ("copy", "invert", "output", "pipe", "pathcopy"):
            assert command in directory_footer, directory_footer
        for mutation in ("delete", "makedir", "movedir", "rename"):
            assert mutation not in directory_footer, directory_footer
        assert "readonly" in directory_footer and "commands" not in directory_footer

        assert_file_tag_state(tui, "one.txt", False)
        assert_file_tag_state(tui, "two.txt", False)
        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        assert_file_tag_state(tui, "one.txt", True)
        assert_file_tag_state(tui, "two.txt", True)
        assert tui.send_and_wait_for_screen_change("i", timeout=2.0)
        assert_file_tag_state(tui, "one.txt", False)
        assert_file_tag_state(tui, "two.txt", False)

        assert tui.send_and_wait_for_screen_change("g", timeout=2.0)
        _assert_archive_file_commands(_footer_text(tui), read_only=True)
        assert tui.send_and_wait_for_screen_change("t", timeout=2.0)
        assert_file_tag_state(tui, "one.txt", True)
        _assert_archive_file_commands(_footer_text(tui), read_only=True)
        assert tui.send_and_wait_for_screen_change(Keys.F7, timeout=2.0)
        _assert_archive_preview_commands(_footer_text(tui), read_only=True)
    finally:
        tui.quit()
