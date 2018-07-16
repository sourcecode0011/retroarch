/*  RetroArch - A frontend for libretro.
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

#include <time.h>

#include <queues/message_queue.h>
#include <retro_miscellaneous.h>

#include "../../config.def.h"
#include "../../gfx/font_driver.h"
#include "../../gfx/video_context_driver.h"

#include "../menu_display.h"

static void *menu_display_caca_get_default_mvp(void)
{
   return NULL;
}

static void menu_display_caca_blend_begin(void)
{
}

static void menu_display_caca_blend_end(void)
{
}

static void menu_display_caca_draw(void *data)
{
   (void)data;
}

static void menu_display_caca_draw_pipeline(void *data)
{
   (void)data;
}

static void menu_display_caca_viewport(void *data)
{
   (void)data;
}

static void menu_display_caca_restore_clear_color(void)
{
}

static void menu_display_caca_clear_color(menu_display_ctx_clearcolor_t *clearcolor)
{
   (void)clearcolor;
}

static bool menu_display_caca_font_init_first(
      void **font_handle, void *video_data,
      const char *font_path, float font_size)
{
   font_data_t **handle = (font_data_t**)font_handle;
   *handle = font_driver_init_first(video_data,
         font_path, font_size, true, FONT_DRIVER_RENDER_CACA);
   return *handle;
}

static const float *menu_display_caca_get_default_vertices(void)
{
   static float dummy[16] = {0.0f};
   return &dummy[0];
}

static const float *menu_display_caca_get_default_tex_coords(void)
{
   static float dummy[16] = {0.0f};
   return &dummy[0];
}

menu_display_ctx_driver_t menu_display_ctx_caca = {
   menu_display_caca_draw,
   menu_display_caca_draw_pipeline,
   menu_display_caca_viewport,
   menu_display_caca_blend_begin,
   menu_display_caca_blend_end,
   menu_display_caca_restore_clear_color,
   menu_display_caca_clear_color,
   menu_display_caca_get_default_mvp,
   menu_display_caca_get_default_vertices,
   menu_display_caca_get_default_tex_coords,
   menu_display_caca_font_init_first,
   MENU_VIDEO_DRIVER_CACA,
   "menu_display_caca",
};
