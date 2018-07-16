/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2016 - Higor Euripedes
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C) 2014-2015 - Jean-André Santoni
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

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <file/file_path.h>
#include <lists/dir_list.h>
#include <compat/posix_string.h>
#include <gfx/math/matrix_4x4.h>
#include <string/stdstring.h>
#include <formats/image.h>
#include <compat/strl.h>
#include <retro_stat.h>
#include <string/stdstring.h>

#include "menu_generic.h"

#include "../../config.def.h"

#include "../../list_special.h"
#include "../../file_path_special.h"

#include "../menu_driver.h"
#include "../menu_animation.h"
#include "../widgets/menu_entry.h"
#include "../menu_display.h"
#include "../menu_navigation.h"
#include "../../retroarch.h"

#include "../../gfx/font_driver.h"

#include "../../core_info.h"
#include "../../configuration.h"
#include "../../runloop.h"
#include "../../verbosity.h"
#include "../../tasks/tasks_internal.h"

#define ZUI_FG_NORMAL         (~0)
#define ZUI_ITEM_SIZE_PX      54

enum zarch_zui_input_state
{
    MENU_ZARCH_MOUSE_X = 0,
    MENU_ZARCH_MOUSE_Y,
    MENU_POINTER_ZARCH_X,
    MENU_POINTER_ZARCH_Y,
    MENU_ZARCH_PRESSED
};

enum zarch_layout_type
{
   LAY_HOME = 0,
   LAY_PICK_CORE,
   LAY_SETTINGS
};

static const float zui_bg_panel[] = {
   0, 0, 0, 0.25,
   0, 0, 0, 0.25,
   0, 0, 0, 0.25,
   0, 0, 0, 0.25,
};

static const float zui_bg_screen[] = {
   0.07, 0.19, 0.26, 0.75,
   0.07, 0.19, 0.26, 0.75,
   0.15, 0.31, 0.47, 0.75,
   0.15, 0.31, 0.47, 0.75,
};

static const float zui_bg_hilite[] = {
   0.22, 0.60, 0.74, 1,
   0.22, 0.60, 0.74, 1,
   0.22, 0.60, 0.74, 1,
   0.22, 0.60, 0.74, 1,
};

static const float zui_bg_pad_hilite[] = {
   0.30, 0.76, 0.93, 1,
   0.30, 0.76, 0.93, 1,
   0.30, 0.76, 0.93, 1,
   0.30, 0.76, 0.93, 1,
};

typedef struct zarch_handle
{
   enum menu_action action;
   bool rendering;
   math_matrix_4x4 mvp;
   unsigned width;
   unsigned height;
   video_font_raster_block_t tmp_block;
   unsigned hash;

   struct {
      unsigned active;
      unsigned hot;
   } item;

   struct
   {
      int    wheel;
   } mouse;

   /* LAY_ROOT's "Load" file browser */
   struct string_list *load_dlist;
   char *load_cwd;
   int  load_dlist_first;

   struct
   {
      menu_texture_item bg;
   } textures;

   /* LAY_ROOT's "Recent" */

   int  recent_dlist_first;

   /* LAY_PICK_CORE */
   int pick_first;
   char pick_content[PATH_MAX_LENGTH];
   const core_info_t *pick_cores;
   size_t pick_supported;

   void *font;
   int font_size;
   int header_height;
   unsigned next_id;
   unsigned prev_id;
   bool     next_selection_set;
} zui_t;

struct zui_tabbed
{
   unsigned prev_id;
   unsigned active_id;
   unsigned next_id;
   int x, y;
   int width;
   int height; /* unused */
   int tabline_size;
   bool update_width;
   bool vertical;
   int tab_width;
   unsigned tab_selection;
   bool inited;
};


static enum zarch_layout_type zarch_layout = LAY_HOME;

static float zarch_zui_strwidth(void *fb_buf, const char *text, float scale)
{
   return font_driver_get_message_width(fb_buf, text, strlen(text), scale);
}

static int16_t zarch_zui_input_state(zui_t *zui, enum zarch_zui_input_state state)
{
   switch (state)
   {
      case MENU_ZARCH_MOUSE_X:
         return menu_input_mouse_state(MENU_MOUSE_X_AXIS);
      case MENU_ZARCH_MOUSE_Y:
         return menu_input_mouse_state(MENU_MOUSE_Y_AXIS);
      case MENU_POINTER_ZARCH_X:
         return menu_input_pointer_state(MENU_POINTER_X_AXIS);
      case MENU_POINTER_ZARCH_Y:
         return menu_input_pointer_state(MENU_POINTER_Y_AXIS);
      case MENU_ZARCH_PRESSED:
         if (     menu_input_mouse_state(MENU_MOUSE_LEFT_BUTTON) 
               || menu_input_pointer_state(MENU_POINTER_PRESSED))
            return 1;
         if (zui->action == MENU_ACTION_OK)
            return 1;
         break;
   }

   return 0;
}

static bool zarch_zui_check_button_down(zui_t *zui,
      unsigned id, int x1, int y1, int x2, int y2)
{
   menu_input_ctx_hitbox_t hitbox;

   hitbox.x1   = x1;
   hitbox.x2   = x2;
   hitbox.y1   = y1;
   hitbox.y2   = y2;

   if (menu_input_mouse_check_vector_inside_hitbox(&hitbox))
      zui->item.hot = id;

   if (     zui->item.hot == id 
         && zarch_zui_input_state(zui, MENU_ZARCH_PRESSED))
   {
      zui->item.active = id;
      return true;
   }

   return false;
}

static bool zarch_zui_check_button_up(zui_t *zui,
      unsigned id, int x1, int y1, int x2, int y2)
{
   menu_input_ctx_hitbox_t hitbox;
   bool result = false;

   hitbox.x1   = x1;
   hitbox.x2   = x2;
   hitbox.y1   = y1;
   hitbox.y2   = y2;

   if (menu_input_mouse_check_vector_inside_hitbox(&hitbox))
      zui->item.hot = id;

   if (     zui->item.active == id 
         && !zarch_zui_input_state(zui, MENU_ZARCH_PRESSED))
   {
      if (zui->item.hot == id)
         result = true;

      zui->item.active = 0;
   }

   zarch_zui_check_button_down(zui, id, x1, y1, x2, y2);

   return result;
}

static unsigned zarch_zui_hash(zui_t *zui, const char *s)
{
   unsigned hval = zui->hash;

   while(*s != 0)
   {
      hval+=*s++;
      hval+=(hval<<10);
      hval^=(hval>>6);
      hval+=(hval<<3);
      hval^=(hval>>11);
      hval+=(hval<<15);
   }
   return zui->hash = hval;
}

static void zarch_zui_draw_text(zui_t *zui,
      uint32_t color, int x, int y, const char *text)
{
   struct font_params params;

   if (!zui || !zui->font || string_is_empty(text))
      return;

   /* need to use height-y because the font renderer 
    * uses a different model-view-projection (MVP). */
   params.x           = x / (float)zui->width;
   params.y           = (zui->height - y) / (float)zui->height;
   params.scale       = 1.0f;
   params.drop_mod    = 0.0f;
   params.drop_x      = 0.0f;
   params.drop_y      = 0.0f;
   params.color       = color;
   params.full_screen = true;
   params.text_align  = TEXT_ALIGN_LEFT;

   video_driver_set_osd_msg(text, &params, zui->font);
}

static bool zarch_zui_button_full(zui_t *zui,
      int x1, int y1, int x2, int y2, const char *label)
{
   unsigned       id = zarch_zui_hash(zui, label);
   bool       active = zarch_zui_check_button_up(zui, id, x1, y1, x2, y2);
   const float *bg   = zui_bg_panel;

   if (zui->item.active == id || zui->item.hot == id)
      bg = zui_bg_hilite;

   menu_display_push_quad(zui->width, zui->height,  bg, x1, y1, x2, y2);
   zarch_zui_draw_text(zui, ZUI_FG_NORMAL, x1+12, y1 + 41, label);

   return active;
}

static bool zarch_zui_button(zui_t *zui, int x1, int y1, const char *label)
{
   if (!zui || !zui->font)
      return false;
   return zarch_zui_button_full(zui, x1, y1, x1 
         + zarch_zui_strwidth(zui->font, label, 1.0) + 24, y1 + 64, label);
}

static bool zarch_zui_list_item(video_frame_info_t *video_info,
      zui_t *zui, struct zui_tabbed *tab, int x1, int y1,
      const char *label, unsigned item_id, const char *entry, bool selected)
{
   menu_animation_ctx_ticker_t ticker;
   unsigned ticker_size;
   char title_buf[PATH_MAX_LENGTH];
   unsigned           id = zarch_zui_hash(zui, label);
   int                x2 = x1 + zui->width - 290 - 40;
   int                y2 = y1 + 50;
   bool           active = zarch_zui_check_button_up(zui, id, x1, y1, x2, y2);
   const float       *bg = zui_bg_panel;
   uint64_t frame_count  = video_info->frame_count;

   title_buf[0] = '\0';

   if (tab->active_id != tab->prev_id)
      tab->prev_id         = tab->active_id;

   if (selected)
   {
      zui->next_id            = item_id;
      zui->next_selection_set = true;
   }

   /* Set background color */
   if (zui->item.active == id || zui->item.hot == id)
      bg = zui_bg_hilite;
   else if (selected)
      bg = zui_bg_pad_hilite;

   ticker_size = x2 / 14;

   ticker.s        = title_buf;
   ticker.len      = ticker_size;
   ticker.idx      = frame_count / 50;
   ticker.str      = label;
   ticker.selected = (bg == zui_bg_hilite || bg == zui_bg_pad_hilite);

   menu_animation_ticker(&ticker);

   menu_display_push_quad(zui->width, zui->height, bg, x1, y1, x2, y2);
   zarch_zui_draw_text(zui, ZUI_FG_NORMAL, 12, y1 + 35, title_buf);

   if (entry)
      zarch_zui_draw_text(zui, ZUI_FG_NORMAL, x2 - 200, y1 + 35, entry);

   return active;
}

static void zarch_zui_tabbed_begin(zui_t *zui, struct zui_tabbed *tab, int x, int y)
{
   tab->x            = x;
   tab->y            = y;
   tab->tabline_size = 60 + 4;
}

static bool zarch_zui_tab(zui_t *zui, struct zui_tabbed *tab,
      const char *label, unsigned tab_id)
{
   bool active;
   int x1, y1, x2, y2;
   unsigned       id = zarch_zui_hash(zui, label);
   int         width = tab->tab_width;
   const float   *bg = zui_bg_panel;
   bool selected     = tab->tab_selection == tab_id; /* TODO/FIXME */

   if (!width)
   {
      if (!zui->font)
         return false;
      width          = zarch_zui_strwidth(zui->font, label, 1.0) + 24;
   }

   x1                = tab->x;
   y1                = tab->y;
   x2                = x1 + width;
   y2                = y1 + 60;
   active            = zarch_zui_check_button_up(zui, id, x1, y1, x2, y2);

   tab->prev_id      = tab->active_id;

   if (zui->item.active == id || tab->active_id == ~0U || !tab->inited)
      tab->active_id    = id;
   else if (id > tab->active_id)
      tab->next_id            = id;

   if (!tab->inited)
      tab->inited = true;

   if (tab->active_id == id || zui->item.active == id || zui->item.hot == id)
      bg             = zui_bg_hilite;
   else if (selected)
      bg             = zui_bg_pad_hilite;

   menu_display_push_quad(zui->width, zui->height,  bg, x1+0, y1+0, x2, y2);
   zarch_zui_draw_text(zui, ZUI_FG_NORMAL, x1+12, y1 + 41, label);

   if (tab->vertical)
      tab->y        += y2 - y1;
   else
      tab->x         = x2;

   return active || (tab->active_id == id);
}


static void zarch_zui_render_lay_settings(zui_t *zui)
{
   int width, x1, y1;
   static struct zui_tabbed tabbed = {~0U};

   tabbed.vertical            = true;
   tabbed.tab_width           = 100;

   zarch_zui_tabbed_begin(zui, &tabbed, zui->width - 100, 20);

   width                      = 290;
   x1                         = zui->width - width - 20;
   y1                         = 20;
   y1                        += 64;

   if (zarch_zui_button_full(zui, x1, y1, x1 + width, y1 + 64, "Back"))
      zarch_layout = LAY_HOME;
}

static bool zarch_zui_gamepad_input(zui_t *zui,
      int *gamepad_index, int *list_first,
      unsigned skip)
{
   unsigned size          = menu_entries_get_size();
   unsigned cutoff_point  = size - 5;

   switch (zui->action)
   {
      case MENU_ACTION_LEFT:
         if (*gamepad_index == 0)
            break;

         *gamepad_index = *gamepad_index - 5;

         if (*gamepad_index < 0)
            *gamepad_index = 0;
         return true;
      case MENU_ACTION_RIGHT:
         if (*gamepad_index == (signed)(size-1))
            break;

         *gamepad_index = *gamepad_index + 5;

         if (*gamepad_index > (signed)(size-1))
            *gamepad_index   = (size -1);

         return true;
      case MENU_ACTION_UP:
         *gamepad_index = *gamepad_index - 1;

         if (*gamepad_index < 0) /* and wraparound enabled */
            *gamepad_index = size -1;
         else if (*gamepad_index >= (signed)cutoff_point) /* if greater than cutoff point, 
                                                don't scroll */
            return false;

         return true;
      case MENU_ACTION_DOWN:
         *gamepad_index = *gamepad_index + 1;

         if (*gamepad_index > (signed)(size - 1)) /* and wraparound enabled */
            *gamepad_index = 0;
         else if (*gamepad_index >= (signed)cutoff_point) /* if greater than cutoff point, 
                                                don't scroll */
            return false;
         return true;
      default:
         {
            *list_first += zui->mouse.wheel;
            if (*list_first < 0)
               *list_first = 0;
            if (*list_first > (int)cutoff_point)
               *list_first = cutoff_point;

            *list_first = MIN(MAX(*list_first, 0), cutoff_point - skip);
         }
         return false;
   }

   return false;
}

static int zarch_zui_render_lay_root_recent(
      video_frame_info_t *video_info,
      zui_t *zui, struct zui_tabbed *tabbed)
{
   if (zarch_zui_tab(zui, tabbed, "Recent", 0))
   {
      static int gamepad_index = 0;
      unsigned size = menu_entries_get_size();
      unsigned i, j = 0;

      if (zarch_zui_gamepad_input(zui, &gamepad_index,
               &zui->recent_dlist_first, 0))
         zui->recent_dlist_first = gamepad_index;

      for (i = zui->recent_dlist_first; i < size; ++i)
      {
         char rich_label[PATH_MAX_LENGTH];
         char entry_value[PATH_MAX_LENGTH];
         menu_entry_t entry                = {{0}};

         rich_label[0] = entry_value[0]    = '\0';

         menu_entry_get(&entry, 0, i, NULL, true);
         menu_entry_get_rich_label(i, rich_label, sizeof(rich_label));
         menu_entry_get_value(i, NULL, entry_value,sizeof(entry_value));

         if (zarch_zui_list_item(
                  video_info,
                  zui, tabbed, 0, 
                  tabbed->tabline_size + j * ZUI_ITEM_SIZE_PX,
                  rich_label, i, entry_value, gamepad_index == (signed)i))
         {
            if (menu_entry_action(&entry, i, MENU_ACTION_OK))
               return 1;
         }

         j++;
      }

   }

   return 0;
}

static void zarch_zui_render_lay_root_load_free(zui_t *zui)
{
   if (!zui)
      return;

   free(zui->load_cwd);
   dir_list_free(zui->load_dlist);
   zui->load_cwd   = NULL;
   zui->load_dlist = NULL;
}

static void zarch_zui_render_lay_root_load_set_new_path(zui_t *zui,
      const char *newpath)
{
   if (!zui)
      return;

   free(zui->load_cwd);
   zui->load_cwd = strdup(newpath);
   dir_list_free(zui->load_dlist);
   zui->load_dlist = NULL;
}

static int zarch_zui_render_lay_root_load(
      video_frame_info_t *video_info,
      zui_t *zui,
      struct zui_tabbed *tabbed)
{
   char parent_dir[PATH_MAX_LENGTH];
   settings_t           *settings   = config_get_ptr();
   core_info_list_t           *list = NULL;

   parent_dir[0] = '\0';

   if (zarch_zui_tab(zui, tabbed, "Load", 1))
   {
      unsigned cwd_offset;

      if (!zui->load_cwd)
         zui->load_cwd = strdup(settings->directory.menu_content);

      if (!zui->load_dlist)
      {
         core_info_t *core_info = NULL;
         core_info_get_current_core(&core_info);

         zui->load_dlist = dir_list_new(zui->load_cwd,
               core_info->supported_extensions, true, true, false, true);
         dir_list_sort(zui->load_dlist, true);
         zui->load_dlist_first  = 0;
      }

      cwd_offset = MIN(strlen(zui->load_cwd), 60);

      zarch_zui_draw_text(zui, ZUI_FG_NORMAL, 15,
            tabbed->tabline_size + 5 + 41,
            &zui->load_cwd[strlen(zui->load_cwd) - cwd_offset]);

      if (zarch_zui_button(zui, zui->width - 290 - 129,
               tabbed->tabline_size + 5, "Home"))
         zarch_zui_render_lay_root_load_free(zui);

      if (zui->load_dlist)
      {
         fill_pathname_parent_dir(parent_dir,
               zui->load_cwd, sizeof(parent_dir));
         if (!string_is_empty(parent_dir) &&
               zarch_zui_list_item(
                  video_info,
                  zui, tabbed, 0,
                  tabbed->tabline_size + 73, " ..", 0, NULL, false /* TODO/FIXME */))
         {
            zarch_zui_render_lay_root_load_set_new_path(zui, parent_dir);
         }
         else
         {
            static int gamepad_index = 0;
            unsigned size = zui->load_dlist->size;
            unsigned i, j = 1;
            unsigned skip = 0;

            for (i = 0; i < size; ++i)
            {
               const char *basename = 
                  path_basename(zui->load_dlist->elems[i].data);
               if (basename[0] != '.')
                  break;
               skip++;
            }

            if (zarch_zui_gamepad_input(zui, &gamepad_index,
                     &zui->load_dlist_first, skip))
               zui->load_dlist_first = gamepad_index;

            for (i = skip + zui->load_dlist_first; i < size; ++i)
            {
               char label[PATH_MAX_LENGTH];
               const char        *path     = NULL;
               const char        *basename = NULL;

               if (j > 10)
                  break;

               label[0] = '\0';

               path     = zui->load_dlist->elems[i].data;
               basename = path_basename(path);

               *label = 0;
               strncat(label, "  ", sizeof(label)-1);
               strncat(label, basename, sizeof(label)-1);

               if (path_is_directory(path))
                  strncat(label, "/", sizeof(label)-1);

               if (zarch_zui_list_item(
                        video_info,
                        zui, tabbed, 0,
                        tabbed->tabline_size + 73 + j * ZUI_ITEM_SIZE_PX,
                        label, i, NULL, gamepad_index == (signed)(i-skip)))
               {
                  if (path_is_directory(path))
                  {
                     zarch_zui_render_lay_root_load_set_new_path(zui, path);
                     break;
                  }

                  zui->pick_cores     = NULL;
                  zui->pick_supported = 0;
                  strlcpy(zui->pick_content,
                        path, sizeof(zui->pick_content));

                  core_info_get_list(&list);

                  core_info_list_get_supported_cores(list, path,
                        &zui->pick_cores, &zui->pick_supported);
                  zarch_layout = LAY_PICK_CORE;
                  break;
               }
               j++;
            }
         }
      }
   }
   else if (zui->load_dlist)
   {
      dir_list_free(zui->load_dlist);
      zui->load_dlist = NULL;
   }

   return 0;
}

static int zarch_zui_render_lay_root_collections(
      zui_t *zui, struct zui_tabbed *tabbed)
{
   if (zarch_zui_tab(zui, tabbed, "Collections", 2))
   {
      /* STUB/FIXME */
   }

   return 0;
}

static int zarch_zui_render_lay_root_downloads(
      zui_t *zui, struct zui_tabbed *tabbed)
{
   if (zarch_zui_tab(zui, tabbed, "Download", 3))
   {
      /* STUB/FIXME */
   }

   return 0;
}

static int zarch_zui_render_lay_root(video_frame_info_t *video_info,
      zui_t *zui)
{
   char item[PATH_MAX_LENGTH];
   static struct zui_tabbed tabbed = {~0U};

   zarch_zui_tabbed_begin(zui, &tabbed, 0, 0);

   tabbed.width            = zui->width - 290 - 40;
   zui->next_selection_set = false;

   if (zarch_zui_render_lay_root_recent(video_info, zui, &tabbed))
      return 0;
   if (zarch_zui_render_lay_root_load(video_info, zui, &tabbed))
      return 0;
   if (zarch_zui_render_lay_root_collections(zui, &tabbed))
      return 0;
   if (zarch_zui_render_lay_root_downloads(zui, &tabbed))
      return 0;

#ifdef ZARCH_DEBUG
   item[0] = '\0';

   snprintf(item, sizeof(item), "item id: %d\n", zui->active_id);
   zarch_zui_draw_text(zui, ZUI_FG_NORMAL, 1600 +12, 300 + 41, item); 
   snprintf(item, sizeof(item), "tab  idx: %d\n", tabbed.active_id);
   zarch_zui_draw_text(zui, ZUI_FG_NORMAL, 1600 +12, 300 + 81, item); 
   snprintf(item, sizeof(item), "item hot idx: %d\n", zui->item.hot);
   zarch_zui_draw_text(zui, ZUI_FG_NORMAL, 1600 +12, 300 + 111, item); 
#endif

   menu_display_push_quad(zui->width, zui->height,
         zui_bg_hilite, 0, 60, zui->width - 290 - 40, 60+4);

   return 0;
}

static int zarch_zui_render_sidebar(zui_t *zui)
{
   int width, x1, y1;
   static struct zui_tabbed tabbed = {~0U};
   tabbed.vertical = true;
   tabbed.tab_width = 100;

   zarch_zui_tabbed_begin(zui, &tabbed, zui->width - 100, 20);

   width = 290;
   x1    = zui->width - width - 20;
   y1    = 20;

   if (zarch_zui_button_full(zui, x1, y1, x1 + width, y1 + 64, "Settings"))
      zarch_layout = LAY_SETTINGS;

   y1 += 64;

   if (zarch_zui_button_full(zui, x1, y1, x1 + width, y1 + 64, "Exit"))
   {
      menu_driver_ctl(RARCH_MENU_CTL_SET_PENDING_SHUTDOWN, NULL);
      return 1;
   }

   return 0;
}

static int zarch_zui_load_content(zui_t *zui, unsigned i)
{
   content_ctx_info_t content_info = {0};

   task_push_content_load_default(zui->pick_cores[i].path,
         zui->pick_content,
         &content_info,
         CORE_TYPE_PLAIN,
         CONTENT_MODE_LOAD_CONTENT_WITH_NEW_CORE_FROM_MENU,
         NULL, NULL);

   zarch_layout = LAY_HOME;

   return 0;
}

static void zarch_zui_draw_cursor(float x, float y)
{
}

static int zarch_zui_render_pick_core(video_frame_info_t *video_info,
      zui_t *zui)
{
   static struct zui_tabbed tabbed = {~0U};
   unsigned i, j = 0;
   if (zui->pick_supported == 1)
   {
      int ret = zarch_zui_load_content(zui, 0);

      (void)ret;

      zarch_layout = LAY_HOME;
      menu_driver_ctl(RARCH_MENU_CTL_SET_PENDING_QUIT, NULL);
      return 1;
   }

   zarch_zui_draw_text(zui, ~0, 8, 18, "Select a core: ");

   if (zarch_zui_button(zui, 0, 18 + zui->font_size, "<- Back"))
      zarch_layout = LAY_HOME;

   if (!zui->pick_supported)
   {
      zarch_zui_list_item(
            video_info,
            zui, &tabbed, 0, ZUI_ITEM_SIZE_PX,
            "Content unsupported", 0, NULL, false /* TODO/FIXME */);
      return 1;
   }

   zui->pick_first += zui->mouse.wheel;

   zui->pick_first = MIN(MAX(zui->pick_first, 0), zui->pick_supported - 5);

   for (i = zui->pick_first; i < zui->pick_supported; ++i)
   {
      if (j > 10)
         break;

      if (zarch_zui_list_item(
               video_info,
               zui, &tabbed, 0, ZUI_ITEM_SIZE_PX + j * ZUI_ITEM_SIZE_PX,
               zui->pick_cores[i].display_name, i, NULL, false))
      {
         int ret = zarch_zui_load_content(zui, i);

         (void)ret;

         zarch_layout = LAY_HOME;

         menu_driver_ctl(RARCH_MENU_CTL_SET_PENDING_QUIT, NULL);
         break;
      }
      j++;
   }

   return 0;
}

static void zarch_frame(void *data, video_frame_info_t *video_info)
{
   unsigned i;
   float coord_color[16];
   float coord_color2[16];
   menu_display_ctx_draw_t draw;
   menu_display_ctx_coord_draw_t coord_draw;
   settings_t *settings    = config_get_ptr();
   zui_t *zui              = (zui_t*)data;
   video_coord_array_t *ca = menu_display_get_coords_array();
   
   if (!zui)
      return;

   video_driver_get_size(&zui->width, &zui->height);

   menu_display_set_viewport(video_info->width, video_info->height);

   for (i = 0; i < 16; i++)
   {
      coord_color[i]  = 0;
      coord_color2[i] = 2.0f;

      if (i == 3 || i == 7 || i == 11 || i == 15)
      {
         coord_color[i]  = 0.10f;
         coord_color2[i] = 0.10f;
      }
   }

   zui->rendering = true;
   zui->hash      = 0;
   zui->item.hot  = 0;

   /* why do i need this? */
   zui->mouse.wheel = menu_input_mouse_state(MENU_MOUSE_WHEEL_DOWN) - 
      menu_input_mouse_state(MENU_MOUSE_WHEEL_UP);

   menu_display_coords_array_reset();

   zui->tmp_block.carr.coords.vertices = 0;

   menu_display_font_bind_block((font_data_t*)zui->font, &zui->tmp_block);

   menu_display_push_quad(zui->width, zui->height, zui_bg_screen,
         0, 0, zui->width, zui->height);
   menu_display_snow(zui->width, zui->height);

   switch (zarch_layout)
   {
      case LAY_HOME:
         if (zarch_zui_render_sidebar(zui))
            return;
         if (zarch_zui_render_lay_root(video_info, zui))
            return;
         break;
      case LAY_SETTINGS:
         zarch_zui_render_lay_settings(zui);
         break;
      case LAY_PICK_CORE:
         if (zarch_zui_render_sidebar(zui))
            return;
         if (zarch_zui_render_pick_core(video_info, zui))
            return;
         break;
   }

   if (settings->menu.mouse.enable)
      zarch_zui_draw_cursor(
            zarch_zui_input_state(zui, MENU_ZARCH_MOUSE_X),
            zarch_zui_input_state(zui, MENU_ZARCH_MOUSE_Y));
         

   if (!zarch_zui_input_state(zui, MENU_ZARCH_PRESSED))
      zui->item.active = 0;
   else if (zui->item.active == 0)
      zui->item.active = -1;

   menu_display_blend_begin();
   
   draw.x           = 0;
   draw.y           = 0;
   draw.width       = zui->width;
   draw.height      = zui->height;
   draw.coords      = (struct video_coords*)ca;
   draw.matrix_data = &zui->mvp;
   draw.texture     = menu_display_white_texture;
   draw.prim_type   = MENU_DISPLAY_PRIM_TRIANGLES;
   draw.pipeline.id = 0;

   menu_display_draw(&draw);
   menu_display_blend_end();

   memset(&draw, 0, sizeof(menu_display_ctx_draw_t));

   coord_draw.ptr       = NULL;

   menu_display_get_tex_coords(&coord_draw);

   draw.width              = zui->width;
   draw.height             = zui->height;
   draw.texture            = zui->textures.bg;
   draw.color              = &coord_color[0];
   draw.vertex             = NULL;
   draw.tex_coord          = coord_draw.ptr;
   draw.vertex_count       = 4;
   draw.prim_type          = MENU_DISPLAY_PRIM_TRIANGLESTRIP;

   if (!video_info->libretro_running && draw.texture)
      draw.color           = &coord_color2[0];

   menu_display_blend_begin();
   draw.x              = 0;
   draw.y              = 0;
   menu_display_draw_bg(&draw, video_info, false);
   menu_display_draw(&draw);
   menu_display_blend_end();

   zui->rendering = false;

   menu_display_font_flush_block(video_info->width, video_info->height, 
         (font_data_t*)zui->font);
   menu_display_unset_viewport(video_info->width, video_info->height);
}

static void *zarch_init(void **userdata)
{
   zui_t *zui                              = NULL;
   settings_t *settings                    = config_get_ptr();
   menu_handle_t *menu                     = (menu_handle_t*)
      calloc(1, sizeof(*menu));

   if (!menu)
      goto error;

   if (!menu_display_init_first_driver())
      goto error;

   zui       = (zui_t*)calloc(1, sizeof(zui_t));

   if (!zui)
      goto error;

   *userdata       = zui;

   if (settings->menu.mouse.enable)
   {
      RARCH_WARN("Forcing menu_mouse_enable=false\n");
      settings->menu.mouse.enable = false;
   }

   zui->header_height  = 1000; /* dpi / 3; */
   zui->font_size       = 28;

   matrix_4x4_ortho(&zui->mvp, 0, 1, 1, 0, 0, 1);

   return menu;
error:
   if (menu)
      free(menu);
   return NULL;
}

static void zarch_free(void *data)
{
   zui_t        *zui                       = (zui_t*)data;

   if (zui)
      video_coord_array_free(&zui->tmp_block.carr);

   font_driver_bind_block(NULL, NULL);
}

static void zarch_context_bg_destroy(void *data)
{
   zui_t        *zui     = (zui_t*)data;
   if (!zui)
      return;
   video_driver_texture_unload(&zui->textures.bg);
   video_driver_texture_unload(&menu_display_white_texture);
}

static void zarch_context_destroy(void *data)
{
   zui_t        *zui = (zui_t*)data;

   /* why on earth is this called twice on exit? */
   if (!zui)
      return;

   menu_display_font_free((font_data_t*)zui->font);
   zarch_context_bg_destroy(data);

   zui->font = NULL;
}

static bool zarch_load_image(void *userdata, 
      void *data, enum menu_image_type type)
{
   zui_t        *zui = (zui_t*)userdata;

   if (!zui || !data)
      return false;

   switch (type)
   {
      case MENU_IMAGE_NONE:
         break;
      case MENU_IMAGE_WALLPAPER:
         zarch_context_bg_destroy(zui);
         video_driver_texture_load(data,
               TEXTURE_FILTER_MIPMAP_LINEAR,
               &zui->textures.bg);
         break;
      case MENU_IMAGE_THUMBNAIL:
         break;
      case MENU_IMAGE_SAVESTATE_THUMBNAIL:
         /* TODO/FIXME -implement */
         break;
   }

   return true;
}

static void zarch_context_reset(void *data)
{
   settings_t *settings  = config_get_ptr();
   zui_t          *zui   = (zui_t*)data;

   if (!zui || !settings)
      return;

   zarch_context_bg_destroy(zui);

   task_push_image_load(settings->path.menu_wallpaper,
         menu_display_handle_wallpaper_upload, NULL);

   menu_display_allocate_white_texture();

   menu_display_set_header_height(zui->header_height);
   zui->font = menu_display_font(APPLICATION_SPECIAL_DIRECTORY_ASSETS_ZARCH_FONT, zui->font_size);
}

static int zarch_iterate(void *data, void *userdata, enum menu_action action)
{
   int ret;
   size_t selection;
   menu_entry_t entry;
   zui_t *zui           = (zui_t*)userdata;

   if (!zui)
      return -1;
   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return 0;

   menu_entry_get(&entry, 0, selection, NULL, false);

   zui->action       = action;

   ret = menu_entry_action(&entry, selection, action);
   if (ret)
      return -1;
   return 0;
}

static bool zarch_menu_init_list(void *data)
{
   menu_displaylist_info_t info = {0};
   file_list_t *menu_stack      = menu_entries_get_menu_stack_ptr(0);
   file_list_t *selection_buf   = menu_entries_get_selection_buf_ptr(0);

   strlcpy(info.label,
         msg_hash_to_str(MENU_ENUM_LABEL_HISTORY_TAB), sizeof(info.label));
   info.enum_idx = MENU_ENUM_LABEL_HISTORY_TAB;

   menu_entries_append_enum(menu_stack,
         info.path, info.label, MENU_ENUM_LABEL_HISTORY_TAB, info.type, info.flags, 0);

   command_event(CMD_EVENT_HISTORY_INIT, NULL);

   info.list  = selection_buf;

   if (menu_displaylist_ctl(DISPLAYLIST_HISTORY, &info))
   {
      info.need_push = true;
      return menu_displaylist_ctl(DISPLAYLIST_PROCESS, &info);
   }

   return false;
}

menu_ctx_driver_t menu_ctx_zarch = {
   NULL,
   NULL,
   zarch_iterate,
   NULL,
   zarch_frame,
   zarch_init,
   zarch_free,
   zarch_context_reset,
   zarch_context_destroy,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   zarch_menu_init_list,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   zarch_load_image,
   "zarch",
   NULL,
   NULL,
   NULL,
   NULL
};
