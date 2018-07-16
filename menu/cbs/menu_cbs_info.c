/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <compat/strl.h>

#include "../menu_driver.h"
#include "../menu_navigation.h"
#include "../menu_cbs.h"

#include "../widgets/menu_dialog.h"

#ifndef BIND_ACTION_INFO
#define BIND_ACTION_INFO(cbs, name) \
   cbs->action_info = name; \
   cbs->action_info_ident = #name;
#endif

static int action_info_default(unsigned type, const char *label)
{
   size_t selection             = 0;
   menu_displaylist_info_t info = {0};
   file_list_t *menu_stack      = menu_entries_get_menu_stack_ptr(0);

   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return 0;

   info.list          = menu_stack;
   info.directory_ptr = selection;
   info.enum_idx      = MENU_ENUM_LABEL_INFO_SCREEN;
   strlcpy(info.label,
         msg_hash_to_str(MENU_ENUM_LABEL_INFO_SCREEN),
        sizeof(info.label));

   if (!menu_displaylist_ctl(DISPLAYLIST_HELP, &info))
      return -1;

   if (!menu_displaylist_ctl(DISPLAYLIST_PROCESS, &info))
      return -1;

   return 0;
}

#ifdef HAVE_CHEEVOS
int  generic_action_ok_help(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx,
      enum msg_hash_enums id, enum menu_dialog_type id2);

static int action_info_cheevos(unsigned type, const char *label)
{
   unsigned new_id        = type - MENU_SETTINGS_CHEEVOS_START;

   menu_dialog_set_current_id(new_id);

   return generic_action_ok_help(NULL, label, new_id, 0, 0,
         MENU_ENUM_LABEL_CHEEVOS_DESCRIPTION,
         MENU_DIALOG_HELP_CHEEVOS_DESCRIPTION);
}
#endif

int menu_cbs_init_bind_info(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx)
{
   if (!cbs)
      return -1;

#ifdef HAVE_CHEEVOS
   if ((type >= MENU_SETTINGS_CHEEVOS_START))
   {
      BIND_ACTION_INFO(cbs, action_info_cheevos);
      return 0;
   }
#endif

   BIND_ACTION_INFO(cbs, action_info_default);

   return -1;
}
