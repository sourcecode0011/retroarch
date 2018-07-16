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
#include <lists/string_list.h>
#include <string/stdstring.h>
#include <file/file_path.h>
#include <file/archive_file.h>

#include <lists/dir_list.h>

#include <boolean.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "menu_filebrowser.h"

#include "../menu_driver.h"
#include "../menu_displaylist.h"

#include "../../configuration.h"
#include "../../paths.h"

static enum filebrowser_enums filebrowser_types = FILEBROWSER_NONE;

enum filebrowser_enums filebrowser_get_type(void)
{
   return filebrowser_types;
}

void filebrowser_clear_type(void)
{
   filebrowser_types = FILEBROWSER_NONE;
}

void filebrowser_set_type(enum filebrowser_enums type)
{
   if (filebrowser_types != FILEBROWSER_SELECT_FILE)
      filebrowser_types = type;
}

void filebrowser_parse(void *data, unsigned type_data, bool extensions_honored)
{
   size_t i, list_size;
   struct string_list *str_list         = NULL;
   unsigned items_found                 = 0;
   unsigned files_count                 = 0;
   unsigned dirs_count                  = 0;
   settings_t *settings                 = config_get_ptr();
   menu_displaylist_info_t *info        = (menu_displaylist_info_t*)data;
   enum menu_displaylist_ctl_state type = (enum menu_displaylist_ctl_state)
                                          type_data;
   bool path_is_compressed              = path_is_compressed_file(info->path);
   bool filter_ext                      =
      settings->menu.navigation.browser.filter.supported_extensions_enable;


   if (string_is_equal(info->label,
            msg_hash_to_str(MENU_ENUM_LABEL_SCAN_FILE)))
      filter_ext = false;

   if (extensions_honored)
      filter_ext = true;

   if (path_is_compressed)
      str_list = file_archive_get_file_list(info->path, info->exts);
   else
      str_list = dir_list_new(info->path,
            filter_ext ? info->exts : NULL,
            true, settings->show_hidden_files, true, false);

#ifdef HAVE_LIBRETRODB
   if (filebrowser_types == FILEBROWSER_SCAN_DIR)
      menu_entries_prepend(info->list,
            msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SCAN_THIS_DIRECTORY),
            msg_hash_to_str(MENU_ENUM_LABEL_SCAN_THIS_DIRECTORY),
            MENU_ENUM_LABEL_SCAN_THIS_DIRECTORY,
            FILE_TYPE_SCAN_DIRECTORY, 0 ,0);
#endif

   if (filebrowser_types == FILEBROWSER_SELECT_DIR)
      menu_entries_prepend(info->list,
            msg_hash_to_str(MENU_ENUM_LABEL_VALUE_USE_THIS_DIRECTORY),
            msg_hash_to_str(MENU_ENUM_LABEL_USE_THIS_DIRECTORY),
            MENU_ENUM_LABEL_USE_THIS_DIRECTORY,
            FILE_TYPE_USE_DIRECTORY, 0 ,0);

   if (!str_list)
   {
      const char *str = path_is_compressed
         ? msg_hash_to_str(MENU_ENUM_LABEL_VALUE_UNABLE_TO_READ_COMPRESSED_FILE)
         : msg_hash_to_str(MENU_ENUM_LABEL_VALUE_DIRECTORY_NOT_FOUND);

      menu_entries_append_enum(info->list, str, "",
            MENU_ENUM_LABEL_VALUE_DIRECTORY_NOT_FOUND, 0, 0, 0);
      goto end;
   }

   dir_list_sort(str_list, true);

   list_size = str_list->size;

   if (list_size == 0)
   {
      string_list_free(str_list);
      str_list = NULL;
   }
   else
   {
      for (i = 0; i < list_size; i++)
      {
         char label[PATH_MAX_LENGTH];
         bool is_dir                   = false;
         enum msg_hash_enums enum_idx  = MSG_UNKNOWN;
         enum msg_file_type file_type  = FILE_TYPE_NONE;
         const char *path              = str_list->elems[i].data;

         label[0] = '\0';

         switch (str_list->elems[i].attr.i)
         {
            case RARCH_DIRECTORY:
               file_type = FILE_TYPE_DIRECTORY;
               break;
            case RARCH_COMPRESSED_ARCHIVE:
               file_type = FILE_TYPE_CARCHIVE;
               break;
            case RARCH_COMPRESSED_FILE_IN_ARCHIVE:
               file_type = FILE_TYPE_IN_CARCHIVE;
               break;
            case RARCH_PLAIN_FILE:
            default:
               file_type = (enum msg_file_type)info->type_default;
               switch (type)
               {
                  /* in case of deferred_core_list we have to interpret
                   * every archive as an archive to disallow instant loading
                   */
                  case DISPLAYLIST_CORES_DETECTED:
                     if (path_is_compressed_file(path))
                        file_type = FILE_TYPE_CARCHIVE;
                     break;
                  default:
                     break;
               }
               break;
         }

         is_dir = (file_type == FILE_TYPE_DIRECTORY);

         if (!is_dir)
         {
            if (filebrowser_types == FILEBROWSER_SELECT_DIR)
               continue;
            if (filebrowser_types == FILEBROWSER_SCAN_DIR)
               continue;
         }

         /* Need to preserve slash first time. */

         if (!string_is_empty(info->path) && !path_is_compressed)
            path = path_basename(path);

         if (filebrowser_types == FILEBROWSER_SELECT_COLLECTION)
         {
            if (is_dir)
               file_type = FILE_TYPE_DIRECTORY;
            else
               file_type = FILE_TYPE_PLAYLIST_COLLECTION;
         }

         if (!is_dir && 
               (settings->multimedia.builtin_mediaplayer_enable ||
                settings->multimedia.builtin_imageviewer_enable))
         {
            switch (path_is_media_type(path))
            {
               case RARCH_CONTENT_MOVIE:
#ifdef HAVE_FFMPEG
                  if (settings->multimedia.builtin_mediaplayer_enable)
                     file_type = FILE_TYPE_MOVIE;
#endif
                  break;
               case RARCH_CONTENT_MUSIC:
#ifdef HAVE_FFMPEG
                  if (settings->multimedia.builtin_mediaplayer_enable)
                     file_type = FILE_TYPE_MUSIC;
#endif
                  break;
               case RARCH_CONTENT_IMAGE:
#ifdef HAVE_IMAGEVIEWER
                  if (settings->multimedia.builtin_imageviewer_enable
                        && type != DISPLAYLIST_IMAGES)
                     file_type = FILE_TYPE_IMAGEVIEWER;
                  else
                     file_type = FILE_TYPE_IMAGE;
#endif
                  if (filebrowser_types == FILEBROWSER_SELECT_FILE)
                     file_type = FILE_TYPE_IMAGE;
                  break;
               default:
                  break;
            }
         }

         switch (file_type)
         {
            case FILE_TYPE_PLAIN:
#if 0
               enum_idx = MENU_ENUM_LABEL_FILE_BROWSER_PLAIN_FILE;
#endif
               files_count++;
               break;
            case FILE_TYPE_MOVIE:
               enum_idx = MENU_ENUM_LABEL_FILE_BROWSER_MOVIE_OPEN;
               files_count++;
               break;
            case FILE_TYPE_MUSIC:
               enum_idx = MENU_ENUM_LABEL_FILE_BROWSER_MUSIC_OPEN;
               files_count++;
               break;
            case FILE_TYPE_IMAGE:
               enum_idx = MENU_ENUM_LABEL_FILE_BROWSER_IMAGE;
               files_count++;
               break;
            case FILE_TYPE_IMAGEVIEWER:
               enum_idx = MENU_ENUM_LABEL_FILE_BROWSER_IMAGE_OPEN_WITH_VIEWER;
               files_count++;
               break;
            case FILE_TYPE_DIRECTORY:
               enum_idx = MENU_ENUM_LABEL_FILE_BROWSER_DIRECTORY;
               dirs_count++;
               break;
            default:
               break;
         }

         items_found++;
         menu_entries_append_enum(info->list, path, label,
               enum_idx,
               file_type, 0, 0);
      }
   }

   if (str_list && str_list->size > 0)
      string_list_free(str_list);

   if (items_found == 0)
   {
      menu_entries_append_enum(info->list,
            msg_hash_to_str(MENU_ENUM_LABEL_VALUE_NO_ITEMS),
            msg_hash_to_str(MENU_ENUM_LABEL_NO_ITEMS),
            MENU_ENUM_LABEL_NO_ITEMS,
            MENU_SETTING_NO_ITEM, 0, 0);
   }

   /* We don't want to show 'filter by extension' for this. */
   if (filebrowser_types == FILEBROWSER_SELECT_DIR)
      goto end;
   if (filebrowser_types == FILEBROWSER_SCAN_DIR)
      goto end;

   if (!extensions_honored && files_count > 0)
      menu_entries_prepend(info->list,
            msg_hash_to_str(MENU_ENUM_LABEL_VALUE_NAVIGATION_BROWSER_FILTER_SUPPORTED_EXTENSIONS_ENABLE),
            msg_hash_to_str(MENU_ENUM_LABEL_NAVIGATION_BROWSER_FILTER_SUPPORTED_EXTENSIONS_ENABLE),
            MENU_ENUM_LABEL_NAVIGATION_BROWSER_FILTER_SUPPORTED_EXTENSIONS_ENABLE,
            0, 0 ,0);

end:
   menu_entries_prepend(info->list,
         msg_hash_to_str(MENU_ENUM_LABEL_VALUE_PARENT_DIRECTORY),
         info->path,
         MENU_ENUM_LABEL_PARENT_DIRECTORY,
         FILE_TYPE_PARENT_DIRECTORY, 0, 0);
}
