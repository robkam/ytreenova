import subprocess
import tarfile
import zipfile
from pathlib import Path

import pytest

UNSUPPORTED_FORMAT_ERROR = -2


@pytest.fixture(scope="session")
def archive_backend_driver(tmp_path_factory):
    repo_root = Path(__file__).resolve().parents[1]
    driver_src = repo_root / "tests" / "archive_backend_driver.c"
    driver_bin = tmp_path_factory.mktemp("archive_backend") / "archive_backend_driver"

    compile_cmd = [
        "cc",
        "-std=c99",
        "-D_GNU_SOURCE",
        "-DHAVE_LIBARCHIVE",
        "-Iinclude",
        str(driver_src),
        "src/fs/archive_write.c",
        "-o",
        str(driver_bin),
        "-larchive",
    ]
    subprocess.run(
        compile_cmd,
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    return driver_bin


def _run_backend(driver, dest_path, source_paths):
    cmd = [str(driver), str(dest_path)] + [str(path) for path in source_paths]
    completed = subprocess.run(cmd, check=True, capture_output=True, text=True)
    return int(completed.stdout.strip())


def _run_add_tree(driver, archive_path, source_path, destination_path):
    completed = subprocess.run(
        [
            str(driver),
            "--add-tree",
            str(archive_path),
            str(source_path),
            destination_path,
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    return tuple(int(value) for value in completed.stdout.split())


def _run_add_dir(driver, archive_path, destination_path):
    completed = subprocess.run(
        [str(driver), "--add-dir", str(archive_path), destination_path],
        check=True,
        capture_output=True,
        text=True,
    )
    return tuple(int(value) for value in completed.stdout.split())


def test_archive_create_zip_from_sources(archive_backend_driver, tmp_path):
    alpha = tmp_path / "alpha.txt"
    beta = tmp_path / "beta.txt"
    alpha.write_text("alpha\n", encoding="utf-8")
    beta.write_text("beta\n", encoding="utf-8")

    dest = tmp_path / "bundle.zip"
    rc = _run_backend(
        archive_backend_driver,
        dest.resolve(),
        [alpha.resolve(), beta.resolve()],
    )

    assert rc == 0
    with zipfile.ZipFile(dest, "r") as zf:
        names = sorted(zf.namelist())
        assert names == ["alpha.txt", "beta.txt"]
        assert zf.read("alpha.txt") == b"alpha\n"
        assert zf.read("beta.txt") == b"beta\n"


def test_archive_create_tar_gz_from_sources(archive_backend_driver, tmp_path):
    alpha = tmp_path / "alpha.txt"
    beta = tmp_path / "beta.txt"
    alpha.write_text("alpha\n", encoding="utf-8")
    beta.write_text("beta\n", encoding="utf-8")

    dest = tmp_path / "bundle.tar.gz"
    rc = _run_backend(
        archive_backend_driver,
        dest.resolve(),
        [alpha.resolve(), beta.resolve()],
    )

    assert rc == 0
    with tarfile.open(dest, "r:gz") as tf:
        names = sorted(member.name for member in tf.getmembers())
        assert names == ["alpha.txt", "beta.txt"]
        assert tf.extractfile("alpha.txt").read() == b"alpha\n"
        assert tf.extractfile("beta.txt").read() == b"beta\n"


def test_archive_create_unsupported_extension_returns_error(
    archive_backend_driver, tmp_path
):
    alpha = tmp_path / "alpha.txt"
    alpha.write_text("alpha\n", encoding="utf-8")

    dest = tmp_path / "bundle.unsupported"
    rc = _run_backend(
        archive_backend_driver, dest.resolve(), [alpha.resolve()]
    )

    assert rc == UNSUPPORTED_FORMAT_ERROR
    assert not dest.exists()


def test_archive_add_tree_reports_progress_during_single_entry_rewrite(
    archive_backend_driver, tmp_path
):
    archive_path = tmp_path / "target.tar"
    existing = tmp_path / "existing.bin"
    existing.write_bytes(b"x" * (256 * 1024))
    with tarfile.open(archive_path, "w") as archive:
        archive.add(existing, arcname="existing.bin")

    source = tmp_path / "source"
    source.mkdir()
    (source / "payload.txt").touch()

    rc, progress_count = _run_add_tree(
        archive_backend_driver, archive_path, source, "copied"
    )

    assert rc == 0
    assert progress_count >= 4
    with tarfile.open(archive_path) as archive:
        assert set(archive.getnames()) == {
            "existing.bin",
            "copied",
            "copied/payload.txt",
        }
        assert archive.extractfile("copied/payload.txt").read() == b""


def test_archive_add_tree_collision_preserves_original_archive(
    archive_backend_driver, tmp_path
):
    archive_path = tmp_path / "target.tar"
    existing = tmp_path / "existing.txt"
    existing.write_text("keep", encoding="utf-8")
    with tarfile.open(archive_path, "w") as archive:
        archive.add(existing, arcname="copied/existing.txt")
    original_bytes = archive_path.read_bytes()

    source = tmp_path / "source"
    source.mkdir()
    (source / "replacement.txt").write_text("replacement", encoding="utf-8")

    rc, _ = _run_add_tree(archive_backend_driver, archive_path, source, "copied")

    assert rc == -1
    assert archive_path.read_bytes() == original_bytes


def test_archive_add_directory_without_filesystem_source(
    archive_backend_driver, tmp_path
):
    archive_path = tmp_path / "target.tar"
    existing = tmp_path / "existing.txt"
    existing.write_text("keep", encoding="utf-8")
    with tarfile.open(archive_path, "w") as archive:
        archive.add(existing, arcname="existing.txt")

    rc, progress_count = _run_add_dir(
        archive_backend_driver, archive_path, "created"
    )

    assert rc == 0
    assert progress_count >= 2
    with tarfile.open(archive_path) as archive:
        members = {member.name: member for member in archive.getmembers()}
        assert set(members) == {"existing.txt", "created"}
        assert members["created"].isdir()
