import hashlib
import tarfile
import time
from pathlib import Path

from ytnova_control import YtreeNovaController
from ytnova_keys import Keys


def _sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _snapshot_tree(root: Path) -> dict[str, str]:
    snapshot = {}
    for path in sorted(root.rglob("*")):
        if path.is_file():
            relpath = str(path.relative_to(root))
            if (
                relpath.startswith(".ytnova")
                or relpath.startswith(".config/ytnova/")
                or relpath.startswith(".local/state/ytnova/")
            ):
                continue
            snapshot[relpath] = _sha256_file(path)
    return snapshot


def _snapshot_archive(path: Path) -> dict[str, str]:
    snapshot = {}
    with tarfile.open(path, "r") as archive:
        for member in sorted(archive.getmembers(), key=lambda m: m.name):
            if not member.isfile():
                continue
            member_file = archive.extractfile(member)
            assert member_file is not None, f"missing archive member data: {member.name}"
            snapshot[member.name] = hashlib.sha256(member_file.read()).hexdigest()
    return snapshot


def _copy_selected_file(controller: YtreeNovaController, source_name: str, new_name: str, to_dir: Path) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.COPY)
    controller.child.expect("COPY")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))
    if to_dir.is_file():
        complete = lambda _lines: new_name in _snapshot_archive(to_dir)
    else:
        complete = lambda _lines: (to_dir / new_name).exists()
    assert controller.wait_for_condition(
        complete,
        timeout=2.0,
        description=f"copy destination {to_dir / new_name}",
    )


def _move_selected_file(controller: YtreeNovaController, source_name: str, new_name: str, to_dir: Path) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.MOVE)
    controller.child.expect("MOVE")
    controller.input_text(new_name)
    controller.child.expect("To Directory")
    controller.input_text(str(to_dir))
    assert controller.wait_for_condition(
        lambda _lines: (to_dir / new_name).exists(),
        timeout=2.0,
        description=f"move destination {to_dir / new_name}",
    )


def _rename_selected_file(
    controller: YtreeNovaController,
    source_name: str,
    new_name: str,
    expected_path: Path,
) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.RENAME)
    controller.child.expect("RENAME")
    controller.input_text(new_name)
    assert controller.wait_for_condition(
        lambda _lines: expected_path.exists(),
        timeout=2.0,
        description=f"rename destination {expected_path}",
    )


def _delete_selected_file(
    controller: YtreeNovaController, source_name: str, source_path: Path
) -> None:
    controller.select_file(source_name)
    controller.child.send(Keys.DELETE)
    controller.child.expect("Delete this file")
    controller.child.send("Y")
    assert controller.wait_for_condition(
        lambda _lines: not source_path.exists(),
        timeout=2.0,
        description=f"deletion of {source_path}",
    )


def test_copy_mutation_updates_only_destination_hashes(ytnova_binary, tmp_path):
    root = tmp_path / "copy_integrity"
    source = root / "source"
    dest = root / "dest"
    source.mkdir(parents=True)
    dest.mkdir()
    (source / "alpha.txt").write_text("alpha payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _copy_selected_file(controller, "alpha.txt", "alpha_copy.txt", dest)
    finally:
        controller.quit()

    expected = dict(before)
    expected["dest/alpha_copy.txt"] = before["source/alpha.txt"]
    assert _snapshot_tree(root) == expected


def test_move_mutation_rehomes_hash_without_corrupting_payload(ytnova_binary, tmp_path):
    root = tmp_path / "move_integrity"
    source = root / "source"
    dest = root / "dest"
    source.mkdir(parents=True)
    dest.mkdir()
    (source / "beta.txt").write_text("beta payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _move_selected_file(controller, "beta.txt", "beta_moved.txt", dest)
    finally:
        controller.quit()

    expected = dict(before)
    moved_hash = expected.pop("source/beta.txt")
    expected["dest/beta_moved.txt"] = moved_hash
    assert _snapshot_tree(root) == expected


def test_rename_mutation_preserves_file_hash(ytnova_binary, tmp_path):
    root = tmp_path / "rename_integrity"
    root.mkdir()
    (root / "gamma.txt").write_text("gamma payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _rename_selected_file(
            controller, "gamma.txt", "gamma_renamed.txt", root / "gamma_renamed.txt"
        )
    finally:
        controller.quit()

    expected = {"gamma_renamed.txt": before["gamma.txt"]}
    assert _snapshot_tree(root) == expected


def test_delete_mutation_removes_only_selected_file(ytnova_binary, tmp_path):
    root = tmp_path / "delete_integrity"
    root.mkdir()
    (root / "keep.txt").write_text("keep", encoding="utf-8")
    (root / "remove.txt").write_text("remove", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        _delete_selected_file(controller, "remove.txt", root / "remove.txt")
    finally:
        controller.quit()

    expected = dict(before)
    expected.pop("remove.txt")
    assert _snapshot_tree(root) == expected


def test_copy_prompt_cancel_keeps_tree_snapshot_identical(ytnova_binary, tmp_path):
    root = tmp_path / "copy_cancel_integrity"
    root.mkdir()
    (root / "cancel.txt").write_text("cancel payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        controller.select_file("cancel.txt")
        controller.child.send(Keys.COPY)
        controller.child.expect("COPY")
        controller.input_text("cancel_copy.txt")
        controller.child.expect("To Directory")
        assert controller.send_and_wait_for_screen_change(Keys.ESC, timeout=1.0)
    finally:
        controller.quit()

    assert _snapshot_tree(root) == before


def test_same_path_copy_rejection_keeps_tree_snapshot_identical(ytnova_binary, tmp_path):
    root = tmp_path / "same_path_copy_integrity"
    root.mkdir()
    (root / "same.txt").write_text("same payload", encoding="utf-8")

    before = _snapshot_tree(root)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        controller.select_file("same.txt")
        controller.child.send(Keys.COPY)
        controller.child.expect("COPY")
        controller.input_text("same.txt")
        controller.child.expect("To Directory")
        controller.input_text("")
        assert controller.wait_for_condition(
            lambda _lines: _snapshot_tree(root) == before,
            timeout=1.0,
            description="same-path copy rejection",
        )
    finally:
        controller.quit()

    assert _snapshot_tree(root) == before


def test_archive_copy_rewrite_updates_member_hashes_deterministically(ytnova_binary, tmp_path):
    import io

    root = tmp_path / "archive_rewrite_integrity"
    root.mkdir()
    source_file = root / "archive_source.txt"
    source_file.write_text("archive payload", encoding="utf-8")

    archive_path = root / "dst.tar"
    with tarfile.open(archive_path, "w") as archive:
        payload = b"keep payload"
        info = tarfile.TarInfo(name="keep.txt")
        info.size = len(payload)
        info.mode = 0o644
        info.mtime = int(time.time())
        archive.addfile(info, io.BytesIO(payload))

    before_members = _snapshot_archive(archive_path)

    controller = YtreeNovaController(ytnova_binary, str(root))
    try:
        controller.wait_for_startup()
        assert controller.send_and_wait_for_screen_change(Keys.LOG, timeout=1.0)
        controller.input_text(str(archive_path))
        assert controller.wait_for_condition(
            lambda lines: lines
            if any("ARCHIVE" in line for line in lines)
            and any("\\ exit" in line for line in lines)
            else False,
            timeout=2.0,
            description="logged archive tree",
        )
        assert controller.send_and_wait_for_condition(
            "\\",
            lambda lines: lines
            if any("archive_source.txt" in line for line in lines)
            and any("FILE" in line and "file view" in line for line in lines)
            else False,
            timeout=1.5,
        )
        assert controller.send_and_wait_for_condition(
            Keys.ESC,
            lambda lines: lines
            if any("DIR" in line and "dir view" in line for line in lines)
            else False,
            timeout=2.0,
        ), controller.last_wait_diagnostic

        _copy_selected_file(controller, "archive_source.txt", "copied_into_archive.txt", archive_path)
    finally:
        controller.quit()

    after_members = _snapshot_archive(archive_path)
    expected_members = dict(before_members)
    expected_members["copied_into_archive.txt"] = _sha256_file(source_file)
    assert after_members == expected_members
