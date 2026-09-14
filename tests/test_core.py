import pytest
import os
import pexpect
import shutil
import subprocess
from pathlib import Path
from ytnova_keys import Keys


@pytest.fixture(scope="session")
def archive_payload_driver(tmp_path_factory):
    repo_root = Path(__file__).resolve().parents[1]
    driver_src = repo_root / "tests" / "archive_payload_driver.c"
    driver_bin = tmp_path_factory.mktemp("archive_payload") / "archive_payload_driver"

    compile_cmd = [
        "cc",
        "-std=c99",
        "-D_GNU_SOURCE",
        "-Iinclude",
        str(driver_src),
        "src/ui/archive_payload.c",
        "-o",
        str(driver_bin),
    ]
    subprocess.run(
        compile_cmd,
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    )
    return driver_bin


def _run_archive_payload_driver(driver, source_paths):
    cmd = [str(driver)] + [str(path) for path in source_paths]
    completed = subprocess.run(cmd, check=True, capture_output=True, text=True)
    lines = [line.strip() for line in completed.stdout.splitlines() if line.strip()]

    rc = int(lines[0].split("=", 1)[1])
    original_source_list = []
    expanded_file_list = []

    for line in lines[1:]:
        if line.startswith("orig:"):
            original_source_list.append(line[len("orig:"):])
        elif line.startswith("exp:"):
            source_path, archive_path = line[len("exp:"):].split("|", 1)
            expanded_file_list.append((source_path, archive_path))

    return rc, original_source_list, expanded_file_list

def test_simple_copy(controller, sandbox):
    yt = controller()
    yt.wait_for_startup()

    # 1. Select
    yt.select_file("root_file.txt")

    # 2. Copy
    assert yt.send_and_wait_for_condition(
        Keys.COPY,
        lambda lines: any("COPY" in line for line in lines),
        timeout=1.5,
    )

    # 3. Input New Name
    yt.input_text("copy.txt")

    # 4. Input Destination
    # Updated to match src/cmd/copy.c: "To Directory:"
    yt.child.expect("To Directory")
    yt.input_text("") # Accept current directory

    # 5. Verify
    copied_path = sandbox / "source" / "copy.txt"
    assert yt.wait_for_condition(
        lambda _lines: copied_path.exists(),
        timeout=2.0,
        description="completed file copy",
    )

    yt.quit()

def test_rename(controller, sandbox):
    yt = controller()
    yt.wait_for_startup()

    # 1. Select
    yt.select_file("root_file.txt")

    # 2. Rename
    assert yt.send_and_wait_for_condition(
        Keys.RENAME,
        lambda lines: any("RENAME" in line for line in lines),
        timeout=1.5,
    )
    # Updated to match src/cmd/rename.c: "RENAME TO:"
    # Use a partial prompt match so wording changes do not break the flow.
    yt.child.expect("RENAME")

    # 3. Input New Name
    yt.input_text("renamed.txt")

    # 4. Verify
    renamed_path = sandbox / "source" / "renamed.txt"
    source_path = sandbox / "source" / "root_file.txt"
    assert yt.wait_for_condition(
        lambda _lines: renamed_path.exists() and not source_path.exists(),
        timeout=2.0,
        description="completed file rename",
    )

    yt.quit()

def test_move(controller, sandbox):
    yt = controller()
    yt.wait_for_startup()

    # 1. Select
    yt.select_file("root_file.txt")

    # 2. Move
    assert yt.send_and_wait_for_condition(
        Keys.MOVE,
        lambda lines: any("MOVE" in line for line in lines),
        timeout=1.5,
    )

    # 3. Input New Name
    yt.input_text("moved.txt")

    # 4. Input Destination
    # Updated to match src/cmd/move.c: "To Directory:"
    yt.child.expect("To Directory")
    yt.input_text("../dest")

    # 5. Verify
    moved_path = sandbox / "dest" / "moved.txt"
    source_path = sandbox / "source" / "root_file.txt"
    assert yt.wait_for_condition(
        lambda _lines: moved_path.exists() and not source_path.exists(),
        timeout=2.0,
        description="completed file move",
    )

    yt.quit()

def test_tagged_copy_overwrite_all_applies_to_remaining_conflicts(controller, sandbox):
    source_dir = sandbox / "source"
    dest_dir = sandbox / "dest"

    (source_dir / "root_file.txt").unlink()
    (source_dir / "alpha.txt").write_text("alpha source content", encoding="utf-8")
    (source_dir / "beta.txt").write_text("beta source content", encoding="utf-8")
    (dest_dir / "alpha.txt").write_text("old alpha content", encoding="utf-8")
    (dest_dir / "beta.txt").write_text("old beta content", encoding="utf-8")

    yt = controller(cwd=str(source_dir))
    overwrite_alpha = r"Overwrite .*alpha\.txt"
    overwrite_beta = r"Overwrite .*beta\.txt"

    try:
        yt.wait_for_startup()

        yt.child.send(Keys.ENTER)
        yt.wait_for_refresh()

        yt.child.send("\x14")  # Ctrl+T (tag all)

        yt.child.send("\x03")  # Ctrl+C (copy tagged)
        yt.child.expect("COPY: TAGGED FILES")
        yt.child.send(Keys.ENTER)  # Keep default "*"
        yt.child.expect("To Directory")
        yt.input_text("../dest")

        prompt_or_conflict = yt.child.expect(
            ["Ask for confirmation for each overwrite", overwrite_alpha, overwrite_beta],
            timeout=3.0,
        )
        assert prompt_or_conflict != 0
        first_conflict = prompt_or_conflict - 1
        yt.child.send("A")

        second_conflict = overwrite_beta if first_conflict == 0 else overwrite_alpha
        with pytest.raises(pexpect.TIMEOUT):
            yt.child.expect(second_conflict, timeout=1.0)

        expected_alpha = dest_dir / "alpha.txt"
        expected_beta = dest_dir / "beta.txt"
        assert yt.wait_for_condition(
            lambda _lines: (
                expected_alpha.exists()
                and expected_beta.exists()
                and expected_alpha.read_text(encoding="utf-8") == "alpha source content"
                and expected_beta.read_text(encoding="utf-8") == "beta source content"
            ),
            timeout=3.0,
            description="both tagged copies to finish",
        )

        assert expected_alpha.read_text(encoding="utf-8") == "alpha source content"
        assert expected_beta.read_text(encoding="utf-8") == "beta source content"
    finally:
        yt.quit()

def test_tagged_move_overwrite_all_applies_to_remaining_conflicts(controller, sandbox):
    source_dir = sandbox / "source"
    dest_dir = source_dir / "move_dest"

    (source_dir / "root_file.txt").unlink()
    (source_dir / "alpha.txt").write_text("alpha source content", encoding="utf-8")
    (source_dir / "beta.txt").write_text("beta source content", encoding="utf-8")
    dest_dir.mkdir(exist_ok=True)
    (dest_dir / "alpha.txt").write_text("old alpha content", encoding="utf-8")
    (dest_dir / "beta.txt").write_text("old beta content", encoding="utf-8")

    yt = controller(cwd=str(source_dir))
    overwrite_alpha = r"Overwrite .*alpha\.txt"
    overwrite_beta = r"Overwrite .*beta\.txt"

    try:
        yt.wait_for_startup()

        yt.child.send(Keys.ENTER)
        yt.wait_for_refresh()

        yt.child.send("\x14")  # Ctrl+T (tag all)

        yt.child.send("\x0e")  # Ctrl+N (move tagged)
        yt.child.expect("MOVE: TAGGED FILES")
        yt.child.send(Keys.ENTER)  # Keep default "*"
        yt.child.expect("To Directory")
        yt.input_text(str(dest_dir))

        prompt_or_conflict = yt.child.expect(
            ["Ask for confirmation for each overwrite", overwrite_alpha, overwrite_beta],
            timeout=3.0,
        )
        assert prompt_or_conflict != 0
        first_conflict = prompt_or_conflict - 1
        yt.child.send("A")

        second_conflict = overwrite_beta if first_conflict == 0 else overwrite_alpha
        with pytest.raises(pexpect.TIMEOUT):
            yt.child.expect(second_conflict, timeout=1.0)

        expected_alpha = dest_dir / "alpha.txt"
        expected_beta = dest_dir / "beta.txt"
        source_alpha = source_dir / "alpha.txt"
        source_beta = source_dir / "beta.txt"
        assert yt.wait_for_condition(
            lambda _lines: (
                expected_alpha.exists()
                and expected_beta.exists()
                and expected_alpha.read_text(encoding="utf-8") == "alpha source content"
                and expected_beta.read_text(encoding="utf-8") == "beta source content"
                and not source_alpha.exists()
                and not source_beta.exists()
            ),
            timeout=3.0,
            description="both tagged moves to finish",
        )

        assert expected_alpha.read_text(encoding="utf-8") == "alpha source content"
        assert expected_beta.read_text(encoding="utf-8") == "beta source content"
        assert not source_alpha.exists()
        assert not source_beta.exists()
    finally:
        yt.quit()

def test_wildcard_rename(controller, sandbox):
    """
    Regression: Rename root_file.txt to new.* (should preserve extension .txt)
    """
    yt = controller()
    yt.wait_for_startup()

    yt.select_file("root_file.txt")

    assert yt.send_and_wait_for_condition(
        Keys.RENAME,
        lambda lines: any("RENAME" in line for line in lines),
        timeout=1.5,
    )
    yt.input_text("new.*")

    expected = sandbox / "source" / "new.txt"
    bad_result = sandbox / "source" / "new.root_file.txt"

    assert yt.wait_for_condition(
        lambda _lines: expected.exists(),
        timeout=1.0,
        description=f"renamed file {expected}",
    )

    if bad_result.exists():
        pytest.fail("Regression: 'new.*' resulted in 'new.root_file.txt'")

    assert expected.exists(), "Rename failed (new.txt not found)"
    yt.quit()

def test_path_copy(controller, sandbox):
    """
    Verifies the PathCopy (Y) command.
    Scenario:
        1. Select 'deep_file.txt' (which is inside deep/nested/).
        2. Perform PathCopy.
        3. Target 'dest' folder using ABSOLUTE path.
        4. Verify that the directory structure 'source/deep/nested/' is recreated in 'dest'.
           (Since root of volume is sandbox, file at sandbox/source/deep/nested is relative as source/deep/nested)
    """
    yt = controller()
    yt.wait_for_startup()

    # 1. Select the nested file
    # select_file uses EXPAND_ALL/SHOWALL/FILTER to find the file.
    yt.select_file("deep_file.txt")

    # 2. Initiate PathCopy
    assert yt.send_and_wait_for_condition(
        Keys.PATHCOPY,
        lambda lines: any("PATHCOPY" in line for line in lines),
        timeout=1.5,
    )

    # 3. Verify Prompt 1 (Filename)
    # Expect "PATHCOPY: " or "AS:"
    yt.child.expect("PATHCOPY")

    # Accept default filename
    yt.child.send(Keys.ENTER)

    # 4. Verify Prompt 2 (Destination Dir)
    # Updated to match src/cmd/copy.c: "To Directory:"
    yt.child.expect("To Directory")

    # 5. Input Destination (ABSOLUTE PATH)
    dest_path = sandbox / "dest"
    yt.input_text(str(dest_path))

    # 6. Handle "Create Directory?" Prompt
    # ytnova might ask to create the structure.
    try:
        # Check for "Create" prompt or timeout if it proceeds automatically
        index = yt.child.expect([r"[Cc]reate", pexpect.TIMEOUT], timeout=1.0)
        if index == 0:
            # If prompt appeared, confirm Yes
            yt.child.send(Keys.CONFIRM_YES)
    except Exception:
        pass

    # 7. Verify Logic
    # Since we operate from 'sandbox' root, and file is in 'source/deep/nested',
    # PathCopy should append 'source/deep/nested' to 'dest'.
    expected_path = sandbox / "dest" / "source" / "deep" / "nested" / "deep_file.txt"

    assert yt.wait_for_condition(
        lambda _lines: expected_path.exists(),
        timeout=2.0,
        description=f"path copy destination {expected_path}",
    )

    # Tripartite Verification
    # System Layer: Does file exist?
    assert expected_path.exists(), f"PathCopy failed: Structure not created at {expected_path}"

    # Data Layer: Is content preserved?
    assert expected_path.read_text(encoding="utf-8") == "deep content"

    yt.quit()


def test_archive_payload_tagged_paths_root_relative(archive_payload_driver, tmp_path):
    tmp_file = (tmp_path / "tmp_side.txt").resolve()
    tmp_file.write_text("tmp\n", encoding="utf-8")

    repo_side_dir = Path.cwd() / ".pytest_archive_payload_root"
    repo_side_dir.mkdir(exist_ok=True)
    repo_file = (repo_side_dir / "repo_side.txt").resolve()
    repo_file.write_text("repo\n", encoding="utf-8")

    try:
        selected = [tmp_file, repo_file]
        rc, original_source_list, expanded_file_list = _run_archive_payload_driver(
            archive_payload_driver,
            selected,
        )
    finally:
        repo_file.unlink(missing_ok=True)
        shutil.rmtree(repo_side_dir, ignore_errors=True)

    assert rc == 0
    assert original_source_list == [str(path) for path in selected]

    expanded_by_source = {source: archive for source, archive in expanded_file_list}
    assert expanded_by_source[str(tmp_file)] == str(tmp_file).lstrip("/")
    assert expanded_by_source[str(repo_file)] == str(repo_file).lstrip("/")


def test_archive_payload_recursive_directory_walking(archive_payload_driver, tmp_path):
    source_dir = (tmp_path / "source_tree").resolve()
    nested_dir = source_dir / "nested"
    source_dir.mkdir()
    nested_dir.mkdir()

    root_file = (source_dir / "root.txt").resolve()
    nested_file = (nested_dir / "leaf.txt").resolve()
    root_file.write_text("root\n", encoding="utf-8")
    nested_file.write_text("leaf\n", encoding="utf-8")

    loop_link = nested_dir / "loop_to_root"
    os.symlink(source_dir, loop_link)

    rc, original_source_list, expanded_file_list = _run_archive_payload_driver(
        archive_payload_driver,
        [source_dir],
    )

    assert rc == 0
    assert original_source_list == [str(source_dir)]

    expanded_relatives = {archive for _, archive in expanded_file_list}
    assert "root.txt" in expanded_relatives
    assert "nested/leaf.txt" in expanded_relatives
    assert all(not rel.startswith("nested/loop_to_root/") for rel in expanded_relatives)
