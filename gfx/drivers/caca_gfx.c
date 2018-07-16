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

#include <caca.h>

#include <retro_miscellaneous.h>

#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif

#include "../common/caca_common.h"

#include "../font_driver.h"

#include "../../driver.h"
#include "../../verbosity.h"

static caca_canvas_t *caca_cv         = NULL;
static caca_dither_t *caca_dither     = NULL;
static caca_display_t *caca_display   = NULL;
static unsigned char *caca_menu_frame = NULL;
static unsigned caca_menu_width       = 0;
static unsigned caca_menu_height      = 0;
static unsigned caca_menu_pitch       = 0;
static unsigned caca_video_width      = 0;
static unsigned caca_video_height     = 0;
static unsigned caca_video_pitch      = 0;
static bool caca_rgb32                = 0;

static void caca_gfx_free(void *data);

static void caca_gfx_create(void)
{
   caca_display = caca_create_display(NULL);
   caca_cv = caca_get_canvas(caca_display);

   if(!caca_video_width || !caca_video_height)
   {
      caca_video_width = caca_get_canvas_width(caca_cv);
      caca_video_height = caca_get_canvas_height(caca_cv);
   }

   if (caca_rgb32)
      caca_dither = caca_create_dither(32, caca_video_width, caca_video_height, caca_video_pitch,
                            0x00ff0000, 0xff00, 0xff, 0x0);
   else
      caca_dither = caca_create_dither(16, caca_video_width, caca_video_height, caca_video_pitch,
                            0xf800, 0x7e0, 0x1f, 0x0);

   video_driver_set_size(&caca_video_width, &caca_video_height);
}

static void *caca_gfx_init(const video_info_t *video,
      const input_driver_t **input, void **input_data)
{
   caca_t *caca        = (caca_t*)calloc(1, sizeof(*caca));

   caca->caca_cv       = &caca_cv;
   caca->caca_dither   = &caca_dither;
   caca->caca_display  = &caca_display;

   *input              = NULL;
   *input_data         = NULL;

   caca_video_width    = video->width;
   caca_video_height   = video->height;
   caca_rgb32          = video->rgb32;

   if (video->rgb32)
      caca_video_pitch = video->width * 4;
   else
      caca_video_pitch = video->width * 2;

   caca_gfx_create();

   if (!caca_cv || !caca_dither || !caca_display)
   {
      /* TODO: handle errors */
   }

   if (video->font_enable)
      font_driver_init_osd(NULL, false, FONT_DRIVER_RENDER_CACA);

   return caca;
}

static bool caca_gfx_frame(void *data, const void *frame,
      unsigned frame_width, unsigned frame_height, uint64_t frame_count,
      unsigned pitch, const char *msg, video_frame_info_t *video_info)
{
   size_t len = 0;
   void *buffer = NULL;
   const void *frame_to_copy = frame;
   unsigned width = 0;
   unsigned height = 0;
   bool draw = true;

   (void)data;
   (void)frame;
   (void)frame_width;
   (void)frame_height;
   (void)pitch;
   (void)msg;

   if (!frame || !frame_width || !frame_height)
      return true;

   if (  caca_video_width  != frame_width   || 
         caca_video_height != frame_height  ||
         caca_video_pitch  != pitch)
   {
      if (frame_width > 4 && frame_height > 4)
      {
         caca_video_width = frame_width;
         caca_video_height = frame_height;
         caca_video_pitch = pitch;
         caca_gfx_free(NULL);
         caca_gfx_create();
      }
   }

   if (!caca_cv)
      return true;

   if (caca_menu_frame && menu_driver_ctl(RARCH_MENU_CTL_IS_ALIVE, NULL))
      frame_to_copy = caca_menu_frame;

   width = caca_get_canvas_width(caca_cv);
   height = caca_get_canvas_height(caca_cv);

   if (  frame_to_copy == frame && 
         frame_width   == 4 && 
         frame_height  == 4 && 
         (frame_width < width && frame_height < height))
      draw = false;

   if (menu_driver_ctl(RARCH_MENU_CTL_IS_ALIVE, NULL))
      draw = false;

   caca_clear_canvas(caca_cv);

#ifdef HAVE_MENU
   menu_driver_frame(video_info);
#endif

   if (msg)
      font_driver_render_msg(video_info, NULL, msg, NULL);

   if (draw)
   {
      caca_dither_bitmap(caca_cv, 0, 0,
            width,
            height,
            caca_dither, frame_to_copy);

      buffer = caca_export_canvas_to_memory(caca_cv, "caca", &len);

      if (buffer)
      {
         if (len)
            caca_refresh_display(caca_display);

         free(buffer);
      }
   }

   return true;
}

static void caca_gfx_set_nonblock_state(void *data, bool toggle)
{
   (void)data;
   (void)toggle;
}

static bool caca_gfx_alive(void *data)
{
   (void)data;
   video_driver_set_size(&caca_video_width, &caca_video_height);
   return true;
}

static bool caca_gfx_focus(void *data)
{
   (void)data;
   return true;
}

static bool caca_gfx_suppress_screensaver(void *data, bool enable)
{
   (void)data;
   (void)enable;
   return false;
}

static bool caca_gfx_has_windowed(void *data)
{
   (void)data;
   return true;
}

static void caca_gfx_free(void *data)
{
   (void)data;

   if (caca_display)
   {
      caca_free_display(caca_display);
      caca_display = NULL;
   }

   if (caca_dither)
   {
      caca_free_dither(caca_dither);
      caca_dither = NULL;
   }

   if (caca_menu_frame)
   {
      free(caca_menu_frame);
      caca_menu_frame = NULL;
   }
}

static bool caca_gfx_set_shader(void *data,
      enum rarch_shader_type type, const char *path)
{
   (void)data;
   (void)type;
   (void)path;

   return false;
}

static void caca_gfx_set_rotation(void *data,
      unsigned rotation)
{
   (void)data;
   (void)rotation;
}

static void caca_gfx_viewport_info(void *data,
      struct video_viewport *vp)
{
   (void)data;
   (void)vp;
}

static bool caca_gfx_read_viewport(void *data, uint8_t *buffer)
{
   (void)data;
   (void)buffer;

   return true;
}

static void caca_set_texture_frame(void *data,
      const void *frame, bool rgb32, unsigned width, unsigned height,
      float alpha)
{
   unsigned pitch = width * 2;

   if (rgb32)
      pitch = width * 4;

   if (caca_menu_frame)
   {
      free(caca_menu_frame);
      caca_menu_frame = NULL;
   }

   if ( !caca_menu_frame || 
         caca_menu_width  != width  || 
         caca_menu_height != height || 
         caca_menu_pitch  != pitch)
      if (pitch && height)
         caca_menu_frame = (unsigned char*)malloc(pitch * height);

   if (caca_menu_frame && frame && pitch && height)
      memcpy(caca_menu_frame, frame, pitch * height);
}

static void caca_set_osd_msg(void *data, const char *msg,
      const void *params, void *font)
{
   video_frame_info_t video_info;
   video_driver_build_info(&video_info);
   font_driver_render_msg(&video_info, font, msg, params);
}

static const video_poke_interface_t caca_poke_interface = {
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
#ifdef HAVE_FBO
   NULL,
#else
   NULL,
#endif
   NULL,
   NULL,
   NULL,
#if defined(HAVE_MENU)
   caca_set_texture_frame,
   NULL,
   caca_set_osd_msg,
   NULL,
#else
   NULL,
   NULL,
   NULL,
   NULL,
#endif

   NULL,
#ifdef HAVE_MENU
   NULL,
#endif
};

static void caca_gfx_get_poke_interface(void *data,
      const video_poke_interface_t **iface)
{
   (void)data;
   *iface = &caca_poke_interface;
}

void caca_gfx_set_viewport(void *data, unsigned viewport_width,
      unsigned viewport_height, bool force_full, bool allow_rotate)
{
}

video_driver_t video_caca = {
   caca_gfx_init,
   caca_gfx_frame,
   caca_gfx_set_nonblock_state,
   caca_gfx_alive,
   caca_gfx_focus,
   caca_gfx_suppress_screensaver,
   caca_gfx_has_windowed,
   caca_gfx_set_shader,
   caca_gfx_free,
   "caca",
   caca_gfx_set_viewport,
   caca_gfx_set_rotation,
   caca_gfx_viewport_info,
   caca_gfx_read_viewport,
   NULL, /* read_frame_raw */

#ifdef HAVE_OVERLAY
  NULL, /* overlay_interface */
#endif
  caca_gfx_get_poke_interface,
};
