import importlib.util
from pathlib import Path
import subprocess
import sys

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPO_ROOT / "scripts" / "generate_help_assets.py"


spec = importlib.util.spec_from_file_location("generate_help_assets", SCRIPT_PATH)
helpgen = importlib.util.module_from_spec(spec)
assert spec.loader is not None
sys.modules[spec.name] = helpgen
spec.loader.exec_module(helpgen)


def test_help_generator_preserves_catalog_context_and_locale_projection():
    f1_topics = helpgen.parse_help_source((REPO_ROOT / "etc" / "help" / "f1.en.md").read_text(encoding="utf-8"))
    de_topics = helpgen.parse_help_source((REPO_ROOT / "etc" / "help" / "f1.de.md").read_text(encoding="utf-8"))
    man_topics = helpgen.parse_help_source(
        (REPO_ROOT / "etc" / "help" / "man.en.md").read_text(encoding="utf-8"),
        require_contextual_f1=False,
        require_reference_sections=True,
    )
    helpgen.validate_topic_inventory(f1_topics)
    helpgen.validate_locale_topic_projection(f1_topics, de_topics, locale_id="de")
    header = helpgen.render_runtime_header(f1_topics, source_path="etc/help/f1.en.md", locale_topics=[("de", "etc/help/f1.de.md", de_topics)])
    assert len(f1_topics) > 0
    assert len(man_topics) > 0
    assert all(topic.contexts is not None for topic in f1_topics)
    assert helpgen.generated_banner("etc/help/man.en.md") in helpgen.render_manpage_markdown(man_topics, usage_mode=False, source_path="etc/help/man.en.md")
    assert helpgen.generated_banner("etc/help/man.en.md") in helpgen.render_manpage_markdown(man_topics, usage_mode=True, source_path="etc/help/man.en.md")
    assert "generated_help_catalogs" in header
    assert "manpage" not in header.lower()


def test_c_literal_escapes_question_marks_that_form_c_trigraphs():
    assert helpgen.c_literal("Use ??-* as a pattern.") == '"Use \\?\\?-* as a pattern."'


def test_manpage_projects_enhanced_keyboard_guidance_without_archive_stats_cross_reference():
    man_topics = helpgen.parse_help_source(
        (REPO_ROOT / "etc" / "help" / "man.en.md").read_text(encoding="utf-8"),
        require_contextual_f1=False,
        require_reference_sections=True,
    )
    manpage = helpgen.render_manpage_markdown(
        man_topics, usage_mode=False, source_path="etc/help/man.en.md"
    )

    assert "keyboard_protocol kitty" in manpage
    assert "`0`: Do nothing on filesystem volumes. In archive lists" in manpage
    assert "`0`: Do nothing on filesystem volumes; use `F6` for stats." not in manpage
    assert "Help popup keys" not in manpage


def test_help_generator_rejects_invalid_or_duplicate_help_strip_keys():
    source = """```ytnova-help-strip
left-back-label: Left back
index-label: Index
index-key: I
navigation-label: Navigation
navigation-key: I
follow-label: Right/Enter follow
quit-label: Esc/Q quit
```

## topic:test
```ytnova-help-meta
title: Test
contexts: none
```
### Contextual F1
One line.
"""

    with pytest.raises(helpgen.HelpSourceError, match="distinct"):
        helpgen.parse_help_source(source, require_help_strip=True)


def test_help_generator_requires_help_strip_keys_in_their_labels():
    source = """```ytnova-help-strip
left-back-label: Left back
index-label: Index
index-key: I
navigation-label: Navigation
navigation-key: W
follow-label: Enter/Right follow
quit-label: Esc/Q quit
```

## topic:test
```ytnova-help-meta
title: Test
contexts: none
```
### Contextual F1
One line.
"""

    with pytest.raises(helpgen.HelpSourceError, match="must occur in navigation-label"):
        helpgen.parse_help_source(source, require_help_strip=True)


def test_help_generator_allows_f1_topics_without_reference_sections():
    f1_source = """## topic:test
```ytnova-help-meta
title: Test
contexts: main.test
```
### Contextual F1
One line.
"""

    topics = helpgen.parse_help_source(f1_source)

    assert topics[0].reference_sections == ()


def test_help_generator_uses_reference_subheadings_without_long_form_marker():
    man_source = """## topic:test
```ytnova-help-meta
title: Test
contexts: none
```
#### Reference
Reference body.
"""

    topics = helpgen.parse_help_source(
        man_source, require_contextual_f1=False, require_reference_sections=True
    )

    assert topics[0].reference_sections[0].title == "Reference"


def test_help_generator_rejects_duplicate_runtime_context_ownership():
    broken_f1 = """## topic:first
```ytnova-help-meta
title: First
contexts: prompt.shared
```
### Contextual F1
One line.
#### Section
Body.

## topic:second
```ytnova-help-meta
title: Second
contexts: prompt.shared
```
### Contextual F1
Another line.
#### Section
Body.
"""
    man_source = """## topic:first
```ytnova-help-meta
title: First
contexts: none
```
### Contextual F1
One line.
#### Section
Body.

## topic:second
```ytnova-help-meta
title: Second
contexts: none
```
### Contextual F1
Another line.
#### Section
Body.
"""

    f1_topics = helpgen.parse_help_source(broken_f1)
    man_topics = helpgen.parse_help_source(man_source)

    with pytest.raises(helpgen.HelpSourceError, match="prompt.shared"):
        helpgen.validate_topic_inventory(f1_topics)


def test_help_generator_validates_inline_links_and_locale_target_parity():
    canonical = """## topic:first
```ytnova-help-meta
title: First
contexts: main.first
```
### Contextual F1
Open [Second](topic:second) or [Third](topic:third).

## topic:second
```ytnova-help-meta
title: Second
contexts: none
```
### Contextual F1
One line.

## topic:third
```ytnova-help-meta
title: Third
contexts: none
```
### Contextual F1
One line.
"""
    localized = canonical.replace(
        "[Second](topic:second) or [Third](topic:third)",
        "[Drittes](topic:third) oder [Zweites](topic:second)",
    )

    helpgen.validate_locale_topic_projection(
        helpgen.parse_help_source(canonical),
        helpgen.parse_help_source(localized),
        locale_id="de",
    )

    broken_locale = localized.replace("topic:second", "topic:third", 1)
    with pytest.raises(helpgen.HelpSourceError, match="inline link targets"):
        helpgen.validate_locale_topic_projection(
            helpgen.parse_help_source(canonical),
            helpgen.parse_help_source(broken_locale),
            locale_id="de",
        )


@pytest.mark.parametrize(
    ("body", "error"),
    (
        ("See [Missing](topic:missing).", "unknown topic"),
        ("See [First](topic:first).", "links to itself"),
        ("See [Broken](topic:second.", "malformed inline topic link"),
        ("See [Broken]( topic:second).", "malformed inline topic link"),
        ("See [Broken](topic :second).", "malformed inline topic link"),
        ("See [Broken](Topic:second).", "malformed inline topic link"),
    ),
)
def test_help_generator_rejects_invalid_inline_links(body, error):
    source = f"""## topic:first
```ytnova-help-meta
title: First
contexts: main.first
```
### Contextual F1
{body}

## topic:second
```ytnova-help-meta
title: Second
contexts: none
```
### Contextual F1
One line.
"""

    with pytest.raises(helpgen.HelpSourceError, match=error):
        helpgen.parse_help_source(source)


def test_help_generator_rejects_additional_f1_sections():
    source = """## topic:first
```ytnova-help-meta
title: First
contexts: main.first
```
### Contextual F1
One line.
### More guidance
Open [Second](topic:second).

## topic:second
```ytnova-help-meta
title: Second
contexts: none
```
### Contextual F1
One line.
"""

    with pytest.raises(helpgen.HelpSourceError, match="only ### Contextual F1"):
        helpgen.parse_help_source(source)


@pytest.mark.parametrize(
    ("links", "error"),
    (
        ("- [Alpha](topic:alpha)", "missing: beta"),
        (
            "- [Alpha](topic:alpha)\n- [Alpha again](topic:alpha)\n- [Beta](topic:beta)",
            "repeats an inline link target",
        ),
        (
            "- [Beta](topic:beta)\n- [Alpha](topic:alpha)",
            "alphabetical by label",
        ),
        (
            "- [Alpha](topic:alpha)\n- [Beta](topic:beta)\n- [Help Navigation](topic:f1-navigation)",
            "unexpected: f1-navigation",
        ),
    ),
)
def test_help_generator_rejects_invalid_index_organization(links, error):
    source = f"""## topic:index
```ytnova-help-meta
title: Help Index
contexts: none
```
### Contextual F1
{links}

## topic:alpha
```ytnova-help-meta
title: Alpha
contexts: main.alpha
```
### Contextual F1
Alpha.

## topic:beta
```ytnova-help-meta
title: Beta
contexts: main.beta
```
### Contextual F1
Beta.

## topic:f1-navigation
```ytnova-help-meta
title: Help Navigation
contexts: none
```
### Contextual F1
Navigation.
"""

    with pytest.raises(helpgen.HelpSourceError, match=error):
        helpgen.validate_index_organization(helpgen.parse_help_source(source))


def test_help_generator_drift_checker_rejects_stale_output(tmp_path):
    man_md = tmp_path / "ytnova.1.md"
    usage_md = tmp_path / "USAGE.md"
    runtime_header = tmp_path / "generated_help_topics.h"

    write_result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_PATH),
            "--f1-source",
            "etc/help/f1.en.md",
            "--f1-locale-source",
            "etc/help/f1.de.md",
            "--man-source",
            "etc/help/man.en.md",
            "--man-md",
            str(man_md),
            "--usage-md",
            str(usage_md),
            "--runtime-header",
            str(runtime_header),
            "--write",
        ],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    assert write_result.returncode == 0, write_result.stderr

    runtime_header.write_text(
        runtime_header.read_text(encoding="utf-8").replace(
            "generated_help_topic_count", "generated_help_topic_total", 1
        ),
        encoding="utf-8",
    )

    check_result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_PATH),
            "--f1-source",
            "etc/help/f1.en.md",
            "--f1-locale-source",
            "etc/help/f1.de.md",
            "--man-source",
            "etc/help/man.en.md",
            "--man-md",
            str(man_md),
            "--usage-md",
            str(usage_md),
            "--runtime-header",
            str(runtime_header),
            "--check",
        ],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )

    assert check_result.returncode != 0
    assert "drift" in (check_result.stdout + check_result.stderr).lower()
