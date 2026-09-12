"""Behavioural contracts for contextual F1 help.

Authored help wording is editable; these tests cover contextual opening,
link navigation, return behaviour, locale dispatch, and resize usability.
"""
from __future__ import annotations

import io
import re
import tarfile
from pathlib import Path

from helpers_ui import drive_action_until, screen_text
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

YTNOVA_BIN = str((Path(__file__).resolve().parents[1] / "build" / "ytnova").resolve())
F1_SOURCES = {"en": Path("etc/help/f1.en.md"), "de": Path("etc/help/f1.de.md")}


def _topics(locale="en"):
    pattern = re.compile(
        r"^## topic:(?P<id>[a-z0-9-]+)\n+```ytnova-help-meta\n"
        r"title: (?P<title>[^\n]+)\ncontexts: (?P<contexts>[^\n]+)\n```"
        r"(?P<body>.*?)(?=^## topic:|\Z)", re.MULTILINE | re.DOTALL,
    )
    return {match.group("id"): match.groupdict() for match in pattern.finditer(F1_SOURCES[locale].read_text(encoding="utf-8"))}


def _topic_for_context(context, locale="en"):
    for topic in _topics(locale).values():
        if context in topic["contexts"].split(","):
            return topic
    raise AssertionError(f"No authored topic owns {context!r}")


def _root(tmp_path, name):
    root = tmp_path / name
    root.mkdir()
    (root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (root / "beta.txt").write_text("beta\n", encoding="utf-8")
    return root


def _spawn(root, *, env_extra=None, dimensions=(36, 120)):
    return YtreeNovaTUI(YTNOVA_BIN, cwd=str(root), env_extra=env_extra, dimensions=dimensions)


def _has_title(topic):
    return lambda lines: lines if any(topic["title"] in line for line in lines) else False


def _open(tui, context, *, key=Keys.F1, locale="en"):
    topic = _topic_for_context(context, locale)
    assert tui.send_and_wait_for_condition(key, _has_title(topic), timeout=1.5), screen_text(tui)
    return topic


def _return_to(tui, predicate):
    assert tui.send_and_wait_for_condition(Keys.ESC, predicate, timeout=1.5), screen_text(tui)


def _follow_any_link(tui, key):
    """Follow a selectable authored link using the shared semantic action driver."""
    before = tui.get_screen_dump()
    return drive_action_until(
        tui, Keys.DOWN,
        lambda _lines: tui.send_and_wait_for_screen_change(key, timeout=0.2),
        max_actions=80, timeout=0.2,
    ) or tui.get_screen_dump() != before


def _open_next_index_topic(tui, index_topic, expected_topic):
    def open_selected(lines):
        if not any(index_topic["title"] in line for line in lines):
            return False
        tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=0.2)
        rendered = tui.get_screen_dump()
        if any(expected_topic["title"] in line for line in rendered):
            return rendered
        if not any(index_topic["title"] in line for line in rendered):
            assert tui.send_and_wait_for_condition(
                Keys.LEFT, _has_title(index_topic), timeout=1.0
            ), screen_text(tui)
        return False

    rendered = drive_action_until(
        tui, Keys.DOWN, open_selected, max_actions=150, timeout=0.2
    )
    if not rendered:
        return False
    assert tui.send_and_wait_for_condition(
        Keys.LEFT, _has_title(index_topic), timeout=1.0
    ), screen_text(tui)
    return rendered


def _create_tar(path):
    with tarfile.open(path, "w") as archive:
        payload = b"inside\n"
        info = tarfile.TarInfo("inside_dir/inside.txt")
        info.size = len(payload)
        archive.addfile(info, io.BytesIO(payload))


def test_directory_help_opens_and_returns_to_invoking_view(tmp_path):
    tui = _spawn(_root(tmp_path, "directory_help"))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _open(tui, "main.dir")
        _return_to(tui, lambda lines: lines if any("alpha.txt" in line for line in lines) else False)
    finally:
        tui.quit()


def test_command_strip_preserves_utf8_labels_used_by_localized_help(tmp_path):
    root = _root(tmp_path, "utf8_command_strip")
    config_dir = root / ".config" / "ytnova"
    config_dir.mkdir(parents=True)
    (config_dir / "commands.conf").write_text(
        "[DIR]\nA | A | Ändern | ACTION_CMD_A |\n", encoding="utf-8"
    )
    tui = _spawn(root)
    try:
        assert tui.wait_for_content("Ändern", timeout=1.5), screen_text(tui)
    finally:
        tui.quit()


def test_help_accepts_arrow_sequences_without_leaving_popup(tmp_path):
    tui = _spawn(_root(tmp_path, "arrow_help"))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        topic = _open(tui, "main.dir")
        for key in ("\033[B", "\033[C", "\033[D", Keys.DOWN, Keys.RIGHT, Keys.LEFT):
            assert tui.send_and_wait_for_condition(key, _has_title(topic), timeout=1.0), screen_text(tui)
    finally:
        tui.quit()


def test_help_navigation_keys_do_not_trigger_locale_mnemonics(tmp_path):
    env = {
        "LC_ALL": "de_DE.UTF-8",
        "LANG": "de_DE.UTF-8",
        "LANGUAGE": "de",
    }
    tui = _spawn(_root(tmp_path, "locale_navigation_key"), env_extra=env)
    try:
        origin = _open(tui, "main.dir", locale="de")
        assert tui.send_and_wait_for_screen_change(Keys.END, timeout=1.5), screen_text(tui)
        assert any(origin["title"] in line for line in tui.get_screen_dump()), screen_text(tui)
    finally:
        tui.quit()


def test_authored_link_supports_right_and_enter_and_left_returns(tmp_path):
    for index, key in enumerate((Keys.RIGHT, Keys.ENTER)):
        tui = _spawn(_root(tmp_path, f"link_{index}"))
        try:
            assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
            origin = _open(tui, "main.dir")
            assert _follow_any_link(tui, key), screen_text(tui)
            assert tui.send_and_wait_for_condition(Keys.LEFT, _has_title(origin), timeout=1.0), screen_text(tui)
        finally:
            tui.quit()


def test_context_owned_help_returns_to_file_and_prompt_invokers(tmp_path):
    tui = _spawn(_root(tmp_path, "owned_contexts"))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("beta.txt", timeout=1.5), screen_text(tui)
        _open(tui, "main.file")
        _return_to(tui, lambda lines: lines if any("beta.txt" in line for line in lines) else False)
        tui.send_keystroke("x")
        assert tui.wait_for_content("COMMAND", timeout=1.0), screen_text(tui)
        _open(tui, "prompt.execute-file")
        _return_to(tui, lambda lines: lines if any("COMMAND" in line for line in lines) else False)
        tui.send_keystroke(Keys.ESC)
    finally:
        tui.quit()


def test_archive_help_uses_context_owned_topic_and_returns(tmp_path):
    root = tmp_path / "archive_help"
    root.mkdir()
    _create_tar(root / "inside.tar")
    tui = _spawn(root)
    try:
        assert tui.send_and_wait_for_screen_change(Keys.ENTER, timeout=1.5)
        assert tui.send_and_wait_for_screen_change(Keys.LOG, timeout=1.5)
        tui.child.send(Keys.ENTER)
        assert tui.wait_for_content("inside_dir", timeout=2.0), screen_text(tui)
        _open(tui, "main.archive-dir")
        _return_to(tui, lambda lines: lines if any("inside_dir" in line for line in lines) else False)
    finally:
        tui.quit()


def test_help_remains_usable_after_supported_resize(tmp_path):
    tui = _spawn(_root(tmp_path, "resize_help"))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _open(tui, "main.dir")
        tui.child.setwinsize(24, 80)
        tui.screen.resize(24, 80)
        assert tui.send_and_wait_for_screen_change(Keys.DOWN, timeout=1.5), screen_text(tui)
        quit_label = re.search(
            r"quit-label: (?P<label>[^\n]+)",
            F1_SOURCES["en"].read_text(encoding="utf-8"),
        ).group("label")
        assert any(quit_label in line for line in tui.get_screen_dump()), screen_text(tui)
        _return_to(tui, lambda lines: lines if any("alpha.txt" in line for line in lines) else False)
    finally:
        tui.quit()


def test_every_topic_renders_and_returns_at_supported_narrow_size(tmp_path):
    topics = _topics()
    index = topics["index"]
    index_targets = re.findall(r"\[[^\]]+\]\(topic:([a-z0-9-]+)\)", index["body"])
    source = F1_SOURCES["en"].read_text(encoding="utf-8")
    quit_label = re.search(r"quit-label: (?P<label>[^\n]+)", source).group("label")
    navigation_key = re.search(r"navigation-key: (?P<key>.)", source).group("key")
    tui = _spawn(_root(tmp_path, "all_narrow_help"), dimensions=(24, 80))
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        _open(tui, "main.dir")
        assert tui.send_and_wait_for_condition("i", _has_title(index), timeout=1.5), screen_text(tui)
        assert any(quit_label in line for line in tui.get_screen_dump()), screen_text(tui)

        for target in index_targets:
            rendered = _open_next_index_topic(tui, index, topics[target])
            assert rendered, f"narrow help did not open {target!r}\n{screen_text(tui)}"
            assert any(quit_label in line for line in rendered), (
                f"narrow help clipped its help strip for {target!r}\n{screen_text(tui)}"
            )

        assert set(index_targets) == set(topics) - {"index", "f1-navigation"}
        assert tui.send_and_wait_for_condition(
            navigation_key, _has_title(topics["f1-navigation"]), timeout=1.5
        ), screen_text(tui)
    finally:
        tui.quit()


def test_locale_owned_help_strip_key_opens_a_contextual_topic(tmp_path):
    root = _root(tmp_path, "locale_help")
    for locale, env in (("en", None), ("de", {"LC_ALL": "de_DE.UTF-8", "LANG": "de_DE.UTF-8", "LANGUAGE": "de"})):
        tui = _spawn(root, env_extra=env)
        try:
            origin = _open(tui, "main.dir", locale=locale)
            source = F1_SOURCES[locale].read_text(encoding="utf-8")
            key = re.search(r"index-key: (?P<key>.)", source).group("key")
            assert tui.send_and_wait_for_screen_change(key, timeout=1.5), screen_text(tui)
            assert tui.send_and_wait_for_condition(Keys.LEFT, _has_title(origin), timeout=1.0), screen_text(tui)
        finally:
            tui.quit()
