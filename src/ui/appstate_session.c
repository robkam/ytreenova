/***************************************************************************
 *
 * src/ui/appstate_session.c
 * Session-routing transition commits for AppState boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_session.h"

BOOL AppStateCommitActivePanel(ViewContext *ctx, YtreeNovaPanel *panel) {
  if (!AppStateValidatedOwnerField("ctx.active"))
    return FALSE;
  if (!ctx || !panel)
    return FALSE;
  if (panel != ctx->left && panel != ctx->right)
    return FALSE;

  ctx->active = panel;
  if (panel->vol != NULL)
    ctx->view_mode = panel->vol->vol_stats.log_mode;
  ctx->dir_mode = panel->dir_mode;
  ctx->fixed_col_width = panel->fixed_col_width;
  return TRUE;
}

BOOL AppStateCommitGlobalSearchTerm(ViewContext *ctx, const char *term) {
  if (!AppStateValidatedOwnerField("ctx.command_state"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->global_search_term[0] = '\0';
  if (term) {
    (void)snprintf(ctx->global_search_term, sizeof(ctx->global_search_term),
                   "%s", term);
    ctx->global_search_term[sizeof(ctx->global_search_term) - 1] = '\0';
  }
  return TRUE;
}

BOOL AppStateCommitRefreshMode(ViewContext *ctx, int refresh_mode) {
  if (!AppStateValidatedOwnerField("ctx.refresh_mode"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->refresh_mode = refresh_mode;
  return TRUE;
}
