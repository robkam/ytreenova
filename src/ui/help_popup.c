/***************************************************************************
 *
 * src/ui/help_popup.c
 * Shared contextual help popup rendering.
 *
 ***************************************************************************/

#include "ytnova_ui.h"

#define HELP_POPUP_HISTORY_MIN_WIDTH 48
#define HELP_POPUP_HISTORY_VERTICAL_MARGIN 6
#define HELP_POPUP_MIN_HEIGHT 7
#define HELP_POPUP_CENTERED_HORIZONTAL_MARGIN 4

static const UICommandStripCommand help_popup_close_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "close"), "Esc",
     "Q", "help-popup.footer"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "OK"), "Enter",
     NULL, "help-popup.footer"}};

static const UICommandStripCommand help_popup_scroll_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "scroll"), "Up",
     "Down", "help-popup.footer"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "page"), "PgUp",
     "PgDn", "help-popup.footer"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "jump"), "Home",
     "End", "help-popup.footer"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "close"), "Esc",
     "Q", "help-popup.footer"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("help-popup.footer", "OK"), "Enter",
     NULL, "help-popup.footer"}};

static int HelpPopupFooterWidth(const UICommandStripCommand *commands,
                                size_t command_count) {
  return UI_CommandStripVisualLength(commands, command_count);
}

static int HelpPopupContentLineCapacity(int height) {
  int content_lines = height - 6;

  if (content_lines < 1)
    return 1;
  return content_lines;
}

static int HelpPopupHeightForContentLines(int content_lines) {
  if (content_lines < 1)
    content_lines = 1;
  return content_lines + 5;
}

static void HelpPopupResolveFooterCommands(
    BOOL scrollable, const UIHelpPopupFooterSpec *footer_spec,
    const UICommandStripCommand **commands_out, size_t *command_count_out) {
  if (commands_out == NULL || command_count_out == NULL)
    return;

  if (footer_spec != NULL && footer_spec->commands != NULL) {
    *commands_out = footer_spec->commands;
    *command_count_out = footer_spec->command_count;
    return;
  }

  *commands_out = scrollable ? help_popup_scroll_commands
                             : help_popup_close_commands;
  *command_count_out =
      scrollable ? sizeof(help_popup_scroll_commands) /
                       sizeof(help_popup_scroll_commands[0])
                 : sizeof(help_popup_close_commands) /
                       sizeof(help_popup_close_commands[0]);
}

static void FillHelpPopupBlankLine(WINDOW *win, int y, int start_x, int width,
                                   int role) {
  int x;

  if (win == NULL || width <= 0)
    return;

  for (x = 0; x < width; ++x)
    mvwaddch(win, y, start_x + x, ' ' | COLOR_PAIR(role));
}

static void RenderHelpPopupFooter(
    WINDOW *win, int y, int start_x, const UICommandStripCommand *commands,
    size_t command_count, const UIHelpPopupFooterSpec *footer_spec) {
  int footer_width;

  if (win == NULL)
    return;

  (void)footer_spec;
  footer_width = getmaxx(win) - start_x - 2;
  if (footer_width > 0)
    FillHelpPopupBlankLine(win, y, start_x, footer_width,
                           UI_ROLE_HELP_FOOTER);

  if (commands != NULL && command_count > 0) {
    UI_RenderCommandStrip(win, y, start_x, commands, command_count,
                          UI_ROLE_HELP_FOOTER, UI_ROLE_HELP_KEYBIND);
  }
}

static void ClearHelpPopupContentArea(WINDOW *win, int start_y, int start_x,
                                      int line_count, int width) {
  int y;

  if (win == NULL || line_count <= 0)
    return;

  for (y = 0; y < line_count; ++y)
    FillHelpPopupBlankLine(win, start_y + y, start_x, width, UI_ROLE_HELP);
}

static void RenderHelpPopupFrame(WINDOW *win, int width, const char *title) {
  const char *translated_title;

  if (win == NULL || title == NULL)
    return;
  translated_title = _(title);

  werase(win);
#ifdef COLOR_SUPPORT
  wattron(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
#endif
  box(win, 0, 0);
#ifdef COLOR_SUPPORT
  wattroff(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
  wattron(win, COLOR_PAIR(UI_ROLE_HELP_HEADING));
#endif
  mvwprintw(win, 1,
            MAXIMUM(2, (width - StrVisualLength(translated_title)) / 2), "%s",
            translated_title);
#ifdef COLOR_SUPPORT
  wattroff(win, COLOR_PAIR(UI_ROLE_HELP_HEADING));
#endif
}

static size_t RenderHelpTextSlice(WINDOW *win, int y, int column,
                                  int available_width, const char *text,
                                  size_t length, int *rendered_width) {
  char fragment[256];
  size_t byte_count;
  int width;

  if (rendered_width != NULL)
    *rendered_width = 0;
  if (win == NULL || text == NULL || length == 0 || available_width <= 0)
    return 0;

  byte_count = MINIMUM(length, sizeof(fragment) - 1);
  memcpy(fragment, text, byte_count);
  fragment[byte_count] = '\0';
  if (StrVisualLength(fragment) > available_width) {
    int fitted_bytes =
        VisualPositionToBytePosition(fragment, available_width);

    if (fitted_bytes < 0)
      return 0;
    byte_count = (size_t)fitted_bytes;
    fragment[byte_count] = '\0';
  }
  if (byte_count == 0)
    return 0;

  width = StrVisualLength(fragment);
  mvwprintw(win, y, column, "%s", fragment);
  if (rendered_width != NULL)
    *rendered_width = width;
  return byte_count;
}

static void RenderHelpInlineText(WINDOW *win, int y, int column, int max_width,
                                 const UIHelpPopupRow *row,
                                 UISemanticRolePair base_role) {
  const char *text;
  size_t offset = 0;
  size_t span_index;
  size_t text_length;
  int used_width = 0;

  if (win == NULL || row == NULL || row->text == NULL || max_width <= 0)
    return;
  text = row->text;
  text_length = strlen(text);

  for (span_index = 0; span_index < row->span_count &&
                       used_width < max_width;
       ++span_index) {
    const UIHelpPopupSpan *span = &row->spans[span_index];
    UISemanticRolePair role;
    int rendered_width;

    if (span->start < offset || span->start >= text_length ||
        span->length == 0)
      continue;
    if (span->start > offset) {
      wattrset(win, COLOR_PAIR(base_role));
      size_t rendered = RenderHelpTextSlice(
          win, y, column + used_width, max_width - used_width, text + offset,
          span->start - offset, &rendered_width);

      used_width += rendered_width;
      offset += rendered;
      if (offset < span->start || used_width >= max_width)
        break;
    }

    role = base_role;
    if (span->kind == UI_HELP_POPUP_SPAN_TERM)
      role = UI_ROLE_HELP_TOPIC;
    else if (span->kind == UI_HELP_POPUP_SPAN_ATTENTION)
      role = UI_ROLE_HELP_ATTENTION;
    else if (span->kind == UI_HELP_POPUP_SPAN_LINK)
      role = row->selected && span->link_index == row->selected_link_index
                 ? UI_ROLE_HELP_LINK_SELECTION
                 : UI_ROLE_HELP_LINK;
    wattrset(win, COLOR_PAIR(role));
    {
      size_t span_length = MINIMUM(span->length, text_length - span->start);
      size_t rendered = RenderHelpTextSlice(
          win, y, column + used_width, max_width - used_width,
          text + span->start, span_length, &rendered_width);

      used_width += rendered_width;
      offset = span->start + rendered;
      if (rendered < span_length)
        break;
    }
  }

  if (offset < text_length && used_width < max_width) {
    int rendered_width;

    wattrset(win, COLOR_PAIR(base_role));
    (void)RenderHelpTextSlice(win, y, column + used_width,
                              max_width - used_width, text + offset,
                              text_length - offset, &rendered_width);
  }
  wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
}

static int HelpPopupRowStartLine(const UIHelpPopupRow *rows, int row_count,
                                 int row_index) {
  int line = 0;
  int i;

  if (rows == NULL || row_count <= 0 || row_index <= 0)
    return 0;
  if (row_index >= row_count)
    row_index = row_count - 1;

  for (i = 1; i <= row_index; ++i)
    line += rows[i].compact_with_previous ? 1 : 2;

  return line;
}

static int HelpPopupTotalContentLines(const UIHelpPopupRow *rows, int row_count) {
  if (rows == NULL || row_count <= 0)
    return 0;
  return HelpPopupRowStartLine(rows, row_count, row_count - 1) + 1;
}

static void HelpPopupVisibleRowWindow(const UIHelpPopupRow *rows, int row_count,
                                      int line_scroll, int content_lines,
                                      int *visible_start_row,
                                      int *visible_row_count) {
  int start_row = row_count;
  int count = 0;
  int i;

  if (visible_start_row == NULL || visible_row_count == NULL) {
    return;
  }
  *visible_start_row = row_count;
  *visible_row_count = 0;
  if (rows == NULL || row_count <= 0 || content_lines <= 0)
    return;

  for (i = 0; i < row_count; ++i) {
    int row_line = HelpPopupRowStartLine(rows, row_count, i);

    if (row_line < line_scroll)
      continue;
    if (row_line >= line_scroll + content_lines)
      break;
    if (start_row == row_count)
      start_row = i;
    count++;
  }

  *visible_start_row = start_row;
  *visible_row_count = count;
}

static void SyncHelpPopupActiveRowScroll(
    const UIHelpPopupFooterSpec *footer_spec, const UIHelpPopupRow *rows,
    int row_count, int content_lines, int *scroll_line_offset) {
  int active_row;
  int max_line_scroll;

  if (footer_spec == NULL || footer_spec->active_row_handler == NULL ||
      scroll_line_offset == NULL || rows == NULL || content_lines <= 0 ||
      row_count <= 0)
    return;

  active_row = footer_spec->active_row_handler(footer_spec->key_data);
  max_line_scroll = HelpPopupTotalContentLines(rows, row_count) - content_lines;
  if (max_line_scroll < 0)
    max_line_scroll = 0;

  if (active_row >= 0 && active_row < row_count) {
    int active_line = HelpPopupRowStartLine(rows, row_count, active_row);

    if (active_line < *scroll_line_offset) {
      *scroll_line_offset = active_line;
    } else if (active_line >= *scroll_line_offset + content_lines) {
      *scroll_line_offset = active_line - content_lines + 1;
    }
  }

  if (*scroll_line_offset < 0)
    *scroll_line_offset = 0;
  if (*scroll_line_offset > max_line_scroll)
    *scroll_line_offset = max_line_scroll;
}

static void UpdateHelpPopupViewport(const UIHelpPopupFooterSpec *footer_spec,
                                    const UIHelpPopupRow *rows, int row_count,
                                    int scroll_line_offset,
                                    int content_lines) {
  int visible_start_row;
  int visible_row_count;

  if (footer_spec == NULL || footer_spec->viewport_handler == NULL)
    return;

  HelpPopupVisibleRowWindow(rows, row_count, scroll_line_offset, content_lines,
                            &visible_start_row, &visible_row_count);
  footer_spec->viewport_handler(footer_spec->key_data, visible_start_row,
                                visible_row_count, row_count);
}

static int HelpPopupRowWidth(const UIHelpPopupRow *row) {
  int width = 0;

  if (row == NULL)
    return 0;

  if (row->prefix != NULL)
    width += StrVisualLength(row->prefix);

  switch (row->kind) {
  case UI_HELP_POPUP_COMMAND_STRIP:
    width += UI_CommandStripVisualLength(row->commands, row->command_count);
    break;
  case UI_HELP_POPUP_LINK_TEXT:
  case UI_HELP_POPUP_TEXT:
    if (row->prefix != NULL && row->prefix[0] != '\0' &&
        row->text != NULL && row->text[0] != '\0')
      width += 2;
    if (row->text != NULL && row->text[0] != '\0')
      width += StrVisualLength(row->text);
    break;
  default:
    break;
  }

  return width;
}

#define HELP_POPUP_CONTENT_START_Y 3
#define HELP_POPUP_CONTENT_START_X 2

static void RenderHelpPopupRow(WINDOW *win, int y, int start_x,
                               int content_width, const UIHelpPopupRow *row) {
  int x = start_x;
  UISemanticRolePair text_role = UI_ROLE_HELP;

  if (win == NULL || row == NULL)
    return;

  if (row->prefix != NULL && row->prefix[0] != '\0') {
    if (row->kind == UI_HELP_POPUP_LINK_TEXT) {
      wattrset(win, COLOR_PAIR(row->selected ? UI_ROLE_HELP_LINK_SELECTION
                                             : UI_ROLE_HELP_LINK));
      mvwprintw(win, y, x, "%s", row->prefix);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    } else {
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP_TOPIC));
      mvwprintw(win, y, x, "%s", row->prefix);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    }
    x += StrVisualLength(row->prefix);
  }

  switch (row->kind) {
  case UI_HELP_POPUP_COMMAND_STRIP:
    UI_RenderCommandStrip(win, y, x, row->commands, row->command_count,
                          UI_ROLE_HELP, UI_ROLE_HELP_KEYBIND);
    break;
  case UI_HELP_POPUP_LINK_TEXT:
  case UI_HELP_POPUP_TEXT:
    if (row->text != NULL && row->text[0] != '\0' &&
        x < start_x + content_width) {
      if (row->prefix != NULL && row->prefix[0] != '\0') {
        mvwprintw(win, y, x, ": ");
        x += 2;
      }
      if (x < start_x + content_width)
        RenderHelpInlineText(win, y, x, content_width - (x - start_x), row,
                             text_role);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    }
    break;
  default:
    break;
  }
}

typedef struct {
  WINDOW *win;
  int height;
  int content_lines;
  int content_width;
  const UIHelpPopupRow *rows;
  size_t row_count;
  const UIHelpPopupFooterSpec *footer_spec;
  const UICommandStripCommand *effective_footer_commands;
  size_t effective_footer_count;
  int *scroll_line_offset;
} HelpPopupRenderState;

static void RenderHelpPopupPage(const HelpPopupRenderState *state) {
  int active_row = -1;
  int i;

  if (state == NULL || state->win == NULL || state->rows == NULL ||
      state->row_count == 0 || state->scroll_line_offset == NULL)
    return;

  UpdateHelpPopupViewport(state->footer_spec, state->rows, (int)state->row_count,
                          *state->scroll_line_offset, state->content_lines);
  SyncHelpPopupActiveRowScroll(state->footer_spec, state->rows,
                               (int)state->row_count, state->content_lines,
                               state->scroll_line_offset);
  UpdateHelpPopupViewport(state->footer_spec, state->rows, (int)state->row_count,
                          *state->scroll_line_offset, state->content_lines);
  if (state->footer_spec != NULL &&
      state->footer_spec->active_row_handler != NULL) {
    active_row =
        state->footer_spec->active_row_handler(state->footer_spec->key_data);
  }

  ClearHelpPopupContentArea(state->win, HELP_POPUP_CONTENT_START_Y,
                            HELP_POPUP_CONTENT_START_X, state->content_lines + 1,
                            state->content_width);
  for (i = 0; i < (int)state->row_count; ++i) {
    int row_y = HELP_POPUP_CONTENT_START_Y +
                HelpPopupRowStartLine(state->rows, (int)state->row_count, i) -
                *state->scroll_line_offset;

    if (row_y < HELP_POPUP_CONTENT_START_Y)
      continue;
    if (row_y >= HELP_POPUP_CONTENT_START_Y + state->content_lines)
      break;

    {
      UIHelpPopupRow render_row = state->rows[i];

      if (state->footer_spec != NULL &&
          state->footer_spec->active_row_handler != NULL) {
        render_row.selected = (i == active_row);
      }
      RenderHelpPopupRow(state->win, row_y, HELP_POPUP_CONTENT_START_X,
                         state->content_width, &render_row);
    }
  }

  RenderHelpPopupFooter(state->win, state->height - 2, 2,
                        state->effective_footer_commands,
                        state->effective_footer_count, state->footer_spec);
  wredrawln(state->win, HELP_POPUP_CONTENT_START_Y,
            state->content_lines + 1);
  wredrawln(state->win, state->height - 2, 1);
  wrefresh(state->win);
}

static void ScrollHelpPopupLine(int delta, const UIHelpPopupRow *rows,
                                int row_count, int content_lines,
                                int *scroll_line_offset) {
  int max_line_scroll;

  if (scroll_line_offset == NULL || rows == NULL || row_count <= 0 ||
      content_lines <= 0 || delta == 0)
    return;

  max_line_scroll = HelpPopupTotalContentLines(rows, row_count) - content_lines;
  if (max_line_scroll < 0)
    max_line_scroll = 0;
  *scroll_line_offset += delta;
  if (*scroll_line_offset < 0)
    *scroll_line_offset = 0;
  if (*scroll_line_offset > max_line_scroll)
    *scroll_line_offset = max_line_scroll;
}

static void HandleHelpPopupScrollKey(int ch, const UIHelpPopupRow *rows,
                                     int row_count, int content_lines,
                                     int *scroll_line_offset) {
  int page_delta;

  if (scroll_line_offset == NULL)
    return;

  switch (ch) {
  case KEY_UP:
    ScrollHelpPopupLine(-1, rows, row_count, content_lines, scroll_line_offset);
    break;
  case KEY_DOWN:
    ScrollHelpPopupLine(1, rows, row_count, content_lines, scroll_line_offset);
    break;
  case KEY_PPAGE:
  case KEY_NPAGE:
    page_delta = (ch == KEY_PPAGE) ? -content_lines : content_lines;
    ScrollHelpPopupLine(page_delta, rows, row_count, content_lines,
                        scroll_line_offset);
    break;
  case KEY_HOME:
    *scroll_line_offset = 0;
    break;
  case KEY_END:
    ScrollHelpPopupLine(HelpPopupTotalContentLines(rows, row_count), rows,
                        row_count, content_lines, scroll_line_offset);
    break;
  default:
    break;
  }
}

static int ShowHelpPopupInternal(ViewContext *ctx, const char *title,
                                 const UIHelpPopupRow *rows, size_t row_count,
                                 BOOL dismiss_any_key,
                                 const UIHelpPopupFooterSpec *footer_spec) {
  const char *translated_title;
  HelpPopupRenderState render_state;
  WINDOW *win;
  int content_width;
  const UICommandStripCommand *effective_footer_commands;
  size_t effective_footer_count;
  int height;
  int i;
  int max_row_width;
  int scroll_line_offset = 0;
  BOOL scrollable;
  int content_lines;
  int total_content_lines;
  int width;
  int win_x;
  int win_y;
  BOOL use_history_geometry;
  int required_footer_width = 0;

  if (ctx == NULL || title == NULL || rows == NULL || row_count == 0)
    return -1;
  translated_title = _(title);

  max_row_width = 0;
  total_content_lines = HelpPopupTotalContentLines(rows, (int)row_count);
  for (i = 0; i < (int)row_count; ++i) {
    int row_width = HelpPopupRowWidth(&rows[i]);

    if (row_width > max_row_width)
      max_row_width = row_width;
  }

  if (footer_spec != NULL && footer_spec->commands != NULL)
    required_footer_width =
        HelpPopupFooterWidth(footer_spec->commands, footer_spec->command_count) +
        4;
  use_history_geometry =
      ctx->layout.main_win_width >= HELP_POPUP_HISTORY_MIN_WIDTH &&
      ctx->layout.main_win_width >= required_footer_width &&
      (LINES - HELP_POPUP_HISTORY_VERTICAL_MARGIN) >=
          HELP_POPUP_HISTORY_VERTICAL_MARGIN;
  if (use_history_geometry) {
    width = MINIMUM(ctx->layout.main_win_width, COLS - 2);
    height = MINIMUM(LINES - HELP_POPUP_HISTORY_VERTICAL_MARGIN, LINES - 2);
    if ((height % 2) == 0)
      height--;
    if (height < HELP_POPUP_MIN_HEIGHT)
      height = HELP_POPUP_MIN_HEIGHT;
    win_x = 1;
    win_y = 2;
  } else {
    int footer_width;
    int visible_content_lines;
    int max_content_lines;

    width = StrVisualLength(translated_title) + 8;
    if (max_row_width + 4 > width)
      width = max_row_width + 4;
    width = MAXIMUM(width, HELP_POPUP_HISTORY_MIN_WIDTH);
    width = MINIMUM(width, COLS - HELP_POPUP_CENTERED_HORIZONTAL_MARGIN);

    max_content_lines = HelpPopupContentLineCapacity(LINES - 2);
    visible_content_lines = total_content_lines;
    if (visible_content_lines > max_content_lines)
      visible_content_lines = max_content_lines;

    scrollable = (total_content_lines > visible_content_lines);
    HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                   &effective_footer_commands,
                                   &effective_footer_count);
    footer_width =
        HelpPopupFooterWidth(effective_footer_commands, effective_footer_count);
    if (footer_width + 4 > width)
      width = MINIMUM(footer_width + 4,
                      COLS - HELP_POPUP_CENTERED_HORIZONTAL_MARGIN);

    height = HelpPopupHeightForContentLines(visible_content_lines);
    height = MAXIMUM(height, HELP_POPUP_MIN_HEIGHT);
    height = MINIMUM(height, LINES - 2);
    win_x = MAXIMUM(1, (COLS - width) / 2);
    win_y = MAXIMUM(1, (LINES - height) / 2);
  }

  content_lines = HelpPopupContentLineCapacity(height);
  scrollable = (total_content_lines > content_lines);
  HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                 &effective_footer_commands,
                                 &effective_footer_count);
  content_width = width - 4;
  if (footer_spec != NULL && footer_spec->initial_scroll_line >= 0)
    scroll_line_offset = footer_spec->initial_scroll_line;
  else if (footer_spec != NULL && footer_spec->initial_visible_row > 0)
    scroll_line_offset = HelpPopupRowStartLine(
        rows, (int)row_count,
        MINIMUM(footer_spec->initial_visible_row, (int)row_count - 1));
  if (scroll_line_offset > total_content_lines - content_lines)
    scroll_line_offset = MAXIMUM(total_content_lines - content_lines, 0);

  win = newwin(height, width, win_y, win_x);
  if (win == NULL)
    return -1;

  UI_Dialog_Push(win, UI_TIER_MODAL);
  keypad(win, TRUE);
  idlok(win, FALSE);
  idcok(win, FALSE);
  idlok(stdscr, FALSE);
  idcok(stdscr, FALSE);
  WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_HELP));
  curs_set(0);

  wattron(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
  RenderHelpPopupFrame(win, width, translated_title);
  wattroff(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
  ClearHelpPopupContentArea(win, HELP_POPUP_CONTENT_START_Y,
                            HELP_POPUP_CONTENT_START_X, content_lines + 1,
                            content_width);
  RenderHelpPopupFooter(win, height - 2, HELP_POPUP_CONTENT_START_X,
                        effective_footer_commands,
                        effective_footer_count, footer_spec);
  wrefresh(win);

  memset(&render_state, 0, sizeof(render_state));
  render_state.win = win;
  render_state.height = height;
  render_state.content_lines = content_lines;
  render_state.content_width = content_width;
  render_state.rows = rows;
  render_state.row_count = row_count;
  render_state.footer_spec = footer_spec;
  render_state.scroll_line_offset = &scroll_line_offset;

  while (1) {
    int ch;

    HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                   &effective_footer_commands,
                                   &effective_footer_count);
    render_state.effective_footer_commands = effective_footer_commands;
    render_state.effective_footer_count = effective_footer_count;
    RenderHelpPopupPage(&render_state);

    ch = WGetch(ctx, win);
    if (ctx->resize_request) {
      if (footer_spec != NULL && footer_spec->final_scroll_line != NULL)
        *footer_spec->final_scroll_line = scroll_line_offset;
      UI_Dialog_Close(ctx, win);
      return 1;
    }
    if (ch == ERR)
      continue;

    if (footer_spec != NULL && footer_spec->key_handler != NULL) {
      int handled = footer_spec->key_handler(ctx, ch, footer_spec->key_data);

      if (handled > 0)
        break;
      if (handled < 0)
        continue;
    }

    if (ch == KEY_F(1) || ch == ESC || ch == 'q' || ch == 'Q' ||
        ((ch == CR || ch == LF) &&
         (footer_spec == NULL || footer_spec->key_handler == NULL)))
      break;
    if (dismiss_any_key)
      break;

    if (!scrollable)
      continue;
    HandleHelpPopupScrollKey(ch, rows, (int)row_count, content_lines,
                             &scroll_line_offset);
  }

  if (footer_spec != NULL && footer_spec->final_scroll_line != NULL)
    *footer_spec->final_scroll_line = scroll_line_offset;
  UI_Dialog_Close(ctx, win);
  return 0;
}

int UI_ShowHelpPopupWithFooter(ViewContext *ctx, const char *title,
                               const UIHelpPopupRow *rows, size_t row_count,
                               const UIHelpPopupFooterSpec *footer_spec) {
  return ShowHelpPopupInternal(ctx, title, rows, row_count, FALSE,
                               footer_spec);
}

int UI_ShowHelpPopup(ViewContext *ctx, const char *title,
                     const UIHelpPopupRow *rows, size_t row_count) {
  return ShowHelpPopupInternal(ctx, title, rows, row_count, FALSE, NULL);
}

int UI_ShowHelpPopupDismissAnyKey(ViewContext *ctx, const char *title,
                                  const UIHelpPopupRow *rows,
                                  size_t row_count) {
  return ShowHelpPopupInternal(ctx, title, rows, row_count, TRUE, NULL);
}
