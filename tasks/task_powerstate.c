/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C) 2016 - Brad Parker
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

#include <stdlib.h>
#include "../../frontend/frontend_driver.h"
#include "tasks_internal.h"

static int              power_percent = 0;
static enum frontend_powerstate state = FRONTEND_POWERSTATE_NONE;

typedef struct
{
   int percent;
   enum frontend_powerstate state;
} powerstate_t;

enum frontend_powerstate get_last_powerstate(int *percent)
{
   if (percent)
      *percent = power_percent;

   return state;
}

static void task_powerstate_cb(void *task_data,
                               void *user_data, const char *error)
{
   powerstate_t *powerstate = (powerstate_t*)task_data;

   power_percent = powerstate->percent;
   state = powerstate->state;

   free(powerstate);
}

static void task_powerstate_handler(retro_task_t *task)
{
   const frontend_ctx_driver_t *frontend = frontend_get_ptr();
   powerstate_t *powerstate = (powerstate_t*)task->state;

   if (frontend && frontend->get_powerstate)
   {
      int seconds = 0;
      powerstate->state = frontend->get_powerstate(&seconds, &powerstate->percent);
   }

   task_set_data(task, powerstate);
   task_set_finished(task, true);
}

void task_push_get_powerstate(void)
{
   retro_task_t *task = (retro_task_t*)calloc(1, sizeof(*task));
   powerstate_t *state = (powerstate_t*)calloc(1, sizeof(*state));

   task->type     = TASK_TYPE_NONE;
   task->state    = state;
   task->handler  = task_powerstate_handler;
   task->callback = task_powerstate_cb;
   task->mute     = true;

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, task);
}
