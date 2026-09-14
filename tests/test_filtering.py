import pytest
import os
import shutil
import tempfile
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

@pytest.fixture
def filter_env(ytnova_binary):
    test_base_dir = tempfile.mkdtemp(prefix="ytnova_filter_")
    for f in ["file1.c", "file2.c", "file3.txt"]:
        with open(os.path.join(test_base_dir, f), "w") as fd:
            fd.write("test")
    yield test_base_dir, ytnova_binary
    shutil.rmtree(test_base_dir)

def test_filter_stats_recalculation(filter_env):
    cwd, binary = filter_env
    tui = YtreeNovaTUI(executable=binary, cwd=cwd)
    # Filter for *.c
    assert tui.send_and_wait_for_condition(
        Keys.FILTER,
        lambda lines: any("FILTER" in line for line in lines),
        timeout=1.0,
    )
    # The prompt might already have something, let's clear it
    tui.send_keystroke("\x15") # C-u
    assert tui.send_and_wait_for_condition(
        "*.c\r",
        lambda lines: all(any(name in line for line in lines) for name in ("file1.c", "file2.c")),
        timeout=1.5,
    )
    
    # Verify Global Mode (S) works
    assert tui.send_and_wait_for_condition(
        Keys.SHOWALL,
        lambda lines: lines
        if all(any(name in line for line in lines) for name in ("FILE", "file1.c", "file2.c"))
        else False,
        timeout=1.5,
    )
    
    screen = "\n".join(tui.get_screen_dump())
    assert "FILE" in screen
    assert "file1.c" in screen
    assert "file2.c" in screen
    assert "file3.txt" not in screen
    
    tui.quit()

def test_show_all_no_matching_files(filter_env):
    cwd, binary = filter_env
    tui = YtreeNovaTUI(executable=binary, cwd=cwd)
    # Filter for non-existent
    assert tui.send_and_wait_for_condition(
        Keys.FILTER,
        lambda lines: any("FILTER" in line for line in lines),
        timeout=1.0,
    )
    tui.send_keystroke("\x15")
    assert tui.send_and_wait_for_screen_change("*.java\r", timeout=1.5)
    
    # Try 'S'
    tui.child.send(Keys.SHOWALL)
    assert tui.wait_for_condition(
        lambda lines: lines
        if any("DIR" in line for line in lines)
        and not any("FILE" in line for line in lines)
        else False,
        timeout=1.0,
        description="directory view to remain active",
    )
    
    # Should stay in DIR view
    screen = "\n".join(tui.get_screen_dump())
    assert "DIR" in screen
    assert "FILE" not in screen
    
    tui.quit()

def test_multi_pattern_filter(ytnova_binary, tmp_path):
    """
    REGRESSION: Filter with multiple patterns (e.g. *.c,*.h) fails.
    """
    d = tmp_path / "filter_multi"
    d.mkdir()
    (d / "file1.c").write_text("c")
    (d / "file2.h").write_text("h")
    (d / "file3.txt").write_text("txt")

    tui = YtreeNovaTUI(executable=ytnova_binary, cwd=str(d))
    # Apply multi-filter
    assert tui.send_and_wait_for_condition(
        Keys.FILTER,
        lambda lines: any("FILTER" in line for line in lines),
        timeout=1.0,
    )
    tui.send_keystroke("\x15") # Clear line
    assert tui.send_and_wait_for_condition(
        "*.c,*.h\r",
        lambda lines: all(any(name in line for line in lines) for name in ("file1.c", "file2.h")),
        timeout=1.5,
    )
    
    # Verify Global Mode
    assert tui.send_and_wait_for_condition(
        Keys.SHOWALL,
        lambda lines: lines
        if any("file1.c" in line for line in lines)
        and any("file2.h" in line for line in lines)
        else False,
        timeout=1.5,
    )
    
    screen = "\n".join(tui.get_screen_dump())
    assert "file1.c" in screen
    assert "file2.h" in screen
    assert "file3.txt" not in screen

    # Test with extra spaces
    assert tui.send_and_wait_for_condition(
        Keys.FILTER,
        lambda lines: any("FILTER" in line for line in lines),
        timeout=1.0,
    )
    tui.send_keystroke("\x15") # Clear line
    assert tui.send_and_wait_for_condition(
        " *.c , *.h \r",
        lambda lines: lines
        if any("file1.c" in line for line in lines)
        and any("file2.h" in line for line in lines)
        else False,
        timeout=1.5,
    )

    screen = "\n".join(tui.get_screen_dump())
    assert "file1.c" in screen
    assert "file2.h" in screen
    
    tui.quit()
