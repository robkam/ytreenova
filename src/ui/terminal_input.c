#include "terminal_input.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#define KITTY_KEYBOARD_FLAG_DISAMBIGUATE 1U
#define KITTY_MODIFIER_SHIFT 1U
#define KITTY_MODIFIER_CONTROL 4U
#define KITTY_MODIFIER_LOCKS (64U | 128U)
#define TERMINAL_INPUT_DECIMAL_BASE 10U
#define TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH 3U
#define TERMINAL_INPUT_PRIVATE_CSI_MIN_LENGTH 4U
#define TERMINAL_INPUT_PROBE_TIMEOUT_MS 80L
#define TERMINAL_INPUT_SEQUENCE_TIMEOUT_MS 25
#define TERMINAL_INPUT_SEQUENCE_CAPACITY 32
#define TERMINAL_INPUT_CSI_U_MIN_LENGTH 5U

static const char kitty_probe_request[] = "\033[>1u\033[?u\033[c";
static const char kitty_pop_request[] = "\033[<u";

static int TerminalInputWriteAll(const char *data, size_t length) {
  size_t written = 0;

  while (written < length) {
    ssize_t result = write(STDOUT_FILENO, data + written, length - written);
    if (result > 0) {
      written += (size_t)result;
      continue;
    }
    if (result < 0 && errno == EINTR)
      continue;
    return -1;
  }
  return 0;
}

static long long TerminalInputMonotonicMilliseconds(void) {
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    return -1;
  return ((long long)now.tv_sec * 1000LL) +
         ((long long)now.tv_nsec / 1000000LL);
}

static BOOL TerminalInputValidParameterBytes(const unsigned char *bytes,
                                             size_t start, size_t end) {
  size_t index;
  BOOL have_digit = FALSE;

  for (index = start; index < end; ++index) {
    if (bytes[index] >= '0' && bytes[index] <= '9') {
      have_digit = TRUE;
      continue;
    }
    if (bytes[index] != ';')
      return FALSE;
  }
  return have_digit;
}

static BOOL TerminalInputParseUnsigned(const unsigned char *bytes, size_t start,
                                       size_t end, unsigned int *value_out) {
  size_t index;
  unsigned int value = 0;

  if (start == end || value_out == NULL)
    return FALSE;
  for (index = start; index < end; ++index) {
    unsigned int digit;
    if (bytes[index] < '0' || bytes[index] > '9')
      return FALSE;
    digit = (unsigned int)(bytes[index] - '0');
    if (value > (UINT_MAX - digit) / TERMINAL_INPUT_DECIMAL_BASE)
      return FALSE;
    value = (value * TERMINAL_INPUT_DECIMAL_BASE) + digit;
  }
  *value_out = value;
  return TRUE;
}

static BOOL TerminalInputProbeResponsesComplete(const unsigned char *bytes,
                                                size_t length) {
  size_t index;
  BOOL status_found = FALSE;
  BOOL device_attributes_found = FALSE;

  for (index = 0;
       index + TERMINAL_INPUT_PRIVATE_CSI_MIN_LENGTH <= length; ++index) {
    size_t end;
    if (bytes[index] != 0x1b || bytes[index + 1U] != '[' ||
        bytes[index + 2U] != '?')
      continue;
    for (end = index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH; end < length;
         ++end) {
      if (bytes[end] == 'u') {
        unsigned int flags;
        if (TerminalInputParseUnsigned(
                bytes, index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH, end,
                &flags))
          status_found = TRUE;
        break;
      }
      if (bytes[end] == 'c' &&
          TerminalInputValidParameterBytes(
              bytes, index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH, end)) {
        device_attributes_found = TRUE;
        break;
      }
      if ((bytes[end] < '0' || bytes[end] > '9') && bytes[end] != ';')
        break;
    }
  }
  return status_found && device_attributes_found;
}

static void TerminalInputPreserveByte(ViewContext *ctx, unsigned char byte) {
  size_t end;

  if (ctx == NULL ||
      ctx->terminal_input_pending_length >= TERMINAL_INPUT_PENDING_CAPACITY)
    return;
  end = (ctx->terminal_input_pending_offset +
         ctx->terminal_input_pending_length) %
        TERMINAL_INPUT_PENDING_CAPACITY;
  ctx->terminal_input_pending[end] = byte;
  ctx->terminal_input_pending_length++;
}

static BOOL TerminalInputClassifyProbe(ViewContext *ctx,
                                       const unsigned char *bytes,
                                       size_t length) {
  size_t index = 0;
  BOOL status_found = FALSE;
  BOOL device_attributes_found = FALSE;
  unsigned int flags = 0;

  while (index < length) {
    if (index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH < length &&
        bytes[index] == 0x1b &&
        bytes[index + 1U] == '[' && bytes[index + 2U] == '?') {
      size_t end;
      for (end = index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH; end < length;
           ++end) {
        if (bytes[end] == 'u') {
          unsigned int parsed_flags;
          if (TerminalInputParseUnsigned(
                  bytes, index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH, end,
                  &parsed_flags)) {
            status_found = TRUE;
            flags = parsed_flags;
            index = end + 1U;
            break;
          }
        } else if (bytes[end] == 'c') {
          if (TerminalInputValidParameterBytes(
                  bytes, index + TERMINAL_INPUT_PRIVATE_CSI_PREFIX_LENGTH,
                  end)) {
            device_attributes_found = TRUE;
            index = end + 1U;
            break;
          }
        } else if ((bytes[end] < '0' || bytes[end] > '9') &&
                   bytes[end] != ';') {
          break;
        }
      }
      if (index == end + 1U)
        continue;
    }
    TerminalInputPreserveByte(ctx, bytes[index]);
    index++;
  }

  return status_found && device_attributes_found &&
         (flags & KITTY_KEYBOARD_FLAG_DISAMBIGUATE) != 0U;
}

static BOOL TerminalInputHasQueuedKey(ViewContext *ctx) {
  int old_delay = -1;
  int key;

  if (ctx == NULL || ctx->terminal_input_unread_key_pending ||
      ctx->terminal_input_pending_length > 0U)
    return TRUE;
  if (ctx->curses_screen == NULL)
    return FALSE;
#ifdef NCURSES_VERSION
  old_delay = wgetdelay(stdscr);
#endif
  wtimeout(stdscr, 0);
  key = wgetch(stdscr);
  wtimeout(stdscr, old_delay);
  if (key == ERR)
    return FALSE;
  ctx->terminal_input_unread_key = key;
  ctx->terminal_input_unread_key_pending = TRUE;
  return TRUE;
}

static int TerminalInputProbe(ViewContext *ctx, BOOL *probe_attempted) {
  unsigned char bytes[TERMINAL_INPUT_PENDING_CAPACITY];
  size_t length = 0;
  long long deadline;

  if (probe_attempted != NULL)
    *probe_attempted = FALSE;
  if (ctx == NULL || probe_attempted == NULL || !isatty(STDIN_FILENO) ||
      !isatty(STDOUT_FILENO))
    return 0;
  if (TerminalInputHasQueuedKey(ctx))
    return 0;
  if (fflush(stdout) != 0)
    return 0;

  *probe_attempted = TRUE;
  if (TerminalInputWriteAll(kitty_probe_request,
                            sizeof(kitty_probe_request) - 1U) != 0)
    return 0;

  deadline = TerminalInputMonotonicMilliseconds();
  if (deadline < 0)
    return 0;
  deadline += TERMINAL_INPUT_PROBE_TIMEOUT_MS;

  while (length < sizeof(bytes)) {
    long long now = TerminalInputMonotonicMilliseconds();
    long long remaining;
    fd_set read_fds;
    struct timeval timeout;
    int ready;

    if (now < 0 || now >= deadline)
      break;
    remaining = deadline - now;
    timeout.tv_sec = (time_t)(remaining / 1000LL);
    timeout.tv_usec = (suseconds_t)((remaining % 1000LL) * 1000LL);
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    ready = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (ready == 0)
      break;
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      ssize_t count = read(STDIN_FILENO, bytes + length, sizeof(bytes) - length);
      if (count > 0) {
        length += (size_t)count;
        if (TerminalInputProbeResponsesComplete(bytes, length))
          break;
        continue;
      }
      if (count < 0 && errno == EINTR)
        continue;
      break;
    }
  }

  return TerminalInputClassifyProbe(ctx, bytes, length) ? 1 : 0;
}

int TerminalInputStart(ViewContext *ctx) {
  BOOL probe_attempted = FALSE;

  if (ctx == NULL)
    return -1;
  if (ctx->terminal_input_session_active)
    return 0;

  ctx->terminal_input_active = FALSE;
  if (TerminalInputProbe(ctx, &probe_attempted)) {
    ctx->terminal_input_capability = TERMINAL_INPUT_KITTY;
    ctx->terminal_input_session_active = TRUE;
    ctx->terminal_input_active = TRUE;
    return 0;
  }

  if (probe_attempted)
    (void)TerminalInputWriteAll(kitty_pop_request,
                                sizeof(kitty_pop_request) - 1U);
  ctx->terminal_input_capability = TERMINAL_INPUT_LEGACY;
  ctx->terminal_input_session_active = TRUE;
  return 0;
}

BOOL TerminalInputSuspend(ViewContext *ctx) {
  if (ctx == NULL || !ctx->terminal_input_session_active)
    return FALSE;

  if (ctx->terminal_input_active)
    (void)TerminalInputWriteAll(kitty_pop_request,
                                sizeof(kitty_pop_request) - 1U);
  ctx->terminal_input_session_active = FALSE;
  ctx->terminal_input_active = FALSE;
  return TRUE;
}

int TerminalInputResume(ViewContext *ctx) {
  if (ctx == NULL)
    return -1;
  if (ctx->terminal_input_session_active)
    return 0;
  return TerminalInputStart(ctx);
}

void TerminalInputShutdown(ViewContext *ctx) {
  (void)TerminalInputSuspend(ctx);
}

static int TerminalInputControlKey(unsigned int codepoint,
                                   unsigned int modifiers) {
  unsigned int modifier_bits;

  if (modifiers == 0U)
    return ERR;
  modifier_bits = modifiers - 1U;
  if ((modifier_bits & KITTY_MODIFIER_CONTROL) == 0U) {
    if ((modifier_bits & ~(KITTY_MODIFIER_SHIFT | KITTY_MODIFIER_LOCKS)) != 0U)
      return ERR;
    return codepoint <= (unsigned int)INT_MAX ? (int)codepoint : ERR;
  }
  if ((modifier_bits & ~(KITTY_MODIFIER_SHIFT | KITTY_MODIFIER_CONTROL |
                         KITTY_MODIFIER_LOCKS)) != 0U)
    return ERR;

  if (codepoint == 'm' || codepoint == 'M')
    return YTNOVA_KEY_CTRL_M;
  if (codepoint >= '@' && codepoint <= '_')
    return (int)(codepoint & 0x1fU);
  if (codepoint >= 'a' && codepoint <= 'z')
    return (int)((codepoint - 'a' + 'A') & 0x1fU);
  return ERR;
}

static int TerminalInputDecodeKittySequence(ViewContext *ctx, WINDOW *win) {
  unsigned char sequence[TERMINAL_INPUT_SEQUENCE_CAPACITY];
  size_t length = 0;
  unsigned int codepoint;
  unsigned int modifiers;
  size_t separator = 0;
  size_t index;
  int old_delay = -1;
  int result = ERR;

  (void)ctx;
#ifdef NCURSES_VERSION
  old_delay = wgetdelay(win);
#endif
  wtimeout(win, TERMINAL_INPUT_SEQUENCE_TIMEOUT_MS);
  while (length < sizeof(sequence)) {
    int ch = wgetch(win);
    if (ch == ERR || ch > UCHAR_MAX)
      break;
    sequence[length++] = (unsigned char)ch;
    if (ch == 'u')
      break;
  }
  wtimeout(win, old_delay);

  if (length < TERMINAL_INPUT_CSI_U_MIN_LENGTH || sequence[0] != '[' ||
      sequence[length - 1U] != 'u')
    goto preserve;
  for (index = 1U; index + 1U < length; ++index) {
    if (sequence[index] == ';') {
      separator = index;
      break;
    }
  }
  if (separator == 0U ||
      !TerminalInputParseUnsigned(sequence, 1U, separator, &codepoint) ||
      !TerminalInputParseUnsigned(sequence, separator + 1U, length - 1U,
                                  &modifiers))
    goto preserve;

  result = TerminalInputControlKey(codepoint, modifiers);
  if (result != ERR)
    return result;

preserve:
  for (index = length; index > 0U; --index)
    (void)ungetch((int)sequence[index - 1U]);
  return ESC;
}

int TerminalInputReadKey(ViewContext *ctx, WINDOW *win) {
  int ch;

  if (win == NULL)
    win = stdscr;
  if (ctx != NULL && ctx->terminal_input_unread_key_pending) {
    ctx->terminal_input_unread_key_pending = FALSE;
    return ctx->terminal_input_unread_key;
  }
  if (ctx != NULL && ctx->terminal_input_pending_length > 0U) {
    ch = (int)ctx->terminal_input_pending[ctx->terminal_input_pending_offset];
    ctx->terminal_input_pending_offset =
        (ctx->terminal_input_pending_offset + 1U) %
        TERMINAL_INPUT_PENDING_CAPACITY;
    ctx->terminal_input_pending_length--;
    return ch;
  }

  ch = wgetch(win);
  if (ctx != NULL && ctx->terminal_input_active && ch == ESC)
    return TerminalInputDecodeKittySequence(ctx, win);
  return ch;
}

int TerminalInputUnreadKey(ViewContext *ctx, int key) {
  if (key == ERR)
    return -1;
  if (ctx == NULL)
    return ungetch(key) == ERR ? -1 : 0;
  if (ctx->terminal_input_unread_key_pending)
    return -1;
  ctx->terminal_input_unread_key = key;
  ctx->terminal_input_unread_key_pending = TRUE;
  return 0;
}
