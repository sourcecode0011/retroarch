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

#include "../../ui_companion_driver.h"

static bool ui_application_null_initialize(void)
{
   return true;
}

static bool ui_application_null_pending_events(void)
{
   return true;
}

static void ui_application_null_process_events(void)
{
}

const ui_application_t ui_application_null = {
   ui_application_null_initialize,
   ui_application_null_pending_events,
   ui_application_null_process_events,
   "null"
};
