/* RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <boolean.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <file/file_path.h>
#include "../ui_companion_driver.h"

typedef struct ui_companion_null
{
   void *empty;
} ui_companion_null_t;

static void ui_companion_null_deinit(void *data)
{
   ui_companion_null_t *handle = (ui_companion_null_t*)data;

   if (handle)
      free(handle);
}

static void *ui_companion_null_init(void)
{
   ui_companion_null_t *handle = (ui_companion_null_t*)calloc(1, sizeof(*handle));

   if (!handle)
      return NULL;

   return handle;
}

static int ui_companion_null_iterate(void *data, unsigned action)
{
   (void)data;
   (void)action;

   return 0;
}

static void ui_companion_null_notify_content_loaded(void *data)
{
   (void)data;
}

static void ui_companion_null_toggle(void *data)
{
   (void)data;
}

static void ui_companion_null_event_command(void *data, enum event_command cmd)
{
   (void)data;
   (void)cmd;
}

static void ui_companion_null_notify_list_pushed(void *data,
        file_list_t *list, file_list_t *menu_list)
{
    (void)data;
    (void)list;
    (void)menu_list;
}

const ui_companion_driver_t ui_companion_null = {
   ui_companion_null_init,
   ui_companion_null_deinit,
   ui_companion_null_iterate,
   ui_companion_null_toggle,
   ui_companion_null_event_command,
   ui_companion_null_notify_content_loaded,
   ui_companion_null_notify_list_pushed,
   NULL,
   NULL,
   NULL,
   NULL,
   &ui_browser_window_null,
   &ui_msg_window_null,
   &ui_window_null,
   &ui_application_null,
   "null",
};
