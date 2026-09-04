/***************************************************************************
 *
 * src/ui/runtime_help.c
 * Generated runtime help topic lookup and popup wiring.
 *
 ***************************************************************************/

#include "../../include/ytnova_ui.h"
#include "../../include/ytnova_appstate_render.h"
#include "../core/generated_help_topics.h"
#include <ctype.h>
#include <string.h>

#define GENERATED_HELP_MAX_FOOTER_COMMANDS 10
#define GENERATED_HELP_MAX_HISTORY 4
#define GENERATED_HELP_NO_SELECTION ((size_t)-1)
#define GENERATED_HELP_MAX_ROWS 128
#define GENERATED_HELP_MAX_TEXT_LINES 128
#define GENERATED_HELP_MAX_TEXT_WIDTH 256
#define GENERATED_HELP_MAX_ITEMS 64
#define GENERATED_HELP_MAX_ITEM_LABEL 64
#define GENERATED_HELP_MAX_ITEM_DETAIL 1024
#define GENERATED_HELP_MAX_INLINE_LINKS 64
#define GENERATED_HELP_MAX_INLINE_SPANS 512
#define GENERATED_HELP_MAX_LINE_SPANS 32
#define GENERATED_HELP_MAX_LINK_TARGET 64
#define GENERATED_HELP_DEFAULT_WRAP_WIDTH 72
#define GENERATED_HELP_MIN_MAIN_WIDTH 8
#define GENERATED_HELP_WRAP_PADDING 4
#define GENERATED_HELP_INTRO_RESERVED_FOOTER_COMMANDS 1
#define GENERATED_HELP_STANDARD_RESERVED_FOOTER_COMMANDS 3
#define GENERATED_HELP_RELATED_LINK_MIN_ROWS 3

typedef struct {
  char label[GENERATED_HELP_MAX_ITEM_LABEL];
  char summary[GENERATED_HELP_MAX_TEXT_WIDTH];
  char detail[GENERATED_HELP_MAX_ITEM_DETAIL];
  const char *linked_topic_id;
  BOOL selectable;
} RuntimeHelpItem;

typedef struct {
  const GeneratedHelpTopic *topic;
  size_t selected_item_index;
  size_t current_detail_index;
  size_t active_inline_link_index;
  int scroll_line_offset;
  BOOL contextual_origin;
} RuntimeHelpView;

typedef struct {
  size_t row_index;
  char target_topic_id[GENERATED_HELP_MAX_LINK_TARGET];
} RuntimeHelpInlineLink;

typedef struct {
  char text[GENERATED_HELP_MAX_TEXT_WIDTH];
  UIHelpPopupSpan spans[GENERATED_HELP_MAX_LINE_SPANS];
  size_t span_count;
} ParsedHelpLine;

typedef struct {
  const GeneratedHelpTopic *topic;
  const char *next_topic_id;
  const UIHelpLabelOverride *label_overrides;
  size_t link_command_count;
  size_t active_link_index;
  size_t label_override_count;
  size_t selected_item_index;
  size_t current_detail_index;
  size_t next_detail_index;
  BOOL back_requested;
  BOOL detail_back_requested;
  BOOL has_history;
  BOOL contextual_list_mode;
  BOOL contextual_origin;
  RuntimeHelpItem items[GENERATED_HELP_MAX_ITEMS];
  UICommandStripCommand footer_commands[GENERATED_HELP_MAX_FOOTER_COMMANDS];
  char footer_keys[GENERATED_HELP_MAX_FOOTER_COMMANDS][2];
  UIHelpPopupRow rows[GENERATED_HELP_MAX_ROWS];
  UIHelpPopupSpan inline_spans[GENERATED_HELP_MAX_INLINE_SPANS];
  RuntimeHelpInlineLink inline_links[GENERATED_HELP_MAX_INLINE_LINKS];
  char text_lines[GENERATED_HELP_MAX_TEXT_LINES][GENERATED_HELP_MAX_TEXT_WIDTH];
  size_t item_first_row[GENERATED_HELP_MAX_ITEMS];
  size_t row_item_index[GENERATED_HELP_MAX_ROWS];
  size_t footer_command_count;
  size_t prefix_row_count;
  size_t row_count;
  size_t item_count;
  size_t inline_span_count;
  size_t inline_link_count;
  size_t active_inline_link_index;
  size_t related_link_start_index;
  size_t related_link_first_row;
  size_t related_link_count;
  size_t active_related_link_index;
  size_t reselection_anchor_index;
  size_t previous_visible_start;
  size_t previous_visible_end;
  BOOL viewport_valid;
  int visible_row_count;
  int visible_row_offset;
  int reselection_direction;
  int wrap_width;
} RuntimeHelpPopupState;

static const GeneratedHelpCatalog *ActiveGeneratedHelpCatalog(void) {
  const char *language = I18n_GetLanguage();

  if (generated_help_catalog_count == 0)
    return NULL;
  if (language != NULL && *language != '\0') {
    size_t i;

    for (i = 0; i < generated_help_catalog_count; ++i) {
      if (generated_help_catalogs[i].locale_id != NULL &&
          strcmp(generated_help_catalogs[i].locale_id, language) == 0) {
        return &generated_help_catalogs[i];
      }
    }
  }
  return &generated_help_catalogs[0];
}

static const GeneratedHelpFooter *ActiveGeneratedHelpFooter(void) {
  const GeneratedHelpCatalog *catalog = ActiveGeneratedHelpCatalog();

  return catalog != NULL ? &catalog->footer : NULL;
}

static BOOL FooterKeyMatches(int ch, const char *footer_key) {
  int key;

  if (footer_key == NULL || footer_key[0] == '\0' || footer_key[1] != '\0')
    return FALSE;
  key = islower((unsigned char)ch) ? toupper((unsigned char)ch) : ch;
  return key == toupper((unsigned char)footer_key[0]);
}

static const GeneratedHelpTopic *ActiveGeneratedHelpTopics(size_t *topic_count) {
  const GeneratedHelpCatalog *catalog = ActiveGeneratedHelpCatalog();

  if (topic_count != NULL)
    *topic_count = catalog != NULL ? catalog->topic_count : 0;
  return catalog != NULL ? catalog->topics : NULL;
}

static size_t FindNextSelectableItem(const RuntimeHelpPopupState *state,
                                     size_t start_index,
                                     size_t end_index_exclusive) {
  size_t i;

  if (state == NULL || start_index >= end_index_exclusive)
    return GENERATED_HELP_NO_SELECTION;

  if (end_index_exclusive > state->item_count)
    end_index_exclusive = state->item_count;
  for (i = start_index; i < end_index_exclusive; ++i) {
    if (state->items[i].selectable)
      return i;
  }

  return GENERATED_HELP_NO_SELECTION;
}

static size_t FindPreviousSelectableItem(const RuntimeHelpPopupState *state,
                                         size_t start_index,
                                         size_t start_limit_inclusive) {
  size_t i;

  if (state == NULL || state->item_count == 0 || start_limit_inclusive >= state->item_count ||
      start_index >= state->item_count || start_index < start_limit_inclusive)
    return GENERATED_HELP_NO_SELECTION;

  i = start_index;
  while (1) {
    if (state->items[i].selectable)
      return i;
    if (i == start_limit_inclusive)
      break;
    i--;
  }

  return GENERATED_HELP_NO_SELECTION;
}

static BOOL GetVisibleContextItemRange(const RuntimeHelpPopupState *state,
                                       size_t *visible_start_out,
                                       size_t *visible_end_out) {
  size_t visible_start_row;
  size_t visible_end_row;
  size_t row;
  size_t visible_start_item = GENERATED_HELP_NO_SELECTION;
  size_t visible_end_item = GENERATED_HELP_NO_SELECTION;

  if (state == NULL || visible_start_out == NULL || visible_end_out == NULL ||
      state->visible_row_count <= 0 || state->item_count == 0)
    return FALSE;

  visible_start_row = (size_t)MAXIMUM(state->visible_row_offset, 0);
  visible_end_row = visible_start_row + (size_t)state->visible_row_count;
  if (visible_end_row > state->row_count)
    visible_end_row = state->row_count;
  if (visible_start_row >= visible_end_row)
    return FALSE;

  for (row = visible_start_row; row < visible_end_row; ++row) {
    size_t item_index = state->row_item_index[row];

    if (item_index == GENERATED_HELP_NO_SELECTION || item_index >= state->item_count)
      continue;
    if (visible_start_item == GENERATED_HELP_NO_SELECTION)
      visible_start_item = item_index;
    visible_end_item = item_index + 1;
  }

  if (visible_start_item == GENERATED_HELP_NO_SELECTION ||
      visible_end_item == GENERATED_HELP_NO_SELECTION ||
      visible_start_item >= visible_end_item)
    return FALSE;

  *visible_start_out = visible_start_item;
  *visible_end_out = visible_end_item;
  return TRUE;
}

static void UpdateGeneratedHelpViewport(void *user_data, int scroll_offset,
                                        int visible_rows, int row_count) {
  RuntimeHelpPopupState *state = (RuntimeHelpPopupState *)user_data;
  size_t anchor_index;
  size_t visible_start;
  size_t visible_end;
  size_t previous_visible_start;
  size_t previous_visible_end;
  size_t next_index;

  (void)row_count;
  if (state == NULL)
    return;

  state->visible_row_offset = scroll_offset;
  state->visible_row_count = visible_rows;
  if (state->reselection_direction == 0 || state->visible_row_count <= 0 ||
      state->item_count == 0)
    return;

  if (!GetVisibleContextItemRange(state, &visible_start, &visible_end)) {
    state->viewport_valid = FALSE;
    return;
  }

  previous_visible_start = state->previous_visible_start;
  previous_visible_end = state->previous_visible_end;

  anchor_index = state->reselection_anchor_index;
  if (anchor_index == GENERATED_HELP_NO_SELECTION &&
      state->selected_item_index != GENERATED_HELP_NO_SELECTION &&
      state->selected_item_index < state->item_count) {
    anchor_index = state->selected_item_index;
  }

  next_index = GENERATED_HELP_NO_SELECTION;
  if (state->reselection_direction < 0) {
    size_t search_limit = visible_end;

    if (state->viewport_valid && visible_start >= previous_visible_start)
      search_limit = visible_start;
    else if (state->viewport_valid && previous_visible_start < search_limit)
      search_limit = previous_visible_start;
    if (anchor_index != GENERATED_HELP_NO_SELECTION && anchor_index < search_limit)
      search_limit = anchor_index;
    if (search_limit > visible_start)
      next_index =
          FindPreviousSelectableItem(state, search_limit - 1, visible_start);
  } else {
    size_t search_start = visible_start;

    if (state->viewport_valid && previous_visible_end > search_start)
      search_start = previous_visible_end;
    if (anchor_index != GENERATED_HELP_NO_SELECTION &&
        anchor_index + 1 > search_start)
      search_start = anchor_index + 1;
    if (search_start < visible_end)
      next_index = FindNextSelectableItem(state, search_start, visible_end);
  }

  state->previous_visible_start = visible_start;
  state->previous_visible_end = visible_end;
  state->viewport_valid = TRUE;

  if (next_index == GENERATED_HELP_NO_SELECTION)
    return;

  state->selected_item_index = next_index;
  state->reselection_direction = 0;
  state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
}

static const GeneratedHelpTopic *FindGeneratedTopicById(const char *topic_id) {
  const GeneratedHelpTopic *topics;
  size_t topic_count;
  size_t i;

  if (topic_id == NULL || topic_id[0] == '\0')
    return NULL;

  topics = ActiveGeneratedHelpTopics(&topic_count);
  for (i = 0; i < topic_count; ++i) {
    if (topics[i].topic_id != NULL && strcmp(topics[i].topic_id, topic_id) == 0) {
      return &topics[i];
    }
  }

  return NULL;
}

static BOOL ContextListContains(const char *contexts_csv,
                                const char *context_id) {
  const char *cursor;
  size_t context_len;

  if (contexts_csv == NULL || context_id == NULL || context_id[0] == '\0')
    return FALSE;

  context_len = strlen(context_id);
  cursor = contexts_csv;
  while (*cursor != '\0') {
    const char *comma = strchr(cursor, ',');
    size_t len = comma != NULL ? (size_t)(comma - cursor) : strlen(cursor);

    if (len == context_len && strncmp(cursor, context_id, len) == 0)
      return TRUE;
    if (comma == NULL)
      break;
    cursor = comma + 1;
  }

  return FALSE;
}

static const GeneratedHelpTopic *FindGeneratedTopicByContext(
    const char *context_id) {
  const GeneratedHelpTopic *topics;
  size_t topic_count;
  size_t i;

  if (context_id == NULL || context_id[0] == '\0')
    return NULL;

  topics = ActiveGeneratedHelpTopics(&topic_count);
  for (i = 0; i < topic_count; ++i) {
    if (ContextListContains(topics[i].contexts_csv, context_id))
      return &topics[i];
  }

  return NULL;
}

static BOOL TopicIdEquals(const GeneratedHelpTopic *topic, const char *topic_id) {
  return topic != NULL && topic->topic_id != NULL && topic_id != NULL &&
         strcmp(topic->topic_id, topic_id) == 0;
}

static void AppendParsedHelpSpan(ParsedHelpLine *line,
                                 UIHelpPopupSpanKind kind, size_t start,
                                 size_t length, size_t link_index) {
  UIHelpPopupSpan *span;

  if (line == NULL || length == 0 ||
      line->span_count >= GENERATED_HELP_MAX_LINE_SPANS)
    return;
  span = &line->spans[line->span_count++];
  span->start = start;
  span->length = length;
  span->link_index = link_index;
  span->kind = kind;
}

static void SortParsedHelpSpans(ParsedHelpLine *line) {
  size_t i;

  if (line == NULL)
    return;
  for (i = 1; i < line->span_count; ++i) {
    UIHelpPopupSpan span = line->spans[i];
    size_t position = i;

    while (position > 0 && line->spans[position - 1].start > span.start) {
      line->spans[position] = line->spans[position - 1];
      position--;
    }
    line->spans[position] = span;
  }
}

static void ParseHelpMarkdown(RuntimeHelpPopupState *state, const char *source,
                              ParsedHelpLine *line) {
  BOOL in_attention = FALSE;
  BOOL in_code = FALSE;
  size_t attention_start = 0;
  size_t code_start = 0;
  size_t out = 0;

  if (line == NULL)
    return;
  memset(line, 0, sizeof(*line));
  if (state == NULL || source == NULL)
    return;

  while (*source != '\0' && out + 1 < sizeof(line->text)) {
    if (!in_code && *source == '[') {
      const char *label_end = strstr(source + 1, "](topic:");
      const char *target_start =
          label_end != NULL ? label_end + strlen("](topic:") : NULL;
      const char *target_end =
          target_start != NULL ? strchr(target_start, ')') : NULL;

      if (label_end != NULL && label_end > source + 1 && target_end != NULL &&
          target_end > target_start) {
        size_t label_start = out;
        size_t target_length = (size_t)(target_end - target_start);

        source++;
        while (source < label_end && out + 1 < sizeof(line->text)) {
          if (*source == '\\' && source + 1 < label_end)
            source++;
          line->text[out++] = *source++;
        }
        if (out > label_start &&
            state->inline_link_count < GENERATED_HELP_MAX_INLINE_LINKS &&
            target_length < sizeof(state->inline_links[0].target_topic_id)) {
          size_t link_index = state->inline_link_count++;
          RuntimeHelpInlineLink *link = &state->inline_links[link_index];

          link->row_index = GENERATED_HELP_NO_SELECTION;
          memcpy(link->target_topic_id, target_start, target_length);
          link->target_topic_id[target_length] = '\0';
          AppendParsedHelpSpan(line, UI_HELP_POPUP_SPAN_LINK, label_start,
                               out - label_start, link_index);
        }
        source = target_end + 1;
        continue;
      }
    }
    if (*source == '\\' && source[1] != '\0') {
      source++;
      line->text[out++] = *source++;
      continue;
    }
    if (*source == '`') {
      if (in_code)
        AppendParsedHelpSpan(line, UI_HELP_POPUP_SPAN_TERM, code_start,
                             out - code_start, GENERATED_HELP_NO_SELECTION);
      else
        code_start = out;
      in_code = !in_code;
      source++;
      continue;
    }
    if (!in_code && source[0] == '*' && source[1] == '*') {
      if (in_attention)
        AppendParsedHelpSpan(line, UI_HELP_POPUP_SPAN_ATTENTION,
                             attention_start, out - attention_start,
                             GENERATED_HELP_NO_SELECTION);
      else
        attention_start = out;
      in_attention = !in_attention;
      source += 2;
      continue;
    }
    if (!in_code && *source == '*') {
      source++;
      continue;
    }
    line->text[out++] = *source++;
  }

  while (out > 0 && isspace((unsigned char)line->text[out - 1]))
    out--;
  line->text[out] = '\0';
  if (in_code && code_start < out)
    AppendParsedHelpSpan(line, UI_HELP_POPUP_SPAN_TERM, code_start,
                         out - code_start, GENERATED_HELP_NO_SELECTION);
  if (in_attention && attention_start < out)
    AppendParsedHelpSpan(line, UI_HELP_POPUP_SPAN_ATTENTION, attention_start,
                         out - attention_start, GENERATED_HELP_NO_SELECTION);
  SortParsedHelpSpans(line);
}

static void AppendHelpTextWithSpacing(RuntimeHelpPopupState *state,
                                      size_t *row_count, size_t *line_index,
                                      const char *text,
                                      BOOL compact_with_previous) {
  size_t len;

  if (state == NULL || row_count == NULL || line_index == NULL || text == NULL ||
      *row_count >= GENERATED_HELP_MAX_ROWS ||
      *line_index >= GENERATED_HELP_MAX_TEXT_LINES)
    return;

  len = strlen(text);
  if (len >= GENERATED_HELP_MAX_TEXT_WIDTH)
    len = GENERATED_HELP_MAX_TEXT_WIDTH - 1;

  memcpy(state->text_lines[*line_index], text, len);
  state->text_lines[*line_index][len] = '\0';
  state->rows[*row_count].kind = UI_HELP_POPUP_TEXT;
  state->rows[*row_count].prefix = NULL;
  state->rows[*row_count].text = state->text_lines[*line_index];
  state->rows[*row_count].commands = NULL;
  state->rows[*row_count].spans = NULL;
  state->rows[*row_count].command_count = 0;
  state->rows[*row_count].span_count = 0;
  state->rows[*row_count].selected_link_index = GENERATED_HELP_NO_SELECTION;
  state->rows[*row_count].selected = FALSE;
  state->rows[*row_count].compact_with_previous = compact_with_previous;
  (*row_count)++;
  (*line_index)++;
}

static void AppendHelpText(RuntimeHelpPopupState *state, size_t *row_count,
                           size_t *line_index, const char *text) {
  AppendHelpTextWithSpacing(state, row_count, line_index, text, FALSE);
}

static size_t NextWrappedHelpChunk(const char **cursor_ptr, int wrap_width,
                                   char *wrapped, size_t wrapped_size) {
  const char *cursor;
  const char *segment_start;
  size_t len;
  size_t split;

  if (cursor_ptr == NULL || wrapped == NULL || wrapped_size == 0)
    return 0;

  cursor = *cursor_ptr;
  if (cursor == NULL)
    return 0;

  while (*cursor != '\0' && isspace((unsigned char)*cursor))
    cursor++;
  if (*cursor == '\0') {
    *cursor_ptr = cursor;
    return 0;
  }

  segment_start = cursor;
  len = strlen(segment_start);
  if (StrVisualLength(segment_start) <= wrap_width) {
    split = len;
  } else {
    int fitted_bytes = VisualPositionToBytePosition(segment_start, wrap_width);

    split = fitted_bytes > 0 ? (size_t)fitted_bytes : 1;
    while (split > 0 && !isspace((unsigned char)segment_start[split]))
      split--;
    if (split == 0) {
      fitted_bytes = VisualPositionToBytePosition(segment_start, wrap_width);
      split = fitted_bytes > 0 ? (size_t)fitted_bytes : 1;
    }
  }

  while (split > 0 && isspace((unsigned char)segment_start[split - 1]))
    split--;
  if (split >= wrapped_size)
    split = wrapped_size - 1;

  memcpy(wrapped, segment_start, split);
  wrapped[split] = '\0';
  *cursor_ptr = segment_start + split;
  return split;
}

static size_t NextWrappedParsedChunk(const ParsedHelpLine *line,
                                     size_t *cursor_offset, int wrap_width,
                                     char *wrapped, size_t wrapped_size,
                                     size_t *chunk_start) {
  const char *segment;
  size_t line_length;
  size_t start;
  size_t split;
  size_t span_index;

  if (line == NULL || cursor_offset == NULL || wrapped == NULL ||
      wrapped_size == 0 || chunk_start == NULL)
    return 0;
  line_length = strlen(line->text);
  start = *cursor_offset;
  while (start < line_length && isspace((unsigned char)line->text[start]))
    start++;
  if (start >= line_length) {
    *cursor_offset = start;
    return 0;
  }

  segment = line->text + start;
  if (StrVisualLength(segment) <= wrap_width) {
    split = line_length - start;
  } else {
    int fitted_bytes = VisualPositionToBytePosition(segment, wrap_width);
    size_t fitted = fitted_bytes > 0 ? (size_t)fitted_bytes : 1;

    split = fitted;
    while (split > 0 && !isspace((unsigned char)segment[split]))
      split--;
    if (split == 0)
      split = fitted;
  }

  for (span_index = 0; span_index < line->span_count; ++span_index) {
    const UIHelpPopupSpan *span = &line->spans[span_index];
    size_t split_offset = start + split;
    size_t span_end = span->start + span->length;

    if (span->kind != UI_HELP_POPUP_SPAN_LINK || span->start >= split_offset ||
        span_end <= split_offset)
      continue;
    if (span->start > start) {
      split = span->start - start;
    } else {
      char label[GENERATED_HELP_MAX_TEXT_WIDTH];
      size_t label_length = MINIMUM(span->length, sizeof(label) - 1);

      memcpy(label, line->text + span->start, label_length);
      label[label_length] = '\0';
      if (StrVisualLength(label) <= wrap_width)
        split = span_end - start;
    }
    break;
  }

  while (split > 0 && isspace((unsigned char)segment[split - 1]))
    split--;
  if (split == 0)
    return 0;
  if (split >= wrapped_size)
    split = wrapped_size - 1;
  memcpy(wrapped, segment, split);
  wrapped[split] = '\0';
  *chunk_start = start;
  *cursor_offset = start + split;
  return split;
}

static void AppendWrappedHelpText(RuntimeHelpPopupState *state, size_t *row_count,
                                  size_t *line_index, const char *text) {
  const char *cursor = text;
  int wrap_width;
  BOOL compact_with_previous = FALSE;

  if (state == NULL || row_count == NULL || line_index == NULL || text == NULL)
    return;

  wrap_width = state->wrap_width > 0 ? state->wrap_width
                                     : GENERATED_HELP_DEFAULT_WRAP_WIDTH;
  while (*cursor != '\0' && *row_count < GENERATED_HELP_MAX_ROWS &&
         *line_index < GENERATED_HELP_MAX_TEXT_LINES) {
    char wrapped[GENERATED_HELP_MAX_TEXT_WIDTH];
    if (NextWrappedHelpChunk(&cursor, wrap_width, wrapped, sizeof(wrapped)) == 0)
      break;
    AppendHelpTextWithSpacing(state, row_count, line_index, wrapped,
                              compact_with_previous);
    compact_with_previous = TRUE;
  }
}

static void AppendWrappedParsedHelpLine(RuntimeHelpPopupState *state,
                                        size_t *row_count, size_t *line_index,
                                        const ParsedHelpLine *line) {
  size_t cursor_offset = 0;
  int wrap_width;
  BOOL compact_with_previous = FALSE;

  if (state == NULL || row_count == NULL || line_index == NULL || line == NULL)
    return;
  wrap_width = state->wrap_width > 0 ? state->wrap_width
                                     : GENERATED_HELP_DEFAULT_WRAP_WIDTH;
  while (line->text[cursor_offset] != '\0' &&
         *row_count < GENERATED_HELP_MAX_ROWS &&
         *line_index < GENERATED_HELP_MAX_TEXT_LINES) {
    char wrapped[GENERATED_HELP_MAX_TEXT_WIDTH];
    size_t chunk_start;
    size_t chunk_length = NextWrappedParsedChunk(
        line, &cursor_offset, wrap_width, wrapped, sizeof(wrapped),
        &chunk_start);
    size_t row_index = *row_count;
    size_t pool_start = state->inline_span_count;
    size_t span_index;

    if (chunk_length == 0)
      break;
    AppendHelpTextWithSpacing(state, row_count, line_index, wrapped,
                              compact_with_previous);
    for (span_index = 0; span_index < line->span_count &&
                         state->inline_span_count <
                             GENERATED_HELP_MAX_INLINE_SPANS;
         ++span_index) {
      const UIHelpPopupSpan *source_span = &line->spans[span_index];
      size_t chunk_end = chunk_start + chunk_length;
      size_t span_end = source_span->start + source_span->length;
      size_t visible_start;
      size_t visible_end;
      UIHelpPopupSpan *row_span;

      if (source_span->start >= chunk_end || span_end <= chunk_start)
        continue;
      visible_start = MAXIMUM(source_span->start, chunk_start);
      visible_end = MINIMUM(span_end, chunk_end);
      row_span = &state->inline_spans[state->inline_span_count++];
      *row_span = *source_span;
      row_span->start = visible_start - chunk_start;
      row_span->length = visible_end - visible_start;
      if (row_span->kind == UI_HELP_POPUP_SPAN_LINK &&
          row_span->link_index < state->inline_link_count &&
          state->inline_links[row_span->link_index].row_index ==
              GENERATED_HELP_NO_SELECTION) {
        state->inline_links[row_span->link_index].row_index = row_index;
      }
    }
    if (state->inline_span_count > pool_start) {
      state->rows[row_index].spans = &state->inline_spans[pool_start];
      state->rows[row_index].span_count = state->inline_span_count - pool_start;
    }
    compact_with_previous = TRUE;
  }
}

static void AppendWrappedContextListRows(RuntimeHelpPopupState *state,
                                         size_t *row_count,
                                         size_t *line_index,
                                         size_t item_index,
                                         size_t selected_item_index) {
  const RuntimeHelpItem *item;
  const char *cursor;
  BOOL first_row = TRUE;
  int continuation_width;

  if (state == NULL || row_count == NULL || line_index == NULL ||
      item_index >= state->item_count)
    return;

  item = &state->items[item_index];
  cursor = item->summary;
  continuation_width = state->wrap_width > 0 ? state->wrap_width
                                             : GENERATED_HELP_DEFAULT_WRAP_WIDTH;
  if (continuation_width < 1)
    continuation_width = 1;

  while (*cursor != '\0' && *row_count < GENERATED_HELP_MAX_ROWS &&
         *line_index < GENERATED_HELP_MAX_TEXT_LINES) {
    char wrapped[GENERATED_HELP_MAX_TEXT_WIDTH];
    int line_width = continuation_width;

    if (first_row && item->label[0] != '\0') {
      line_width -= StrVisualLength(item->label) + 2;
      if (line_width < 1)
        line_width = 1;
    }

    if (NextWrappedHelpChunk(&cursor, line_width, wrapped, sizeof(wrapped)) == 0)
      break;

    if (state->item_first_row[item_index] == GENERATED_HELP_NO_SELECTION)
      state->item_first_row[item_index] = *row_count;
    state->row_item_index[*row_count] = item_index;
    state->rows[*row_count].kind =
        (first_row && item->selectable) ? UI_HELP_POPUP_LINK_TEXT
                                        : UI_HELP_POPUP_TEXT;
    state->rows[*row_count].prefix = first_row ? item->label : NULL;
    memcpy(state->text_lines[*line_index], wrapped, strlen(wrapped) + 1);
    state->rows[*row_count].text = state->text_lines[*line_index];
    state->rows[*row_count].commands = NULL;
    state->rows[*row_count].spans = NULL;
    state->rows[*row_count].command_count = 0;
    state->rows[*row_count].span_count = 0;
    state->rows[*row_count].selected_link_index = GENERATED_HELP_NO_SELECTION;
    state->rows[*row_count].selected =
        (first_row && item_index == selected_item_index);
    state->rows[*row_count].compact_with_previous = !first_row;
    (*row_count)++;
    (*line_index)++;
    first_row = FALSE;
  }
}

static BOOL TopicUsesContextualItemList(const GeneratedHelpTopic *topic,
                                        BOOL contextual_origin) {
  (void)topic;
  (void)contextual_origin;
  return FALSE;
}

static void AppendFooterCommand(RuntimeHelpPopupState *state,
                                size_t *command_count,
                                UICommandStripLayout layout, const char *label,
                                const char *primary_key) {
  UICommandStripCommand *command;

  if (state == NULL || command_count == NULL ||
      *command_count >= GENERATED_HELP_MAX_FOOTER_COMMANDS)
    return;
  command = &state->footer_commands[(*command_count)++];
  command->layout = layout;
  command->label = label;
  command->primary_key = primary_key;
  command->secondary_key = NULL;
  command->translation_context = NULL;
}

static size_t BuildFooterCommands(RuntimeHelpPopupState *state) {
  const GeneratedHelpFooter *footer = ActiveGeneratedHelpFooter();
  size_t command_count = 0;
  BOOL show_index;
  BOOL show_navigation;

  if (state == NULL || state->topic == NULL || footer == NULL)
    return 0;

  state->related_link_first_row = GENERATED_HELP_NO_SELECTION;
  state->related_link_count = 0;
  state->active_related_link_index = GENERATED_HELP_NO_SELECTION;
  show_index = !TopicIdEquals(state->topic, "index");
  show_navigation = !TopicIdEquals(state->topic, "f1-navigation");

  state->link_command_count = 0;
  state->active_link_index = GENERATED_HELP_NO_SELECTION;
  AppendFooterCommand(state, &command_count, UI_COMMAND_LAYOUT_MNEMONIC,
                      footer->left_back_label, NULL);
  AppendFooterCommand(state, &command_count, UI_COMMAND_LAYOUT_MNEMONIC,
                      footer->follow_label, NULL);
  if (show_index)
    AppendFooterCommand(state, &command_count, UI_COMMAND_LAYOUT_KEY_PREFIX,
                        footer->index_label, footer->index_key);
  if (show_navigation)
    AppendFooterCommand(state, &command_count, UI_COMMAND_LAYOUT_KEY_PREFIX,
                        footer->navigation_label, footer->navigation_key);
  AppendFooterCommand(state, &command_count, UI_COMMAND_LAYOUT_MNEMONIC,
                      footer->quit_label, NULL);
  return command_count;
}

static size_t BuildContextListRows(RuntimeHelpPopupState *state,
                                   size_t selected_item_index,
                                   const UIHelpPopupRow *prefix_rows,
                                   size_t prefix_row_count) {
  size_t row_count = 0;
  size_t line_index = 0;
  size_t index;

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
    state->rows[row_count].selected = FALSE;
    state->rows[row_count].selected_link_index = GENERATED_HELP_NO_SELECTION;
    state->row_item_index[row_count] = GENERATED_HELP_NO_SELECTION;
  }

  for (index = 0;
       index < state->item_count && row_count < GENERATED_HELP_MAX_ROWS;
       ++index) {
    if (index == state->related_link_start_index &&
        row_count + 2 <= GENERATED_HELP_MAX_ROWS) {
      state->rows[row_count].kind = UI_HELP_POPUP_TEXT;
      state->rows[row_count].prefix = NP_("runtime-help", "Related help");
      state->rows[row_count].text = NULL;
      state->rows[row_count].commands = NULL;
      state->rows[row_count].spans = NULL;
      state->rows[row_count].command_count = 0;
      state->rows[row_count].span_count = 0;
      state->rows[row_count].selected_link_index = GENERATED_HELP_NO_SELECTION;
      state->rows[row_count].selected = FALSE;
      state->rows[row_count].compact_with_previous = FALSE;
      state->row_item_index[row_count] = GENERATED_HELP_NO_SELECTION;
      row_count++;
    }
    AppendWrappedContextListRows(state, &row_count, &line_index, index,
                                 selected_item_index);
  }

  return row_count;
}

static size_t BuildDetailRows(RuntimeHelpPopupState *state,
                              size_t current_detail_index,
                              const UIHelpPopupRow *prefix_rows,
                              size_t prefix_row_count) {
  size_t row_count = 0;
  size_t line_index = 0;

  if (state == NULL || current_detail_index >= state->item_count)
    return 0;

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
    state->rows[row_count].selected = FALSE;
    state->rows[row_count].selected_link_index = GENERATED_HELP_NO_SELECTION;
  }

  AppendWrappedHelpText(state, &row_count, &line_index,
                        state->items[current_detail_index].detail);
  return row_count;
}

static BOOL HelpLineIsUnavailableCommand(
    const RuntimeHelpPopupState *state, const char *line) {
  size_t index;
  const char *label_start;
  const char *label_end;

  if (state == NULL || line == NULL || line[0] != '`')
    return FALSE;
  label_start = line + 1;
  label_end = strchr(label_start, '`');
  if (label_end == NULL || label_end[1] != ':')
    return FALSE;

  for (index = 0; index < state->label_override_count; ++index) {
    const UIHelpLabelOverride *override = &state->label_overrides[index];
    size_t label_len;

    if (override->canonical_label == NULL)
      continue;
    label_len = strlen(override->canonical_label);
    if ((size_t)(label_end - label_start) == label_len &&
        strncmp(label_start, override->canonical_label, label_len) == 0)
      return override->display_label == NULL;
  }
  return FALSE;
}

static size_t BuildTextRows(RuntimeHelpPopupState *state,
                            const UIHelpPopupRow *prefix_rows,
                            size_t prefix_row_count) {
  size_t row_count = 0;
  size_t line_index = 0;
  const char *cursor;

  if (state == NULL || state->topic == NULL)
    return 0;

  if (state->contextual_list_mode) {
    if (state->current_detail_index != GENERATED_HELP_NO_SELECTION)
      return BuildDetailRows(state, state->current_detail_index, prefix_rows,
                             prefix_row_count);
    return BuildContextListRows(state, state->selected_item_index, prefix_rows,
                                prefix_row_count);
  }

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
    state->rows[row_count].selected = FALSE;
    state->rows[row_count].selected_link_index = GENERATED_HELP_NO_SELECTION;
  }

  cursor = state->topic->contextual_f1;
  while (cursor != NULL && *cursor != '\0' &&
         row_count < GENERATED_HELP_MAX_ROWS &&
         line_index < GENERATED_HELP_MAX_TEXT_LINES) {
    const char *line_break = strchr(cursor, '\n');
    size_t len =
        line_break != NULL ? (size_t)(line_break - cursor) : strlen(cursor);

    if (len > 0) {
      char line[GENERATED_HELP_MAX_TEXT_WIDTH];
      ParsedHelpLine parsed_line;

      if (len >= sizeof(line))
        len = sizeof(line) - 1;
      memcpy(line, cursor, len);
      line[len] = '\0';
      if (!HelpLineIsUnavailableCommand(state, line)) {
        ParseHelpMarkdown(state, line, &parsed_line);
        AppendWrappedParsedHelpLine(state, &row_count, &line_index,
                                    &parsed_line);
      }
    } else {
      AppendHelpText(state, &row_count, &line_index, "");
    }

    if (line_break == NULL)
      break;
    cursor = line_break + 1;
  }

  return row_count;
}

static void ResetContextualHelpSelection(RuntimeHelpPopupState *state) {
  if (state == NULL)
    return;

  state->selected_item_index = GENERATED_HELP_NO_SELECTION;
  state->reselection_direction = 0;
  state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
}

static int HandleContextualListFooterKey(RuntimeHelpPopupState *state, int ch) {
  size_t visible_start;
  size_t visible_end;
  const GeneratedHelpFooter *footer = ActiveGeneratedHelpFooter();

  if (state == NULL || footer == NULL)
    return 0;

  if (state->current_detail_index != GENERATED_HELP_NO_SELECTION) {
    if (ch == KEY_LEFT) {
      state->detail_back_requested = TRUE;
      return 1;
    }
    if (FooterKeyMatches(ch, footer->index_key)) {
      if (!TopicIdEquals(state->topic, "index")) {
        state->next_topic_id = "index";
        return 1;
      }
      return -1;
    }
    if (FooterKeyMatches(ch, footer->navigation_key)) {
      if (!TopicIdEquals(state->topic, "f1-navigation")) {
        state->next_topic_id = "f1-navigation";
        return 1;
      }
      return -1;
    }
    return 0;
  }

  if (ch == KEY_LEFT && state->has_history) {
    state->back_requested = TRUE;
    return 1;
  }

  if (ch == KEY_UP || ch == KEY_DOWN) {
    size_t next_index;

    if (state->item_count == 0 || state->visible_row_count <= 0)
      return 0;

    if (!GetVisibleContextItemRange(state, &visible_start, &visible_end))
      return 0;

    if (state->selected_item_index == GENERATED_HELP_NO_SELECTION) {
      if (state->reselection_direction != 0 &&
          state->reselection_anchor_index != GENERATED_HELP_NO_SELECTION) {
        if (ch == KEY_UP && state->reselection_direction > 0)
          state->reselection_direction = -1;
        else if (ch == KEY_DOWN && state->reselection_direction < 0)
          state->reselection_direction = 1;
        return 0;
      }

      if (ch == KEY_UP) {
        next_index = FindPreviousSelectableItem(
            state, visible_end > 0 ? visible_end - 1 : visible_start,
            visible_start);
      } else {
        next_index = FindNextSelectableItem(state, visible_start, visible_end);
      }

      if (next_index == GENERATED_HELP_NO_SELECTION)
        return 0;

      state->selected_item_index = next_index;
      state->reselection_direction = 0;
      state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
      return -1;
    }

    if (state->selected_item_index < visible_start ||
        state->selected_item_index >= visible_end) {
      state->reselection_direction = (ch == KEY_UP) ? -1 : 1;
      state->reselection_anchor_index = state->selected_item_index;
      return 0;
    }

    next_index = GENERATED_HELP_NO_SELECTION;
    if (ch == KEY_UP) {
      if (state->selected_item_index > visible_start) {
        next_index = FindPreviousSelectableItem(
            state, state->selected_item_index - 1, visible_start);
      }
      if (next_index == GENERATED_HELP_NO_SELECTION &&
          state->selected_item_index > 0) {
        state->reselection_direction = -1;
        state->reselection_anchor_index = state->selected_item_index;
        return 0;
      }
    } else {
      if (state->selected_item_index + 1 < visible_end) {
        next_index = FindNextSelectableItem(state,
                                            state->selected_item_index + 1,
                                            visible_end);
      }
      if (next_index == GENERATED_HELP_NO_SELECTION &&
          state->selected_item_index + 1 < state->item_count) {
        state->reselection_direction = 1;
        state->reselection_anchor_index = state->selected_item_index;
        return 0;
      }
    }

    if (next_index == GENERATED_HELP_NO_SELECTION)
      return 0;

    state->selected_item_index = next_index;
    state->reselection_direction = 0;
    state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
    return -1;
  }

  if (ch == KEY_HOME || ch == KEY_END || ch == KEY_PPAGE || ch == KEY_NPAGE) {
    ResetContextualHelpSelection(state);
    return 0;
  }

  if (ch == KEY_RIGHT || ch == CR || ch == LF) {
    if (state->item_count == 0 ||
        state->selected_item_index == GENERATED_HELP_NO_SELECTION ||
        state->selected_item_index >= state->item_count)
      return -1;
    if (!state->items[state->selected_item_index].selectable)
      return -1;
    if (state->items[state->selected_item_index].linked_topic_id != NULL) {
      state->next_topic_id =
          state->items[state->selected_item_index].linked_topic_id;
      state->reselection_direction = 0;
      state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
      return 1;
    }
    state->next_detail_index = state->selected_item_index;
    state->reselection_direction = 0;
    state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
    return 1;
  }

  if (FooterKeyMatches(ch, footer->index_key)) {
    if (state->topic->topic_id != NULL &&
        strcmp(state->topic->topic_id, "index") != 0) {
      state->next_topic_id = "index";
      return 1;
    }
    return -1;
  }
  if (FooterKeyMatches(ch, footer->navigation_key)) {
    if (state->topic->topic_id != NULL &&
        strcmp(state->topic->topic_id, "f1-navigation") != 0) {
      state->next_topic_id = "f1-navigation";
      return 1;
    }
    return -1;
  }

  return 0;
}

static BOOL GeneratedHelpRowIsVisible(const RuntimeHelpPopupState *state,
                                      size_t row_index) {
  size_t visible_start;
  size_t visible_end;

  if (state == NULL || state->visible_row_count <= 0)
    return FALSE;
  visible_start = (size_t)MAXIMUM(state->visible_row_offset, 0);
  visible_end = visible_start + (size_t)state->visible_row_count;
  return row_index >= visible_start && row_index < visible_end;
}

static BOOL InlineHelpLinkIsAvailable(const RuntimeHelpPopupState *state,
                                      size_t link_index) {
  return state != NULL && link_index < state->inline_link_count &&
         state->inline_links[link_index].row_index < state->row_count;
}

static void SelectInlineHelpLink(RuntimeHelpPopupState *state,
                                 size_t link_index) {
  size_t row_index;

  if (state == NULL)
    return;
  if (state->active_inline_link_index < state->inline_link_count) {
    row_index =
        state->inline_links[state->active_inline_link_index].row_index;
    if (row_index < state->row_count)
      state->rows[row_index].selected_link_index =
          GENERATED_HELP_NO_SELECTION;
  }
  state->active_inline_link_index = link_index;
  if (!InlineHelpLinkIsAvailable(state, link_index))
    return;
  row_index = state->inline_links[link_index].row_index;
  state->rows[row_index].selected_link_index = link_index;
}

static size_t FindVisibleInlineHelpLink(const RuntimeHelpPopupState *state,
                                        BOOL reverse) {
  size_t index;

  if (state == NULL)
    return GENERATED_HELP_NO_SELECTION;
  if (reverse) {
    for (index = state->inline_link_count; index > 0; --index) {
      size_t candidate = index - 1;

      if (InlineHelpLinkIsAvailable(state, candidate) &&
          GeneratedHelpRowIsVisible(
              state, state->inline_links[candidate].row_index))
        return candidate;
    }
    return GENERATED_HELP_NO_SELECTION;
  }
  for (index = 0; index < state->inline_link_count; ++index) {
    if (InlineHelpLinkIsAvailable(state, index) &&
        GeneratedHelpRowIsVisible(state,
                                  state->inline_links[index].row_index))
      return index;
  }
  return GENERATED_HELP_NO_SELECTION;
}

static size_t FindAdjacentInlineHelpLink(const RuntimeHelpPopupState *state,
                                         size_t active_index, BOOL reverse) {
  size_t index;

  if (state == NULL || active_index >= state->inline_link_count)
    return GENERATED_HELP_NO_SELECTION;
  if (reverse) {
    for (index = active_index; index > 0; --index) {
      if (InlineHelpLinkIsAvailable(state, index - 1))
        return index - 1;
    }
    return GENERATED_HELP_NO_SELECTION;
  }
  for (index = active_index + 1; index < state->inline_link_count; ++index) {
    if (InlineHelpLinkIsAvailable(state, index))
      return index;
  }
  return GENERATED_HELP_NO_SELECTION;
}

static int HandleGeneratedHelpFooterKey(ViewContext *ctx, int ch,
                                        void *user_data) {
  RuntimeHelpPopupState *state = (RuntimeHelpPopupState *)user_data;
  const GeneratedHelpFooter *footer = ActiveGeneratedHelpFooter();

  (void)ctx;
  if (state == NULL || state->topic == NULL || footer == NULL)
    return 0;

  if (state->contextual_list_mode)
    return HandleContextualListFooterKey(state, ch);

  if (ch == KEY_LEFT) {
    if (state->has_history) {
      state->back_requested = TRUE;
      return 1;
    }
    return 0;
  }
  if (FooterKeyMatches(ch, footer->index_key)) {
    if (!TopicIdEquals(state->topic, "index")) {
      state->next_topic_id = "index";
      return 1;
    }
    return -1;
  }

  if (FooterKeyMatches(ch, footer->navigation_key)) {
    if (!TopicIdEquals(state->topic, "f1-navigation")) {
      state->next_topic_id = "f1-navigation";
      return 1;
    }
    return -1;
  }

  if (ch == KEY_UP || ch == KEY_DOWN) {
    BOOL reverse = ch == KEY_UP;

    if (state->inline_link_count > 0) {
      size_t next_index;

      if (!InlineHelpLinkIsAvailable(state,
                                     state->active_inline_link_index)) {
        next_index = FindVisibleInlineHelpLink(state, reverse);
        if (next_index == GENERATED_HELP_NO_SELECTION)
          return 0;
        SelectInlineHelpLink(state, next_index);
        return -1;
      }

      next_index = FindAdjacentInlineHelpLink(
          state, state->active_inline_link_index, reverse);
      if (next_index == GENERATED_HELP_NO_SELECTION)
        return 0;
      if (!GeneratedHelpRowIsVisible(
              state, state->inline_links[next_index].row_index))
        return 0;
      SelectInlineHelpLink(state, next_index);
      return -1;
    }

    if (state->related_link_count == 0)
      return 0;
    if (state->active_related_link_index >= state->related_link_count) {
      size_t first_visible =
          (size_t)MAXIMUM(state->visible_row_offset, 0);
      size_t visible_end =
          first_visible + (size_t)MAXIMUM(state->visible_row_count, 0);
      size_t index;

      if (reverse) {
        for (index = state->related_link_count; index > 0; --index) {
          size_t row = state->related_link_first_row + index - 1;

          if (row >= first_visible && row < visible_end) {
            state->active_related_link_index = index - 1;
            return -1;
          }
        }
      } else {
        for (index = 0; index < state->related_link_count; ++index) {
          size_t row = state->related_link_first_row + index;

          if (row >= first_visible && row < visible_end) {
            state->active_related_link_index = index;
            return -1;
          }
        }
      }
      return 0;
    }

    if (reverse) {
      if (state->active_related_link_index == 0)
        return 0;
      if (!GeneratedHelpRowIsVisible(
              state, state->related_link_first_row +
                         state->active_related_link_index - 1))
        return 0;
      state->active_related_link_index--;
    } else {
      if (state->active_related_link_index + 1 >= state->related_link_count)
        return 0;
      if (!GeneratedHelpRowIsVisible(
              state, state->related_link_first_row +
                         state->active_related_link_index + 1))
        return 0;
      state->active_related_link_index++;
    }
    return -1;
  }

  if (ch == KEY_RIGHT || ch == CR || ch == LF) {
    if (InlineHelpLinkIsAvailable(state,
                                  state->active_inline_link_index)) {
      state->next_topic_id =
          state->inline_links[state->active_inline_link_index].target_topic_id;
      return 1;
    }
    if (state->related_link_count == 0 ||
        state->active_related_link_index >= state->related_link_count)
      return 0;

    state->next_topic_id =
        state->topic->explainer_links[state->active_related_link_index]
            .target_topic_id;
    return 1;
  }

  return 0;
}

static int GetGeneratedHelpActiveRow(const void *user_data) {
  const RuntimeHelpPopupState *state = (const RuntimeHelpPopupState *)user_data;
  size_t visible_start;
  size_t visible_end;

  if (state == NULL)
    return -1;

  if (!state->contextual_list_mode &&
      InlineHelpLinkIsAvailable(state, state->active_inline_link_index)) {
    size_t row =
        state->inline_links[state->active_inline_link_index].row_index;

    return GeneratedHelpRowIsVisible(state, row) ? (int)row : -1;
  }

  if (!state->contextual_list_mode && state->related_link_count > 0 &&
      state->active_related_link_index < state->related_link_count) {
    size_t row =
        state->related_link_first_row + state->active_related_link_index;

    return GeneratedHelpRowIsVisible(state, row) ? (int)row : -1;
  }

  if (!state->contextual_list_mode ||
      state->current_detail_index != GENERATED_HELP_NO_SELECTION)
    return -1;

  if (state->selected_item_index == GENERATED_HELP_NO_SELECTION ||
      state->selected_item_index >= state->item_count)
    return -1;

  if (state->item_first_row[state->selected_item_index] ==
      GENERATED_HELP_NO_SELECTION)
    return -1;

  if (state->visible_row_count > 0) {
    if (!GetVisibleContextItemRange(state, &visible_start, &visible_end))
      return -1;

    if (state->selected_item_index < visible_start ||
        state->selected_item_index >= visible_end)
      return -1;
  }

  return (int)state->item_first_row[state->selected_item_index];
}

int UI_ShowGeneratedContextHelpWithOverrides(
    ViewContext *ctx, const char *context_id, const UIHelpPopupRow *prefix_rows,
    size_t prefix_row_count, const UIHelpLabelOverride *label_overrides,
    size_t label_override_count) {
  RuntimeHelpView history[GENERATED_HELP_MAX_HISTORY];
  size_t history_count = 0;
  RuntimeHelpView current_view;
  const GeneratedHelpTopic *topic;

  if (ctx == NULL || context_id == NULL || context_id[0] == '\0')
    return -1;

  topic = FindGeneratedTopicByContext(context_id);
  if (topic == NULL)
    return -1;

  current_view.topic = topic;
  current_view.selected_item_index = GENERATED_HELP_NO_SELECTION;
  current_view.current_detail_index = GENERATED_HELP_NO_SELECTION;
  current_view.active_inline_link_index = GENERATED_HELP_NO_SELECTION;
  current_view.scroll_line_offset = 0;
  current_view.contextual_origin =
      TopicUsesContextualItemList(topic, prefix_row_count);

  while (current_view.topic != NULL) {
    RuntimeHelpPopupState state;
    UIHelpPopupFooterSpec footer_spec;
    const GeneratedHelpTopic *next_topic;
    const char *title;

    memset(&state, 0, sizeof(state));
    memset(state.item_first_row, 0xFF, sizeof(state.item_first_row));
    memset(state.row_item_index, 0xFF, sizeof(state.row_item_index));
    state.topic = current_view.topic;
    state.label_overrides = label_overrides;
    state.label_override_count = label_override_count;
    state.selected_item_index = current_view.selected_item_index;
    state.current_detail_index = current_view.current_detail_index;
    state.active_inline_link_index = current_view.active_inline_link_index;
    state.visible_row_offset = current_view.scroll_line_offset;
    state.contextual_origin = current_view.contextual_origin;
    state.next_detail_index = GENERATED_HELP_NO_SELECTION;
    state.has_history = history_count > 0;
    state.prefix_row_count = prefix_row_count;
    state.reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
    state.related_link_start_index = GENERATED_HELP_NO_SELECTION;
    state.previous_visible_start = 0;
    state.previous_visible_end = 0;
    state.viewport_valid = FALSE;
    state.wrap_width =
        ctx->layout.main_win_width > GENERATED_HELP_MIN_MAIN_WIDTH
            ? ctx->layout.main_win_width - GENERATED_HELP_WRAP_PADDING
            : GENERATED_HELP_DEFAULT_WRAP_WIDTH;
    state.contextual_list_mode =
        TopicUsesContextualItemList(current_view.topic, prefix_row_count);
    if (state.selected_item_index >= state.item_count)
      state.selected_item_index = GENERATED_HELP_NO_SELECTION;
    if (state.current_detail_index >= state.item_count)
      state.current_detail_index = GENERATED_HELP_NO_SELECTION;

    state.footer_command_count = BuildFooterCommands(&state);
    state.wrap_width =
        MAXIMUM(state.wrap_width,
                UI_CommandStripVisualLength(state.footer_commands,
                                            state.footer_command_count));
    state.row_count = BuildTextRows(&state, prefix_rows, prefix_row_count);
    if (state.row_count == 0)
      return -1;

    memset(&footer_spec, 0, sizeof(footer_spec));
    footer_spec.commands = state.footer_commands;
    footer_spec.command_count = state.footer_command_count;
    footer_spec.link_command_count = state.link_command_count;
    footer_spec.active_command_index = state.active_link_index;
    footer_spec.key_handler = HandleGeneratedHelpFooterKey;
    footer_spec.active_row_handler = GetGeneratedHelpActiveRow;
    footer_spec.viewport_handler = UpdateGeneratedHelpViewport;
    footer_spec.initial_scroll_line = current_view.scroll_line_offset;
    footer_spec.final_scroll_line = &state.visible_row_offset;
    footer_spec.key_data = &state;

    title = current_view.topic->title;
    if (UI_ShowHelpPopupWithFooter(ctx, title, state.rows, state.row_count,
                                   &footer_spec) > 0 &&
        ctx->resize_request) {
      (void)AppStateClearResizeRequest(ctx);
      RefreshView(ctx, GetSelectedDirEntry(ctx, ctx->active->vol));
      continue;
    }

    current_view.selected_item_index = state.selected_item_index;
    current_view.active_inline_link_index = state.active_inline_link_index;
    current_view.scroll_line_offset = state.visible_row_offset;

    if (state.detail_back_requested) {
      current_view.current_detail_index = GENERATED_HELP_NO_SELECTION;
      continue;
    }

    if (state.next_detail_index != GENERATED_HELP_NO_SELECTION) {
      current_view.current_detail_index = state.next_detail_index;
      continue;
    }

    if (state.back_requested) {
      if (history_count == 0)
        break;
      current_view = history[history_count - 1];
      history_count--;
      continue;
    }

    if (state.next_topic_id == NULL)
      break;
    next_topic = FindGeneratedTopicById(state.next_topic_id);
    if (next_topic == NULL)
      break;
    if (history_count < GENERATED_HELP_MAX_HISTORY)
      history[history_count++] = current_view;
    current_view.topic = next_topic;
    current_view.selected_item_index = 0;
    current_view.current_detail_index = GENERATED_HELP_NO_SELECTION;
    current_view.active_inline_link_index = GENERATED_HELP_NO_SELECTION;
    current_view.scroll_line_offset = 0;
    current_view.contextual_origin = FALSE;
  }

  RefreshView(ctx, GetSelectedDirEntry(ctx, ctx->active->vol));
  return 0;
}

int UI_ShowGeneratedContextHelp(ViewContext *ctx, const char *context_id,
                                const UIHelpPopupRow *prefix_rows,
                                size_t prefix_row_count) {
  return UI_ShowGeneratedContextHelpWithOverrides(ctx, context_id, prefix_rows,
                                                  prefix_row_count, NULL, 0);
}

int UI_ShowGeneratedContextHelpCallback(ViewContext *ctx, void *help_data) {
  const char *context_id = (const char *)help_data;

  return UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0);
}
