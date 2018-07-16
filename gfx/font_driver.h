/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
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

#ifndef __FONT_DRIVER_H__
#define __FONT_DRIVER_H__

#include <stdint.h>

#include <boolean.h>
#include <retro_common_api.h>

#include "video_driver.h"

RETRO_BEGIN_DECLS

enum font_driver_render_api
{
   FONT_DRIVER_RENDER_DONT_CARE,
   FONT_DRIVER_RENDER_OPENGL_API,
   FONT_DRIVER_RENDER_DIRECT3D_API,
   FONT_DRIVER_RENDER_VITA2D,
   FONT_DRIVER_RENDER_CTR,
   FONT_DRIVER_RENDER_VULKAN_API,
   FONT_DRIVER_RENDER_CACA,
   FONT_DRIVER_RENDER_GDI
};

enum text_alignment
{
   TEXT_ALIGN_LEFT = 0,
   TEXT_ALIGN_RIGHT,
   TEXT_ALIGN_CENTER
};

/* All coordinates and offsets are top-left oriented.
 *
 * This is a texture-atlas approach which allows text to 
 * be drawn in a single draw call.
 *
 * It is up to the code using this interface to actually 
 * generate proper vertex buffers and upload the atlas texture to GPU. */

struct font_glyph
{
   unsigned width;
   unsigned height;

   /* Texel coordinate offset for top-left pixel of this glyph. */
   unsigned atlas_offset_x;
   unsigned atlas_offset_y;

   /* When drawing this glyph, apply an offset to 
    * current X/Y draw coordinate. */
   int draw_offset_x;
   int draw_offset_y;

   /* Advance X/Y draw coordinates after drawing this glyph. */
   int advance_x;
   int advance_y;
};

struct font_atlas
{
   uint8_t *buffer; /* Alpha channel. */
   unsigned width;
   unsigned height;
   bool dirty;
};

struct font_params
{
   float x;
   float y;
   float scale;
   /* Drop shadow color multiplier. */
   float drop_mod;
   /* Drop shadow offset.
    * If both are 0, no drop shadow will be rendered. */
   int drop_x, drop_y;
   /* Drop shadow alpha */
   float drop_alpha;
   /* ABGR. Use the macros. */
   uint32_t color;
   bool full_screen;
   enum text_alignment text_align;
};

typedef struct font_renderer
{
   void *(*init)(void *data, const char *font_path, float font_size);
   void (*free)(void *data);
   void (*render_msg)(
         video_frame_info_t *video_info,
         void *data, const char *msg,
         const void *params);
   const char *ident;

   const struct font_glyph *(*get_glyph)(void *data, uint32_t code);
   void (*bind_block)(void *data, void *block);
   void (*flush)(unsigned width, unsigned height, void *data);
   
   int (*get_message_width)(void *data, const char *msg, unsigned msg_len_full, float scale);
} font_renderer_t;

typedef struct font_renderer_driver
{
   void *(*init)(const char *font_path, float font_size);

   struct font_atlas *(*get_atlas)(void *data);

   /* Returns NULL if no glyph for this code is found. */
   const struct font_glyph *(*get_glyph)(void *data, uint32_t code);

   void (*free)(void *data);

   const char *(*get_default_font)(void);

   const char *ident;
   
   int (*get_line_height)(void* data);
} font_renderer_driver_t;

typedef struct
{
   const font_renderer_t *renderer;
   void *renderer_data;
   float size;
} font_data_t;

/* font_path can be NULL for default font. */
int font_renderer_create_default(const void **driver,
      void **handle, const char *font_path, unsigned font_size);
      
void font_driver_render_msg(video_frame_info_t *video_info,
      void *font_data, const char *msg, const void *params);

void font_driver_bind_block(void *font_data, void *block);

int font_driver_get_message_width(void *font_data, const char *msg, unsigned len, float scale);

void font_driver_flush(unsigned width, unsigned height, void *font_data);

void font_driver_free(void *font_data);

font_data_t *font_driver_init_first(void *video_data, const char *font_path,
      float font_size, bool threading_hint, enum font_driver_render_api api);

void font_driver_init_osd(void *video_data, bool threading_hint, enum font_driver_render_api api);
void font_driver_free_osd(void);

extern font_renderer_t gl_raster_font;
extern font_renderer_t libdbg_font;
extern font_renderer_t d3d_xbox360_font;
extern font_renderer_t d3d_xdk1_font;
extern font_renderer_t d3d_win32_font;
extern font_renderer_t vita2d_vita_font;
extern font_renderer_t ctr_font;
extern font_renderer_t vulkan_raster_font;
extern font_renderer_t caca_font;
extern font_renderer_t gdi_font;

extern font_renderer_driver_t stb_font_renderer;
extern font_renderer_driver_t stb_unicode_font_renderer;
extern font_renderer_driver_t freetype_font_renderer;
extern font_renderer_driver_t coretext_font_renderer;
extern font_renderer_driver_t bitmap_font_renderer;

RETRO_END_DECLS

#endif
