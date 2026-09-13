/***************************************************************************
 *
 * src/ui/color.c
 * Dynamic Colors-Support
 *
 ***************************************************************************/

#include "ytnova_ui.h"

#include <limits.h>

#ifdef COLOR_SUPPORT

UIColor ui_colors[] = {{"dynamic_text", UI_ROLE_DYNAMIC_TEXT, 7, 0},
                       {"static_text", UI_ROLE_STATIC_TEXT, 7, 0},
                       {"keybind", UI_ROLE_KEYBIND, 15, 0},
                       {"footer", UI_ROLE_FOOTER, 7, 0},
                       {"help", UI_ROLE_HELP, 7, 0},
                       {"help_footer", UI_ROLE_HELP_FOOTER, 7, 0},
                       {"help_heading", UI_ROLE_HELP_HEADING, 7, 0},
                       {"help_topic", UI_ROLE_HELP_TOPIC, 7, 0},
                       {"help_attention", UI_ROLE_HELP_ATTENTION, 7, 0},
                       {"help_alert", UI_ROLE_HELP_ALERT, 7, 0},
                       {"help_keybind", UI_ROLE_HELP_KEYBIND, 15, 0},
                       {"help_link", UI_ROLE_HELP_LINK, 6, 0},
                       {"help_link_selection", UI_ROLE_HELP_LINK_SELECTION, 3, 0},
                       {"help_box_lines", UI_ROLE_HELP_BOX_LINES, 7, 0},
                       {"picker", UI_ROLE_PICKER, 7, 0},
                       {"picker_selection", UI_ROLE_PICKER_SELECTION, 0, 3},
                       {"selection", UI_ROLE_SELECTION, 0, 3},
                       {"box_lines", UI_ROLE_BOX_LINES, 8, 0},
                       {"tree_lines", UI_ROLE_TREE_LINES, 7, 0},
                       {"margin", UI_ROLE_MARGIN, 7, 0},
                       {"dialog", UI_ROLE_DIALOG, 7, 0},
                       {"info", UI_ROLE_INFO, 15, 4},
                       {"warning", UI_ROLE_WARNING, 11, 0},
                       {"error", UI_ROLE_ERROR, 15, 1},
                       {"search_hit", UI_ROLE_SEARCH_HIT, 15, 3},
                       {"disabled", UI_ROLE_DISABLED, 8, 0}};

int NUM_UI_COLORS = sizeof(ui_colors) / sizeof(ui_colors[0]);

struct _ui_color_snapshot {
  int count;
  int *fg;
  int *bg;
};

typedef struct {
  const char *name;
  int value;
} ColorMapEntry;

static const ColorMapEntry color_map[] = {{"black", COLOR_BLACK},
                                          {"red", COLOR_RED},
                                          {"green", COLOR_GREEN},
                                          {"yellow", COLOR_YELLOW},
                                          {"blue", COLOR_BLUE},
                                          {"magenta", COLOR_MAGENTA},
                                          {"cyan", COLOR_CYAN},
                                          {"white", COLOR_WHITE},
                                          {NULL, -1}};

static int NormalizeColorIndex(int color, int color_limit) {
  if (color == -1)
    return -1;
  if (color < -1)
    return COLOR_WHITE;
  if (color_limit <= 0)
    return color;
  if (color < color_limit)
    return color;

  /* Bright ANSI colors fall back to base colors on 8-color terminals. */
  if (color_limit <= 8 && color <= 15)
    return color - 8;

  return color % color_limit;
}

static BOOL ParseColorToken(const char *token, int color_limit, int *color) {
  const char *name;
  BOOL bright = FALSE;
  int i;
  char *endptr;

  if (!token || !*token || !color)
    return FALSE;

  name = token;
  if (*name == '+') {
    bright = TRUE;
    ++name;
  }
  if (!*name)
    return FALSE;

  if (strcasecmp(name, "grey") == 0 || strcasecmp(name, "gray") == 0) {
    *color = bright ? COLOR_WHITE : NormalizeColorIndex(8, color_limit);
    return TRUE;
  }

  for (i = 0; color_map[i].name; i++) {
    if (strcasecmp(name, color_map[i].name) == 0) {
      *color = NormalizeColorIndex(
          bright ? color_map[i].value + 8 : color_map[i].value, color_limit);
      return TRUE;
    }
  }

  if (!bright) {
    long val;

    errno = 0;
    val = strtol(name, &endptr, 10);
    if (errno == 0 && endptr != name && *endptr == '\0' && val >= 0 &&
        val <= 255) {
      *color = NormalizeColorIndex((int)val, color_limit);
      return TRUE;
    }
  }

  return FALSE;
}

static int UIColorBackground(int pair_id) {
  int i;

  for (i = 0; i < NUM_UI_COLORS; i++) {
    if (ui_colors[i].id == pair_id)
      return ui_colors[i].bg;
  }

  return COLOR_BLACK;
}

static int UIColorForeground(int pair_id) {
  int i;

  for (i = 0; i < NUM_UI_COLORS; i++) {
    if (ui_colors[i].id == pair_id)
      return ui_colors[i].fg;
  }

  return COLOR_WHITE;
}

chtype UISelectionAttrForBase(const ViewContext *ctx, int base_role) {
  int selection_role = UI_ROLE_SELECTION;

  if (ctx == NULL || !ctx->color_enabled)
    return A_REVERSE;

  if (base_role == UI_ROLE_PICKER)
    selection_role = UI_ROLE_PICKER_SELECTION;

  if (UIColorForeground(selection_role) == UIColorForeground(base_role) &&
      UIColorBackground(selection_role) == UIColorBackground(base_role)) {
    return COLOR_PAIR(base_role) | A_REVERSE;
  }

  return COLOR_PAIR(selection_role);
}

chtype UIKeybindAttrForBase(int overlay_role, int base_role) {
  int pair_id = -1;

  if (overlay_role == UI_ROLE_KEYBIND)
    pair_id = UI_KEYBIND_BASE_PAIR + (base_role - 1);
  else if (overlay_role == UI_ROLE_HELP_KEYBIND)
    pair_id = UI_HELP_KEYBIND_BASE_PAIR + (base_role - 1);
  else
    return COLOR_PAIR(overlay_role);
  if (base_role < UI_ROLE_DYNAMIC_TEXT || base_role >= NUM_UI_COLOR_PAIRS)
    return COLOR_PAIR(overlay_role);

  return COLOR_PAIR(pair_id);
}

UIColorSnapshot *UIColorSnapshot_Create(void) {
  int i;
  UIColorSnapshot *snapshot = xmalloc(sizeof(*snapshot));

  snapshot->count = NUM_UI_COLORS;
  snapshot->fg = xmalloc(sizeof(snapshot->fg[0]) * snapshot->count);
  snapshot->bg = xmalloc(sizeof(snapshot->bg[0]) * snapshot->count);

  for (i = 0; i < snapshot->count; ++i) {
    snapshot->fg[i] = ui_colors[i].fg;
    snapshot->bg[i] = ui_colors[i].bg;
  }

  return snapshot;
}

void UIColorSnapshot_Restore(UIColorSnapshot *snapshot) {
  int i;
  int count;

  if (snapshot == NULL)
    return;

  count = snapshot->count < NUM_UI_COLORS ? snapshot->count : NUM_UI_COLORS;
  for (i = 0; i < count; ++i) {
    ui_colors[i].fg = snapshot->fg[i];
    ui_colors[i].bg = snapshot->bg[i];
  }
}

void UIColorSnapshot_Free(UIColorSnapshot *snapshot) {
  if (snapshot == NULL)
    return;

  free(snapshot->fg);
  free(snapshot->bg);
  free(snapshot);
}

BOOL ParseColorStringStrict(const char *color_str, int *fg, int *bg) {
  char *dup, *token, *saveptr;
  int *target;
  int color_limit;
  BOOL have_fg = FALSE;
  BOOL expect_bg = FALSE;

  if (!color_str || !fg || !bg)
    return FALSE;

  dup = xstrdup(color_str);

  target = fg;
  token = strtok_r(dup, " ,\t\r\n", &saveptr);

  /* Determine maximum valid color index.
     Use global COLORS if initialized, otherwise assume 256 for config parsing
     safety. */
  color_limit = (COLORS > 0) ? COLORS : 256;

  while (token) {
    int parsed;

    if (strcasecmp(token, "on") == 0) {
      if (!have_fg || expect_bg) {
        free(dup);
        return FALSE;
      }
      target = bg;
      expect_bg = TRUE;
    } else if (ParseColorToken(token, color_limit, &parsed)) {
      if (target == bg && *bg != -1) {
        free(dup);
        return FALSE;
      }
      *target = parsed;
      if (target == fg) {
        have_fg = TRUE;
      } else {
        expect_bg = FALSE;
      }
      target = bg;
    } else {
      free(dup);
      return FALSE;
    }

    token = strtok_r(NULL, " ,\t\r\n", &saveptr);
  }
  free(dup);
  return have_fg && !expect_bg;
}

void ParseColorString(const char *color_str, int *fg, int *bg) {
  (void)ParseColorStringStrict(color_str, fg, bg);
}

void UpdateUIColor(const char *name, int fg, int bg) {
  int i;

  if (name == NULL)
    return;

  for (i = 0; i < NUM_UI_COLORS; i++) {
    if (strcasecmp(name, ui_colors[i].name) == 0) {
      ui_colors[i].fg = fg;
      ui_colors[i].bg = bg;
      return;
    }
  }
}

void AddFileColorRule(ViewContext *ctx, const char *pattern, int fg, int bg) {
  FileColorRule *new_rule = xmalloc(sizeof(FileColorRule));
  FileColorRule *tail;

  new_rule->pattern = xstrdup(pattern);
  new_rule->fg = fg;
  new_rule->bg = (bg == -1) ? UIColorBackground(UI_ROLE_DYNAMIC_TEXT) : bg;
  new_rule->pair_id = FILE_COLOR_PAIR_UNASSIGNED;
  new_rule->next = NULL;

  if (ctx->file_color_rules_head == NULL) {
    ctx->file_color_rules_head = new_rule;
    return;
  }

  tail = (FileColorRule *)ctx->file_color_rules_head;
  while (tail->next != NULL)
    tail = tail->next;
  tail->next = new_rule;
}

static int FindReusableFileColorPair(const FileColorRule *head,
                                     const FileColorRule *rule) {
  const FileColorRule *candidate;

  for (candidate = head; candidate != NULL && candidate != rule;
       candidate = candidate->next) {
    if (candidate->fg == rule->fg && candidate->bg == rule->bg &&
        candidate->pair_id != FILE_COLOR_PAIR_UNASSIGNED)
      return candidate->pair_id;
  }

  return FILE_COLOR_PAIR_UNASSIGNED;
}

static int UsableColorPairLimit(void) {
  int limit = COLOR_PAIRS;

  if (limit > SHRT_MAX + 1)
    limit = SHRT_MAX + 1;
  return limit;
}

static BOOL ReserveColorPairStorage(void) {
  int limit = UsableColorPairLimit();

  if (limit <= 1)
    return FALSE;

  /* ncurses indexes color-pair records by address, so later table growth can
   * invalidate its lookup tree.  Grow the table before defining live pairs. */
#if defined(NCURSES_VERSION) && defined(NCURSES_EXT_COLORS)
  return init_extended_pair(limit - 1, COLOR_WHITE, COLOR_BLACK) != ERR;
#else
  return TRUE;
#endif
}

static void InitUsableColorPair(int pair_id, int fg, int bg, int pair_limit) {
  if (pair_id <= 0 || pair_id >= pair_limit)
    return;
  (void)init_pair((short)pair_id, (short)fg, (short)bg);
}

void ReinitColorPairs(ViewContext *ctx) {
  int i;
  FileColorRule *rule;
  int pair_limit;
  int next_pair_id = F_COLOR_PAIR_BASE;

  if (!ctx->color_enabled)
    return;
  pair_limit = UsableColorPairLimit();

  /* Initialize UI colors */
  for (i = 0; i < NUM_UI_COLORS; i++) {
    InitUsableColorPair(ui_colors[i].id,
                        NormalizeColorIndex(ui_colors[i].fg, COLORS),
                        NormalizeColorIndex(ui_colors[i].bg, COLORS), pair_limit);
  }
  InitUsableColorPair(
      UI_VIEWER_FRAME_PAIR,
      NormalizeColorIndex(UIColorForeground(UI_ROLE_BOX_LINES), COLORS),
      NormalizeColorIndex(UIColorBackground(UI_ROLE_DYNAMIC_TEXT), COLORS),
      pair_limit);
  for (i = UI_ROLE_DYNAMIC_TEXT; i < NUM_UI_COLOR_PAIRS; ++i) {
    InitUsableColorPair(
        UI_KEYBIND_BASE_PAIR + (i - 1),
        NormalizeColorIndex(UIColorForeground(UI_ROLE_KEYBIND), COLORS),
        NormalizeColorIndex(UIColorBackground(i), COLORS), pair_limit);
    InitUsableColorPair(
        UI_HELP_KEYBIND_BASE_PAIR + (i - 1),
        NormalizeColorIndex(UIColorForeground(UI_ROLE_HELP_KEYBIND), COLORS),
        NormalizeColorIndex(UIColorBackground(UI_ROLE_HELP_KEYBIND), COLORS),
        pair_limit);
  }

  /* File-type selectors with the same style share one terminal color pair. */
  for (rule = ctx->file_color_rules_head; rule != NULL; rule = rule->next) {
    if (rule->pair_id == FILE_COLOR_PAIR_UNASSIGNED) {
      rule->pair_id = FindReusableFileColorPair(ctx->file_color_rules_head, rule);
      if (rule->pair_id == FILE_COLOR_PAIR_UNASSIGNED) {
        if (next_pair_id >= pair_limit)
          continue;
        rule->pair_id = next_pair_id++;
      }
    }
    if (rule->pair_id < F_COLOR_PAIR_BASE || rule->pair_id >= pair_limit)
      continue;
    InitUsableColorPair(rule->pair_id, NormalizeColorIndex(rule->fg, COLORS),
                        NormalizeColorIndex(rule->bg, COLORS), pair_limit);
  }
}

void StartColors(ViewContext *ctx) {
  ctx->color_enabled = FALSE;
  if (start_color() == ERR)
    return;
  if (COLORS < 8 ||
      COLOR_PAIRS < 64) { /* Check for a reasonable number of pairs */
    return;               /* No color support */
  }
  if (!ReserveColorPairStorage())
    return;

  ctx->color_enabled = TRUE;
  ReinitColorPairs(ctx);
}

static int FileColorPairOrDefault(const FileColorRule *rule) {
  if (rule == NULL || rule->pair_id < F_COLOR_PAIR_BASE ||
      rule->pair_id >= UsableColorPairLimit())
    return UI_ROLE_DYNAMIC_TEXT;
  return rule->pair_id;
}

int GetFileTypeColor(const ViewContext *ctx, const FileEntry *fe_ptr) {
  FileColorRule *rule;

  if (!fe_ptr)
    return UI_ROLE_DYNAMIC_TEXT;

  for (rule = ctx->file_color_rules_head; rule != NULL; rule = rule->next) {
    if (S_ISLNK(fe_ptr->stat_struct.st_mode) &&
        strcmp(rule->pattern, "LINK") == 0) {
      return FileColorPairOrDefault(rule);
    }
    if ((fe_ptr->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) &&
        strcmp(rule->pattern, "EXEC") == 0) {
      return FileColorPairOrDefault(rule);
    }

    /* Check for wildcard extension match */
    if (rule->pattern[0] == '*' && rule->pattern[1] == '.') {
      const char *ext = rule->pattern + 1;
      int name_len = strlen(fe_ptr->name);
      int ext_len = strlen(ext);
      if (name_len > ext_len &&
          strcasecmp(fe_ptr->name + name_len - ext_len, ext) == 0) {
        return FileColorPairOrDefault(rule);
      }
    }
  }

  return UI_ROLE_DYNAMIC_TEXT; /* Default */
}

void WbkgdSet(const ViewContext *ctx, WINDOW *w, chtype c) {
  if (ctx->color_enabled) {
    wbkgdset(w, c);
  } else {
    c &= ~A_BOLD;
    if (c == COLOR_PAIR(UI_ROLE_SELECTION) || c == COLOR_PAIR(UI_ROLE_KEYBIND)) {

      wattrset(w, A_REVERSE);
    } else {
      wattrset(w, 0);
    }
  }
}

#endif /* COLOR_SUPPORT */
