/***************************************************************************
 *
 * src/ui/print_controller.c
 * Print UI orchestration and prompt flow
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>

static const char output_destination_help_context[] =
    "prompt.output-destination";
static const char output_separator_help_context[] =
    "prompt.output-separator";
static const UICommandStripCommand output_file_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-destination.hints", "format"),
     "F3", NULL, "output-destination.hints"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-destination.hints", "history"),
     "Up", NULL, "output-destination.hints"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-destination.hints", "OK"),
     "Enter", NULL, "output-destination.hints"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-destination.hints", "cancel"),
     "Esc", NULL, "output-destination.hints"}};
static const UICommandStripCommand output_command_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-command.hints", "history"),
     "Up", NULL, "output-command.hints"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-command.hints", "OK"), "Enter",
     NULL, "output-command.hints"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("output-command.hints", "cancel"),
     "Esc", NULL, "output-command.hints"}};

enum { OUTPUT_FORMAT_CYCLE_KEY = 3 };

typedef struct {
  char prompt[COMMAND_LINE_LENGTH + 1];
  PrintConfig *config;
} OutputDestinationPromptState;

static void ClearPrintPrompt(ViewContext *ctx) {
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
}

static BOOL HasNonWhitespace(const char *text) {
  if (!text)
    return FALSE;
  while (*text) {
    if (!isspace((unsigned char)*text))
      return TRUE;
    text++;
  }
  return FALSE;
}

static PrintFormat NextOutputFormat(PrintFormat format) {
  switch (format) {
  case PRINT_FORMAT_RAW:
    return PRINT_FORMAT_FRAMED;
  case PRINT_FORMAT_FRAMED:
    return PRINT_FORMAT_PAGEBREAK;
  case PRINT_FORMAT_PAGEBREAK:
  default:
    return PRINT_FORMAT_RAW;
  }
}

static const char *OutputFormatName(PrintFormat format) {
  switch (format) {
  case PRINT_FORMAT_FRAMED:
    return _("Framed");
  case PRINT_FORMAT_PAGEBREAK:
    return _("Page break");
  case PRINT_FORMAT_RAW:
  default:
    return _("Raw");
  }
}

static void UpdateOutputDestinationPromptLabel(
    OutputDestinationPromptState *state) {
  char cwd[PATH_LENGTH + 1];

  if (!state || !state->config)
    return;

  if (state->config->destination == PRINT_DESTINATION_COMMAND) {
    snprintf(state->prompt, sizeof(state->prompt), "Printer command:");
  } else if (getcwd(cwd, sizeof(cwd))) {
    snprintf(state->prompt, sizeof(state->prompt),
             "Output file [%s] (CWD: %.180s):",
             OutputFormatName(state->config->format), cwd);
  } else {
    snprintf(state->prompt, sizeof(state->prompt), "Output file [%s]:",
             OutputFormatName(state->config->format));
  }
}

static int PromptOutputSeparator(ViewContext *ctx, PrintConfig *config,
                                 PrintFormat format) {
  char frame_sep[32] = "";
  const char *separator_prompt;

  if (!ctx || !config)
    return ESC;
  if (format != PRINT_FORMAT_FRAMED && format != PRINT_FORMAT_PAGEBREAK)
    return CR;

  separator_prompt = (format == PRINT_FORMAT_PAGEBREAK)
                         ? "Page break separator (default: ```): "
                         : "Frame separator (default: ```): ";
  if (UI_ReadStringWithHelp(ctx, ctx->active, separator_prompt, frame_sep,
                            sizeof(frame_sep) - 1, HST_PRINT_FRAME, NULL, 0,
                            UI_ShowGeneratedContextHelpCallback,
                            (void *)output_separator_help_context) == ESC) {
    return ESC;
  }
  if (frame_sep[0] == '\0') {
    snprintf(frame_sep, sizeof(frame_sep), "```");
  }
  snprintf(config->frame_separator, sizeof(config->frame_separator), "%s",
           frame_sep);
  return CR;
}

static BOOL HandleOutputDestinationPromptAction(
    ViewContext *ctx, YtreeNovaPanel *panel, int ch, const char *buffer,
    int max_len, int *cursor_pos, void *action_data) {
  OutputDestinationPromptState *state =
      (OutputDestinationPromptState *)action_data;
  PrintFormat next_format;

  (void)panel;
  (void)buffer;
  (void)max_len;
  (void)cursor_pos;

  if (!state || !state->config)
    return FALSE;

  switch (ch) {
#ifdef KEY_F
  case KEY_F(OUTPUT_FORMAT_CYCLE_KEY):
#endif
    next_format = NextOutputFormat(state->config->format);
    if (PromptOutputSeparator(ctx, state->config, next_format) == ESC)
      return TRUE;
    state->config->format = next_format;
    UpdateOutputDestinationPromptLabel(state);
    return TRUE;
  default:
    return FALSE;
  }
}

static int PromptOutputDestination(ViewContext *ctx, PrintConfig *config) {
  OutputDestinationPromptState state;
  UIPromptOptions options;
  int history_type;

  if (!ctx || !config)
    return ESC;

  memset(&state, 0, sizeof(state));
  state.config = config;
  UpdateOutputDestinationPromptLabel(&state);

  memset(&options, 0, sizeof(options));
  if (config->destination == PRINT_DESTINATION_COMMAND) {
    options.hints_override = output_command_hint_commands;
    options.hints_override_count = sizeof(output_command_hint_commands) /
                                   sizeof(output_command_hint_commands[0]);
    history_type = HST_PIPE;
  } else {
    options.hints_override = output_file_hint_commands;
    options.hints_override_count = sizeof(output_file_hint_commands) /
                                   sizeof(output_file_hint_commands[0]);
    history_type = HST_FILE;
  }
  options.help_callback = UI_ShowGeneratedContextHelpCallback;
  options.help_data = (void *)output_destination_help_context;
  if (config->destination == PRINT_DESTINATION_FILE) {
    options.action_handler = HandleOutputDestinationPromptAction;
    options.action_data = &state;
  }

  return UI_ReadStringWithPromptOptions(ctx, ctx->active, state.prompt,
                                        config->print_to, PATH_LENGTH,
                                        history_type, &options);
}

void UI_HandlePrintController(ViewContext *ctx, DirEntry *dir_entry,
                              BOOL tagged) {
  PrintConfig config;
  int term;
  int is_pipe = TRUE;
  char error_target[PATH_LENGTH + 1];
  PrintWriteStatus status;

  memset(&config, 0, sizeof(config));
  error_target[0] = '\0';

  if (tagged && dir_entry->tagged_files == 0) {
    UI_Beep(ctx, FALSE);
    return;
  }

  ClearHelp(ctx);

  term = InputChoiceWithHelp(ctx,
                             "Output to: (F)ile, (H)ardcopy  (Esc) cancel  ",
                             "FHC\033",
                             UI_ShowGeneratedContextHelpCallback,
                             (void *)output_destination_help_context);
  if (term == ESC) {
    ClearPrintPrompt(ctx);
    return;
  }
  config.destination = (term == 'H' || term == 'C') ? PRINT_DESTINATION_COMMAND
                                     : PRINT_DESTINATION_FILE;

  config.format = PRINT_FORMAT_RAW;
  if (PromptOutputDestination(ctx, &config) != CR) {
    ClearPrintPrompt(ctx);
    return;
  }

  if (!HasNonWhitespace(config.print_to)) {
    UI_Message(ctx, "No destination specified");
    ClearPrintPrompt(ctx);
    return;
  }

  endwin();
  SuspendClock(ctx);

  status = Cmd_WritePrintOutput(ctx, dir_entry, tagged, &config, &is_pipe,
                                error_target);
  if (status == PRINT_WRITE_OK && is_pipe) {
    HitReturnToContinue();
  }

  InitClock(ctx);
  if (dir_entry != NULL) {
    RefreshView(ctx, dir_entry);
  } else {
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
  }

  if (status == PRINT_WRITE_OPEN_FAILED) {
    if (is_pipe) {
      UI_Message(ctx, "execution of command*%s*failed", error_target);
    } else {
      UI_Message(ctx, "Failed to open file*%s*", error_target);
    }
  } else if (status == PRINT_WRITE_IO_ERROR) {
    UI_Message(ctx, "Output operation failed");
  } else if (status == PRINT_WRITE_NO_DESTINATION) {
    UI_Message(ctx, "No destination specified");
  }

  ClearPrintPrompt(ctx);
}

void UI_HandlePrint(ViewContext *ctx, DirEntry *dir_entry, BOOL tagged) {
  UI_HandlePrintController(ctx, dir_entry, tagged);
}
