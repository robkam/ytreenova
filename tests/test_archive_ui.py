import io
import os
import re
import subprocess
import tarfile
import zipfile

import pytest

from helpers_ui import dismiss_archive_unsafe_warnings
from helpers_ui import footer_lines as _footer_lines
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys


def _create_zip(path, entries):
    with zipfile.ZipFile(path, "w") as zf:
        for name, data in entries.items():
            zf.writestr(name, data)


def _zip_names(path):
    with zipfile.ZipFile(path, "r") as zf:
        return sorted(zf.namelist())


def _zip_read_text(path, name):
    with zipfile.ZipFile(path, "r") as zf:
        return zf.read(name).decode("utf-8")


def _create_tar(path, entries):
    with tarfile.open(path, "w") as tf:
        for name, data in entries.items():
            payload = data.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            tf.addfile(info, io.BytesIO(payload))


def _create_tar_with_empty_dir(path, empty_dir_name, extra_file_count=0):
    with tarfile.open(path, "w") as tf:
        dir_info = tarfile.TarInfo(name=f"{empty_dir_name}/")
        dir_info.type = tarfile.DIRTYPE
        dir_info.mode = 0o755
        tf.addfile(dir_info)

        for index in range(extra_file_count):
            payload = f"payload-{index}\n".encode("utf-8")
            info = tarfile.TarInfo(name=f"bulk_{index:04d}.txt")
            info.size = len(payload)
            info.mode = 0o644
            tf.addfile(info, io.BytesIO(payload))


def _enter_archive_from_selected_file(tui):
    assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=2.0)
    assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=2.0)
    tui.child.send(Keys.ENTER)
    assert dismiss_archive_unsafe_warnings(
        tui, "Skipped unsafe archive member path", "ARCHIVE", Keys.ENTER
    )








def test_archive_output_flow_writes_selected_entry_to_file(ytnova_binary, tmp_path):
    root = tmp_path / "archive_output_flow"
    root.mkdir()
    archive_path = root / "output_flow.tar"
    _create_tar(archive_path, {"inside_dir/inside.txt": "inside payload"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)

        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert tui.wait_for_content("inside.txt", timeout=2.0), "\n".join(
            tui.get_screen_dump()
        )

        tui.send_keystroke("o", wait=0.2)
        assert tui.wait_for_content("Output to:", timeout=1.0), "\n".join(
            tui.get_screen_dump()
        )

        tui.send_keystroke("F", wait=0.2)
        assert tui.wait_for_content("Output file [Raw]", timeout=1.0), "\n".join(
            tui.get_screen_dump()
        )

        out_path = root / "archive_output.txt"
        tui.send_keystroke(f"{out_path}\r", wait=0.1)
        assert tui.wait_for_condition(
            lambda _lines: out_path.exists(),
            timeout=2.0,
            description="archive output file creation",
        )
        assert out_path.read_text(encoding="utf-8") == "inside payload\n"
    finally:
        tui.quit()


def test_archive_copy_to_existing_destination_shows_size_time_comparison(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_copy_existing_destination_conflict"
    root.mkdir()
    archive_path = root / "source.tar"
    destination_dir = root / "dest"
    destination_dir.mkdir()
    destination = destination_dir / "inside.txt"

    with tarfile.open(archive_path, "w") as tf:
        payload = "archive payload grows\n".encode("utf-8")
        info = tarfile.TarInfo(name="inside.txt")
        info.size = len(payload)
        info.mode = 0o644
        info.mtime = 1700000000
        tf.addfile(info, io.BytesIO(payload))

    destination.write_text("old\n", encoding="utf-8")
    os.utime(destination, (1690000000, 1690000000))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)

        tui.send_keystroke(Keys.ENTER, wait=0.5)
        assert tui.wait_for_content("inside.txt", timeout=2.0), "\n".join(
            tui.get_screen_dump()
        )

        tui.child.send(Keys.COPY)
        tui.child.expect("COPY:", timeout=1.5)
        tui.child.send(Keys.ENTER)
        tui.child.expect("To Directory:", timeout=1.5)
        tui.child.send(f"{destination_dir}\r")
        assert tui.wait_for_content("Overwrite", timeout=1.5), "\n".join(
            tui.get_screen_dump()
        )

        screen = "\n".join(tui.get_screen_dump())
        normalized = " ".join(screen.split())
        assert "inside.txt" in normalized, screen
        assert re.search(
            r"src [0-9]+(?:\.[0-9])?[BKMGTP] \d{4}-\d{2}-\d{2} \d{2}:\d{2}",
            normalized,
            re.IGNORECASE,
        ), screen
        assert re.search(
            r"dst [0-9]+(?:\.[0-9])?[BKMGTP] \d{4}-\d{2}-\d{2} \d{2}:\d{2}",
            normalized,
            re.IGNORECASE,
        ), screen
        assert re.search(
            r"(same size|dst smaller|dst bigger), (same time|dst older|dst newer)",
            normalized.lower(),
        ), screen

        tui.child.send(Keys.ESC)
    finally:
        tui.quit()


def test_archive_internal_path_trust_rejects_unsafe_members(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_internal_path_trust_rejects"
    root.mkdir()
    archive_path = root / "trust.tar"
    _create_tar(
        archive_path,
        {
            "safe_member.txt": "safe payload",
            "../unsafe_dotdot_member.txt": "bad",
            "/unsafe_absolute_member.txt": "bad",
            "nested//unsafe_empty_segment_member.txt": "bad",
            "nested/./unsafe_dot_segment_member.txt": "bad",
            r"nested\\unsafe_separator_ambiguity_member.txt": "bad",
        },
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
        tui.send_keystroke(Keys.EXPAND_ALL, wait=0.5)

        assert not tui.wait_for_content("unsafe_dotdot_member.txt", timeout=1.0)
        assert not tui.wait_for_content("unsafe_absolute_member.txt", timeout=1.0)
        assert not tui.wait_for_content(
            "unsafe_empty_segment_member.txt", timeout=1.0
        )
        assert not tui.wait_for_content("unsafe_dot_segment_member.txt", timeout=1.0)
        assert not tui.wait_for_content(
            "unsafe_separator_ambiguity_member.txt", timeout=1.0
        )
    finally:
        tui.quit()


def test_archive_traversal_rejection_tree_load_filters_unsafe_variants(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_traversal_tree_filters"
    root.mkdir()
    archive_path = root / "traversal_filters.tar"
    _create_tar(
        archive_path,
        {
            "safe_member.txt": "safe payload",
            "../unsafe_dotdot_member.txt": "bad dotdot",
            "/unsafe_absolute_member.txt": "bad absolute",
            "nested//unsafe_empty_segment_member.txt": "bad empty segment",
            "nested/./unsafe_dot_segment_member.txt": "bad dot segment",
            r"nested\\unsafe_separator_ambiguity_member.txt": "bad separator",
        },
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
        tui.send_keystroke(Keys.EXPAND_ALL, wait=0.5)

        assert not tui.wait_for_content("unsafe_dotdot_member.txt", timeout=1.0)
        assert not tui.wait_for_content("unsafe_absolute_member.txt", timeout=1.0)
        assert not tui.wait_for_content(
            "unsafe_empty_segment_member.txt", timeout=1.0
        )
        assert not tui.wait_for_content("unsafe_dot_segment_member.txt", timeout=1.0)
        assert not tui.wait_for_content(
            "unsafe_separator_ambiguity_member.txt", timeout=1.0
        )
    finally:
        tui.quit()


def test_archive_internal_path_trust_safe_member_still_viewable(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_internal_path_trust_safe"
    root.mkdir()
    archive_path = root / "trust_safe.tar"
    _create_tar(
        archive_path,
        {
            "safe_member.txt": "safe payload",
            "../unsafe_dotdot_member.txt": "bad",
        },
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
        assert not tui.wait_for_content("No files extracted.", timeout=0.8)
    finally:
        tui.quit()


def test_archive_internal_path_trust_trailing_slash_empty_dir_visible(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_internal_path_trust_trailing_dir"
    root.mkdir()
    archive_path = root / "trust_trailing_dir.tar"

    with tarfile.open(archive_path, "w") as tf:
        dir_info = tarfile.TarInfo(name="safe_dir/")
        dir_info.type = tarfile.DIRTYPE
        dir_info.mode = 0o755
        tf.addfile(dir_info)

        payload = "anchor payload".encode("utf-8")
        file_info = tarfile.TarInfo(name="safe_anchor.txt")
        file_info.size = len(payload)
        file_info.mode = 0o644
        tf.addfile(file_info, io.BytesIO(payload))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_anchor.txt", timeout=3.0)
        tui.send_keystroke(Keys.EXPAND_ALL, wait=0.5)
        assert tui.wait_for_content("safe_dir", timeout=3.0)
    finally:
        tui.quit()


def test_archive_root_dot_member_is_ignored_without_warning(ytnova_binary, tmp_path):
    root = tmp_path / "archive_root_dot_member"
    root.mkdir()
    archive_path = root / "dot_root.tar"

    with tarfile.open(archive_path, "w") as tf:
        dot_info = tarfile.TarInfo(name=".")
        dot_info.type = tarfile.DIRTYPE
        dot_info.mode = 0o755
        tf.addfile(dot_info)

        payload = "safe payload".encode("utf-8")
        file_info = tarfile.TarInfo(name="safe_member.txt")
        file_info.size = len(payload)
        file_info.mode = 0o644
        tf.addfile(file_info, io.BytesIO(payload))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.5)
        tui.send_keystroke(Keys.LOG, wait=0.3)
        tui.send_keystroke(Keys.ENTER, wait=0.8)

        assert not tui.wait_for_content("Skipped unsafe archive member path", timeout=1.0)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
    finally:
        tui.quit()


def test_archive_single_directory_keeps_archive_container_as_tree_root(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_root_projection"
    root.mkdir()
    archive_path = root / "single.tar"
    _create_tar(archive_path, {"only/nested/value.txt": "payload"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)
        assert tui.wait_for_content("single.tar", timeout=2.0)
        assert tui.wait_for_content("only", timeout=2.0)
        screen = "\n".join(tui.get_screen_dump())
        assert "single.tar/only" not in screen
    finally:
        tui.quit()


def test_archive_f7_preview_renders_member_content(ytnova_binary, tmp_path):
    root = tmp_path / "archive_f7_preview"
    root.mkdir()
    archive_path = root / "preview.tar"
    _create_tar(archive_path, {"safe_member.txt": "safe payload"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)

        tui.send_keystroke(Keys.F7, wait=0.8)
        assert not tui.wait_for_content("Error opening file:", timeout=0.8)
        assert tui.wait_for_content("safe payload", timeout=3.0)
    finally:
        tui.quit()


def test_archive_traversal_rejection_view_flow_ignores_unsafe_members(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_traversal_view_flow"
    root.mkdir()
    archive_path = root / "view_flow.tar"
    _create_tar(
        archive_path,
        {
            "safe_member.txt": "safe preview payload",
            "../unsafe_dotdot_member.txt": "UNSAFE_SHOULD_NEVER_RENDER",
            "/unsafe_absolute_member.txt": "UNSAFE_SHOULD_NEVER_RENDER",
            "nested/./unsafe_dot_segment_member.txt": "UNSAFE_SHOULD_NEVER_RENDER",
        },
    )

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("safe_member.txt", timeout=3.0)
        tui.send_keystroke(Keys.EXPAND_ALL, wait=0.5)

        assert not tui.wait_for_content("unsafe_dotdot_member.txt", timeout=1.0)
        assert not tui.wait_for_content("unsafe_absolute_member.txt", timeout=1.0)
        assert not tui.wait_for_content("unsafe_dot_segment_member.txt", timeout=1.0)

        tui.send_keystroke(Keys.F7, wait=0.8)
        assert not tui.wait_for_content("Error opening file:", timeout=0.8)
        assert tui.wait_for_content("safe preview payload", timeout=3.0)
        assert not tui.wait_for_content("UNSAFE_SHOULD_NEVER_RENDER", timeout=1.0)
    finally:
        tui.quit()


def test_archive_create_overwrite_prompt_respects_no_then_yes(ytnova_binary, tmp_path):
    root = tmp_path / "overwrite_prompt"
    root.mkdir()
    source_file = root / "0_source.txt"
    source_file.write_text("new payload", encoding="utf-8")
    archive_path = root / "z_existing.zip"
    _create_zip(archive_path, {"existing.txt": "old payload"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("0_source.txt", timeout=3.0)

        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{archive_path}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite z_existing.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("n", wait=0.3)

        assert _zip_names(archive_path) == ["existing.txt"]
        assert _zip_read_text(archive_path, "existing.txt") == "old payload"

        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{archive_path}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite z_existing.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.6)

        assert tui.wait_for_condition(
            lambda _lines: "0_source.txt" in _zip_names(archive_path),
            timeout=5.0,
            description="overwritten archive payload",
        )
        assert "0_source.txt" in _zip_names(archive_path)
        assert _zip_read_text(archive_path, "0_source.txt") == "new payload"
    finally:
        tui.quit()


def test_archive_create_ctrl_o_opens_output_prompt(ytnova_binary, tmp_path):
    root = tmp_path / "ctrl_o_archive"
    root.mkdir()
    source_file = root / "source.txt"
    source_file.write_text("payload", encoding="utf-8")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("source.txt", timeout=3.0)
        tui.send_keystroke("t", wait=0.2)

        tui.send_keystroke(Keys.CTRL_O, wait=0.2)
        assert tui.wait_for_content("Output to:", timeout=3.0)
        assert not tui.wait_for_content("Create archive:", timeout=0.5)
    finally:
        tui.quit()


def test_archive_create_inside_source_directory_is_allowed(ytnova_binary, tmp_path):
    root = tmp_path / "inside_source_allowed"
    root.mkdir()
    child = root / "child"
    child.mkdir()
    (child / "nested.txt").write_text("payload", encoding="utf-8")
    destination = child / "out.zip"

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Recursive? (Y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.8)

        assert destination.exists()
        assert _zip_names(destination) == ["child/nested.txt"]
        assert _zip_read_text(destination, "child/nested.txt") == "payload"
    finally:
        tui.quit()


def test_archive_create_overwrite_excludes_destination_from_payload(
    ytnova_binary, tmp_path
):
    root = tmp_path / "overwrite_excludes_dest"
    root.mkdir()
    child = root / "child"
    child.mkdir()
    (child / "a.txt").write_text("alpha", encoding="utf-8")
    (child / "b.txt").write_text("beta", encoding="utf-8")
    destination = child / "out.zip"
    _create_zip(destination, {"stale.txt": "stale"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Recursive? (Y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite out.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.8)

        assert tui.wait_for_condition(
            lambda _lines: _zip_names(destination) == ["child/a.txt", "child/b.txt"],
            timeout=5.0,
            description="overwritten archive payload without destination",
        )
        names = _zip_names(destination)
        assert names == ["child/a.txt", "child/b.txt"]
        assert "out.zip" not in names
        assert _zip_read_text(destination, "child/a.txt") == "alpha"
        assert _zip_read_text(destination, "child/b.txt") == "beta"
    finally:
        tui.quit()


def test_archive_create_exclusion_empty_payload_shows_status_and_aborts(
    ytnova_binary, tmp_path
):
    root = tmp_path / "empty_after_exclusion"
    root.mkdir()
    destination = root / "only.zip"
    _create_zip(destination, {"keep.txt": "keep"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("only.zip", timeout=3.0)

        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.3)
        assert tui.wait_for_content("Overwrite only.zip? (y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.4)
        assert tui.wait_for_content("Nothing to archive", timeout=3.0)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        screen = "\n".join(tui.get_screen_dump())
        assert "Nothing to archive" not in screen
        assert _zip_names(destination) == ["keep.txt"]
        assert _zip_read_text(destination, "keep.txt") == "keep"
    finally:
        tui.quit()


def test_archive_create_inside_source_round_trip_integrity(ytnova_binary, tmp_path):
    root = tmp_path / "round_trip_integrity"
    root.mkdir()
    source = root / "source"
    source.mkdir()
    (source / "alpha.txt").write_text("alpha payload", encoding="utf-8")
    (source / "beta.txt").write_text("beta payload", encoding="utf-8")
    (source / "nested").mkdir()
    (source / "nested" / "gamma.txt").write_text("gamma payload", encoding="utf-8")
    destination = source / "bundle.zip"
    extract_root = root / "extracted"
    extract_root.mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Recursive? (Y/n)", timeout=3.0)
        tui.send_keystroke("y", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.8)
    finally:
        tui.quit()

    with zipfile.ZipFile(destination, "r") as zf:
        zf.extractall(extract_root)

    expected = {
        "source/alpha.txt": "alpha payload",
        "source/beta.txt": "beta payload",
        "source/nested/gamma.txt": "gamma payload",
    }

    extracted_files = sorted(
        str(path.relative_to(extract_root))
        for path in extract_root.rglob("*")
        if path.is_file()
    )
    assert extracted_files == sorted(expected.keys())
    for rel_path, expected_text in expected.items():
        assert (extract_root / rel_path).read_text(encoding="utf-8") == expected_text


def test_archive_create_unsupported_format_shows_and_clears_status_error(
    ytnova_binary, tmp_path
):
    root = tmp_path / "unsupported"
    root.mkdir()
    (root / "source.txt").write_text("payload", encoding="utf-8")
    destination = root / "out.7z"

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("source.txt", timeout=3.0)

        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.4)
        assert tui.wait_for_content("Unsupported archive format: .7z", timeout=3.0)

        tui.send_keystroke(Keys.DOWN, wait=0.3)
        screen = "\n".join(tui.get_screen_dump())
        assert "Unsupported archive format: .7z" not in screen
        assert not destination.exists()
    finally:
        tui.quit()


def test_archive_create_rejects_suffix_without_basename(ytnova_binary, tmp_path):
    root = tmp_path / "archive_suffix_only"
    root.mkdir()
    (root / "source.txt").write_text("payload", encoding="utf-8")
    destination = root / ".tar.gz"

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        tui.send_keystroke(Keys.ENTER, wait=0.4)
        assert tui.wait_for_content("source.txt", timeout=3.0)

        tui.send_keystroke("Z", wait=0.2)
        assert tui.wait_for_content("Create archive:", timeout=3.0)
        tui.send_keystroke(f"{destination}\r", wait=0.4)
        assert tui.wait_for_content("Archive name required before suffix", timeout=3.0)
        assert not destination.exists()
    finally:
        tui.quit()




def test_archive_makedir_updates_visible_view_without_manual_refresh(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_mkdir_refresh"
    root.mkdir()
    archive_path = root / "mkdir_refresh.tar"
    _create_tar(archive_path, {"inside.txt": "payload"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=3.0)

        tui.send_keystroke("m", wait=0.2)
        assert tui.wait_for_content("MAKE DIRECTORY:", timeout=2.0)
        tui.send_keystroke("new_dir\r", wait=0.4)
        assert tui.wait_for_content("new_dir", timeout=3.0)
    finally:
        tui.quit()




def test_archive_delete_nested_directory_restores_footer_shows_spinner_and_updates_view(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_delete_dir_refresh"
    root.mkdir()
    archive_path = root / "delete_dir_refresh.tar"
    with tarfile.open(archive_path, "w") as tf:
        dir_info = tarfile.TarInfo(name="emptydir/")
        dir_info.type = tarfile.DIRTYPE
        dir_info.mode = 0o755
        tf.addfile(dir_info)
        payload = b"nested payload"
        nested = tarfile.TarInfo(name="emptydir/nested.txt")
        nested.size = len(payload)
        nested.mode = 0o644
        tf.addfile(nested, io.BytesIO(payload))

        sibling_dir = tarfile.TarInfo(name="otherdir/")
        sibling_dir.type = tarfile.DIRTYPE
        sibling_dir.mode = 0o755
        tf.addfile(sibling_dir)

        for name, data in {
            "otherdir/keep.txt": "keep",
            "root_keep.txt": "keep",
        }.items():
            payload = data.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            tf.addfile(info, io.BytesIO(payload))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=3.0)
        assert tui.wait_for_content("emptydir", timeout=3.0)
        tui.send_keystroke(Keys.DOWN, wait=0.2)

        tui.send_keystroke("d", wait=0.2)
        assert tui.wait_for_content("Delete this directory", timeout=2.0)
        tui.send_keystroke("Y", wait=0.1)
        assert tui.wait_for_condition(
            lambda lines: not any("emptydir" in line for line in lines),
            timeout=5.0,
            poll_interval=0.05,
        ), "\n".join(tui.get_screen_dump())

        with tarfile.open(archive_path, "r") as tf:
            assert not any(name.startswith("emptydir/") for name in tf.getnames())

        footer_lines = [line.lower() for line in _footer_lines(tui)]
        assert footer_lines[0].strip(), "\n".join(footer_lines)
        assert "archive" in footer_lines[0], "\n".join(footer_lines)
    finally:
        tui.quit()


def test_archive_directory_copy_recursively_preserves_source(ytnova_binary, tmp_path):
    root = tmp_path / "archive_directory_copy"
    root.mkdir()
    destination = root / "destination"
    destination.mkdir()
    archive_path = root / "source.tar"
    with tarfile.open(archive_path, "w") as tf:
        for directory in ("bundle/", "bundle/nested/"):
            info = tarfile.TarInfo(directory)
            info.type = tarfile.DIRTYPE
            tf.addfile(info)
        payload = b"payload"
        info = tarfile.TarInfo("bundle/nested/value.txt")
        info.size = len(payload)
        tf.addfile(info, io.BytesIO(payload))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("bundle", timeout=3.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.send_keystroke(Keys.COPY, wait=0.1)
        assert tui.wait_for_content("COPY:", timeout=2.0)
        tui.send_keystroke("\x15bundle_copy\r", wait=0.1)
        assert tui.wait_for_content("To Directory", timeout=2.0)
        tui.send_keystroke(f"\x15{destination}\r", wait=0.1)
        copied = destination / "bundle_copy" / "nested" / "value.txt"
        assert tui.wait_for_condition(
            lambda _lines: copied.exists() and copied.read_text() == "payload",
            timeout=5.0,
            description="recursive archive directory copy",
        )
        with tarfile.open(archive_path, "r") as tf:
            assert tf.extractfile("bundle/nested/value.txt").read() == b"payload"
    finally:
        tui.quit()

def test_archive_directory_move_to_filesystem_removes_source(ytnova_binary, tmp_path):
    root = tmp_path / "archive_directory_copy"
    root.mkdir()
    destination = root / "destination"
    destination.mkdir()
    archive_path = root / "source.tar"
    with tarfile.open(archive_path, "w") as tf:
        for directory in ("bundle/", "bundle/nested/"):
            info = tarfile.TarInfo(directory)
            info.type = tarfile.DIRTYPE
            tf.addfile(info)
        sibling = tarfile.TarInfo("sibling/")
        sibling.type = tarfile.DIRTYPE
        tf.addfile(sibling)
        payload = b"payload"
        info = tarfile.TarInfo("bundle/nested/value.txt")
        info.size = len(payload)
        tf.addfile(info, io.BytesIO(payload))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("bundle", timeout=3.0)
        tui.send_keystroke(Keys.DOWN, wait=0.1)
        tui.send_keystroke("V", wait=0.1)
        assert tui.wait_for_content("MOVE:", timeout=2.0)
        tui.send_keystroke("\x15bundle_move\r", wait=0.1)
        assert tui.wait_for_content("To Directory", timeout=2.0)
        tui.send_keystroke(f"\x15{destination}\r", wait=0.1)
        copied = destination / "bundle_move" / "nested" / "value.txt"
        assert tui.wait_for_condition(
            lambda _lines: copied.exists() and copied.read_text() == "payload",
            timeout=5.0,
            description="recursive archive directory copy",
        )
        with tarfile.open(archive_path, "r") as tf:
            assert "bundle/nested/value.txt" not in tf.getnames()
    finally:
        tui.quit()

def test_archive_directory_pathcopy_recursively_preserves_source(ytnova_binary, tmp_path):
    root = tmp_path / "archive_directory_copy"
    root.mkdir()
    destination = root / "destination"
    destination.mkdir()
    archive_path = root / "source.tar"
    with tarfile.open(archive_path, "w") as tf:
        for directory in ("bundle/", "bundle/nested/"):
            info = tarfile.TarInfo(directory)
            info.type = tarfile.DIRTYPE
            tf.addfile(info)
        payload = b"payload"
        info = tarfile.TarInfo("bundle/nested/value.txt")
        info.size = len(payload)
        tf.addfile(info, io.BytesIO(payload))

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("bundle", timeout=3.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.send_keystroke(Keys.PATHCOPY, wait=0.1)
        assert tui.wait_for_content("PATHCOPY:", timeout=2.0)
        tui.send_keystroke("\x15bundle_pathcopy\r", wait=0.1)
        assert tui.wait_for_content("To Directory", timeout=2.0)
        tui.send_keystroke(f"\x15{destination}\r", wait=0.1)
        copied = destination / "bundle_pathcopy" / "nested" / "value.txt"
        assert tui.wait_for_condition(
            lambda _lines: copied.exists() and copied.read_text() == "payload",
            timeout=5.0,
            description="recursive archive directory copy",
        )
        with tarfile.open(archive_path, "r") as tf:
            assert tf.extractfile("bundle/nested/value.txt").read() == b"payload"
    finally:
        tui.quit()


def test_read_only_archive_hides_mutations_and_rejects_move(ytnova_binary, tmp_path):
    root = tmp_path / "read_only_archive"
    root.mkdir()
    member = root / "member.txt"
    member.write_text("payload", encoding="utf-8")
    archive_path = root / "readonly.a"
    subprocess.run(["ar", "rcs", str(archive_path), str(member)], check=True)
    member.unlink()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=3.0)
        footer = " ".join(_footer_lines(tui)).lower()
        assert "delete" not in footer and "rename" not in footer and "movedir" not in footer
        tui.send_keystroke(Keys.F1, wait=0.1)
        assert tui.wait_for_content("Archive Directory Help", timeout=2.0)
        help_text = "\n".join(tui.get_screen_dump()).lower()
        unavailable_descriptions = (
            "delete" + " the selected archive directory",
            "rename" + " the selected archive directory",
        )
        assert all(description not in help_text for description in unavailable_descriptions)
        tui.send_keystroke("\033", wait=0.1)
        tui.send_keystroke("V", wait=0.1)
        assert tui.wait_for_content(
            "This archive does not support directory transfer", timeout=2.0
        ), "\n".join(tui.get_screen_dump())
    finally:
        tui.quit()


def test_split_panels_keep_footer_context_with_archive_and_filesystem_volumes(
    ytnova_binary, tmp_path
):
    root = tmp_path / "archive_footer_panel_isolation"
    root.mkdir()
    archive_path = root / "panel.tar"
    _create_tar(archive_path, {"member.txt": "archive payload"})
    (root / "filesystem_child").mkdir()

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)

        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)

        filesystem_footer_lines = _footer_lines(tui)
        filesystem_footer = "\n".join(filesystem_footer_lines).upper()
        assert re.match(r"\s*DIR\b", filesystem_footer_lines[0]), filesystem_footer
        tui.send_keystroke("m", wait=0.1)
        assert tui.wait_for_content("MAKE DIRECTORY", timeout=2.0)
        tui.send_keystroke("\033", wait=0.1)

        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)
        archive_footer_lines = _footer_lines(tui)
        archive_footer = "\n".join(archive_footer_lines).upper()
        assert re.match(r"\s*ARCHIVE\b", archive_footer_lines[0]), archive_footer
    finally:
        tui.quit()


@pytest.mark.parametrize("key, copied_name, removes_source", [
    (Keys.COPY, "copied", False),
    (Keys.PATHCOPY, "pathcopied", False),
    ("V", "moved", True),
])
def test_filesystem_directory_transfer_to_logged_archive(
    ytnova_binary, tmp_path, key, copied_name, removes_source
):
    root = tmp_path / "filesystem_to_archive_directory"
    root.mkdir()
    source = root / "source"
    source.mkdir()
    (source / "payload.txt").write_text("payload", encoding="utf-8")
    nested = source / "nested"
    nested.mkdir()
    (nested / "leaf.txt").write_text("nested payload", encoding="utf-8")
    archive_path = root / "target.tar"
    _create_tar(archive_path, {})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=2.0)
        tui.send_keystroke(f"\x15{archive_path}\r", wait=0.1)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)
        assert tui.send_and_wait_for_screen_change("\\", timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.send_keystroke(key, wait=0.1)
        assert tui.wait_for_content("MOVE:" if removes_source else "COPY:", timeout=2.0)
        tui.send_keystroke(f"\x15{copied_name}\r", wait=0.1)
        assert tui.wait_for_content("To Directory", timeout=2.0)
        tui.send_keystroke(f"\x15{archive_path}\r", wait=0.1)
        def archive_contains_tree(_lines):
            with tarfile.open(archive_path) as archive:
                names = archive.getnames()
                return (f"{copied_name}/payload.txt" in names and
                        f"{copied_name}/nested/leaf.txt" in names)

        assert tui.wait_for_condition(archive_contains_tree, timeout=4.0,
                                      description="archive directory insertion"), "\n".join(tui.get_screen_dump())
        assert source.exists() is not removes_source

        assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=2.0)
        tui.child.send(f"\x15{archive_path}\r")
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)
        assert tui.wait_for_content(copied_name, timeout=3.0), "\n".join(
            tui.get_screen_dump()
        )
    finally:
        tui.quit()


def test_filesystem_directory_archive_transfer_shows_progress_while_rewriting(
    ytnova_binary, tmp_path
):
    root = tmp_path / "filesystem_to_archive_progress"
    root.mkdir()
    source = root / "source"
    source.mkdir()
    for index in range(32):
        (source / f"item_{index:02d}.txt").write_bytes(b"x" * 65536)
    archive_path = root / "target.tar"
    _create_tar(archive_path, {})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=2.0)
        tui.child.send(f"\x15{archive_path}\r")
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)
        assert tui.send_and_wait_for_screen_change("\\", timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.child.send(Keys.COPY)
        assert tui.wait_for_content("COPY:", timeout=2.0)
        tui.child.send("\x15copied\r")
        assert tui.wait_for_content("To Directory", timeout=2.0)
        tui.child.send(f"\x15{archive_path}\r")

        assert tui.wait_for_content("ARCHIVE COPY:", timeout=3.0), "\n".join(
            tui.get_screen_dump()
        )

        def archive_rewrite_finished(_lines):
            with tarfile.open(archive_path) as archive:
                return "copied/item_31.txt" in archive.getnames()

        assert tui.wait_for_condition(
            archive_rewrite_finished,
            timeout=10.0,
            poll_interval=0.05,
            description="archive rewrite completion",
        )
    finally:
        tui.quit()


def test_split_directory_copy_refreshes_inactive_archive_panel(
    ytnova_binary, tmp_path
):
    root = tmp_path / "split_directory_archive_refresh"
    root.mkdir()
    source = root / "source"
    source.mkdir()
    (source / "payload.txt").write_text("payload", encoding="utf-8")
    archive_path = root / "target.tar"
    _create_tar(archive_path, {})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.send_and_wait_for_screen_change(Keys.F8, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)
        _enter_archive_from_selected_file(tui)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)

        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.child.send(Keys.COPY)
        assert tui.wait_for_content("COPY:", timeout=2.0)
        tui.child.send("\x15copied\r")
        assert tui.wait_for_content("To Directory", timeout=2.0)
        tui.child.send(f"\x15{archive_path}\r")

        assert tui.wait_for_content("copied", timeout=4.0), "\n".join(
            tui.get_screen_dump()
        )
        assert tui.send_and_wait_for_screen_change(Keys.TAB, timeout=2.0)
        assert tui.wait_for_content("copied", timeout=2.0)
    finally:
        tui.quit()


def test_filesystem_directory_move_to_logged_archive_collision_preserves_source(
    ytnova_binary, tmp_path
):
    root = tmp_path / "filesystem_to_archive_directory_collision"
    root.mkdir()
    source = root / "source"
    source.mkdir()
    (source / "payload.txt").write_text("payload", encoding="utf-8")
    archive_path = root / "target.tar"
    _create_tar(archive_path, {"moved": "existing destination"})

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(root))
    try:
        assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=2.0)
        tui.send_keystroke(f"\x15{archive_path}\r", wait=0.1)
        assert tui.wait_for_content("ARCHIVE", timeout=2.0)
        assert tui.send_and_wait_for_screen_change("\\", timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.ESC, timeout=2.0)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=2.0)
        tui.send_keystroke("V", wait=0.1)
        assert tui.wait_for_content("MOVE:", timeout=2.0)
        tui.send_keystroke("\x15moved\r", wait=0.1)
        assert tui.wait_for_content("To Directory", timeout=2.0)
        assert tui.send_and_wait_for_screen_change(
            f"\x15{archive_path}\r", timeout=2.0
        )
        assert source.is_dir()
        assert (source / "payload.txt").read_text(encoding="utf-8") == "payload"
        with tarfile.open(archive_path) as archive:
            assert archive.getnames() == ["moved"]
    finally:
        tui.quit()
