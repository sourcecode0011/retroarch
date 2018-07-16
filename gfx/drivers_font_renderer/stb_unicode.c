/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2015-2014 - Hans-Kristian Arntzen
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

#include <ctype.h>

#include <file/file_path.h>
#include <streams/file_stream.h>
#include <retro_miscellaneous.h>

#include "../font_driver.h"
#include "../../verbosity.h"

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STBTT_STATIC
#define STBRP_STATIC
#define static static INLINE
#include "../../deps/stb/stb_rect_pack.h"
#include "../../deps/stb/stb_truetype.h"
#undef static
#endif

typedef struct
{
   uint8_t *font_data;
   stbtt_fontinfo info;

   int max_glyph_width;
   int max_glyph_height;
   int line_height;
   float scale_factor;
   struct font_atlas atlas;
   struct font_glyph glyphs[256];
   uint8_t uc_to_id[0x10000];
   uint16_t id_to_uc[256];
   unsigned last_used[256];
   unsigned usage_counter;

} stb_unicode_font_renderer_t;

static struct font_atlas *font_renderer_stb_unicode_get_atlas(void *data)
{
   stb_unicode_font_renderer_t *self = (stb_unicode_font_renderer_t*)data;
   return &self->atlas;
}

static void font_renderer_stb_unicode_free(void *data)
{
   stb_unicode_font_renderer_t *self = (stb_unicode_font_renderer_t*)data;

   free(self->atlas.buffer);
   free(self->font_data);
   free(self);
}

static unsigned font_renderer_stb_unicode_get_slot(stb_unicode_font_renderer_t *handle)
{
   int i;
   unsigned oldest = 1;

   for (i = 2; i < 256; i++)
      if(handle->last_used[i] < handle->last_used[oldest])
         oldest = i;

   handle->uc_to_id[handle->id_to_uc[oldest]] = 0;
   handle->id_to_uc[oldest] = 0;
   return oldest;
}

static uint32_t font_renderer_stb_unicode_update_atlas(
      stb_unicode_font_renderer_t *self, uint32_t charcode)
{
   int advance_width, left_side_bearing;
   int id, glyph_index, offset_x, offset_y;
   struct font_glyph *glyph = NULL;
   uint8_t *dst             = NULL;
   int x0                   = 0;
   int y1                   = 0;

   if(charcode > 0xFFFF)
      return 0;

   if(self->uc_to_id[charcode])
      return self->uc_to_id[charcode];

   id                       = font_renderer_stb_unicode_get_slot(self);
   self->id_to_uc[id]       = charcode;
   self->uc_to_id[charcode] = id;
   self->atlas.dirty        = true;

   glyph                    = &self->glyphs[id];

   glyph_index              = stbtt_FindGlyphIndex(&self->info, charcode);

   offset_x                 = (id % 16) * self->max_glyph_width;
   offset_y                 = (id / 16) * self->max_glyph_height;

   dst                      = self->atlas.buffer + offset_x + offset_y 
      * self->atlas.width;

   stbtt_MakeGlyphBitmap(&self->info, dst, self->max_glyph_width, self->max_glyph_height,
         self->atlas.width, self->scale_factor, self->scale_factor, glyph_index);

   stbtt_GetGlyphHMetrics(&self->info, glyph_index, &advance_width, &left_side_bearing);
   stbtt_GetGlyphBox(&self->info, glyph_index, &x0, NULL, NULL, &y1);

   glyph->advance_x      = advance_width * self->scale_factor;
   glyph->atlas_offset_x = offset_x;
   glyph->atlas_offset_y = offset_y;
   glyph->draw_offset_x  = x0 * self->scale_factor;
   glyph->draw_offset_y  = - y1 * self->scale_factor;
   glyph->width          = self->max_glyph_width;
   glyph->height         = self->max_glyph_height;

   return id;
}

static bool font_renderer_stb_unicode_create_atlas(
      stb_unicode_font_renderer_t *self, float font_size)
{
   int i;

   self->max_glyph_width  = font_size < 0 ? -font_size : font_size;
   self->max_glyph_height = font_size < 0 ? -font_size : font_size;
   self->atlas.width      = self->max_glyph_width * 16;
   self->atlas.height     = self->max_glyph_height * 16;
   self->atlas.buffer     = (uint8_t*)calloc(self->atlas.height, self->atlas.width);

   if (!self->atlas.buffer)
      return false;

   self->usage_counter = 1;

   for (i = 0; i < 256; ++i)
   {
      int id = font_renderer_stb_unicode_update_atlas(self, i);
      if(id)
         self->last_used[id] = self->usage_counter++;

   }

   return true;
}

static const struct font_glyph *font_renderer_stb_unicode_get_glyph(
      void *data, uint32_t code)
{
   unsigned id;
   stb_unicode_font_renderer_t *self = (stb_unicode_font_renderer_t*)data;

   if(!self || code > 0xFFFF)
      return NULL;

   id = self->uc_to_id[code];

   if(!id)
      id = font_renderer_stb_unicode_update_atlas(self, code);

   self->last_used[id] = self->usage_counter++;

   return &self->glyphs[id];
}

static void *font_renderer_stb_unicode_init(const char *font_path, float font_size)
{
   int ascent, descent, line_gap;
   stb_unicode_font_renderer_t *self = 
      (stb_unicode_font_renderer_t*)calloc(1, sizeof(*self));

   if (!self || font_size < 1.0)
      goto error;

   /* See https://github.com/nothings/stb/blob/master/stb_truetype.h#L539 */
   font_size = STBTT_POINT_SIZE(font_size);

   if (!filestream_read_file(font_path, (void**)&self->font_data, NULL))
      goto error;

   if (!stbtt_InitFont(&self->info, self->font_data,
            stbtt_GetFontOffsetForIndex(self->font_data, 0)))
      goto error;

   stbtt_GetFontVMetrics(&self->info, &ascent, &descent, &line_gap);

   if (font_size < 0)
      self->scale_factor = stbtt_ScaleForMappingEmToPixels(&self->info, -font_size);
   else
      self->scale_factor = stbtt_ScaleForPixelHeight(&self->info, font_size);

   self->line_height  = (ascent - descent) * self->scale_factor;

   if (!font_renderer_stb_unicode_create_atlas(self, font_size))
      goto error;

   return self;

error:
   if (self)
      font_renderer_stb_unicode_free(self);
   return NULL;
}

static const char *font_renderer_stb_unicode_get_default_font(void)
{
   static const char *paths[] = {
#if defined(_WIN32)
      "C:\\Windows\\Fonts\\consola.ttf",
      "C:\\Windows\\Fonts\\verdana.ttf",
#elif defined(__APPLE__)
      "/Library/Fonts/Microsoft/Candara.ttf",
      "/Library/Fonts/Verdana.ttf",
      "/Library/Fonts/Tahoma.ttf",
      "/Library/Fonts/Andale Mono.ttf",
      "/Library/Fonts/Courier New.ttf",
#elif defined(__ANDROID_API__)
      "/system/fonts/DroidSansMono.ttf",
      "/system/fonts/CutiveMono.ttf",
      "/system/fonts/DroidSans.ttf",
#elif defined(VITA)
      "vs0:data/external/font/pvf/c041056ts.ttf",
      "vs0:data/external/font/pvf/d013013ds.ttf",
      "vs0:data/external/font/pvf/e046323ms.ttf",
      "vs0:data/external/font/pvf/e046323ts.ttf",
      "vs0:data/external/font/pvf/k006004ds.ttf",
      "vs0:data/external/font/pvf/n023055ms.ttf",
      "vs0:data/external/font/pvf/n023055ts.ttf",
#else
      "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
      "/usr/share/fonts/TTF/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf",
      "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
#endif
      "osd-font.ttf",
      NULL
   };

   const char **p;

   for (p = paths; *p; ++p)
      if (path_file_exists(*p))
         return *p;

   return NULL;
}

static int font_renderer_stb_unicode_get_line_height(void* data)
{
   stb_unicode_font_renderer_t *handle = (stb_unicode_font_renderer_t*)data;
   return handle->line_height;
}

font_renderer_driver_t stb_unicode_font_renderer = {
   font_renderer_stb_unicode_init,
   font_renderer_stb_unicode_get_atlas,
   font_renderer_stb_unicode_get_glyph,
   font_renderer_stb_unicode_free,
   font_renderer_stb_unicode_get_default_font,
   "stb-unicode",
   font_renderer_stb_unicode_get_line_height,
};
