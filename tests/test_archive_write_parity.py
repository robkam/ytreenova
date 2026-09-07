import io
from pathlib import Path
import pexpect
import pytest
import tarfile
import time

from ytnova_control import YtreeNovaController
from ytnova_keys import Keys


def _create_archive(path, entries):
    with tarfile.open(path, "w") as tf:
        for name, content in entries.items():
            payload = content.encode("utf-8")
            info = tarfile.TarInfo(name=name)
            info.size = len(payload)
            info.mode = 0o644
            info.mtime = int(time.time())
            tf.addfile(info, io.BytesIO(payload))


def _archive_names(path):
    with tarfile.open(path, "r") as tf:
        return sorted(member.name.rstrip("/") for member in tf.getmembers())


def _archive_read_text(path, name):
    with tarfile.open(path, "r") as tf:
        fobj = tf.extractfile(name)
        assert fobj is not None, f"missing archive member: {name}"
        return fobj.read().decode("utf-8")


def _wait_for_archive_names(controller, archive_path, predicate, description):
    def archive_matches(_lines):
        try:
            return predicate(_archive_names(archive_path))
        except (OSError, tarfile.TarError):
            return False

    assert controller.wait_for_condition(
        archive_matches, timeout=5.0, description=description
    )
    return _archive_names(archive_path)


def _wait_for_file_payload(controller, path, payload, description):
    def file_matches(_lines):
        try:
            return path.exists() and path.read_text(encoding="utf-8") == payload
        except OSError:
            return False

    assert controller.wait_for_condition(
        file_matches, timeout=5.0, description=description
    )


def _screen_text(controller):
    before = controller.child.before if isinstance(controller.child.before, str) else ""
    after = controller.child.after if isinstance(controller.child.after, str) else ""
    return before + after


def _log_archive(controller, archive_path):
    assert controller.send_and_wait_for_screen_change(Keys.LOG)
    controller.input_text(str(archive_path))
    controller.child.expect("ARCHIVE")


def _log_archive_dismissing_unsafe_warnings(controller, archive_path):
    assert controller.send_and_wait_for_screen_change(Keys.LOG)
    controller.input_text(str(archive_path))
    _dismiss_unsafe_archive_warning(controller, "ARCHIVE", timeout=0.8)


def _dismiss_unsafe_archive_warning(controller, completion, timeout):
    idx = controller.child.expect(
        [r"Skipped unsafe archive member path", completion, pexpect.TIMEOUT],
        timeout=timeout,
    )
    if idx == 0:
        assert controller.send_and_wait_for_screen_change(Keys.ENTER)
        return _dismiss_unsafe_archive_warning(controller, completion, timeout)
    assert idx == 1, f"Archive did not reach {completion!r} after safety warnings."


def _exit_archive_keep_volume(controller):
    assert controller.send_and_wait_for_screen_change("\\")


def _exit_archive_plain(controller):
    assert controller.send_and_wait_for_screen_change(Keys.LEFT)


def _copy_selected_file(controller, new_name, to_dir):
    controller.child.send(Keys.COPY)
    controller.child.expect("COPY")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))


def _move_selected_file(controller, new_name, to_dir):
    controller.child.send(Keys.MOVE)
    controller.child.expect("MOVE")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))


def _enter_archive_member_list_dismissing_unsafe_warnings(controller, anchor_member):
    assert controller.send_and_wait_for_screen_change(Keys.ENTER)
    _dismiss_unsafe_archive_warning(controller, anchor_member, timeout=0.6)


def test_archive_copy_matrix_fs_to_vfs(ytnova_binary, tmp_path):
    root = tmp_path / "copy_fs_to_vfs"
    root.mkdir()
    (root / "fs_source.txt").write_text("fs payload", encoding="utf-8")
    dst_archive = root / "dst.tar"
    _create_archive(dst_archive, {})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    assert yt.send_and_wait_for_screen_change(Keys.ESC)

    yt.select_file("fs_source.txt")
    _copy_selected_file(yt, "copied_from_fs.txt", dst_archive)

    dst_names = _wait_for_archive_names(
        yt,
        dst_archive,
        lambda names: "copied_from_fs.txt" in names,
        "destination member after filesystem-to-archive copy",
    )
    assert "copied_from_fs.txt" in dst_names
    assert _archive_read_text(dst_archive, "copied_from_fs.txt") == "fs payload"

    yt.quit()


def test_archive_copy_matrix_vfs_to_fs(ytnova_binary, tmp_path):
    root = tmp_path / "copy_vfs_to_fs"
    root.mkdir()
    out_dir = root / "out"
    out_dir.mkdir()
    src_archive = root / "src.tar"
    _create_archive(src_archive, {"src.txt": "from archive"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, src_archive)
    assert yt.send_and_wait_for_screen_change(Keys.ENTER)
    yt.child.expect("src.txt")

    _copy_selected_file(yt, "copied_to_fs.txt", out_dir)

    copied = out_dir / "copied_to_fs.txt"
    _wait_for_file_payload(
        yt, copied, "from archive", "filesystem payload after archive copy"
    )

    yt.quit()


def test_archive_traversal_rejection_copy_vfs_to_fs_never_writes_outside_destination(
    ytnova_binary, tmp_path
):
    root = tmp_path / "copy_vfs_to_fs_traversal_rejection"
    root.mkdir()
    out_dir = root / "out"
    out_dir.mkdir()

    token = str(time.time_ns())
    dotdot_escape_name = f"escape_dotdot_{token}.txt"
    dotdot_escape_path = root / dotdot_escape_name
    absolute_escape_path = Path("/tmp") / f"ytnova_archive_abs_escape_{token}.txt"

    src_archive = root / "src.tar"
    _create_archive(
        src_archive,
        {
            "safe_member.txt": "from archive",
            f"../{dotdot_escape_name}": "unsafe dotdot",
            str(absolute_escape_path): "unsafe absolute",
            "nested//unsafe_empty_segment_member.txt": "unsafe empty segment",
            "nested/./unsafe_dot_segment_member.txt": "unsafe dot segment",
        },
    )

    yt = YtreeNovaController(ytnova_binary, str(root))
    try:
        yt.wait_for_startup()
        _log_archive_dismissing_unsafe_warnings(yt, src_archive)
        _enter_archive_member_list_dismissing_unsafe_warnings(yt, "safe_member.txt")

        _copy_selected_file(yt, "copied_safe.txt", out_dir)

        copied = out_dir / "copied_safe.txt"
        _wait_for_file_payload(
            yt,
            copied,
            "from archive",
            "safe filesystem payload after archive traversal rejection",
        )
        assert sorted(path.name for path in out_dir.iterdir()) == ["copied_safe.txt"]
        assert not dotdot_escape_path.exists()
        assert not absolute_escape_path.exists()
    finally:
        yt.quit()


def test_archive_copy_matrix_vfs_to_vfs(ytnova_binary, tmp_path):
    root = tmp_path / "copy_vfs_to_vfs"
    root.mkdir()
    src_archive = root / "src.tar"
    dst_archive = root / "dst.tar"
    _create_archive(src_archive, {"src.txt": "vfs payload"})
    _create_archive(dst_archive, {"keep.txt": "keep"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    _log_archive(yt, src_archive)
    assert yt.send_and_wait_for_screen_change(Keys.ENTER)
    yt.child.expect("src.txt")

    _copy_selected_file(yt, "copied_from_vfs.txt", dst_archive)

    src_names = _archive_names(src_archive)
    dst_names = _wait_for_archive_names(
        yt,
        dst_archive,
        lambda names: "copied_from_vfs.txt" in names,
        "destination member after archive copy",
    )
    assert "src.txt" in src_names
    assert "copied_from_vfs.txt" in dst_names
    assert _archive_read_text(dst_archive, "copied_from_vfs.txt") == "vfs payload"

    yt.quit()


def test_archive_move_matrix_fs_to_vfs(ytnova_binary, tmp_path):
    root = tmp_path / "move_fs_to_vfs"
    root.mkdir()
    (root / "fs_source.txt").write_text("fs move payload", encoding="utf-8")
    dst_archive = root / "dst.tar"
    _create_archive(dst_archive, {})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    assert yt.send_and_wait_for_screen_change(Keys.ESC)

    yt.select_file("fs_source.txt")
    _move_selected_file(yt, "moved_from_fs.txt", dst_archive)

    assert yt.wait_for_condition(
        lambda _lines: not (root / "fs_source.txt").exists(),
        timeout=5.0,
        description="filesystem source removal after archive move",
    )
    assert not (root / "fs_source.txt").exists()
    _wait_for_archive_names(
        yt,
        dst_archive,
        lambda names: "moved_from_fs.txt" in names,
        "archive member after filesystem-to-archive move",
    )
    assert _archive_read_text(dst_archive, "moved_from_fs.txt") == "fs move payload"

    yt.quit()


def test_archive_move_matrix_vfs_to_fs(ytnova_binary, tmp_path):
    root = tmp_path / "move_vfs_to_fs"
    root.mkdir()
    out_dir = root / "out"
    out_dir.mkdir()
    src_archive = root / "src.tar"
    _create_archive(src_archive, {"src.txt": "archive move payload"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, src_archive)
    assert yt.send_and_wait_for_screen_change(Keys.ENTER)
    yt.child.expect("src.txt")

    _move_selected_file(yt, "moved_to_fs.txt", out_dir)

    moved = out_dir / "moved_to_fs.txt"
    _wait_for_file_payload(
        yt, moved, "archive move payload", "filesystem payload after archive move"
    )
    _wait_for_archive_names(
        yt,
        src_archive,
        lambda names: "src.txt" not in names,
        "source removal after archive-to-filesystem move",
    )

    yt.quit()


def test_archive_move_matrix_vfs_to_vfs(ytnova_binary, tmp_path):
    root = tmp_path / "move_vfs_to_vfs"
    root.mkdir()
    src_archive = root / "src.tar"
    dst_archive = root / "dst.tar"
    _create_archive(src_archive, {"src.txt": "archive move"})
    _create_archive(dst_archive, {"keep.txt": "keep"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, dst_archive)
    _exit_archive_keep_volume(yt)
    _log_archive(yt, src_archive)
    assert yt.send_and_wait_for_screen_change(Keys.ENTER)
    yt.child.expect("src.txt")

    _move_selected_file(yt, "moved_to_other_vfs.txt", dst_archive)

    src_names = _wait_for_archive_names(
        yt,
        src_archive,
        lambda names: "src.txt" not in names,
        "source removal after archive-to-archive move",
    )
    dst_names = _wait_for_archive_names(
        yt,
        dst_archive,
        lambda names: "moved_to_other_vfs.txt" in names,
        "destination member after archive-to-archive move",
    )
    assert "src.txt" not in src_names
    assert "moved_to_other_vfs.txt" in dst_names
    assert _archive_read_text(dst_archive, "moved_to_other_vfs.txt") == "archive move"

    yt.quit()


def test_archive_create_rename_parity(ytnova_binary, tmp_path):
    root = tmp_path / "create_rename"
    root.mkdir()
    archive_path = root / "ops.tar"
    _create_archive(archive_path, {"old.txt": "old payload"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, archive_path)

    yt.child.send("M")
    yt.child.expect("MAKE DIRECTORY")
    yt.input_text("newdir")

    assert yt.send_and_wait_for_screen_change(Keys.ENTER)
    yt.child.expect("old.txt")

    yt.child.send(Keys.RENAME)
    yt.child.expect("RENAME")
    yt.input_text("renamed.txt")

    names = _archive_names(archive_path)
    assert "newdir" in names
    assert "old.txt" not in names
    assert "renamed.txt" in names

    yt.quit()


def test_archive_delete_parity(ytnova_binary, tmp_path):
    root = tmp_path / "archive_delete"
    root.mkdir()
    archive_path = root / "ops.tar"
    _create_archive(archive_path, {"delete_me.txt": "payload"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    yt.wait_for_startup()
    _log_archive(yt, archive_path)

    assert yt.send_and_wait_for_screen_change(Keys.ENTER)
    yt.child.expect("delete_me.txt")

    yt.child.send(Keys.DELETE)
    yt.child.expect("Delete this file")
    assert yt.send_and_wait_for_screen_change("Y")

    assert "delete_me.txt" not in _archive_names(archive_path)

    yt.quit()


@pytest.mark.parametrize(
    ("action", "destination_name", "source_retained"),
    [
        (Keys.COPY, "copied_bundle", True),
        (Keys.PATHCOPY, "pathcopied_bundle", True),
        ("V", "moved_bundle", False),
    ],
)
def test_archive_directory_transfer_matrix_vfs_to_vfs(
    ytnova_binary, tmp_path, action, destination_name, source_retained
):
    root = tmp_path / "directory_vfs_to_vfs"
    root.mkdir()
    src_archive = root / "src.tar"
    dst_archive = root / "dst.tar"
    source_entries = {
        "bundle/nested/value.txt": "recursive archive payload",
        "bundle/peer.txt": "peer payload",
    }
    if action == "V":
        source_entries["sibling/anchor.txt"] = "selection anchor"
    _create_archive(src_archive, source_entries)
    _create_archive(dst_archive, {"keep.txt": "keep"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    try:
        yt.wait_for_startup()
        _log_archive(yt, dst_archive)
        _exit_archive_keep_volume(yt)
        _log_archive(yt, src_archive)
        assert yt.send_and_wait_for_screen_change(Keys.DOWN)
        yt.child.send(action)
        yt.child.expect(
            "PATHCOPY"
            if action == Keys.PATHCOPY
            else "COPY"
            if action == Keys.COPY
            else "MOVE"
        )
        yt.input_text(destination_name)
        yt.child.expect("To Directory")
        yt.input_text(str(dst_archive))

        destination_names = _wait_for_archive_names(
            yt,
            dst_archive,
            lambda names: (
                f"{destination_name}/nested/value.txt" in names
                and f"{destination_name}/peer.txt" in names
            ),
            "recursive archive destination members",
        )
        assert "keep.txt" in destination_names
        assert _archive_read_text(
            dst_archive, f"{destination_name}/nested/value.txt"
        ) == "recursive archive payload"
        assert (
            _archive_read_text(dst_archive, f"{destination_name}/peer.txt")
            == "peer payload"
        )

        source_names = _archive_names(src_archive)
        assert ("bundle/nested/value.txt" in source_names) == source_retained
        assert ("bundle/peer.txt" in source_names) == source_retained

        _exit_archive_keep_volume(yt)
        _log_archive(yt, dst_archive)
        yt.child.expect(destination_name)
    finally:
        yt.quit()


def test_archive_directory_transfer_rejects_destination_collision(
    ytnova_binary, tmp_path
):
    root = tmp_path / "directory_vfs_collision"
    root.mkdir()
    src_archive = root / "src.tar"
    dst_archive = root / "dst.tar"
    _create_archive(src_archive, {"bundle/nested/value.txt": "source payload"})
    _create_archive(
        dst_archive,
        {
            "bundle/nested/value.txt": "destination payload",
            "keep.txt": "keep",
        },
    )

    yt = YtreeNovaController(ytnova_binary, str(root))
    try:
        yt.wait_for_startup()
        _log_archive(yt, dst_archive)
        _exit_archive_keep_volume(yt)
        _log_archive(yt, src_archive)
        yt.child.send(Keys.COPY)
        yt.child.expect("COPY")
        yt.input_text("bundle")
        yt.child.expect("To Directory")
        yt.input_text(str(dst_archive))

        assert (
            _archive_read_text(dst_archive, "bundle/nested/value.txt")
            == "destination payload"
        )
        assert (
            _archive_read_text(src_archive, "bundle/nested/value.txt")
            == "source payload"
        )
    finally:
        yt.quit()


def test_archive_directory_move_preserves_source_when_source_delete_fails(
    ytnova_binary, tmp_path
):
    root = tmp_path / "directory_move_delete_failure"
    root.mkdir()
    source_parent = root / "source_volume"
    source_parent.mkdir()
    src_archive = source_parent / "src.tar"
    dst_archive = root / "dst.tar"
    entries = {
        "bundle/nested/value.txt": "source payload",
        "sibling/anchor.txt": "anchor",
    }
    _create_archive(src_archive, entries)
    _create_archive(dst_archive, {"keep.txt": "keep"})

    yt = YtreeNovaController(ytnova_binary, str(root))
    try:
        yt.wait_for_startup()
        _log_archive(yt, dst_archive)
        _exit_archive_keep_volume(yt)
        _log_archive(yt, src_archive)
        assert yt.send_and_wait_for_screen_change(Keys.DOWN)
        source_parent.chmod(0o555)
        yt.child.send("V")
        yt.child.expect("MOVE")
        yt.input_text("moved_bundle")
        yt.child.expect("To Directory")
        yt.input_text(str(dst_archive))

        _wait_for_archive_names(
            yt,
            dst_archive,
            lambda names: "moved_bundle/nested/value.txt" in names,
            "destination write before injected source removal failure",
        )
        assert (
            _archive_read_text(dst_archive, "moved_bundle/nested/value.txt")
            == "source payload"
        )
        assert (
            _archive_read_text(src_archive, "bundle/nested/value.txt")
            == "source payload"
        )
    finally:
        source_parent.chmod(0o755)
        yt.quit()
