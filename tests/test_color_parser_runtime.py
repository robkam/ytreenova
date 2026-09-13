import os
import subprocess
from pathlib import Path


def test_color_parser_accepts_theme_style_syntax(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "color_parser_driver.c"
    binary = tmp_path / "color_parser_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

typedef struct {
  const char *text;
  int expected_fg;
  int expected_bg;
} ColorCase;

static int expect_color(const ColorCase *entry) {
  int fg = -1;
  int bg = -1;

  ParseColorString(entry->text, &fg, &bg);
  if (fg != entry->expected_fg || bg != entry->expected_bg) {
    fprintf(stderr, "%s => %d,%d expected %d,%d\n", entry->text, fg, bg,
            entry->expected_fg, entry->expected_bg);
    return 1;
  }
  return 0;
}

int main(void) {
  const ColorCase cases[] = {
      {"grey on blue", 8, COLOR_BLUE},
      {"gray,black", 8, COLOR_BLACK},
      {"+grey on black", COLOR_WHITE, COLOR_BLACK},
      {"+red on +grey", 9, COLOR_WHITE},
      {"red", COLOR_RED, -1},
      {"cyan,blue", COLOR_CYAN, COLOR_BLUE},
  };
  size_t i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    if (expect_color(&cases[i]) != 0)
      return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=repo_root, check=True)


def test_update_ui_color_ignores_legacy_aliases(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "color_alias_driver.c"
    binary = tmp_path / "color_alias_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UIColor ui_colors[];
extern int NUM_UI_COLORS;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static UIColor *find_color(const char *name) {
  int i;

  for (i = 0; i < NUM_UI_COLORS; ++i) {
    if (strcmp(ui_colors[i].name, name) == 0)
      return &ui_colors[i];
  }
  return NULL;
}

static int expect_unchanged(const char *name, int fg, int bg) {
  UIColor *entry = find_color(name);

  if (entry == NULL || entry->fg != fg || entry->bg != bg) {
    fprintf(stderr, "%s changed through a legacy alias\n", name);
    return 1;
  }
  return 0;
}

int main(void) {
  UIColor *dynamic_text = find_color("dynamic_text");
  UIColor *dialog = find_color("dialog");
  UIColor *warning = find_color("warning");
  UIColor *search_hit = find_color("search_hit");
  int dynamic_fg;
  int dynamic_bg;
  int dialog_fg;
  int dialog_bg;
  int warning_fg;
  int warning_bg;
  int search_fg;
  int search_bg;

  if (dynamic_text == NULL || dialog == NULL || warning == NULL ||
      search_hit == NULL)
    return 1;

  dynamic_fg = dynamic_text->fg;
  dynamic_bg = dynamic_text->bg;
  dialog_fg = dialog->fg;
  dialog_bg = dialog->bg;
  warning_fg = warning->fg;
  warning_bg = warning->bg;
  search_fg = search_hit->fg;
  search_bg = search_hit->bg;

  UpdateUIColor("DIR_COLOR", COLOR_RED, COLOR_BLUE);
  UpdateUIColor("FILE_COLOR", COLOR_RED, COLOR_BLUE);
  UpdateUIColor("DIALOG_COLOR", COLOR_RED, COLOR_BLUE);
  UpdateUIColor("WARN_COLOR", COLOR_RED, COLOR_BLUE);
  UpdateUIColor("GLOBAL_COLOR", COLOR_RED, COLOR_BLUE);

  if (expect_unchanged("dynamic_text", dynamic_fg, dynamic_bg) != 0 ||
      expect_unchanged("dialog", dialog_fg, dialog_bg) != 0 ||
      expect_unchanged("warning", warning_fg, warning_bg) != 0 ||
      expect_unchanged("search_hit", search_fg, search_bg) != 0)
    return 1;

  UpdateUIColor("dynamic_text", COLOR_RED, COLOR_BLUE);
  if (dynamic_text->fg != COLOR_RED || dynamic_text->bg != COLOR_BLUE) {
    fprintf(stderr, "canonical color role was not updated\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=repo_root, check=True)


def test_file_color_rules_preserve_profile_order(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "file_color_order_driver.c"
    binary = tmp_path / "file_color_order_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(void) {
  ViewContext ctx;
  FileColorRule *head;

  memset(&ctx, 0, sizeof(ctx));
  AddFileColorRule(&ctx, "*.sh", COLOR_CYAN, COLOR_BLACK);
  AddFileColorRule(&ctx, "*.py", COLOR_GREEN, COLOR_BLACK);
  head = (FileColorRule *)ctx.file_color_rules_head;

  if (head == NULL || strcmp(head->pattern, "*.sh") != 0 ||
      head->next == NULL || strcmp(head->next->pattern, "*.py") != 0) {
    fprintf(stderr, "file color rule order was not preserved\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=repo_root, check=True)


def test_legacy_profile_color_sections_are_ignored(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    profile = tmp_path / "palette.conf"
    driver = tmp_path / "palette_driver.c"
    binary = tmp_path / "palette_driver"

    profile.write_text(
        """
[COLORS]
DIALOG_COLOR = white on blue

[FILE_COLORS]
archives = red: tar,tgz,zip
scripts = +cyan on black: sh,bash
special = cyan: LINK,EXEC
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>

static int captured_colors;
static int captured_rules;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++captured_colors;
}

static void capture_file_color_rule(ViewContext *ctx, const char *pattern,
                                     int fg, int bg) {
  (void)ctx;
  (void)pattern;
  (void)fg;
  (void)bg;
  ++captured_rules;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = capture_file_color_rule;

  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "ReadProfile failed\n");
    return 1;
  }

  if (captured_colors != 0 || captured_rules != 0) {
    fprintf(stderr, "legacy color sections changed runtime colors: %d/%d\n",
            captured_colors, captured_rules);
    return 1;
  }

  FreeProfileRuntimeData(&ctx);
  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/profile.c",
            "src/ui/color.c",
            "src/util/atomic_file.c",
            "src/util/memory_utils.c",
            "src/util/string_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(profile)], cwd=repo_root, check=True)


def test_profile_validation_matches_startup_for_legacy_and_unknown_sections(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    profile = tmp_path / "legacy_sections.conf"
    driver = tmp_path / "profile_validation_driver.c"
    binary = tmp_path / "profile_validation_driver"

    profile.write_text(
        """
[GLOBAL]
THEME=quiet-blue
IGNORED_GLOBAL=value

[COLORS]
DIALOG_COLOR = white on blue

[FILE_COLORS]
archives = red: tar,tgz,zip

[UNKNOWN]
IGNORED = value
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));

  if (ValidateProfileFile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "profile validation rejected startup-compatible sections\n");
    return 1;
  }
  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "ReadProfile failed\n");
    return 1;
  }
  if (strcmp(GetProfileValue(&ctx, "THEME"), "quiet-blue") != 0) {
    fprintf(stderr, "global profile value was not applied\n");
    return 1;
  }

  FreeProfileRuntimeData(&ctx);
  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/profile.c",
            "src/ui/color.c",
            "src/util/atomic_file.c",
            "src/util/memory_utils.c",
            "src/util/string_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(profile)], cwd=repo_root, check=True)


def test_theme_file_loader_maps_roles_and_palette_rules(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_loader_driver.c"
    binary = tmp_path / "theme_loader_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: tar,zip
scripts = +cyan on black: sh
links = cyan: LINK
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

typedef struct {
  char pattern[32];
  int fg;
  int bg;
} CapturedRule;

static CapturedColor colors[32];
static int color_count;
static CapturedRule rules[8];
static int rule_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static void capture_file_color_rule(ViewContext *ctx, const char *pattern,
                                    int fg, int bg) {
  (void)ctx;
  if (rule_count >= 8)
    return;
  snprintf(rules[rule_count].pattern, sizeof(rules[rule_count].pattern), "%s",
           pattern);
  rules[rule_count].fg = fg;
  rules[rule_count].bg = bg;
  ++rule_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

static int expect_rule(int index, const char *pattern, int fg, int bg) {
  if (strcmp(rules[index].pattern, pattern) != 0 || rules[index].fg != fg ||
      rules[index].bg != bg) {
    fprintf(stderr, "rule %d => %s %d,%d expected %s %d,%d\n", index,
            rules[index].pattern, rules[index].fg, rules[index].bg, pattern,
            fg, bg);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = capture_file_color_rule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (expect_color("dynamic_text", 15, COLOR_BLUE) != 0 ||
      expect_color("tree_lines", 15, COLOR_BLUE) != 0 ||
      expect_color("margin", 15, COLOR_BLUE) != 0 ||
      expect_color("box_lines", COLOR_CYAN, COLOR_BLUE) != 0 ||
      expect_color("selection", COLOR_BLACK, COLOR_WHITE) != 0 ||
      expect_color("error", 15, COLOR_RED) != 0 ||
      expect_color("disabled", 8, COLOR_BLUE) != 0)
    return 1;

  if (rule_count != 4) {
    fprintf(stderr, "captured %d rules\n", rule_count);
    return 1;
  }

  if (expect_rule(0, "*.tar", COLOR_RED, -1) != 0 ||
      expect_rule(1, "*.zip", COLOR_RED, -1) != 0 ||
      expect_rule(2, "*.sh", 14, COLOR_BLACK) != 0 ||
      expect_rule(3, "LINK", COLOR_CYAN, -1) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_margin_defaults_to_dynamic_text_when_omitted(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_margin_default_driver.c"
    binary = tmp_path / "theme_margin_default_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (expect_color("margin", 15, COLOR_BLUE) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_roles_without_explicit_background_inherit_theme_background(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_background_inheritance_driver.c"
    binary = tmp_path / "theme_background_inheritance_driver"

    theme_file.write_text(
        """
[theme sample]
background = red
box_lines = cyan
tree_lines = +white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
footer = white
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white
help_link = cyan
help_link_selection = yellow
info = +white
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (expect_color("box_lines", COLOR_CYAN, COLOR_RED) != 0 ||
      expect_color("tree_lines", 15, COLOR_RED) != 0 ||
      expect_color("margin", 15, COLOR_RED) != 0 ||
      expect_color("static_text", COLOR_WHITE, COLOR_RED) != 0 ||
      expect_color("dynamic_text", 15, COLOR_RED) != 0 ||
      expect_color("keybind", 15, COLOR_RED) != 0 ||
      expect_color("help", COLOR_WHITE, COLOR_RED) != 0 ||
      expect_color("info", 15, COLOR_RED) != 0 ||
      expect_color("disabled", 8, COLOR_RED) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_help_box_lines_default_to_help_colors(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_help_box_lines_default_driver.c"
    binary = tmp_path / "theme_help_box_lines_default_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan
tree_lines = +white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
footer = white
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on red
help_link = cyan
help_link_selection = yellow
info = +white
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

    if (expect_color("help_box_lines", COLOR_WHITE, COLOR_RED) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_palette_omitted_background_inherits_theme_filename_background(
    tmp_path,
):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_background_driver.c"
    binary = tmp_path / "theme_background_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.zip") != 0 ||
      rule->fg != COLOR_RED || rule->bg != COLOR_BLUE) {
    fprintf(stderr, "file rule did not inherit theme filename background\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_loader_skips_color_application_in_no_color_build(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_no_color_driver.c"
    binary = tmp_path / "theme_no_color_driver"

    theme_file.write_text(
        """
[theme sample]
background = white on blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int color_count;
static int file_rule_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

static void capture_file_color_rule(ViewContext *ctx, const char *pattern,
                                    int fg, int bg) {
  (void)ctx;
  (void)pattern;
  (void)fg;
  (void)bg;
  ++file_rule_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule existing_rule;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  memset(&existing_rule, 0, sizeof(existing_rule));
  existing_rule.pattern = "*.old";
  existing_rule.fg = COLOR_GREEN;
  existing_rule.bg = COLOR_BLACK;
  ctx.file_color_rules_head = &existing_rule;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = capture_file_color_rule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed without COLOR_SUPPORT\n");
    return 1;
  }
  if (color_count != 0 || file_rule_count != 0) {
    fprintf(stderr, "no-color theme load mutated color state: %d/%d\n",
            color_count, file_rule_count);
    return 1;
  }
  if (ctx.file_color_rules_head != &existing_rule) {
    fprintf(stderr, "no-color theme load replaced file palette\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_background_role_prefers_explicit_background_color(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_explicit_background_driver.c"
    binary = tmp_path / "theme_explicit_background_driver"

    theme_file.write_text(
        """
[theme sample]
background = white on blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white
keybind = +white
footer = white
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white
help_link = cyan
help_link_selection = yellow
info = +white
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UIColor ui_colors[];

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (ui_colors[0].bg != COLOR_BLUE) {
    fprintf(stderr, "dynamic_text inherited %d instead of %d\n",
            ui_colors[0].bg, COLOR_BLUE);
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.zip") != 0 ||
      rule->bg != COLOR_BLUE) {
    fprintf(stderr, "file rule inherited %d instead of %d\n",
            rule == NULL ? -1 : rule->bg, COLOR_BLUE);
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_loader_rejects_overlong_role_values(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_overlong_role_driver.c"
    binary = tmp_path / "theme_overlong_role_driver"

    theme_file.write_text(
        f"""
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = {'white' + (' ' * 200) + 'on blue'}
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on blue
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "overlong theme role unexpectedly loaded\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_loader_requires_footer_and_help_link_roles(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_missing_help_footer_roles_driver.c"
    binary = tmp_path / "theme_missing_help_footer_roles_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan
tree_lines = +white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
selection = black on white
dialog = white
picker = black on cyan
picker_selection = selection
help = white
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "theme missing footer/help-link roles unexpectedly loaded\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_loader_applies_explicit_footer_background(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_footer_background_driver.c"
    binary = tmp_path / "theme_footer_background_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = white
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = yellow
footer = white on magenta
selection = black on cyan
dialog = white
picker = white on cyan
help = black on white
help_keybind = yellow
help_link = black on cyan
help_link_selection = yellow on cyan
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (expect_color("footer", COLOR_WHITE, COLOR_MAGENTA) != 0 ||
      expect_color("help", COLOR_BLACK, 7) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_loader_applies_help_role_fallback_chain(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_help_role_fallback_driver.c"
    binary = tmp_path / "theme_help_role_fallback_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = white
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = yellow
footer = white
selection = black on cyan
dialog = white
picker = white on cyan
help = black on white
help_footer = white on magenta
help_heading = yellow
help_link = black on cyan
help_link_selection = yellow on cyan
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  color_count++;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0) {
      if (colors[i].fg != fg || colors[i].bg != bg) {
        fprintf(stderr, "%s expected %d/%d got %d/%d\n", name, fg, bg,
                colors[i].fg, colors[i].bg);
        return 1;
      }
      return 0;
    }
  }

  fprintf(stderr, "missing color %s\n", name);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "theme failed to load\n");
    return 1;
  }

  if (expect_color("help_footer", COLOR_WHITE, COLOR_MAGENTA) != 0)
    return 1;
  if (expect_color("help_heading", COLOR_YELLOW, COLOR_WHITE) != 0)
    return 1;
  if (expect_color("help_topic", COLOR_YELLOW, COLOR_WHITE) != 0)
    return 1;
  if (expect_color("help_attention", COLOR_YELLOW, COLOR_WHITE) != 0)
    return 1;
  if (expect_color("help_alert", COLOR_YELLOW, COLOR_WHITE) != 0)
    return 1;
  if (expect_color("help_keybind", COLOR_YELLOW, COLOR_MAGENTA) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_theme_loader_rejects_duplicate_role_entries(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_duplicate_role_driver.c"
    binary = tmp_path / "theme_duplicate_role_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = white
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = yellow
footer = white on magenta
footer = white
selection = black on cyan
dialog = white
picker = white on cyan
help = black on white
help_keybind = yellow
help_link = black on cyan
help_link_selection = yellow on cyan
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "theme with duplicate role entries unexpectedly loaded\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_startup_theme_loader_falls_back_when_user_theme_catalog_is_invalid(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    config_home = tmp_path / "config-home"
    theme_dir = config_home / "ytnova"
    theme_dir.mkdir(parents=True)
    theme_file = theme_dir / "themes.conf"
    driver = tmp_path / "startup_theme_fallback_driver.c"
    binary = tmp_path / "startup_theme_fallback_driver"

    theme_file.write_text(
        """
[theme norton-blue]
background = blue
box_lines = white
tree_lines = white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = yellow
footer = white on magenta
footer = white
selection = black on cyan
dialog = white
picker = white on cyan
help = black on white
help_keybind = yellow
help_link = black on cyan
help_link_selection = yellow on cyan
info = black on cyan
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char name[32];
  int fg;
  int bg;
} CapturedColor;

static CapturedColor colors[32];
static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *get_profile_value(const ViewContext *ctx, const char *name) {
  (void)ctx;
  return strcmp(name, "THEME") == 0 ? (char *)"norton-blue" : NULL;
}

static void capture_parse_color(const char *color_str, int *fg, int *bg) {
  ParseColorString(color_str, fg, bg);
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (color_count >= 32)
    return;
  snprintf(colors[color_count].name, sizeof(colors[color_count].name), "%s",
           name);
  colors[color_count].fg = fg;
  colors[color_count].bg = bg;
  ++color_count;
}

static int expect_color(const char *name, int fg, int bg) {
  int i;

  for (i = 0; i < color_count; ++i) {
    if (strcmp(colors[i].name, name) == 0 && colors[i].fg == fg &&
        colors[i].bg == bg)
      return 0;
  }

  fprintf(stderr, "missing color %s %d,%d\n", name, fg, bg);
  return 1;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.core_init_ops.get_profile_value = get_profile_value;
  ctx.hook_parse_color = capture_parse_color;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (LoadStartupTheme(&ctx) != 0) {
    fprintf(stderr, "LoadStartupTheme failed\n");
    return 1;
  }

  if (strcmp(ctx.theme_file_path, argv[1]) == 0) {
    fprintf(stderr, "startup fallback reused invalid user theme path\n");
    return 1;
  }

  if (expect_color("help", COLOR_BLACK, 7) != 0 ||
      expect_color("footer", COLOR_WHITE, COLOR_BLUE) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run(
        [str(binary), str(theme_file)],
        cwd=repo_root,
        check=True,
        env={**os.environ, "XDG_CONFIG_HOME": str(config_home)},
    )


def test_failed_theme_load_keeps_previous_file_palette(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_failure_driver.c"
    binary = tmp_path / "theme_failure_driver"

    theme_file.write_text(
        """
[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "missing theme unexpectedly loaded\n");
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->next != NULL) {
    fprintf(stderr, "previous file palette was not retained\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_invalid_theme_load_keeps_previous_runtime_state(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    incomplete_theme = tmp_path / "incomplete.conf"
    invalid_theme = tmp_path / "invalid.conf"
    invalid_background_theme = tmp_path / "invalid_background.conf"
    invalid_palette_theme = tmp_path / "invalid_palette.conf"
    driver = tmp_path / "theme_atomic_failure_driver.c"
    binary = tmp_path / "theme_atomic_failure_driver"

    incomplete_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )
    invalid_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = white on not-a-color
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )
    invalid_palette_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red on not-a-color: zip
""",
        encoding="utf-8",
    )
    invalid_background_theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = white on -1
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
archives = red: zip
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

static int expect_previous_state(ViewContext *ctx, const char *path) {
  FileColorRule *rule;

  color_count = 0;
  if (ReadThemeFile(ctx, path, "sample") == 0) {
    fprintf(stderr, "%s unexpectedly loaded\n", path);
    return 1;
  }
  if (color_count != 0) {
    fprintf(stderr, "%s changed %d colors\n", path, color_count);
    return 1;
  }

  rule = (FileColorRule *)ctx->file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->fg != COLOR_GREEN || rule->bg != COLOR_BLACK || rule->next != NULL) {
    fprintf(stderr, "%s did not preserve previous file palette\n", path);
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 5)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (expect_previous_state(&ctx, argv[1]) != 0)
    return 1;
  if (expect_previous_state(&ctx, argv[2]) != 0)
    return 1;
  if (expect_previous_state(&ctx, argv[3]) != 0)
    return 1;
  if (expect_previous_state(&ctx, argv[4]) != 0)
    return 1;

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run(
        [
            str(binary),
            str(incomplete_theme),
            str(invalid_theme),
            str(invalid_palette_theme),
            str(invalid_background_theme),
        ],
        cwd=repo_root,
        check=True,
    )


def test_theme_palette_rejects_wildcard_selectors(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme_file = tmp_path / "themes.conf"
    driver = tmp_path / "theme_wildcard_selector_driver.c"
    binary = tmp_path / "theme_wildcard_selector_driver"

    theme_file.write_text(
        """
[theme sample]
background = blue
box_lines = cyan on blue
tree_lines = +white on blue
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on blue

[file-types sample]
wildcards = red: *.log,?
""",
        encoding="utf-8",
    )

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") == 0) {
    fprintf(stderr, "wildcard selector unexpectedly loaded\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme_file)], cwd=repo_root, check=True)


def test_invalid_user_theme_catalog_blocks_packaged_fallback(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_dir = home / ".config" / "ytnova"
    preferred_dir.mkdir(parents=True)
    (preferred_dir / "themes.conf").write_text(
        """
[theme quiet-blue]
background = blue
box_lines = cyan on blue
""",
        encoding="utf-8",
    )
    driver = tmp_path / "theme_path_failure_driver.c"
    binary = tmp_path / "theme_path_failure_driver"

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  FileColorRule *rule;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (LoadConfiguredTheme(&ctx) == 0) {
    fprintf(stderr, "invalid user theme fell through to packaged catalog\n");
    return 1;
  }
  if (color_count != 0) {
    fprintf(stderr, "invalid user theme changed %d colors\n", color_count);
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->fg != COLOR_GREEN || rule->bg != COLOR_BLACK || rule->next != NULL) {
    fprintf(stderr, "invalid user theme did not preserve previous palette\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=repo_root, check=True)


def test_packaged_theme_fallback_uses_installed_catalog_path(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_dir = home / ".config" / "ytnova"
    installed_dir = tmp_path / "installed" / "share" / "ytnova"
    installed_theme = installed_dir / "ytnova.themes"
    driver = tmp_path / "theme_installed_fallback_driver.c"
    binary = tmp_path / "theme_installed_fallback_driver"

    home.mkdir()
    preferred_dir.mkdir(parents=True)
    installed_dir.mkdir(parents=True)
    (preferred_dir / "themes.conf").write_text(
        """
[theme spare]
background = black
box_lines = white on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
footer = white on black
selection = black on white
dialog = white on black
picker = black on white
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on black
""",
        encoding="utf-8",
    )
    installed_theme.write_text(
        """
[theme quiet-blue]
background = black
box_lines = red on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
footer = white on black
selection = black on white
dialog = white on black
picker = black on cyan
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int box_lines_fg = -1;
static int box_lines_bg = -1;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (strcmp(name, "box_lines") == 0) {
    box_lines_fg = fg;
    box_lines_bg = bg;
  }
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "packaged theme was not loaded from installed path\n");
    return 1;
  }
  if (box_lines_fg != COLOR_RED || box_lines_bg != COLOR_BLACK) {
    fprintf(stderr, "installed packaged theme did not override compiled defaults\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            f'-DPACKAGED_THEME_PATH="{installed_theme}"',
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def test_invalid_packaged_theme_catalog_falls_back_to_compiled_default(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    installed_dir = tmp_path / "installed" / "share" / "ytnova"
    installed_theme = installed_dir / "ytnova.themes"
    driver = tmp_path / "theme_invalid_packaged_fallback_driver.c"
    binary = tmp_path / "theme_invalid_packaged_fallback_driver"

    home.mkdir()
    installed_dir.mkdir(parents=True)
    installed_theme.write_text(
        """
[theme quiet-blue]
background = not-a-color
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int box_lines_fg = -1;
static int box_lines_bg = -1;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (strcmp(name, "box_lines") == 0) {
    box_lines_fg = fg;
    box_lines_bg = bg;
  }
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "invalid packaged theme blocked compiled fallback\n");
    return 1;
  }
  if (box_lines_fg != COLOR_CYAN || box_lines_bg != COLOR_BLUE) {
    fprintf(stderr, "compiled fallback did not restore default box_lines\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            f'-DPACKAGED_THEME_PATH="{installed_theme}"',
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def test_unreadable_user_theme_catalog_blocks_packaged_fallback(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_theme = home / ".config" / "ytnova" / "themes.conf"
    preferred_theme.mkdir(parents=True)
    driver = tmp_path / "theme_unreadable_path_driver.c"
    binary = tmp_path / "theme_unreadable_path_driver"

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) == 0) {
    fprintf(stderr, "unreadable user theme catalog fell through to packaged catalog\n");
    return 1;
  }
  if (color_count != 0) {
    fprintf(stderr, "unreadable user theme changed %d colors\n", color_count);
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=repo_root, check=True)


def test_valid_user_theme_catalog_missing_theme_allows_compiled_fallback(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    preferred_dir = home / ".config" / "ytnova"
    installed_dir = tmp_path / "installed" / "share" / "ytnova"
    installed_theme = installed_dir / "ytnova.themes"
    preferred_dir.mkdir(parents=True)
    installed_dir.mkdir(parents=True)
    installed_theme.write_text(
        """
[theme spare]
background = black
box_lines = white on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
footer = white on black
selection = black on white
dialog = white on black
picker = black on white
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on black
""",
        encoding="utf-8",
    )
    (preferred_dir / "themes.conf").write_text(
        """
[theme spare]
background = black
box_lines = white on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
footer = white on black
selection = black on white
dialog = white on black
picker = black on white
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on black
""",
        encoding="utf-8",
    )
    driver = tmp_path / "theme_missing_fallback_driver.c"
    binary = tmp_path / "theme_missing_fallback_driver"

    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

FILE *__wrap_tmpfile(void) { return NULL; }

FILE *__wrap_tmpfile64(void) { return NULL; }

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "valid user catalog without requested theme blocked compiled fallback\n");
    return 1;
  }
  if (color_count == 0) {
    fprintf(stderr, "compiled fallback did not apply colors\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            f'-DPACKAGED_THEME_PATH="{installed_theme}"',
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-Wl,--wrap=tmpfile",
            "-Wl,--wrap=tmpfile64",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def test_missing_user_theme_catalog_uses_compiled_default_without_creating_file(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    missing_packaged = tmp_path / "missing" / "ytnova.themes"
    driver = tmp_path / "theme_seeded_default_driver.c"
    binary = tmp_path / "theme_seeded_default_driver"

    home.mkdir()
    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int color_count;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  (void)name;
  (void)fg;
  (void)bg;
  ++color_count;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  char seeded_path[PATH_LENGTH + 1];

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "missing user theme catalog did not load compiled defaults\n");
    return 1;
  }
  if (color_count == 0) {
    fprintf(stderr, "compiled default theme did not apply colors\n");
    return 1;
  }

  if (snprintf(seeded_path, sizeof(seeded_path), "%s/.config/ytnova/themes.conf",
               argv[1]) < 0 ||
      strlen(argv[1]) + strlen("/.config/ytnova/themes.conf") >=
          sizeof(seeded_path))
    return 1;
  if (access(seeded_path, F_OK) == 0) {
    fprintf(stderr, "startup created a user theme file\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            f'-DPACKAGED_THEME_PATH="{missing_packaged}"',
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def _assert_legacy_theme_seed_rebases_background_roles(
    tmp_path, theme_name, theme_text, extra_assertion=""
):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    theme_dir = home / ".config" / "ytnova"
    theme_file = theme_dir / "themes.conf"
    safe_name = theme_name.replace("-", "_")
    driver = tmp_path / f"{safe_name}_rebase_driver.c"
    binary = tmp_path / f"{safe_name}_rebase_driver"

    theme_dir.mkdir(parents=True)
    theme_file.write_text(theme_text, encoding="utf-8")
    driver.write_text(
        f'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int box_lines_bg = -1;
static int tree_lines_bg = -1;
static int static_text_bg = -1;
static int dynamic_text_bg = -1;
static int keybind_bg = -1;
static int help_bg = -1;
static int info_bg = -1;
static int dialog_bg = -1;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {{
  (void)ctx;
  (void)fmt;
  return 0;
}}

static char *configured_theme(const ViewContext *ctx, const char *name) {{
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "{theme_name}";
  return NULL;
}}

static void capture_update_color(const char *name, int fg, int bg) {{
  (void)fg;
  if (strcmp(name, "box_lines") == 0)
    box_lines_bg = bg;
  else if (strcmp(name, "tree_lines") == 0)
    tree_lines_bg = bg;
  else if (strcmp(name, "static_text") == 0)
    static_text_bg = bg;
  else if (strcmp(name, "dynamic_text") == 0)
    dynamic_text_bg = bg;
  else if (strcmp(name, "keybind") == 0)
    keybind_bg = bg;
  else if (strcmp(name, "help") == 0)
    help_bg = bg;
  else if (strcmp(name, "info") == 0)
    info_bg = bg;
  else if (strcmp(name, "dialog") == 0)
    dialog_bg = bg;
}}

int main(int argc, char **argv) {{
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) != 0) {{
    fprintf(stderr, "legacy theme did not load\\n");
    return 1;
  }}

  if (box_lines_bg != COLOR_RED || tree_lines_bg != COLOR_RED ||
      static_text_bg != COLOR_RED || dynamic_text_bg != COLOR_RED ||
      keybind_bg != COLOR_RED || help_bg != COLOR_RED ||
      info_bg != COLOR_RED) {{
    fprintf(stderr, "legacy seed did not rebase inherited backgrounds\\n");
    return 1;
  }}
{extra_assertion}
  return 0;
}}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def test_legacy_classic_blue_seed_rebases_redundant_background_roles(tmp_path):
    _assert_legacy_theme_seed_rebases_background_roles(
        tmp_path,
        "quiet-blue",
        """
[theme quiet-blue]
background = red
box_lines = cyan on blue
tree_lines = +white on blue
margin = dynamic_text
static_text = white on blue
dynamic_text = +white on blue
keybind = +white on blue
footer = white on blue
selection = black on +grey
dialog = black on +grey
picker = black on +grey
help = white on blue
help_link = cyan on blue
help_link_selection = yellow on blue
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
""",
    )


def test_legacy_custom_theme_seed_rebases_redundant_background_roles(tmp_path):
    _assert_legacy_theme_seed_rebases_background_roles(
        tmp_path,
        "custom-night",
        """
[theme custom-night]
background = red
box_lines = grey on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = +white on black
keybind = +white on black
footer = white on black
selection = black on yellow
dialog = white on black
picker = black on yellow
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = +white on black
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey on black
""",
        extra_assertion="""
  if (dialog_bg != COLOR_BLACK) {
    fprintf(stderr, "custom dialog background should remain explicit\\n");
    return 1;
  }
""",
    )


def test_missing_user_theme_catalog_uses_packaged_catalog_without_creating_file(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    installed_dir = tmp_path / "installed" / "share" / "ytnova"
    installed_theme = installed_dir / "ytnova.themes"
    driver = tmp_path / "theme_seed_before_packaged_driver.c"
    binary = tmp_path / "theme_seed_before_packaged_driver"

    home.mkdir()
    installed_dir.mkdir(parents=True)
    installed_theme.write_text(
        """
[theme quiet-blue]
background = black
box_lines = red on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
footer = white on black
selection = black on white
dialog = white on black
picker = black on cyan
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int box_lines_fg = -1;
static int box_lines_bg = -1;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "quiet-blue";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (strcmp(name, "box_lines") == 0) {
    box_lines_fg = fg;
    box_lines_bg = bg;
  }
}

int main(int argc, char **argv) {
  ViewContext ctx;
  char seeded_path[PATH_LENGTH + 1];

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "missing user theme catalog was not loaded\n");
    return 1;
  }
  if (box_lines_fg != COLOR_RED || box_lines_bg != COLOR_BLACK) {
    fprintf(stderr, "packaged catalog was not loaded before compiled defaults\n");
    return 1;
  }
  if (snprintf(seeded_path, sizeof(seeded_path), "%s/.config/ytnova/themes.conf",
               argv[1]) < 0 ||
      strlen(argv[1]) + strlen("/.config/ytnova/themes.conf") >=
          sizeof(seeded_path))
    return 1;
  if (access(seeded_path, F_OK) == 0) {
    fprintf(stderr, "startup created a user theme file before packaged fallback\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            f'-DPACKAGED_THEME_PATH="{installed_theme}"',
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def test_seeded_theme_miss_uses_installed_packaged_catalog(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    home = tmp_path / "home"
    installed_dir = tmp_path / "installed" / "share" / "ytnova"
    installed_theme = installed_dir / "ytnova.themes"
    driver = tmp_path / "theme_seeded_packaged_retry_driver.c"
    binary = tmp_path / "theme_seeded_packaged_retry_driver"

    home.mkdir()
    installed_dir.mkdir(parents=True)
    installed_theme.write_text(
        """
[theme packaged-only]
background = black
box_lines = red on black
tree_lines = white on black
margin = dynamic_text
static_text = white on black
dynamic_text = white on black
keybind = +white on black
footer = white on black
selection = black on white
dialog = white on black
picker = black on white
help = white on black
help_link = cyan on black
help_link_selection = yellow on black
info = white on black
warning = black on yellow
error = white on red
search_hit = black on yellow
disabled = grey on black
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int box_lines_fg = -1;
static int box_lines_bg = -1;

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static char *configured_theme(const ViewContext *ctx, const char *name) {
  (void)ctx;
  if (strcmp(name, "THEME") == 0)
    return "packaged-only";
  return NULL;
}

static void capture_update_color(const char *name, int fg, int bg) {
  if (strcmp(name, "box_lines") == 0) {
    box_lines_fg = fg;
    box_lines_bg = bg;
  }
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;
  if (unsetenv("XDG_CONFIG_HOME") != 0 ||
      setenv("HOME", argv[1], 1) != 0)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = capture_update_color;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  ctx.core_init_ops.get_profile_value = configured_theme;

  if (LoadConfiguredTheme(&ctx) != 0) {
    fprintf(stderr, "seed miss did not use the installed packaged catalog\n");
    return 1;
  }
  if (box_lines_fg != COLOR_RED || box_lines_bg != COLOR_BLACK) {
    fprintf(stderr, "installed packaged fallback did not apply packaged-only colors\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            f'-DPACKAGED_THEME_PATH="{installed_theme}"',
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(home)], cwd=tmp_path, check=True)


def test_picker_selection_can_fall_back_to_inverse_when_theme_matches_base(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "picker_selection_inverse_driver.c"
    binary = tmp_path / "picker_selection_inverse_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(void) {
  ViewContext ctx;

  memset(&ctx, 0, sizeof(ctx));
  ctx.color_enabled = TRUE;

  UpdateUIColor("picker", COLOR_WHITE, COLOR_BLUE);
  UpdateUIColor("picker_selection", COLOR_WHITE, COLOR_BLUE);

  if (UISelectionAttrForBase(&ctx, UI_ROLE_PICKER) !=
      (COLOR_PAIR(UI_ROLE_PICKER) | A_REVERSE)) {
    fprintf(stderr, "picker selection did not fall back to inverse\n");
    return 1;
  }

  UpdateUIColor("picker_selection", COLOR_BLACK, COLOR_WHITE);
  if (UISelectionAttrForBase(&ctx, UI_ROLE_PICKER) !=
      COLOR_PAIR(UI_ROLE_PICKER_SELECTION)) {
    fprintf(stderr, "distinct picker selection did not stay explicit\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=tmp_path, check=True)


def test_color_pair_startup_reserves_storage_before_rebuild(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    driver = tmp_path / "color_pair_reinit_driver.c"
    binary = tmp_path / "color_pair_reinit_driver"

    driver.write_text(
        r'''
#include "ytnova_ui.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int first_pair;
static int highest_pair;
static int init_pair_calls;

static void observe_pair(int pair) {
  if (init_pair_calls == 0)
    first_pair = pair;
  if (pair > highest_pair)
    highest_pair = pair;
  init_pair_calls++;
}

int __wrap_init_pair(short pair, short foreground, short background) {
  (void)foreground;
  (void)background;
  observe_pair(pair);
  return OK;
}

int __wrap_init_extended_pair(int pair, int foreground, int background) {
  (void)foreground;
  (void)background;
  observe_pair(pair);
  return OK;
}

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(void) {
  ViewContext ctx;
  SCREEN *screen;
  FILE *input;
  FILE *output;
  char pattern[16];
  int foreground;
  int background;
  int expected_pair;
  int startup_calls;

  input = fopen("/dev/null", "r");
  output = fopen("/dev/null", "w");
  if (input == NULL || output == NULL)
    return 1;

  screen = newterm("xterm", output, input);
  if (screen == NULL)
    return 2;
  set_term(screen);

  memset(&ctx, 0, sizeof(ctx));
  for (foreground = 0; foreground < 8; foreground++) {
    for (background = 0; background < 8; background++) {
      snprintf(pattern, sizeof(pattern), ".%d_%d", foreground, background);
      AddFileColorRule(&ctx, pattern, foreground, background);
    }
  }

  StartColors(&ctx);
  expected_pair = COLOR_PAIRS - 1;
  if (expected_pair > SHRT_MAX)
    expected_pair = SHRT_MAX;
  if (!ctx.color_enabled || init_pair_calls == 0 ||
      first_pair != expected_pair || highest_pair != expected_pair) {
    fprintf(stderr, "first pair %d did not reserve usable pair %d\n",
            first_pair, expected_pair);
    return 4;
  }
  startup_calls = init_pair_calls;

  UpdateUIColor("dynamic_text", COLOR_RED, COLOR_BLUE);
  first_pair = 0;
  highest_pair = 0;
  init_pair_calls = 0;
  ReinitColorPairs(&ctx);
  if (init_pair_calls == 0 || startup_calls != init_pair_calls + 1) {
    fprintf(stderr, "rebuild call count changed from %d to %d\n",
            startup_calls - 1, init_pair_calls);
    return 5;
  }

  endwin();
  delscreen(screen);
  fclose(input);
  fclose(output);
  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-Wl,--wrap=init_pair",
            "-Wl,--wrap=init_extended_pair",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary)], cwd=tmp_path, check=True)


def test_picker_selection_role_can_override_picker_highlight(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    theme = tmp_path / "sample.themes"
    driver = tmp_path / "picker_selection_role_driver.c"
    binary = tmp_path / "picker_selection_role_driver"

    theme.write_text(
        """
[theme sample]
background = blue
box_lines = cyan
tree_lines = +white
margin = dynamic_text
static_text = white
dynamic_text = +white
keybind = +white
footer = white
selection = black on cyan
dialog = white
picker = white on cyan
picker_selection = black on white
help = white
help_link = cyan
help_link_selection = yellow
info = +white on blue
warning = black on yellow
error = +white on red
search_hit = black on yellow
disabled = grey
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdio.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;

  if (argc != 2)
    return 1;

  memset(&ctx, 0, sizeof(ctx));
  ctx.color_enabled = TRUE;
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_update_ui_color = UpdateUIColor;
  ctx.hook_add_file_color_rule = AddFileColorRule;

  if (ReadThemeFile(&ctx, argv[1], "sample") != 0) {
    fprintf(stderr, "ReadThemeFile failed\n");
    return 1;
  }

  if (UISelectionAttrForBase(&ctx, UI_ROLE_PICKER) !=
      COLOR_PAIR(UI_ROLE_PICKER_SELECTION)) {
    fprintf(stderr, "picker selection role did not override picker highlight\n");
    return 1;
  }

  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/theme.c",
            "src/core/config_paths.c",
            "src/ui/color.c",
            "src/util/memory_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run([str(binary), str(theme)], cwd=tmp_path, check=True)




def test_profile_runtime_snapshot_restores_values_and_palette(tmp_path):
    repo_root = Path(__file__).resolve().parents[1]
    original = tmp_path / "original.conf"
    changed = tmp_path / "changed.conf"
    driver = tmp_path / "profile_snapshot_driver.c"
    binary = tmp_path / "profile_snapshot_driver"

    original.write_text(
        """
[GLOBAL]
THEME=quiet-blue
SMALLWINDOWSKIP=1
""",
        encoding="utf-8",
    )
    changed.write_text(
        """
[GLOBAL]
THEME=missing-theme
SMALLWINDOWSKIP=0
""",
        encoding="utf-8",
    )
    driver.write_text(
        r'''
#include "config.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
  return 0;
}

static void free_file_rules(FileColorRule *rule) {
  while (rule != NULL) {
    FileColorRule *next = rule->next;
    free(rule->pattern);
    free(rule);
    rule = next;
  }
}

int main(int argc, char **argv) {
  ViewContext ctx;
  ProfileRuntimeSnapshot *snapshot;
  FileColorRule *rule;

  if (argc != 3)
    return 2;

  memset(&ctx, 0, sizeof(ctx));
  ctx.hook_parse_color = ParseColorString;
  ctx.hook_add_file_color_rule = AddFileColorRule;
  AddFileColorRule(&ctx, "*.old", COLOR_GREEN, COLOR_BLACK);

  if (ReadProfile(&ctx, argv[1]) != 0) {
    fprintf(stderr, "original profile failed\n");
    return 1;
  }

  snapshot = ProfileRuntimeSnapshot_Create(&ctx);
  if (ReadProfile(&ctx, argv[2]) != 0) {
    fprintf(stderr, "changed profile failed\n");
    return 1;
  }

  if (strcmp(GetProfileValue(&ctx, "THEME"), "missing-theme") != 0) {
    fprintf(stderr, "changed profile was not applied before restore\n");
    return 1;
  }

  ProfileRuntimeSnapshot_Restore(&ctx, snapshot);
  ProfileRuntimeSnapshot_Free(snapshot);

  if (strcmp(GetProfileValue(&ctx, "THEME"), "quiet-blue") != 0 ||
      strcmp(GetProfileValue(&ctx, "SMALLWINDOWSKIP"), "1") != 0) {
    fprintf(stderr, "profile values were not restored\n");
    return 1;
  }

  rule = (FileColorRule *)ctx.file_color_rules_head;
  if (rule == NULL || strcmp(rule->pattern, "*.old") != 0 ||
      rule->fg != COLOR_GREEN || rule->bg != COLOR_BLACK ||
      rule->next != NULL) {
    fprintf(stderr, "file palette was not restored\n");
    return 1;
  }

  free_file_rules((FileColorRule *)ctx.file_color_rules_head);
  ctx.file_color_rules_head = NULL;
  FreeProfileRuntimeData(&ctx);
  return 0;
}
''',
        encoding="utf-8",
    )

    subprocess.run(
        [
            "cc",
            "-D_GNU_SOURCE",
            "-DCOLOR_SUPPORT",
            "-Iinclude",
            str(driver),
            "src/cmd/profile.c",
            "src/ui/color.c",
            "src/util/atomic_file.c",
            "src/util/memory_utils.c",
            "src/util/string_utils.c",
            "-lncursesw",
            "-ltinfo",
            "-o",
            str(binary),
        ],
        cwd=repo_root,
        check=True,
    )
    subprocess.run(
        [str(binary), str(original), str(changed)], cwd=repo_root, check=True
    )
