"""Positive locale, theme, and size resilience evidence for behavioural tests."""
from __future__ import annotations

import json
import re
import shutil
import subprocess
from pathlib import Path

import pytest

from helpers_ui import complete_modal_round_trip, screen_text
from test_help_text_contract import _follow_any_link, _open, _return_to
from tui_harness import YtreeNovaTUI
from ytnova_keys import Keys

REPO_ROOT = Path(__file__).resolve().parents[1]
YTNOVA_BIN = str((REPO_ROOT / "build" / "ytnova").resolve())
MATRIX = json.loads(
    (REPO_ROOT / "tests" / "contract_resilience_matrix.json").read_text(
        encoding="utf-8"
    )
)


def _matrix_cases():
    cases = []
    for locale in MATRIX["locale_sources"]:
        for theme in MATRIX["themes"]:
            for profile, size in MATRIX["size_profiles"].items():
                cases.append(
                    pytest.param(
                        locale,
                        theme,
                        (size["rows"], size["columns"]),
                        id=f"{locale}-{theme}-{profile}",
                    )
                )
    return cases


def _help_asset_locale_sources():
    make_database = subprocess.run(
        ["make", "-pn"], cwd=REPO_ROOT, capture_output=True, text=True, check=True
    ).stdout
    match = re.search(r"^HELP_F1_SOURCE\s*=\s*(?P<source>\S+)$", make_database, re.MULTILINE)
    assert match, "make must declare the canonical F1 source passed to help-assets"
    master_source = match.group("source")
    return {
        master_source,
        *(str(path) for path in (REPO_ROOT / "etc/help").glob("f1.*.md")),
    }


def test_matrix_locale_catalog_matches_help_asset_sources():
    expected_sources = {
        str((REPO_ROOT / source).relative_to(REPO_ROOT))
        for source in _help_asset_locale_sources()
    }
    assert set(MATRIX["locale_sources"].values()) == expected_sources


def _root(tmp_path, locale, theme):
    root = tmp_path / f"matrix_{locale}_{theme}"
    root.mkdir()
    (root / "alpha.txt").write_text("alpha\n", encoding="utf-8")
    (root / "beta.txt").write_text("beta\n", encoding="utf-8")
    (root / ".ytnova").write_text(f"[GLOBAL]\nTHEME={theme}\n", encoding="utf-8")
    shutil.copyfile(REPO_ROOT / "etc" / "ytnova.themes", root / ".ytnova.themes")
    return root


def _locale_environment(locale):
    if locale == "en":
        return None
    return {"LC_ALL": "de_DE.UTF-8", "LANG": "de_DE.UTF-8", "LANGUAGE": "de"}


@pytest.mark.parametrize(("locale", "theme", "dimensions"), _matrix_cases())
def test_contract_resilience_matrix_preserves_interactive_capabilities(
    tmp_path, locale, theme, dimensions
):
    root = _root(tmp_path, locale, theme)
    tui = YtreeNovaTUI(
        YTNOVA_BIN,
        cwd=str(root),
        env_extra=_locale_environment(locale),
        dimensions=dimensions,
    )
    try:
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)

        origin = _open(tui, "main.dir", locale=locale)
        assert _follow_any_link(tui, Keys.RIGHT), screen_text(tui)
        assert tui.send_and_wait_for_condition(
            Keys.LEFT,
            lambda lines: lines if any(origin["title"] in line for line in lines) else False,
            timeout=1.5,
        ), screen_text(tui)
        _return_to(
            tui,
            lambda lines: lines
            if any("alpha.txt" in line for line in lines)
            and all(origin["title"] not in line for line in lines)
            else False,
        )

        assert complete_modal_round_trip(tui, Keys.F9, Keys.ESC), screen_text(tui)
        tui.send_keystroke(Keys.ESC)

        tui.send_keystroke(Keys.ENTER)
        assert tui.wait_for_content("alpha.txt", timeout=1.5), screen_text(tui)
        assert complete_modal_round_trip(
            tui,
            Keys.DELETE,
            Keys.CONFIRM_NO,
        ), screen_text(tui)
        tui.send_keystroke(Keys.CONFIRM_NO)
    finally:
        tui.quit()
