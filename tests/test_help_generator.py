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
    man_topics = helpgen.parse_help_source((REPO_ROOT / "etc" / "help" / "man.en.md").read_text(encoding="utf-8"))
    helpgen.validate_topic_inventory(f1_topics)
    helpgen.validate_locale_topic_projection(f1_topics, de_topics, locale_id="de")
    header = helpgen.render_runtime_header(f1_topics, source_path="etc/help/f1.en.md", locale_topics=[("de", "etc/help/f1.de.md", de_topics)])
    assert len(f1_topics) > 0
    assert len(man_topics) > 0
    assert all(topic.contexts is not None for topic in f1_topics)
    assert helpgen.generated_banner("etc/help/man.en.md") in helpgen.render_manpage_markdown(man_topics, usage_mode=False, source_path="etc/help/man.en.md")
    assert helpgen.generated_banner("etc/help/man.en.md") in helpgen.render_manpage_markdown(man_topics, usage_mode=True, source_path="etc/help/man.en.md")
    assert "generated_help_catalogs" in header


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


def test_help_generator_allows_f1_topics_without_long_form_sections():
    f1_source = """## topic:test
```ytnova-help-meta
title: Test
contexts: main.test
```
### Contextual F1
One line.
"""

    topics = helpgen.parse_help_source(f1_source)

    assert topics[0].long_form_sections == ()


def test_help_generator_requires_long_form_sections_for_man_source():
    man_source = """## topic:test
```ytnova-help-meta
title: Test
contexts: none
```
### Contextual F1
One line.
"""

    with pytest.raises(helpgen.HelpSourceError, match="missing ### Long form"):
        helpgen.parse_help_source(man_source, require_long_form=True)


def test_help_generator_rejects_duplicate_runtime_context_ownership():
    broken_f1 = """## topic:first
```ytnova-help-meta
title: First
contexts: prompt.shared
```
### Contextual F1
One line.
### Long form
#### Section
Body.

## topic:second
```ytnova-help-meta
title: Second
contexts: prompt.shared
```
### Contextual F1
Another line.
### Long form
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
### Long form
#### Section
Body.

## topic:second
```ytnova-help-meta
title: Second
contexts: none
```
### Contextual F1
Another line.
### Long form
#### Section
Body.
"""

    f1_topics = helpgen.parse_help_source(broken_f1)
    man_topics = helpgen.parse_help_source(man_source)

    with pytest.raises(helpgen.HelpSourceError, match="prompt.shared"):
        helpgen.validate_topic_inventory(f1_topics)


def test_help_generator_allows_locales_to_reorder_explainer_links():
    canonical = """## topic:first
```ytnova-help-meta
title: First
contexts: main.first
```
### Contextual F1
One line.
### Explainer links
- [Second](topic:second)
- [Third](topic:third)

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
        "- [Second](topic:second)\n- [Third](topic:third)",
        "- [Drittes](topic:third)\n- [Zweites](topic:second)",
    )

    helpgen.validate_locale_topic_projection(
        helpgen.parse_help_source(canonical),
        helpgen.parse_help_source(localized),
        locale_id="de",
    )


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
