/***************************************************************************
 *
 * src/ui/display.c
 * Functions for handling the display
 *
 ***************************************************************************/

#include "../../include/ytnova.h"
#include "../../include/ytnova_cmd.h"
#include "../../include/ytnova_fs.h"
#include "../../include/ytnova_panel_anchor.h"
#include "../../include/ytnova_ui.h"
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_render.h"
#include "ytnova_appstate_window.h"
#include <assert.h>


static const UICommandStripCommand history_help_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("history.commands", "help"), "F1",
     NULL, "history.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("history.commands", "Delete"), "D", NULL,
     "history.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("history.commands", "Pin/unpin"), "P",
     NULL, "history.commands"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("history.commands", "select"), "Enter",
     NULL, "history.commands"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("history.commands", "cancel"), "Esc",
     NULL, "history.commands"}};

enum { HISTORY_DIALOG_COMMAND_STRIP_X = 2 };

typedef struct {
  UICommandStripCommand command;
  const char *primary_action_id;
  const char *secondary_action_id;
} FooterCommandSpec;

typedef struct {
  UICommandStripCommand command;
  char label[COMMAND_PRESENTATION_LABEL_LENGTH];
  char primary_key[COMMAND_PRESENTATION_SHOWN_LENGTH];
  char secondary_key[COMMAND_PRESENTATION_SHOWN_LENGTH];
  char rendered_text[160];
  int key_class;
} ResolvedFooterCommand;

typedef struct {
  size_t line_counts[2];
  size_t visible_count;
  BOOL truncated;
  size_t truncated_row;
  size_t truncated_index;
  int truncated_width;
} FooterPackResult;

typedef struct {
  const char *canonical_label;
  const char *action_id;
} HelpLabelOverrideSpec;

typedef struct {
  const FooterCommandSpec *specs;
  size_t spec_count;
  const HelpLabelOverrideSpec *override_specs;
  size_t override_spec_count;
} HelpLabelOverridePlan;

typedef struct {
  size_t fit_count;
  BOOL truncated;
  size_t truncated_index;
  int truncated_width;
  int used_width;
  size_t represented_count;
} FooterRowFit;

#define FOOTER_COMMAND_COLUMN 9
#define HELP_LABEL_OVERRIDE_TEXT_LENGTH 160
#define FOOTER_STATIC(layout, label, key1, key2)                                 \
  { { layout, label, key1, key2, NULL }, NULL, NULL }
#define FOOTER_ACTION(layout, label, key1, key2, action_id)                      \
  { { layout, label, key1, key2, NULL }, action_id, NULL }
#define FOOTER_ACTIONS(layout, label, key1, key2, action_id, action_id2)         \
  { { layout, label, key1, key2, NULL }, action_id, action_id2 }
#define FOOTER_ACTION_ALT_KEY(layout, label, key1, key2, action_id)              \
  { { layout, label, key1, key2, NULL }, action_id, NULL }

static BOOL ArchiveFooterCommandAvailable(const ViewContext *ctx, BOOL is_dir,
                                          const FooterCommandSpec *spec);

static int FooterCommandKeyClass(const UICommandStripCommand *command) {
  const char *key;

  if (command == NULL)
    return 2;

  key = command->primary_key;
  if (key == NULL || key[0] == '\0')
    return 2;
  if (isdigit((unsigned char)key[0]))
    return 0;
  if (key[0] == '^' && isalpha((unsigned char)key[1]) && key[2] == '\0')
    return 1;
  if (isalpha((unsigned char)key[0]))
    return 1;
  return 2;
}

static int FooterCommandKeySortGroup(const char *key) {
  if (key == NULL || key[0] == '\0')
    return 3;

  if (isdigit((unsigned char)key[0]))
    return 0;

  if (isalpha((unsigned char)key[0])) {
    if (key[1] == '\0')
      return 0;
    if (toupper((unsigned char)key[0]) == 'F' &&
        isdigit((unsigned char)key[1]))
      return 1;
    return 2;
  }

  return 3;
}

static int FooterCommandKeyNaturalNumber(const char *key) {
  int value = 0;
  size_t i;

  if (key == NULL)
    return -1;

  for (i = 0; key[i] != '\0'; ++i) {
    if (!isdigit((unsigned char)key[i]))
      return -1;
    value = value * 10 + (key[i] - '0');
  }

  return i > 0 ? value : -1;
}

static BOOL FooterCommandKeyIsNumericRange(const char *key) {
  const char *cursor;

  if (key == NULL || !isdigit((unsigned char)key[0]))
    return FALSE;

  cursor = key;
  while (isdigit((unsigned char)*cursor))
    ++cursor;
  if (cursor[0] != '.' || cursor[1] != '.')
    return FALSE;
  cursor += 2;
  if (!isdigit((unsigned char)*cursor))
    return FALSE;
  while (isdigit((unsigned char)*cursor))
    ++cursor;
  return *cursor == '\0';
}

static int CompareFooterCommandKeys(const char *left, const char *right) {
  const char *left_sort;
  const char *right_sort;
  int left_group;
  int right_group;
  int cmp;

  if (left == NULL || left[0] == '\0')
    return (right == NULL || right[0] == '\0') ? 0 : 1;
  if (right == NULL || right[0] == '\0')
    return -1;

  left_sort = (left[0] == '^' && isalpha((unsigned char)left[1]) &&
               left[2] == '\0')
                  ? left + 1
                  : left;
  right_sort = (right[0] == '^' && isalpha((unsigned char)right[1]) &&
                right[2] == '\0')
                   ? right + 1
                   : right;

  left_group = FooterCommandKeySortGroup(left_sort);
  right_group = FooterCommandKeySortGroup(right_sort);
  if (left_group != right_group)
    return left_group - right_group;

  if (left_group == 1) {
    int left_number = FooterCommandKeyNaturalNumber(left + 1);
    int right_number = FooterCommandKeyNaturalNumber(right + 1);

    if (left_number != right_number)
      return left_number - right_number;
  } else if (left_group == 0 && isdigit((unsigned char)left[0]) &&
             isdigit((unsigned char)right[0])) {
    BOOL left_is_range = FooterCommandKeyIsNumericRange(left);
    BOOL right_is_range = FooterCommandKeyIsNumericRange(right);
    int left_number = 0;
    int right_number = 0;
    const char *left_end = left;
    const char *right_end = right;

    if (left_is_range != right_is_range)
      return left_is_range ? -1 : 1;

    while (isdigit((unsigned char)*left_end)) {
      left_number = left_number * 10 + (*left_end - '0');
      ++left_end;
    }
    while (isdigit((unsigned char)*right_end)) {
      right_number = right_number * 10 + (*right_end - '0');
      ++right_end;
    }
    if (left_number != right_number)
      return left_number - right_number;
    cmp = strcasecmp(left_end, right_end);
    if (cmp != 0)
      return cmp;
  }

  cmp = strcasecmp(left_sort, right_sort);
  if (cmp != 0)
    return cmp;
  if (left_sort != left && right_sort == right)
    return 1;
  if (left_sort == left && right_sort != right)
    return -1;
  return strcasecmp(left, right);
}

static int CompareResolvedFooterCommands(const ResolvedFooterCommand *left,
                                         const ResolvedFooterCommand *right) {
  int cmp;

  if (left->key_class != right->key_class)
    return left->key_class - right->key_class;

  cmp = CompareFooterCommandKeys(left->primary_key, right->primary_key);
  if (cmp != 0)
    return cmp;

  cmp = CompareFooterCommandKeys(left->secondary_key, right->secondary_key);
  if (cmp != 0)
    return cmp;

  cmp = strcasecmp(left->label, right->label);
  if (cmp != 0)
    return cmp;

  return strcasecmp(left->rendered_text, right->rendered_text);
}

static FooterRowFit FitFooterRow(const UICommandStripCommand *commands,
                                 size_t command_count, int available_width) {
  FooterRowFit fit;
  int line_width = 0;

  memset(&fit, 0, sizeof(fit));
  while (fit.fit_count < command_count) {
    int command_width =
        UI_CommandStripVisualLength(&commands[fit.fit_count], 1);
    int separator_width = fit.fit_count > 0 ? 2 : 0;

    if (line_width + separator_width + command_width <= available_width) {
      line_width += separator_width + command_width;
      ++fit.fit_count;
      continue;
    }

    if (fit.fit_count == 0 || available_width - line_width - separator_width >= 3) {
      fit.truncated = TRUE;
      fit.truncated_index = fit.fit_count;
      fit.truncated_width = available_width - line_width - separator_width;
      if (fit.truncated_width < 0)
        fit.truncated_width = 0;
      fit.used_width = line_width + separator_width + fit.truncated_width;
      fit.represented_count = fit.fit_count + 1;
      return fit;
    }

    --fit.fit_count;
    line_width = fit.fit_count > 0
                     ? UI_CommandStripVisualLength(commands, fit.fit_count)
                     : 0;
    fit.truncated = TRUE;
    fit.truncated_index = fit.fit_count;
    fit.truncated_width =
        available_width - line_width - (fit.fit_count > 0 ? 2 : 0);
    if (fit.truncated_width < 0)
      fit.truncated_width = 0;
    fit.used_width =
        line_width + (fit.fit_count > 0 ? 2 : 0) + fit.truncated_width;
    fit.represented_count = fit.fit_count + 1;
    return fit;
  }

  fit.used_width = line_width;
  fit.represented_count = fit.fit_count;
  return fit;
}

static void SortResolvedFooterCommands(ResolvedFooterCommand *resolved,
                                       UICommandStripCommand *commands,
                                       size_t command_count) {
  size_t i;

  if (resolved == NULL || commands == NULL)
    return;

  for (i = 1; i < command_count; ++i) {
    ResolvedFooterCommand current = resolved[i];
    size_t j = i;

    while (j > 0 &&
           CompareResolvedFooterCommands(&current, &resolved[j - 1]) < 0) {
      resolved[j] = resolved[j - 1];
      --j;
    }
    resolved[j] = current;
  }

  for (i = 0; i < command_count; ++i)
    resolved[i].command.label = resolved[i].label;
  for (i = 0; i < command_count; ++i)
    resolved[i].command.primary_key = resolved[i].primary_key;
  for (i = 0; i < command_count; ++i)
    resolved[i].command.secondary_key =
        resolved[i].secondary_key[0] != '\0' ? resolved[i].secondary_key : NULL;
  for (i = 0; i < command_count; ++i)
    commands[i] = resolved[i].command;
}

static const FooterCommandSpec dir_footer_standard_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "dir view", "1..9", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Attributes", "A", NULL,
                  "ACTION_CMD_A"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Copy", "C", NULL,
                  "ACTION_CMD_C"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL,
                  "ACTION_CMD_D"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Global", "G", NULL,
                  "ACTION_CMD_G"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL,
                  "ACTION_INVERT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL,
                  "ACTION_COMPARE_DIR"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "volume", "K", NULL,
                  "ACTION_VOL_MENU"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Makedir", "M", NULL,
                  "ACTION_CMD_M"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Newfile", "N", NULL,
                  "ACTION_CMD_MKFILE"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL,
                  "ACTION_CMD_P"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Output", "O", NULL,
                  "ACTION_CMD_PRINT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL,
                  "ACTION_CMD_R"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "movedir", "V", NULL,
                  "ACTION_CMD_V"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "execute", "X", NULL,
                  "ACTION_CMD_X"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "archive", "Z", NULL,
                  "ACTION_CMD_I"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL,
                  "ACTION_LIST_JUMP"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL,
                  "ACTION_TOGGLE_HIDDEN")};

static const FooterCommandSpec dir_footer_ll_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "dir view", "1..9", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "reload", "^L", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT")};

static const FooterCommandSpec dir_footer_archive_to_root_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "dir view", "1..9", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "copy", "C", NULL,
                  "ACTION_CMD_C"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL,
                  "ACTION_CMD_D"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Global", "G", NULL,
                  "ACTION_CMD_G"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL,
                  "ACTION_COMPARE_DIR"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "volume", "K", NULL,
                  "ACTION_VOL_MENU"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Makedir", "M", NULL,
                  "ACTION_CMD_M"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL,
                  "ACTION_CMD_P"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Output", "O", NULL,
                  "ACTION_CMD_PRINT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "move", "V", NULL,
                  "ACTION_CMD_V"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "pathcopy", "Y", NULL,
                  "ACTION_CMD_Y"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL,
                  "ACTION_CMD_R"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL,
                  "ACTION_LIST_JUMP"),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "root", "\\", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL,
                  "ACTION_TOGGLE_HIDDEN")};

static const FooterCommandSpec dir_footer_archive_exit_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "dir view", "1..9", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "copy", "C", NULL,
                  "ACTION_CMD_C"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL,
                  "ACTION_CMD_D"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Global", "G", NULL,
                  "ACTION_CMD_G"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL,
                  "ACTION_COMPARE_DIR"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "volume", "K", NULL,
                  "ACTION_VOL_MENU"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Makedir", "M", NULL,
                  "ACTION_CMD_M"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL,
                  "ACTION_CMD_P"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Output", "O", NULL,
                  "ACTION_CMD_PRINT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "move", "V", NULL,
                  "ACTION_CMD_V"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "pathcopy", "Y", NULL,
                  "ACTION_CMD_Y"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL,
                  "ACTION_CMD_R"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL,
                  "ACTION_LIST_JUMP"),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "exit", "\\", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL,
                  "ACTION_TOGGLE_HIDDEN")};

static const FooterCommandSpec file_footer_standard_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "file view", "1..9", NULL),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Attributes", "A", "^A",
                   "ACTION_CMD_A", "ACTION_CMD_TAGGED_A"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "copy", "C", "^C",
                   "ACTION_CMD_C", "ACTION_CMD_TAGGED_C"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Delete", "D", "^D",
                   "ACTION_CMD_D", "ACTION_CMD_TAGGED_D"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Edit", "E", NULL,
                  "ACTION_CMD_E"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Hex", "H", NULL,
                  "ACTION_CMD_H"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL,
                  "ACTION_INVERT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL,
                  "ACTION_COMPARE_FILE"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "volume", "K", NULL,
                  "ACTION_VOL_MENU"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "move", "M", "^N",
                   "ACTION_CMD_M", "ACTION_CMD_TAGGED_M"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Newfile", "N", NULL,
                  "ACTION_CMD_MKFILE"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Pipe", "P", "^P",
                   "ACTION_CMD_P", "ACTION_CMD_TAGGED_P"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Rename", "R", "^R",
                   "ACTION_CMD_R", "ACTION_CMD_TAGGED_R"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Sort", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_CTRL_MNEMONIC, "Search", "^S", NULL,
                  "ACTION_CMD_TAGGED_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "view", "V", "^V",
                   "ACTION_CMD_V", "ACTION_CMD_TAGGED_V"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Output", "O", "^O",
                   "ACTION_CMD_PRINT", "ACTION_CMD_TAGGED_PRINT"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "execute", "X", "^X",
                   "ACTION_CMD_X", "ACTION_CMD_TAGGED_X"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "pathcopy", "Y", "^Y",
                   "ACTION_CMD_Y", "ACTION_CMD_TAGGED_Y"),
    FOOTER_ACTION_ALT_KEY(UI_COMMAND_LAYOUT_KEY_PREFIX, "archive", "Z", "^Z",
                          "ACTION_CMD_I"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL,
                  "ACTION_LIST_JUMP"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL,
                  "ACTION_TOGGLE_HIDDEN")};

static const FooterCommandSpec file_footer_ll_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "file view", "1..9", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "redraw", "^L", NULL),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Sort", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_CTRL_MNEMONIC, "Search", "^S", NULL,
                  "ACTION_CMD_TAGGED_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT")};

static const FooterCommandSpec file_footer_archive_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "file view", "1..9", NULL),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "copy", "C", "^C",
                   "ACTION_CMD_C", "ACTION_CMD_TAGGED_C"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Delete", "D", "^D",
                   "ACTION_CMD_D", "ACTION_CMD_TAGGED_D"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Hex", "H", NULL,
                  "ACTION_CMD_H"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL,
                  "ACTION_INVERT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL,
                  "ACTION_COMPARE_FILE"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "volume", "K", NULL,
                  "ACTION_VOL_MENU"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL, "ACTION_LOG"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "move", "M", "^N",
                   "ACTION_CMD_M", "ACTION_CMD_TAGGED_M"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Pipe", "P", "^P",
                   "ACTION_CMD_P", "ACTION_CMD_TAGGED_P"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Output", "O", "^O",
                   "ACTION_CMD_PRINT", "ACTION_CMD_TAGGED_PRINT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL,
                  "ACTION_QUIT"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Rename", "R", "^R",
                   "ACTION_CMD_R", "ACTION_CMD_TAGGED_R"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Sort", "S", NULL,
                  "ACTION_CMD_S"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_CTRL_MNEMONIC, "Search", "^S", NULL,
                  "ACTION_CMD_TAGGED_S"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "view", "V", "^V",
                   "ACTION_CMD_V", "ACTION_CMD_TAGGED_V"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "pathcopy", "Y", "^Y",
                   "ACTION_CMD_Y", "ACTION_CMD_TAGGED_Y"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL,
                  "ACTION_LIST_JUMP"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL,
                  "ACTION_TOGGLE_HIDDEN")};

static const FooterCommandSpec preview_footer_specs[] = {
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Attributes", "A", "^A",
                   "ACTION_CMD_A", "ACTION_CMD_TAGGED_A"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "copy", "C", "^C",
                   "ACTION_CMD_C", "ACTION_CMD_TAGGED_C"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Delete", "D", "^D",
                   "ACTION_CMD_D", "ACTION_CMD_TAGGED_D"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Edit", "E", NULL,
                  "ACTION_CMD_E"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL,
                  "ACTION_FILTER"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_CTRL_MNEMONIC, "Search", "^S", NULL,
                  "ACTION_CMD_TAGGED_S"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL,
                  "ACTION_INVERT"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL,
                  "ACTION_COMPARE_FILE"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "move", "M", "^N",
                   "ACTION_CMD_M", "ACTION_CMD_TAGGED_M"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_MNEMONIC, "Newfile", "N", NULL,
                  "ACTION_CMD_MKFILE"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Rename", "R", "^R",
                   "ACTION_CMD_R", "ACTION_CMD_TAGGED_R"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Tag", "T", "^T",
                   "ACTION_TAG", "ACTION_TAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Untag", "U", "^U",
                   "ACTION_UNTAG", "ACTION_UNTAG_ALL"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "view", "V", "^V",
                   "ACTION_CMD_V", "ACTION_CMD_TAGGED_V"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Output", "O", "^O",
                   "ACTION_CMD_PRINT", "ACTION_CMD_TAGGED_PRINT"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "execute", "X", "^X",
                   "ACTION_CMD_X", "ACTION_CMD_TAGGED_X"),
    FOOTER_ACTIONS(UI_COMMAND_LAYOUT_ALT_MNEMONIC, "pathcopy", "Y", "^Y",
                   "ACTION_CMD_Y", "ACTION_CMD_TAGGED_Y"),
    FOOTER_ACTION_ALT_KEY(UI_COMMAND_LAYOUT_KEY_PREFIX, "archive", "Z", "^Z",
                          "ACTION_CMD_I"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL,
                  "ACTION_LIST_JUMP"),
    FOOTER_ACTION(UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL,
                  "ACTION_TOGGLE_HIDDEN")};

static const FooterCommandSpec dir_footer_nav_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL)};

static const FooterCommandSpec file_footer_nav_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL)};

static const FooterCommandSpec file_footer_nav_to_dir_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "to dir", "\\", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL)};

static const FooterCommandSpec preview_footer_nav_specs[] = {
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "exit preview", "F7", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "applications", "F9", NULL),
    FOOTER_STATIC(UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL)};

static BOOL ActiveFooterVolumeIsArchive(const ViewContext *ctx) {
  return ctx != NULL && ctx->active != NULL && ctx->active->vol != NULL &&
         ctx->active->vol->vol_stats.log_mode == ARCHIVE_MODE;
}

static const char *FooterContextName(const ViewContext *ctx, BOOL is_dir) {
  if (ActiveFooterVolumeIsArchive(ctx))
    return is_dir ? "archive_dir" : "archive_file";
  return is_dir ? "dir" : "file";
}

static int CommandPresentationEntriesForFooter(
    const ViewContext *ctx, BOOL is_dir, const CommandPresentationOverride **entries,
    size_t *entry_count) {
  if (ctx == NULL || entries == NULL || entry_count == NULL)
    return -1;

  if (ActiveFooterVolumeIsArchive(ctx)) {
    if (is_dir) {
      *entries = ctx->archive_dir_command_presentations;
      *entry_count = ctx->archive_dir_command_presentation_count;
    } else {
      *entries = ctx->archive_file_command_presentations;
      *entry_count = ctx->archive_file_command_presentation_count;
    }
    return 0;
  }

  if (is_dir) {
    *entries = ctx->dir_command_presentations;
    *entry_count = ctx->dir_command_presentation_count;
  } else {
    *entries = ctx->file_command_presentations;
    *entry_count = ctx->file_command_presentation_count;
  }
  return 0;
}

static const CommandPresentationOverride *
FindCommandPresentationOverride(const ViewContext *ctx, BOOL is_dir,
                                const char *action_id) {
  const CommandPresentationOverride *entries;
  size_t entry_count;
  size_t index;

  if (ctx == NULL || action_id == NULL)
    return NULL;
  if (CommandPresentationEntriesForFooter(ctx, is_dir, &entries, &entry_count) !=
      0)
    return NULL;

  for (index = 0; index < entry_count; ++index) {
    if (strcmp(entries[index].action_id, action_id) == 0)
      return &entries[index];
  }
  return NULL;
}

static void NormalizeFooterLabel(UICommandStripLayout layout, char *label) {
  if (label == NULL || label[0] == '\0')
    return;
  if ((layout == UI_COMMAND_LAYOUT_KEY_PREFIX ||
       layout == UI_COMMAND_LAYOUT_ALT_MNEMONIC) &&
      isalpha((unsigned char)label[0])) {
    label[0] = (char)tolower((unsigned char)label[0]);
  }
}

static void ResolveFooterActionKey(const ViewContext *ctx, BOOL is_dir,
                                   const char *action_id,
                                   const char *fallback_key, char *out,
                                   size_t out_size) {
  if (out == NULL || out_size == 0)
    return;
  out[0] = '\0';

  if (action_id != NULL) {
    const CommandPresentationOverride *presentation =
        FindCommandPresentationOverride(ctx, is_dir, action_id);

    if (presentation != NULL && presentation->shown[0] != '\0') {
      (void)snprintf(out, out_size, "%s", presentation->shown);
      return;
    }
    {
      int default_key =
          CommandActionDefaultKeyCode(FooterContextName(ctx, is_dir), action_id);

      if (default_key >= 0) {
        int effective_key = ResolveUserActionBindingKey(ctx, is_dir, default_key);

        if (CommandKeyCodeToToken(effective_key, out, out_size) == 0)
          return;
      }
    }
  }

  if (fallback_key != NULL)
    (void)snprintf(out, out_size, "%s", fallback_key);
}

static void ResolveFooterCommandSpec(const ViewContext *ctx, BOOL is_dir,
                                     const FooterCommandSpec *spec,
                                     ResolvedFooterCommand *resolved) {
  const CommandPresentationOverride *presentation;
  const char *label = NULL;

  memset(resolved, 0, sizeof(*resolved));
  resolved->command.layout = spec->command.layout;

  if (spec->primary_action_id != NULL) {
    presentation =
        FindCommandPresentationOverride(ctx, is_dir, spec->primary_action_id);
    if (presentation != NULL && presentation->label[0] != '\0')
      label = presentation->label;
  }
  if (label == NULL)
    label = spec->command.label;
  if (label != NULL)
    (void)snprintf(resolved->label, sizeof(resolved->label), "%s", label);
  NormalizeFooterLabel(spec->command.layout, resolved->label);

  ResolveFooterActionKey(ctx, is_dir, spec->primary_action_id,
                         spec->command.primary_key, resolved->primary_key,
                         sizeof(resolved->primary_key));
  ResolveFooterActionKey(ctx, is_dir, spec->secondary_action_id,
                         spec->command.secondary_key, resolved->secondary_key,
                         sizeof(resolved->secondary_key));
  if (spec->secondary_action_id != NULL &&
      strcmp(spec->secondary_action_id, "ACTION_CMD_TAGGED_C") == 0) {
    (void)snprintf(resolved->secondary_key, sizeof(resolved->secondary_key),
                   "%s", "^C");
  }

  if (!is_dir && IsViKeysEnabled(ctx) && spec->primary_action_id != NULL) {
    if (strcmp(spec->primary_action_id, "ACTION_CMD_D") == 0) {
      (void)snprintf(resolved->primary_key, sizeof(resolved->primary_key), "%s",
                     "d");
    } else if (strcmp(spec->primary_action_id, "ACTION_UNTAG") == 0) {
      (void)snprintf(resolved->primary_key, sizeof(resolved->primary_key), "%s",
                     "u");
    }
  }

  resolved->command.label = resolved->label;
  resolved->command.primary_key = resolved->primary_key;
  resolved->command.secondary_key =
      resolved->secondary_key[0] != '\0' ? resolved->secondary_key : NULL;
  resolved->key_class = FooterCommandKeyClass(&resolved->command);
  (void)UI_FormatCommandStripEntryText(&resolved->command, resolved->rendered_text,
                                       sizeof(resolved->rendered_text));
}

static const FooterCommandSpec *
FindFooterSpecByPrimaryAction(const FooterCommandSpec *specs, size_t spec_count,
                              const char *action_id) {
  size_t index;

  if (specs == NULL || action_id == NULL)
    return NULL;

  for (index = 0; index < spec_count; ++index) {
    if (specs[index].primary_action_id != NULL &&
        strcmp(specs[index].primary_action_id, action_id) == 0)
      return &specs[index];
  }

  return NULL;
}

static size_t BuildHelpLabelOverrides(
    const ViewContext *ctx, BOOL is_dir, const HelpLabelOverridePlan *plan,
    UIHelpLabelOverride *overrides,
    char labels[][HELP_LABEL_OVERRIDE_TEXT_LENGTH], size_t max_overrides) {
  size_t count = 0;
  size_t index;

  if (ctx == NULL || plan == NULL || plan->specs == NULL ||
      plan->override_specs == NULL || overrides == NULL || labels == NULL)
    return 0;

  for (index = 0;
       index < plan->override_spec_count && count < max_overrides; ++index) {
    const FooterCommandSpec *spec = FindFooterSpecByPrimaryAction(
        plan->specs, plan->spec_count, plan->override_specs[index].action_id);

    overrides[count].canonical_label =
        plan->override_specs[index].canonical_label;
    overrides[count].display_label = NULL;
    if (spec != NULL && ArchiveFooterCommandAvailable(ctx, is_dir, spec)) {
      ResolvedFooterCommand resolved;

      ResolveFooterCommandSpec(ctx, is_dir, spec, &resolved);
      snprintf(labels[count], sizeof(labels[count]), "%s",
               resolved.rendered_text);
      overrides[count].display_label = labels[count];
    }
    count++;
  }

  return count;
}

static FooterPackResult PackFooterCommands(const UICommandStripCommand *commands,
                                           size_t command_count,
                                           int available_width, size_t rows,
                                           BOOL prefer_newfile_split) {
  FooterPackResult result;
  size_t split_index;
  size_t best_split = 0;
  size_t preferred_split = 0;
  FooterRowFit best_row1_fit;
  BOOL best_found = FALSE;
  size_t best_full_visible = 0;
  size_t best_represented = 0;
  int best_balance_delta = 0;

  memset(&result, 0, sizeof(result));

  if (rows != 2 || command_count == 0) {
    size_t row_index;
    size_t start_index = 0;

    for (row_index = 0; row_index < rows; ++row_index) {
      FooterRowFit fit = FitFooterRow(commands + start_index,
                                      command_count - start_index,
                                      available_width);

      result.line_counts[row_index] = fit.fit_count;
      result.visible_count += fit.fit_count;
      if (fit.truncated) {
        result.truncated = TRUE;
        result.truncated_row = row_index;
        result.truncated_index = start_index + fit.truncated_index;
        result.truncated_width = fit.truncated_width;
        return result;
      }
      start_index += fit.fit_count;
    }
    return result;
  }

  memset(&best_row1_fit, 0, sizeof(best_row1_fit));
  if (prefer_newfile_split) {
    for (preferred_split = 0; preferred_split < command_count;
         ++preferred_split) {
      if (strcmp(commands[preferred_split].label, "Newfile") == 0) {
        break;
      }
    }
    if (preferred_split >= command_count) {
      for (preferred_split = 0; preferred_split < command_count;
           ++preferred_split) {
        if (strcmp(commands[preferred_split].label, "Pipe") == 0) {
          break;
        }
      }
    }
    if (preferred_split == 0 || preferred_split >= command_count)
      preferred_split = 0;
  }

  for (split_index = 1; split_index <= command_count; ++split_index) {
    FooterRowFit row1_fit;
    int row0_width = UI_CommandStripVisualLength(commands, split_index);
    int balance_delta;
    int preferred_delta;
    int best_preferred_delta;
    size_t full_visible;
    size_t represented;

    if (row0_width > available_width)
      break;

    row1_fit = FitFooterRow(commands + split_index, command_count - split_index,
                            available_width);
    full_visible = split_index + row1_fit.fit_count;
    represented = split_index + row1_fit.represented_count;
    balance_delta = abs(row0_width - row1_fit.used_width);
    preferred_delta =
        preferred_split > 0 ? abs((int)split_index - (int)preferred_split) : 0;
    best_preferred_delta =
        preferred_split > 0 ? abs((int)best_split - (int)preferred_split) : 0;

    if (!best_found || represented > best_represented ||
        (represented == best_represented && full_visible > best_full_visible) ||
        (represented == best_represented && full_visible == best_full_visible &&
         balance_delta < best_balance_delta) ||
        (represented == best_represented && full_visible == best_full_visible &&
         balance_delta == best_balance_delta && preferred_split > 0 &&
         preferred_delta < best_preferred_delta)) {
      best_found = TRUE;
      best_split = split_index;
      best_row1_fit = row1_fit;
      best_represented = represented;
      best_full_visible = full_visible;
      best_balance_delta = balance_delta;
    }
  }

  if (!best_found) {
    FooterRowFit fit = FitFooterRow(commands, command_count, available_width);

    result.line_counts[0] = fit.fit_count;
    result.visible_count = fit.fit_count;
    result.truncated = fit.truncated;
    result.truncated_row = 0;
    result.truncated_index = fit.truncated_index;
    result.truncated_width = fit.truncated_width;
    return result;
  }

  result.line_counts[0] = best_split;
  result.line_counts[1] = best_row1_fit.fit_count;
  result.visible_count = best_split + best_row1_fit.fit_count;
  if (best_row1_fit.truncated) {
    result.truncated = TRUE;
    result.truncated_row = 1;
    result.truncated_index = best_split + best_row1_fit.truncated_index;
    result.truncated_width = best_row1_fit.truncated_width;
  }
  return result;
}

static void RenderFooterSignpost(WINDOW *win, int y, const char *signpost) {
  char padded[FOOTER_COMMAND_COLUMN];

  if (win == NULL || signpost == NULL)
    return;

  (void)snprintf(padded, sizeof(padded), "%-*.*s", FOOTER_COMMAND_COLUMN - 1,
                 FOOTER_COMMAND_COLUMN - 1, signpost);
  if (strncmp(signpost, "9-4", 3) == 0) {
#ifdef COLOR_SUPPORT
    PrintSpecialString(win, y, 0, "9-4",
                       UI_KEYBIND_BASE_PAIR + (UI_ROLE_FOOTER - 1));
    PrintSpecialString(win, y, 3, padded + 3, UI_ROLE_FOOTER);
#else
    PrintSpecialString(win, y, 0, "9-4", A_BOLD);
    PrintSpecialString(win, y, 3, padded + 3, A_NORMAL);
#endif
  } else {
#ifdef COLOR_SUPPORT
    PrintSpecialString(win, y, 0, padded, UI_ROLE_FOOTER);
#else
    PrintSpecialString(win, y, 0, padded, A_NORMAL);
#endif
  }
}

static void RenderPackedFooterLine(WINDOW *win, int y, const char *signpost,
                                   const UICommandStripCommand *commands,
                                   size_t command_count,
                                   const UICommandStripCommand *truncated_command,
                                   int truncated_width) {
  char truncated_text[160];
  char clipped[160];
  int x = FOOTER_COMMAND_COLUMN;
  int visible_prefix;
  int dots;

  if (win == NULL)
    return;

  wmove(win, y, 0);
  wclrtoeol(win);
  if (signpost != NULL && signpost[0] != '\0')
    RenderFooterSignpost(win, y, signpost);
  if (commands != NULL && command_count > 0) {
    UI_RenderCommandStrip(win, y, FOOTER_COMMAND_COLUMN, commands, command_count,
                          UI_ROLE_FOOTER, UI_ROLE_KEYBIND);
    x += UI_CommandStripVisualLength(commands, command_count);
  }
  if (truncated_command == NULL || truncated_width <= 0)
    return;

  if (command_count > 0) {
    PrintSpecialString(win, y, x, "  ", UI_ROLE_FOOTER);
    x += 2;
  }

  (void)UI_FormatCommandStripEntryText(truncated_command, truncated_text,
                                       sizeof(truncated_text));
  dots = truncated_width >= 3 ? 3 : truncated_width;
  visible_prefix = truncated_width - dots;
  if (dots <= 0)
    return;
  (void)snprintf(clipped, sizeof(clipped), "%.*s%.*s", visible_prefix,
                 truncated_text, dots, "...");
#ifdef COLOR_SUPPORT
  wmove(win, y, x);
  (void)WAttrAddStr(win, COLOR_PAIR(UI_ROLE_FOOTER), clipped);
#else
  (void)MvWAddStr(win, y, x, clipped);
#endif
}

static const FooterCommandSpec *GetDirFooterSpecs(const ViewContext *ctx,
                                                  const DirEntry *dir_entry,
                                                  size_t *command_count,
                                                  const char **line0_signpost,
                                                  const char **line1_signpost) {
  if (ctx->view_mode == LL_FILE_MODE) {
    *command_count =
        sizeof(dir_footer_ll_specs) / sizeof(dir_footer_ll_specs[0]);
    *line0_signpost = "DIR";
    *line1_signpost = "";
    return dir_footer_ll_specs;
  }
  if (ActiveFooterVolumeIsArchive(ctx)) {
    *line0_signpost = "ARCHIVE";
    *line1_signpost = "COMMANDS";
    if (dir_entry != NULL && dir_entry->up_tree != NULL) {
      *command_count = sizeof(dir_footer_archive_to_root_specs) /
                       sizeof(dir_footer_archive_to_root_specs[0]);
      return dir_footer_archive_to_root_specs;
    }
    *command_count = sizeof(dir_footer_archive_exit_specs) /
                     sizeof(dir_footer_archive_exit_specs[0]);
    return dir_footer_archive_exit_specs;
  }

  *command_count =
      sizeof(dir_footer_standard_specs) / sizeof(dir_footer_standard_specs[0]);
  *line0_signpost = "DIR";
  *line1_signpost = "COMMANDS";
  return dir_footer_standard_specs;
}

static const FooterCommandSpec *
GetDirFooterNavSpecs(const ViewContext *ctx, const DirEntry *dir_entry,
                     size_t *command_count, const char **signpost) {
  *signpost = "9-4 File";
  *command_count =
      sizeof(dir_footer_nav_specs) / sizeof(dir_footer_nav_specs[0]);
  return dir_footer_nav_specs;
}

static const FooterCommandSpec *GetFileFooterSpecs(const ViewContext *ctx,
                                                   size_t *command_count,
                                                   const char **line0_signpost,
                                                   const char **line1_signpost) {
  if (ctx->view_mode == LL_FILE_MODE) {
    *command_count =
        sizeof(file_footer_ll_specs) / sizeof(file_footer_ll_specs[0]);
    *line0_signpost = "FILE";
    *line1_signpost = "";
    return file_footer_ll_specs;
  }
  if (ActiveFooterVolumeIsArchive(ctx)) {
    *command_count = sizeof(file_footer_archive_specs) /
                     sizeof(file_footer_archive_specs[0]);
    *line0_signpost = "ARCHIVE";
    *line1_signpost = "COMMANDS";
    return file_footer_archive_specs;
  }

  *command_count =
      sizeof(file_footer_standard_specs) / sizeof(file_footer_standard_specs[0]);
  *line0_signpost = "FILE";
  *line1_signpost = "COMMANDS";
  return file_footer_standard_specs;
}

static const FooterCommandSpec *
GetFileFooterNavSpecs(const DirEntry *dir_entry, size_t *command_count,
                      const char **signpost) {
  *signpost = "9-4 Tree";
  if (dir_entry != NULL && dir_entry->global_flag) {
    *command_count = sizeof(file_footer_nav_to_dir_specs) /
                     sizeof(file_footer_nav_to_dir_specs[0]);
    return file_footer_nav_to_dir_specs;
  }

  *command_count =
      sizeof(file_footer_nav_specs) / sizeof(file_footer_nav_specs[0]);
  return file_footer_nav_specs;
}

static unsigned int ArchiveCapabilityForFooterAction(const char *action_id,
                                                     BOOL is_dir) {
  if (action_id == NULL)
    return 0;
  if (strcmp(action_id, "ACTION_CMD_C") == 0 ||
      strcmp(action_id, "ACTION_CMD_Y") == 0 ||
      strcmp(action_id, "ACTION_CMD_TAGGED_C") == 0 ||
      strcmp(action_id, "ACTION_CMD_TAGGED_Y") == 0)
    return ARCHIVE_CAP_COPY_OUT;
  if (strcmp(action_id, "ACTION_CMD_D") == 0 ||
      strcmp(action_id, "ACTION_CMD_TAGGED_D") == 0)
    return ARCHIVE_CAP_DELETE;
  if (strcmp(action_id, "ACTION_CMD_R") == 0 ||
      strcmp(action_id, "ACTION_CMD_TAGGED_R") == 0)
    return ARCHIVE_CAP_RENAME;
  if (strcmp(action_id, "ACTION_CMD_M") == 0)
    return is_dir ? ARCHIVE_CAP_ADD : ARCHIVE_CAP_MOVE;
  if (strcmp(action_id, "ACTION_CMD_V") == 0 ||
      strcmp(action_id, "ACTION_CMD_TAGGED_M") == 0)
    return ARCHIVE_CAP_MOVE;
  return 0;
}

static BOOL ArchiveFooterCommandAvailable(const ViewContext *ctx, BOOL is_dir,
                                          const FooterCommandSpec *spec) {
  unsigned int primary;
  unsigned int secondary;

  if (!ActiveFooterVolumeIsArchive(ctx))
    return TRUE;

  primary = ArchiveCapabilityForFooterAction(spec->primary_action_id, is_dir);
  secondary =
      ArchiveCapabilityForFooterAction(spec->secondary_action_id, is_dir);
  return (primary == 0 ||
          (ctx->active->vol->vol_stats.archive_capabilities & primary)) &&
         (secondary == 0 ||
          (ctx->active->vol->vol_stats.archive_capabilities & secondary));
}

static size_t ResolveFooterCommandList(const ViewContext *ctx, BOOL is_dir,
                                       const FooterCommandSpec *specs,
                                       size_t spec_count,
                                       ResolvedFooterCommand *resolved,
                                       UICommandStripCommand *commands) {
  size_t index;
  size_t count = 0;

  for (index = 0; index < spec_count; ++index) {
    if (!ArchiveFooterCommandAvailable(ctx, is_dir, &specs[index]))
      continue;
    ResolveFooterCommandSpec(ctx, is_dir, &specs[index], &resolved[count]);
    commands[count] = resolved[count].command;
    count++;
  }
  SortResolvedFooterCommands(resolved, commands, count);
  return count;
}

static void RenderFooterTopRowsWithSplitPreference(
    ViewContext *ctx, const char *line0_signpost, const char *line1_signpost,
    const UICommandStripCommand *commands, size_t command_count,
    BOOL prefer_newfile_split) {
  FooterPackResult pack;
  int available_width;

  available_width = getmaxx(ctx->ctx_menu_window) - FOOTER_COMMAND_COLUMN;
  if (available_width < 0)
    available_width = 0;
  pack = PackFooterCommands(commands, command_count, available_width, 2,
                            prefer_newfile_split);

  RenderPackedFooterLine(ctx->ctx_menu_window, 0, line0_signpost, commands,
                         pack.line_counts[0], NULL, 0);
  RenderPackedFooterLine(ctx->ctx_menu_window, 1, line1_signpost,
                         commands + pack.line_counts[0], pack.line_counts[1],
                         pack.truncated && pack.truncated_row == 1
                             ? &commands[pack.truncated_index]
                             : NULL,
                         pack.truncated && pack.truncated_row == 1
                             ? pack.truncated_width
                             : 0);
}

static void RenderFooterTopRows(ViewContext *ctx, const char *line0_signpost,
                                const char *line1_signpost,
                                const UICommandStripCommand *commands,
                                size_t command_count) {
  RenderFooterTopRowsWithSplitPreference(ctx, line0_signpost, line1_signpost,
                                         commands, command_count, FALSE);
}

static void RenderFooterNavRow(ViewContext *ctx, const char *signpost,
                               const FooterCommandSpec *specs,
                               size_t spec_count) {
  ResolvedFooterCommand resolved[16];
  UICommandStripCommand commands[16];
  FooterPackResult pack;
  int available_width;

  spec_count = ResolveFooterCommandList(
      ctx, AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE, specs,
      spec_count, resolved, commands);
  available_width = getmaxx(ctx->ctx_menu_window) - FOOTER_COMMAND_COLUMN;
  if (available_width < 0)
    available_width = 0;
  pack = PackFooterCommands(commands, spec_count, available_width, 1, FALSE);
  if (pack.truncated && spec_count > 0) {
    UICommandStripCommand fallback_commands[16];
    size_t command_index;
    size_t fallback_count = 0;
    BOOL dropped_stats = FALSE;

    for (command_index = 0; command_index < spec_count; ++command_index) {
      if (!dropped_stats && strcmp(commands[command_index].primary_key, "F6") == 0 &&
          strcmp(commands[command_index].label, "stats") == 0) {
        dropped_stats = TRUE;
        continue;
      }
      fallback_commands[fallback_count++] = commands[command_index];
    }

    if (dropped_stats) {
      FooterPackResult fallback_pack =
          PackFooterCommands(fallback_commands, fallback_count, available_width, 1,
                             FALSE);

      if (!fallback_pack.truncated ||
          fallback_pack.visible_count > pack.visible_count ||
          (fallback_pack.visible_count == pack.visible_count &&
           fallback_pack.truncated_index > pack.truncated_index)) {
        memcpy(commands, fallback_commands, fallback_count * sizeof(commands[0]));
        pack = fallback_pack;
      }
    }
  }
  RenderPackedFooterLine(ctx->ctx_menu_window, 2, signpost, commands,
                         pack.line_counts[0],
                         pack.truncated ? &commands[pack.truncated_index] : NULL,
                         pack.truncated ? pack.truncated_width : 0);
}

void DisplayDirHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  ResolvedFooterCommand resolved[32];
  UICommandStripCommand commands[32];
  const FooterCommandSpec *specs;
  const FooterCommandSpec *nav_specs;
  const char *line0_signpost;
  const char *line1_signpost;
  const char *nav_signpost;
  size_t spec_count;
  size_t nav_count;

  if (!ctx->ctx_menu_window)
    return;

  werase(ctx->ctx_menu_window);
  specs = GetDirFooterSpecs(ctx, dir_entry, &spec_count, &line0_signpost,
                            &line1_signpost);
  spec_count = ResolveFooterCommandList(ctx, TRUE, specs, spec_count, resolved,
                                        commands);
  RenderFooterTopRows(ctx, line0_signpost, line1_signpost, commands, spec_count);
  nav_specs = GetDirFooterNavSpecs(ctx, dir_entry, &nav_count, &nav_signpost);
  RenderFooterNavRow(ctx, nav_signpost, nav_specs, nav_count);
  UI_RenderStatusLineError(ctx);
  UI_RenderStatusLineNotice(ctx);
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayFileHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  ResolvedFooterCommand resolved[32];
  UICommandStripCommand commands[32];
  const FooterCommandSpec *specs;
  const FooterCommandSpec *nav_specs;
  const char *line0_signpost;
  const char *line1_signpost;
  const char *nav_signpost;
  size_t spec_count;
  size_t nav_count;

  if (!ctx->ctx_menu_window)
    return;

  werase(ctx->ctx_menu_window);
  specs =
      GetFileFooterSpecs(ctx, &spec_count, &line0_signpost, &line1_signpost);
  spec_count = ResolveFooterCommandList(ctx, FALSE, specs, spec_count, resolved,
                                        commands);
  if (dir_entry != NULL && dir_entry->global_flag) {
    RenderFooterTopRowsWithSplitPreference(ctx, line0_signpost,
                                           line1_signpost, commands, spec_count,
                                           TRUE);
  } else {
    RenderFooterTopRows(ctx, line0_signpost, line1_signpost, commands, spec_count);
  }
  nav_specs = GetFileFooterNavSpecs(dir_entry, &nav_count, &nav_signpost);
  RenderFooterNavRow(ctx, nav_signpost, nav_specs, nav_count);
  UI_RenderStatusLineError(ctx);
  UI_RenderStatusLineNotice(ctx);
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayHistoryHelp(ViewContext *ctx) {
  int window_height;
  int window_width;

  if (!ctx->ctx_history_window)
    return;

  GetMaxYX(ctx->ctx_history_window, &window_height, &window_width);
  if (window_height <= 0 || window_width <= 0)
    return;

#ifdef COLOR_SUPPORT
  if (ctx->color_enabled)
    wattrset(ctx->ctx_history_window, COLOR_PAIR(UI_ROLE_PICKER));
  else
    wattrset(ctx->ctx_history_window, 0);
#else
  wattrset(ctx->ctx_history_window, 0);
#endif
  mvwhline(ctx->ctx_history_window, window_height - 1, 0, ' ', window_width);

  UI_RenderCommandStrip(
      ctx->ctx_history_window, window_height - 1, HISTORY_DIALOG_COMMAND_STRIP_X,
      history_help_commands,
      sizeof(history_help_commands) / sizeof(history_help_commands[0]),
      UI_ROLE_PICKER, UI_ROLE_KEYBIND);
}

int UI_ShowHistoryHelpPopup(ViewContext *ctx) {
  return UI_ShowGeneratedContextHelp(ctx, "dialog.history", NULL, 0);
}

void DisplayPreviewHelp(ViewContext *ctx) {
  ResolvedFooterCommand resolved[32];
  ResolvedFooterCommand nav_resolved[8];
  UICommandStripCommand commands[32];
  UICommandStripCommand nav_commands[8];
  FooterPackResult pack;
  FooterPackResult nav_pack;
  int available_width;
  int base_y;
  size_t spec_count =
      sizeof(preview_footer_specs) / sizeof(preview_footer_specs[0]);
  size_t nav_count =
      sizeof(preview_footer_nav_specs) / sizeof(preview_footer_nav_specs[0]);
  size_t line1_count;
  const UICommandStripCommand *line0_truncated = NULL;
  const UICommandStripCommand *line1_truncated = NULL;

  if (ctx == NULL || ctx->ctx_border_window == NULL)
    return;

  available_width = getmaxx(ctx->ctx_border_window) - FOOTER_COMMAND_COLUMN;
  if (available_width < 0)
    available_width = 0;

  ResolveFooterCommandList(ctx, FALSE, preview_footer_specs, spec_count, resolved,
                           commands);
  pack = PackFooterCommands(commands, spec_count, available_width, 2, FALSE);
  line1_count =
      pack.visible_count > pack.line_counts[0] ? pack.visible_count - pack.line_counts[0]
                                               : 0;
  if (pack.truncated) {
    if (pack.truncated_row == 0)
      line0_truncated = &commands[pack.truncated_index];
    else
      line1_truncated = &commands[pack.truncated_index];
  }

  ResolveFooterCommandList(ctx, FALSE, preview_footer_nav_specs, nav_count,
                           nav_resolved, nav_commands);
  nav_pack = PackFooterCommands(nav_commands, nav_count, available_width, 1, FALSE);
  base_y = Y_PROMPT(ctx) - 1;

  RenderPackedFooterLine(ctx->ctx_border_window, base_y, "PREVIEW", commands,
                         pack.line_counts[0], line0_truncated,
                         (line0_truncated != NULL) ? pack.truncated_width : 0);
  RenderPackedFooterLine(ctx->ctx_border_window, base_y + 1, "COMMANDS",
                         commands + pack.line_counts[0], line1_count,
                         line1_truncated,
                         (line1_truncated != NULL) ? pack.truncated_width : 0);
  RenderPackedFooterLine(ctx->ctx_border_window, base_y + 2, "", nav_commands,
                         nav_pack.line_counts[0],
                         nav_pack.truncated ? &nav_commands[nav_pack.truncated_index]
                                            : NULL,
                         nav_pack.truncated ? nav_pack.truncated_width : 0);
}

static const HelpLabelOverrideSpec dir_help_label_specs[] = {
    {"Attributes", "ACTION_CMD_A"},
    {"Copy", "ACTION_CMD_C"},
    {"Delete", "ACTION_CMD_D"},
    {"Filter", "ACTION_FILTER"},
    {"Global", "ACTION_CMD_G"},
    {"Invert Tags", "ACTION_INVERT"},
    {"Compare", "ACTION_COMPARE_DIR"},
    {"Volume", "ACTION_VOL_MENU"},
    {"Log", "ACTION_LOG"},
    {"Makedir", "ACTION_CMD_M"},
    {"New File", "ACTION_CMD_MKFILE"},
    {"Pipe", "ACTION_CMD_P"},
    {"Output", "ACTION_CMD_PRINT"},
    {"Quit", "ACTION_QUIT"},
    {"Rename", "ACTION_CMD_R"},
    {"Showall", "ACTION_CMD_S"},
    {"Tag", "ACTION_TAG"},
    {"Untag", "ACTION_UNTAG"},
    {"MoveDir", "ACTION_CMD_V"},
    {"Execute", "ACTION_CMD_X"},
    {"Archive", "ACTION_CMD_I"},
    {"Jump", "ACTION_LIST_JUMP"},
    {"Dotfiles", "ACTION_TOGGLE_HIDDEN"}};

static const HelpLabelOverridePlan dir_help_label_plan = {
    dir_footer_standard_specs,
    sizeof(dir_footer_standard_specs) / sizeof(dir_footer_standard_specs[0]),
    dir_help_label_specs,
    sizeof(dir_help_label_specs) / sizeof(dir_help_label_specs[0])};

static const HelpLabelOverrideSpec archive_dir_help_label_specs[] = {
    {"Copy", "ACTION_CMD_C"},
    {"Delete", "ACTION_CMD_D"},
    {"Filter", "ACTION_FILTER"},
    {"Global", "ACTION_CMD_G"},
    {"Compare", "ACTION_COMPARE_DIR"},
    {"Volume", "ACTION_VOL_MENU"},
    {"Log", "ACTION_LOG"},
    {"Makedir", "ACTION_CMD_M"},
    {"Pipe", "ACTION_CMD_P"},
    {"Output", "ACTION_CMD_PRINT"},
    {"Move", "ACTION_CMD_V"},
    {"Pathcopy", "ACTION_CMD_Y"},
    {"Quit", "ACTION_QUIT"},
    {"Rename", "ACTION_CMD_R"},
    {"Showall", "ACTION_CMD_S"},
    {"Tag", "ACTION_TAG"},
    {"Untag", "ACTION_UNTAG"},
    {"Jump", "ACTION_LIST_JUMP"},
    {"Dotfiles", "ACTION_TOGGLE_HIDDEN"}};

static const HelpLabelOverridePlan archive_dir_help_label_plan = {
    dir_footer_archive_to_root_specs,
    sizeof(dir_footer_archive_to_root_specs) /
        sizeof(dir_footer_archive_to_root_specs[0]),
    archive_dir_help_label_specs,
    sizeof(archive_dir_help_label_specs) /
        sizeof(archive_dir_help_label_specs[0])};

static const HelpLabelOverrideSpec file_help_label_specs[] = {
    {"Attributes", "ACTION_CMD_A"},
    {"Copy", "ACTION_CMD_C"},
    {"Delete", "ACTION_CMD_D"},
    {"Edit", "ACTION_CMD_E"},
    {"Filter", "ACTION_FILTER"},
    {"Hex", "ACTION_CMD_H"},
    {"Invert Tags", "ACTION_INVERT"},
    {"Compare", "ACTION_COMPARE_FILE"},
    {"Volume", "ACTION_VOL_MENU"},
    {"Log", "ACTION_LOG"},
    {"Move", "ACTION_CMD_M"},
    {"New File", "ACTION_CMD_MKFILE"},
    {"Pipe", "ACTION_CMD_P"},
    {"Quit", "ACTION_QUIT"},
    {"Rename", "ACTION_CMD_R"},
    {"Sort", "ACTION_CMD_S"},
    {"Tag", "ACTION_TAG"},
    {"Untag", "ACTION_UNTAG"},
    {"View", "ACTION_CMD_V"},
    {"Output", "ACTION_CMD_PRINT"},
    {"Execute", "ACTION_CMD_X"},
    {"Pathcopy", "ACTION_CMD_Y"},
    {"Archive", "ACTION_CMD_I"},
    {"Jump", "ACTION_LIST_JUMP"},
    {"Dotfiles", "ACTION_TOGGLE_HIDDEN"}};

static const HelpLabelOverridePlan file_help_label_plan = {
    file_footer_standard_specs,
    sizeof(file_footer_standard_specs) / sizeof(file_footer_standard_specs[0]),
    file_help_label_specs,
    sizeof(file_help_label_specs) / sizeof(file_help_label_specs[0])};

static const HelpLabelOverrideSpec archive_file_help_label_specs[] = {
    {"Copy", "ACTION_CMD_C"},
    {"Delete", "ACTION_CMD_D"},
    {"Filter", "ACTION_FILTER"},
    {"Hex", "ACTION_CMD_H"},
    {"Invert Tags", "ACTION_INVERT"},
    {"Compare", "ACTION_COMPARE_FILE"},
    {"Volume", "ACTION_VOL_MENU"},
    {"Log", "ACTION_LOG"},
    {"Move", "ACTION_CMD_M"},
    {"Pipe", "ACTION_CMD_P"},
    {"Output", "ACTION_CMD_PRINT"},
    {"Quit", "ACTION_QUIT"},
    {"Rename", "ACTION_CMD_R"},
    {"Sort", "ACTION_CMD_S"},
    {"Tag", "ACTION_TAG"},
    {"Untag", "ACTION_UNTAG"},
    {"View", "ACTION_CMD_V"},
    {"Pathcopy", "ACTION_CMD_Y"},
    {"Jump", "ACTION_LIST_JUMP"},
    {"Dotfiles", "ACTION_TOGGLE_HIDDEN"}};

static const HelpLabelOverridePlan archive_file_help_label_plan = {
    file_footer_archive_specs,
    sizeof(file_footer_archive_specs) / sizeof(file_footer_archive_specs[0]),
    archive_file_help_label_specs,
    sizeof(archive_file_help_label_specs) /
        sizeof(archive_file_help_label_specs[0])};

static const HelpLabelOverrideSpec preview_help_label_specs[] = {
    {"Attributes", "ACTION_CMD_A"},
    {"Copy", "ACTION_CMD_C"},
    {"Delete", "ACTION_CMD_D"},
    {"Edit", "ACTION_CMD_E"},
    {"Filter", "ACTION_FILTER"},
    {"Invert Tags", "ACTION_INVERT"},
    {"Compare", "ACTION_COMPARE_FILE"},
    {"Move", "ACTION_CMD_M"},
    {"New File", "ACTION_CMD_MKFILE"},
    {"Rename", "ACTION_CMD_R"},
    {"Tag", "ACTION_TAG"},
    {"Untag", "ACTION_UNTAG"},
    {"View", "ACTION_CMD_V"},
    {"Output", "ACTION_CMD_PRINT"},
    {"Execute", "ACTION_CMD_X"},
    {"Pathcopy", "ACTION_CMD_Y"},
    {"Archive", "ACTION_CMD_I"},
    {"Jump", "ACTION_LIST_JUMP"},
    {"Dotfiles", "ACTION_TOGGLE_HIDDEN"}};

static const HelpLabelOverridePlan preview_help_label_plan = {
    preview_footer_specs,
    sizeof(preview_footer_specs) / sizeof(preview_footer_specs[0]),
    preview_help_label_specs,
    sizeof(preview_help_label_specs) / sizeof(preview_help_label_specs[0])};

static int ShowGeneratedHelpForPlan(ViewContext *ctx, BOOL is_dir,
                                    const char *context_id,
                                    const HelpLabelOverridePlan *plan) {
  size_t max_overrides;
  UIHelpLabelOverride *label_overrides;
  char(*label_text)[HELP_LABEL_OVERRIDE_TEXT_LENGTH];
  int rc;

  if (ctx == NULL || context_id == NULL || plan == NULL)
    return -1;

  max_overrides = plan->override_spec_count;
  label_overrides = xcalloc(max_overrides, sizeof(*label_overrides));
  label_text = xcalloc(max_overrides, sizeof(*label_text));
  rc = UI_ShowGeneratedContextHelpWithOverrides(
      ctx, context_id, NULL, 0, label_overrides,
      BuildHelpLabelOverrides(ctx, is_dir, plan, label_overrides, label_text,
                              max_overrides));
  free(label_text);
  free(label_overrides);
  return rc;
}

int UI_ShowIntegratedHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  ViewFocus active_focus;

  if (ctx == NULL)
    return -1;

  active_focus = AppStateResolveActivePanelFocus(ctx);
  if (ctx->preview_mode)
    return ShowGeneratedHelpForPlan(
        ctx, FALSE, (active_focus == FOCUS_TREE) ? "overlay.f7-dir"
                                                 : "overlay.f7-file",
        &preview_help_label_plan);

  if (active_focus == FOCUS_TREE) {
    if (ActiveFooterVolumeIsArchive(ctx))
      return ShowGeneratedHelpForPlan(ctx, TRUE, "main.archive-dir",
                                      &archive_dir_help_label_plan);
    if (ctx->is_split_screen)
      return ShowGeneratedHelpForPlan(ctx, TRUE, "overlay.f8-dir",
                                      &dir_help_label_plan);
    return ShowGeneratedHelpForPlan(ctx, TRUE, "main.dir",
                                    &dir_help_label_plan);
  }

  if (ActiveFooterVolumeIsArchive(ctx))
    return ShowGeneratedHelpForPlan(ctx, FALSE, "main.archive-file",
                                    &archive_file_help_label_plan);
  if (ctx->is_split_screen)
    return ShowGeneratedHelpForPlan(ctx, FALSE, "overlay.f8-file",
                                    &file_help_label_plan);
  if (dir_entry == NULL || !dir_entry->global_flag)
    return ShowGeneratedHelpForPlan(ctx, FALSE, "main.file",
                                    &file_help_label_plan);
  return ShowGeneratedHelpForPlan(
      ctx, FALSE, dir_entry->global_all_volumes ? "main.global" : "main.showall",
      &file_help_label_plan);
}

void ClearHelp(ViewContext *ctx) {
  if (ctx == NULL)
    return;
  if (ctx->ctx_menu_window) {
    werase(ctx->ctx_menu_window);
    wnoutrefresh(ctx->ctx_menu_window);
  }
}

/*
 * DisplayHeaderPath
 * Prints the current path in the top-left header area.
 * This function is designed to be called whenever the path changes,
 * ensuring immediate visual feedback.
 */
void DisplayHeaderPath(ViewContext *ctx, const char *path) {
  char display_buffer[PATH_LENGTH + 1];
  int available_width;

  if (!ctx->ctx_path_window)
    return;

  available_width = getmaxx(ctx->ctx_path_window);

  CutPathname(display_buffer, path, available_width);

  DEBUG_LOG("DisplayHeaderPath: path='%s' cut='%s' avail=%d", path,
            display_buffer, available_width);

  WbkgdSet(ctx, ctx->ctx_path_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  wattrset(ctx->ctx_path_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  werase(ctx->ctx_path_window);
  mvwaddstr(ctx->ctx_path_window, 0, 0, display_buffer);
  wnoutrefresh(ctx->ctx_path_window);
}

static int PanelDataRightX(const YtreeNovaPanel *panel) {
  if (!panel)
    return 0;
  return panel->dir_x + panel->dir_w + panel->stats_width;
}

static int SplitSeparatorX(const YtreeNovaPanel *panel) {
  if (!panel)
    return 0;
  return panel->dir_x + panel->dir_w + panel->stats_width;
}

static void DisplaySplitTopFilter(ViewContext *ctx, const YtreeNovaPanel *panel,
                                  int start_x, int available_width) {
  char filter_label[FILE_SPEC_LENGTH + 3];
  char display_buffer[sizeof(filter_label)];
  const char *file_spec;

  if (!ctx || !ctx->ctx_border_window || !panel || !panel->vol ||
      available_width <= 0)
    return;

  file_spec = panel->vol->vol_stats.file_spec;
  if (!file_spec[0])
    file_spec = DEFAULT_FILE_SPEC;

  snprintf(filter_label, sizeof(filter_label), "<%s>", file_spec);
  CutName(display_buffer, filter_label, available_width);

  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  mvwaddstr(ctx->ctx_border_window, 1, start_x, display_buffer);
  wattrset(ctx->ctx_border_window, A_NORMAL);
}

static void DisplaySplitTopFilters(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window || !ctx->is_split_screen || !ctx->left ||
      !ctx->right)
    return;

  DisplaySplitTopFilter(ctx, ctx->left, ctx->left->dir_x, ctx->left->dir_w);
  DisplaySplitTopFilter(ctx, ctx->right, ctx->right->dir_x, ctx->right->dir_w);
}

void DisplayMenu(ViewContext *ctx) {
  const int L_BORDER_FOR_DISPLAY = COLS - ctx->layout.stats_width - 1;
  int bottom_y = ctx->layout.bottom_border_y;

  /* Explicitly Clear all relevant windows - NO wnoutrefresh here */
  werase(ctx->ctx_border_window);

  /* Draw Header Label */
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_STATIC_TEXT));
  mvwaddstr(ctx->ctx_border_window, 0, 0, "Path: ");
  wattrset(ctx->ctx_border_window, A_NORMAL);

  /* Path will be filled in by caller (RefreshView) using
   * GetPath(dir_entry) */

  /* --- NATIVE ACS BORDERS --- */
  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);

  /* Outer Box Frame (Data Area) */
  mvwhline(ctx->ctx_border_window, 1, 0, ACS_HLINE, L_BORDER_FOR_DISPLAY);
  mvwhline(ctx->ctx_border_window, bottom_y, 0, ACS_HLINE,
           L_BORDER_FOR_DISPLAY);
  mvwvline(ctx->ctx_border_window, 1, 0, ACS_VLINE, bottom_y - 1);
  mvwvline(ctx->ctx_border_window, 1, L_BORDER_FOR_DISPLAY, ACS_VLINE,
           bottom_y - 1);

  /* Corners */
  mvwaddch(ctx->ctx_border_window, 1, 0, ACS_ULCORNER);
  mvwaddch(ctx->ctx_border_window, 1, L_BORDER_FOR_DISPLAY, ACS_URCORNER);
  mvwaddch(ctx->ctx_border_window, bottom_y, 0, ACS_LLCORNER);
  mvwaddch(ctx->ctx_border_window, bottom_y, L_BORDER_FOR_DISPLAY,
           ACS_LRCORNER);

  /* Sub-window separators */
  if (ctx->preview_mode) {
    int sep_x = ctx->layout.preview_win_x - 1;
    mvwvline(ctx->ctx_border_window, 2, sep_x, ACS_VLINE, bottom_y - 2);
    mvwaddch(ctx->ctx_border_window, 1, sep_x, ACS_TTEE);
    mvwaddch(ctx->ctx_border_window, bottom_y, sep_x, ACS_BTEE);
  } else {
    /* Vertical Split Separator */
    if (ctx->is_split_screen && ctx->left) {
      int split_x = SplitSeparatorX(ctx->left);
      mvwvline(ctx->ctx_border_window, 2, split_x, ACS_VLINE, bottom_y - 2);
      mvwaddch(ctx->ctx_border_window, 1, split_x, ACS_TTEE);
      mvwaddch(ctx->ctx_border_window, bottom_y, split_x, ACS_BTEE);
    }
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);

  DisplaySplitTopFilters(ctx);
}

void SwitchToSmallFileWindow(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_file_window);
  int separator_width = COLS - ctx->layout.stats_width - 1;
  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);
  mvwhline(ctx->ctx_border_window, separator_y, 1, ACS_HLINE,
           separator_width - 1);
  mvwaddch(ctx->ctx_border_window, separator_y, 0, ACS_LTEE);
  mvwaddch(ctx->ctx_border_window, separator_y, separator_width, ACS_RTEE);

  if (ctx->layout.stats_width == 0) {
    mvwaddch(ctx->ctx_border_window, separator_y, COLS - 1, ACS_RTEE);
  }

  /* Restore Split Screen Junction if visible */
  if (ctx->is_split_screen && ctx->left) {
    int split_x = SplitSeparatorX(ctx->left);
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_PLUS);
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);

  AppStateSetPanelFileWindowHandle(ctx, ctx->active, FALSE);
}

void SwitchToBigFileWindow(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_file_window);

  /* Erase the horizontal separator line completely */
  int separator_width = COLS - ctx->layout.stats_width - 1;
  wmove(ctx->ctx_border_window, separator_y, 0);
  whline(ctx->ctx_border_window, ' ', separator_width + 1);

  /* Draw vertical borders at left and right edges of dir window */
  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);
  mvwaddch(ctx->ctx_border_window, separator_y, ctx->layout.dir_win_x - 1,
           ACS_VLINE);
  mvwaddch(ctx->ctx_border_window, separator_y,
           ctx->layout.dir_win_x + ctx->layout.dir_win_width, ACS_VLINE);
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);

  AppStateSetPanelFileWindowHandle(ctx, ctx->active, TRUE);
}

void MapF2Window(ViewContext *ctx) {
  werase(ctx->ctx_f2_window);
}

void UnmapF2Window(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_f2_window);
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES));
  if (ctx->ctx_file_window == ctx->ctx_big_file_window) {
    mvwaddch(ctx->ctx_border_window, separator_y, ctx->layout.dir_win_x - 1,
             '|');
    mvwaddch(ctx->ctx_border_window, separator_y,
             ctx->layout.dir_win_x + ctx->layout.dir_win_width, '|');
  } else {
    int separator_width = COLS - ctx->layout.stats_width - 1;
    wattron(ctx->ctx_border_window, A_ALTCHARSET);
    mvwhline(ctx->ctx_border_window, separator_y, 1, ACS_HLINE,
             separator_width - 1);
    mvwaddch(ctx->ctx_border_window, separator_y, 0, ACS_LTEE);
    mvwaddch(ctx->ctx_border_window, separator_y, separator_width, ACS_RTEE);
    wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  }
  wattrset(ctx->ctx_border_window, A_NORMAL);
}


void RefreshWindow(WINDOW *win) { wnoutrefresh(win); }

static BOOL IsPanelSavedBigFileMode(const YtreeNovaPanel *panel);

static void ComputePanelRenderPosition(const YtreeNovaPanel *panel, int idx,
                                       int *begin_out, int *cursor_out) {
  int height;

  if (!begin_out || !cursor_out)
    return;
  *begin_out = 0;
  *cursor_out = 0;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list ||
      panel->vol->total_dirs <= 0)
    return;

  if (idx < 0)
    idx = 0;
  if (idx >= panel->vol->total_dirs)
    idx = panel->vol->total_dirs - 1;

  height = panel->pan_dir_window ? getmaxy(panel->pan_dir_window) : 1;
  if (height < 1)
    height = 1;

  *begin_out = panel->disp_begin_pos;
  *cursor_out = panel->cursor_pos;
  if (!PanelComputeViewportPosition(panel, idx, height, begin_out, cursor_out))
    return;
}

static DirEntry *ResolvePanelFileAnchor(const YtreeNovaPanel *panel) {
  if (!panel || !panel->vol || panel->saved_focus != FOCUS_FILE)
    return NULL;
  assert(panel->file_selection_dir_path[0] != '\0');
  if (panel->file_selection_dir_path[0] == '\0')
    return NULL;

  return ResolvePanelAnchorTarget(panel, panel->vol,
                                  panel->file_selection_dir_path);
}

static DirEntry *ResolvePanelFileAnchorForRender(ViewContext *ctx,
                                                 const YtreeNovaPanel *panel) {
  (void)ctx;
  return ResolvePanelFileAnchor(panel);
}

void RenderInactivePanel(ViewContext *ctx, YtreeNovaPanel *panel) {
  if (!panel || !panel->vol || !panel->pan_dir_window)
    return;

  int total = panel->vol->total_dirs;
  int begin = panel->disp_begin_pos;
  int cursor = panel->cursor_pos;
  int selected_idx = GetPanelVisibleSelectionIndex(panel);

  if (total > 0 && (begin + cursor >= total)) {
    begin = 0;
    cursor = 0;
  }

  if (total <= 0)
    return;

  if (panel->saved_focus == FOCUS_FILE &&
      ResolvePanelFileAnchorForRender(ctx, panel)) {
    begin = panel->disp_begin_pos;
    cursor = panel->cursor_pos;
  }

  {
    int render_start = panel->start_file;
    int render_cursor = 0;
    int render_begin = begin;
    int render_tree_cursor = cursor;
    int idx = selected_idx;
    DirEntry *render_dir = NULL;
    const DirEntry *de = NULL;

    if (idx < 0 || idx >= total)
      return;
    ComputePanelRenderPosition(panel, idx, &render_begin, &render_tree_cursor);
    begin = render_begin;

    de = panel->vol->dir_entry_list[idx].dir_entry;
    if (!de)
      return;

    if (panel->saved_focus == FOCUS_FILE) {
      BOOL refresh_file_cache = FALSE;
      char render_dir_path[PATH_LENGTH + 1];

      render_dir = ResolvePanelFileAnchorForRender(ctx, panel);

      if (!render_dir)
        render_dir = (DirEntry *)de;

      GetPath(render_dir, render_dir_path);
      render_dir_path[PATH_LENGTH] = '\0';

      /* Inactive-panel rendering must not rewrite the frozen snapshot. Keep
       * the anchor local and rebuild cache only when the saved directory is
       * genuinely missing or stale. */
      DirOps_ReloadPanelFileAnchorIfMissing(ctx, panel, render_dir);
      if (panel->file_entry_list == NULL ||
          strcmp(panel->file_selection_dir_path, render_dir_path) != 0) {
        refresh_file_cache = TRUE;
      }
      if (refresh_file_cache) {
        FreeFileEntryList(panel);
        BuildFileEntryList(ctx, panel);
      }
    } else if (!panel->file_entry_list) {
      BuildFileEntryList(ctx, panel);
    }

    if (render_dir)
      de = render_dir;

    render_cursor = de->cursor_pos;
    if (panel->saved_focus == FOCUS_FILE) {
      render_start = panel->start_file;
      render_cursor = panel->file_cursor_pos;
    }
    AppStateClampRenderFileViewport(panel->file_count, &render_start,
                                    &render_cursor);

    if (IsPanelSavedBigFileMode(panel) && panel->pan_big_file_window) {
      int file_hilight = -1;

      if (panel->saved_focus == FOCUS_FILE) {
        file_hilight =
            AppStateResolveRenderFileHighlight(panel->file_count, render_start,
                                               render_cursor);
      }
      DEBUG_LOG("RenderInactivePanel:file path='%s' start=%d cursor=%d count=%u",
                panel->file_selection_dir_path[0] ? panel->file_selection_dir_path
                                                  : "<none>",
                render_start, render_cursor, panel->file_count);
      if (panel->pan_dir_window) {
        werase(panel->pan_dir_window);
        wnoutrefresh(panel->pan_dir_window);
      }
      DisplayFiles(ctx, panel, de, render_start, file_hilight, 0,
                   panel->pan_big_file_window);
      wnoutrefresh(panel->pan_big_file_window);
      return;
    }

    if (panel->pan_dir_window) {
      int tree_hilight = selected_idx;
      if (panel->saved_focus == FOCUS_FILE)
        tree_hilight = -1;
      DisplayTree(ctx, panel->vol, panel->pan_dir_window, begin, tree_hilight,
                  FALSE);
      wnoutrefresh(panel->pan_dir_window);
    }

    if (panel->pan_file_window) {
      if (panel->saved_focus != FOCUS_FILE && ctx &&
          AppStateResolveActivePanelFocus(ctx) == FOCUS_FILE &&
          panel->file_count == 0) {
        werase(panel->pan_file_window);
        wnoutrefresh(panel->pan_file_window);
      } else {
        int file_hilight = -1;

        if (panel->saved_focus == FOCUS_FILE) {
          file_hilight =
              AppStateResolveRenderFileHighlight(panel->file_count, render_start,
                                                 render_cursor);
        }
        DisplayFiles(ctx, panel, de, render_start, file_hilight, 0,
                     panel->pan_file_window);
        wnoutrefresh(panel->pan_file_window);
      }
    }
  }
}

static BOOL IsActivePanelBigFileMode(const ViewContext *ctx,
                                     const DirEntry *dir_entry) {
  if (!ctx)
    return FALSE;

  if (!dir_entry)
    return FALSE;

  if (!ctx->active || AppStateResolveActivePanelFocus(ctx) != FOCUS_FILE)
    return FALSE;

  return (ctx->active->saved_big_file_view || dir_entry->global_flag ||
          dir_entry->tagged_flag);
}

static BOOL IsPanelSavedBigFileMode(const YtreeNovaPanel *panel) {
  if (!panel)
    return FALSE;

  return (panel->saved_focus == FOCUS_FILE && panel->saved_big_file_view);
}

static void DrawSplitSeparatorRow(ViewContext *ctx, BOOL left_big,
                                  BOOL right_big) {
  int separator_y;
  int data_right_x;
  int split_x;

  if (!ctx || !ctx->ctx_border_window || !ctx->left)
    return;

  separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;
  data_right_x = ctx->right ? PanelDataRightX(ctx->right)
                            : COLS - ctx->layout.stats_width - 1;
  split_x = SplitSeparatorX(ctx->left);

  /* Clear the entire separator row before redrawing split-aware junctions. */
  wmove(ctx->ctx_border_window, separator_y, 0);
  whline(ctx->ctx_border_window, ' ', data_right_x + 1);

  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);

  mvwaddch(ctx->ctx_border_window, separator_y, 0,
           left_big ? ACS_VLINE : ACS_LTEE);
  mvwaddch(ctx->ctx_border_window, separator_y, data_right_x,
           right_big ? ACS_VLINE : ACS_RTEE);

  if (!left_big && split_x > 1) {
    mvwhline(ctx->ctx_border_window, separator_y, 1, ACS_HLINE, split_x - 1);
  }
  if (!right_big && data_right_x - split_x > 1) {
    mvwhline(ctx->ctx_border_window, separator_y, split_x + 1, ACS_HLINE,
             data_right_x - split_x - 1);
  }

  if (!left_big && !right_big) {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_PLUS);
  } else if (!left_big && right_big) {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_RTEE);
  } else if (left_big && !right_big) {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_LTEE);
  } else {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_VLINE);
  }

  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);
}

/*
 * CENTRALIZED REDRAW FUNCTION
 * Handles the complexities of Split/Big/Preview modes in one place.
 * Use this to ensure all borders, stats, and content are consistent.
 */
void RefreshView(ViewContext *ctx, DirEntry *dir_entry) {
  if (!AppStateValidatedDispatchSurface("surface.render-reflow-projection"))
    return;
  if (!AppStateValidatedEvent("event.render-reflow"))
    return;

  const Statistic *s = &ctx->active->vol->vol_stats;
  BOOL needs_window_recreate = FALSE;
  BOOL active_big_mode;
  int previous_left_width = ctx->left ? ctx->left->dir_w : 0;
  int previous_right_width = ctx->right ? ctx->right->dir_w : 0;

  if (ctx->active == NULL)
    MESSAGE(ctx, "FATAL: RefreshView called with NULL ctx->active");

  /* 1. Re-evaluate Layout; only recreate windows on actual resize */
  Layout_Recalculate(ctx);
  if ((ctx->left && previous_left_width != ctx->left->dir_w) ||
      (ctx->right && previous_right_width != ctx->right->dir_w)) {
    needs_window_recreate = TRUE;
  }
  if (ctx->cached_lines != LINES || ctx->cached_cols != COLS) {
    if (!AppStateCommitTerminalGeometryCache(ctx, LINES, COLS))
      return;
    needs_window_recreate = TRUE;
  }

  /* Preview mode requires a dedicated window topology, not just resized
   * geometry values. Recreate if the preview window lifecycle is out of sync.
   */
  if (ctx->preview_mode && ctx->ctx_preview_window == NULL) {
    needs_window_recreate = TRUE;
  } else if (!ctx->preview_mode && ctx->ctx_preview_window != NULL) {
    needs_window_recreate = TRUE;
  }

  if (needs_window_recreate) {
    ReCreateWindows(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
  }

  /* 3. Draw Borders and Dynamic Static Frames into ctx_border_window */
  DisplayMenu(ctx);
  touchwin(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  /* 4. Render Stats (updates ctx_border_window) */
  if (!ctx->preview_mode) {
    if (!ctx->is_split_screen) {
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
    }
  }

  /* 5. Refresh Background/Border Window SECOND (z=0) */

  /* 6. Update Header Path (already drawn to border window) */
  {
    char path[PATH_LENGTH + 1];
    DirEntry *path_dir = dir_entry;
    ViewFocus active_focus = AppStateResolveActivePanelFocus(ctx);

    if (!ctx->preview_mode && dir_entry && active_focus == FOCUS_FILE &&
        ctx->active && ctx->active->file_entry_list && ctx->active->file_count > 0) {
      int idx = dir_entry->start_file + dir_entry->cursor_pos;
      if (idx >= 0 && (unsigned int)idx < ctx->active->file_count) {
        FileEntry *fe = ctx->active->file_entry_list[idx].file;
        if (fe && fe->dir_entry)
          path_dir = fe->dir_entry;
      }
    }

    GetPath(path_dir, path);
    DisplayHeaderPath(ctx, path);
  }

  /* 7. Draw Content Panels THIRD (z=1) */
  active_big_mode = IsActivePanelBigFileMode(ctx, dir_entry);

  if (ctx->preview_mode) {
    /* Preview mode always uses the active panel's big file window as the
     * left list pane. Avoid SwitchToBigFileWindow because its separator
     * surgery assumes standard tree/file geometry and corrupts preview
     * borders.
     */
    AppStateSetPanelFileWindowHandle(ctx, ctx->active, TRUE);
    DisplayFileWindow(ctx, ctx->active, dir_entry);
    if (ctx->ctx_preview_window)
      wnoutrefresh(ctx->ctx_preview_window);
  } else {
    if (ctx->is_split_screen && ctx->left && ctx->right && ctx->active) {
      BOOL left_big_mode;
      BOOL right_big_mode;
      YtreeNovaPanel *inactive;

      inactive = (ctx->active == ctx->left) ? ctx->right : ctx->left;

      left_big_mode = (ctx->active == ctx->left)
                          ? active_big_mode
                          : IsPanelSavedBigFileMode(ctx->left);
      right_big_mode = (ctx->active == ctx->right)
                           ? active_big_mode
                           : IsPanelSavedBigFileMode(ctx->right);

      AppStateSetPanelFileWindowHandle(ctx, ctx->left, left_big_mode);
      AppStateSetPanelFileWindowHandle(ctx, ctx->right, right_big_mode);
      AppStateSetPanelFileWindowHandle(ctx, ctx->active, active_big_mode);

      DrawSplitSeparatorRow(ctx, left_big_mode, right_big_mode);
      /* Draw after the separator, which clears its full row. */
      DisplayPanelStatistics(ctx, ctx->left);
      DisplayPanelStatistics(ctx, ctx->right);
      wnoutrefresh(ctx->ctx_border_window);

      if (!active_big_mode && ctx->active->pan_dir_window) {
        BOOL tree_highlight = (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE);
        DisplayTree(ctx, ctx->active->vol, ctx->active->pan_dir_window,
                    ctx->active->disp_begin_pos,
                    GetPanelVisibleSelectionIndex(ctx->active),
                    tree_highlight);
        wnoutrefresh(ctx->active->pan_dir_window);
      }
      DisplayFileWindow(ctx, ctx->active, dir_entry);
      RenderInactivePanel(ctx, inactive);
    } else {
      if (active_big_mode) {
        SwitchToBigFileWindow(ctx);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
      } else {
        SwitchToSmallFileWindow(ctx);
        if (ctx->active && ctx->active->pan_dir_window) {
          BOOL tree_highlight = (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE);
          DisplayTree(ctx, ctx->active->vol, ctx->active->pan_dir_window,
                      ctx->active->disp_begin_pos,
                      GetPanelVisibleSelectionIndex(ctx->active),
                      tree_highlight);
          wnoutrefresh(ctx->active->pan_dir_window);
        }
        DisplayFileWindow(ctx, ctx->active, dir_entry);
      }
    }
  }

  /* 8. Update Footer Help and Refresh Menu Window LAST (z=2) */
  if (ctx->preview_mode) {
    DisplayPreviewHelp(ctx);
  } else {
    if (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE) {
      DisplayDirHelp(ctx, dir_entry);
    } else {
      DisplayFileHelp(ctx, dir_entry);
    }
    if (ctx->ctx_menu_window)
      wnoutrefresh(ctx->ctx_menu_window);
  }

  UI_Dialog_RefreshAll(ctx);
  ClockHandler(ctx, 0);
  doupdate();
}
