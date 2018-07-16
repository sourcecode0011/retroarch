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

#include <retro_miscellaneous.h>

#include <gfx/math/matrix_4x4.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../menu_display.h"

#include "../../retroarch.h"
#include "../../gfx/font_driver.h"
#include "../../gfx/video_context_driver.h"
#include "../../gfx/drivers/d3d.h"
#include "../../gfx/common/d3d_common.h"

#ifdef HAVE_D3D9
#include "../../gfx/include/d3d9/d3dx9math.h"
#endif

#define BYTE_CLAMP(i) (int) ((((i) > 255) ? 255 : (((i) < 0) ? 0 : (i))))

static const float d3d_vertexes[] = {
   0, 0,
   1, 0,
   0, 1,
   1, 1
};

static const float d3d_tex_coords[] = {
   0, 1,
   1, 1,
   0, 0,
   1, 0
};

static const float *menu_display_d3d_get_default_vertices(void)
{
   return &d3d_vertexes[0];
}

static const float *menu_display_d3d_get_default_tex_coords(void)
{
   return &d3d_tex_coords[0];
}

static void *menu_display_d3d_get_default_mvp(void)
{
#ifndef _XBOX
   static math_matrix_4x4 default_mvp;
   D3DXMATRIX ortho, mvp;
#endif
   d3d_video_t *d3d = (d3d_video_t*)video_driver_get_ptr(false);

   if (!d3d)
      return NULL;
#ifdef _XBOX
   return NULL; /* TODO/FIXME */
#else
   D3DXMatrixOrthoOffCenterLH(&ortho, 0,
         d3d->final_viewport.Width, 0, d3d->final_viewport.Height, 0, 1);
   D3DXMatrixTranspose(&mvp, &ortho);
   memcpy(default_mvp.data, (FLOAT*)mvp, sizeof(default_mvp.data));

   return &default_mvp;
#endif
}

static unsigned menu_display_prim_to_d3d_enum(
      enum menu_display_prim_type prim_type)
{
   switch (prim_type)
   {
      case MENU_DISPLAY_PRIM_TRIANGLES:
      case MENU_DISPLAY_PRIM_TRIANGLESTRIP:
         return D3DPT_TRIANGLESTRIP;
      case MENU_DISPLAY_PRIM_NONE:
      default:
         break;
   }

   return 0;
}

static void menu_display_d3d_blend_begin(void)
{
   d3d_video_t *d3d = (d3d_video_t*)video_driver_get_ptr(false);

   if (!d3d)
      return;

   d3d_enable_blend_func(d3d->dev);
}

static void menu_display_d3d_blend_end(void)
{
   d3d_video_t *d3d = (d3d_video_t*)video_driver_get_ptr(false);

   if (!d3d)
      return;

   d3d_disable_blend_func(d3d->dev);
}

static void menu_display_d3d_viewport(void *data)
{
   D3DVIEWPORT                vp = {0};
   d3d_video_t              *d3d = (d3d_video_t*)video_driver_get_ptr(false);
   menu_display_ctx_draw_t *draw = (menu_display_ctx_draw_t*)data;

   if (!d3d || !draw)
      return;

   vp.X      = draw->x;
   vp.Y      = draw->y;
   vp.Width  = draw->width;
   vp.Height = draw->height;
   vp.MinZ   = 0.0f;
   vp.MaxZ   = 1.0f;

   d3d_set_viewports(d3d->dev, &vp);
}

static void menu_display_d3d_bind_texture(void *data)
{
   d3d_video_t              *d3d = (d3d_video_t*)video_driver_get_ptr(false);
   menu_display_ctx_draw_t *draw = (menu_display_ctx_draw_t*)data;
   
   if (!d3d || !draw)
      return;

   d3d_set_texture(d3d->dev, 0, (LPDIRECT3DTEXTURE)draw->texture);
   d3d_set_sampler_address_u(d3d->dev, 0, D3DTADDRESS_BORDER);
   d3d_set_sampler_address_v(d3d->dev, 0, D3DTADDRESS_BORDER);
   d3d_set_sampler_minfilter(d3d->dev, 0, D3DTEXF_LINEAR);
   d3d_set_sampler_magfilter(d3d->dev, 0, D3DTEXF_LINEAR);
}

static void menu_display_d3d_draw(void *data)
{
#if 0
   math_matrix_4x4          *mat = NULL;
#endif
   d3d_video_t              *d3d = (d3d_video_t*)video_driver_get_ptr(false);
   menu_display_ctx_draw_t *draw = (menu_display_ctx_draw_t*)data;

   if (!d3d || !draw)
      return;
   
   if (!draw->coords->vertex)
      draw->coords->vertex        = menu_display_d3d_get_default_vertices();
   if (!draw->coords->tex_coord)
      draw->coords->tex_coord     = menu_display_d3d_get_default_tex_coords();
   if (!draw->coords->lut_tex_coord)
      draw->coords->lut_tex_coord = menu_display_d3d_get_default_tex_coords();

   menu_display_d3d_viewport(draw);
   menu_display_d3d_bind_texture(draw);

#if 0
   mat = (math_matrix_4x4*)draw->matrix_data;
   if (!mat)
      mat                         = (math_matrix_4x4*)
         menu_display_d3d_get_default_mvp();
   video_shader_driver_set_coords(draw->coords);
   video_shader_driver_set_mvp(mat);
#endif

   d3d_draw_primitive(d3d->dev, (D3DPRIMITIVETYPE)
         menu_display_prim_to_d3d_enum(draw->prim_type),
         0, draw->coords->vertices);
}

static void menu_display_d3d_draw_pipeline(void *data)
{
#if defined(HAVE_HLSL) || defined(HAVE_CG)
   video_shader_ctx_info_t shader_info;
   menu_display_ctx_draw_t *draw     = (menu_display_ctx_draw_t*)data;
   struct uniform_info uniform_param = {0};
   static float t                    = 0;
   video_coord_array_t *ca             = NULL;

   ca = menu_display_get_coords_array();

   draw->x           = 0;
   draw->y           = 0;
   draw->coords      = (struct video_coords*)(&ca->coords);
   draw->matrix_data = NULL;

   switch (draw->pipeline.id)
   {
      case VIDEO_SHADER_MENU:
      case VIDEO_SHADER_MENU_2:
      case VIDEO_SHADER_MENU_3:
         shader_info.data                = NULL;
         shader_info.idx                 = draw->pipeline.id;
         shader_info.set_active          = true;

         video_shader_driver_use(shader_info);

         t += 0.01;

         uniform_param.enabled           = true;
         uniform_param.lookup.enable     = true;
         uniform_param.lookup.add_prefix = true;
         uniform_param.lookup.idx        = draw->pipeline.id;
         uniform_param.lookup.type       = SHADER_PROGRAM_VERTEX;
         uniform_param.type              = UNIFORM_1F;
         uniform_param.lookup.ident      = "time";
         uniform_param.result.f.v0       = t;

         video_shader_driver_set_parameter(uniform_param);
         break;
   }
#endif
}

static void menu_display_d3d_restore_clear_color(void)
{
   d3d_video_t *d3d = (d3d_video_t*)video_driver_get_ptr(false);
   DWORD    clear_color = 0x00000000;

   d3d_clear(d3d->dev, 0, NULL, D3DCLEAR_TARGET, clear_color, 0, 0);
}

static void menu_display_d3d_clear_color(menu_display_ctx_clearcolor_t *clearcolor)
{
   DWORD    clear_color                      = 0;
   d3d_video_t *d3d = (d3d_video_t*)video_driver_get_ptr(false);

   if (!d3d || !clearcolor)
      return;
   
   clear_color = D3DCOLOR_ARGB(
         BYTE_CLAMP(clearcolor->a * 255.0f), /* A */
         BYTE_CLAMP(clearcolor->r * 255.0f), /* R */
         BYTE_CLAMP(clearcolor->g * 255.0f), /* G */
         BYTE_CLAMP(clearcolor->b * 255.0f)  /* B */
         );

   d3d_clear(d3d->dev, 0, NULL, D3DCLEAR_TARGET, clear_color, 0, 0);
}

static bool menu_display_d3d_font_init_first(
      void **font_handle, void *video_data,
      const char *font_path, float font_size)
{
   font_data_t **handle = (font_data_t**)font_handle;
   *handle = font_driver_init_first(video_data,
         font_path, font_size, true, FONT_DRIVER_RENDER_DIRECT3D_API);
   return *handle;
}

menu_display_ctx_driver_t menu_display_ctx_d3d = {
   menu_display_d3d_draw,
   menu_display_d3d_draw_pipeline,
   menu_display_d3d_viewport,
   menu_display_d3d_blend_begin,
   menu_display_d3d_blend_end,
   menu_display_d3d_restore_clear_color,
   menu_display_d3d_clear_color,
   menu_display_d3d_get_default_mvp,
   menu_display_d3d_get_default_vertices,
   menu_display_d3d_get_default_tex_coords,
   menu_display_d3d_font_init_first,
   MENU_VIDEO_DRIVER_DIRECT3D,
   "menu_display_d3d",
};
