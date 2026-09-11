#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include "ytnova_defs.h"

extern int TerminalInputStart(ViewContext *ctx);
extern BOOL TerminalInputSuspend(ViewContext *ctx);
extern int TerminalInputResume(ViewContext *ctx);
extern void TerminalInputShutdown(ViewContext *ctx);
extern int TerminalInputReadKey(ViewContext *ctx, WINDOW *win);
extern int TerminalInputUnreadKey(ViewContext *ctx, int key);

#endif
