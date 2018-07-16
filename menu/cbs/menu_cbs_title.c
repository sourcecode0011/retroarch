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

#include <lists/string_list.h>
#include <string/stdstring.h>
#include <file/file_path.h>

#include <compat/strl.h>

#include "../menu_driver.h"
#include "../menu_cbs.h"

#ifndef BIND_ACTION_GET_TITLE
#define BIND_ACTION_GET_TITLE(cbs, name) \
   cbs->action_get_title = name; \
   cbs->action_get_title_ident = #name;
#endif

static void replace_chars(char *str, char c1, char c2)
{
   char *pos = NULL;
   while((pos = strchr(str, c1)))
      *pos = c2;
}

static void sanitize_to_string(char *s, const char *label, size_t len)
{
   char new_label[255];

   new_label[0] = '\0';

   strlcpy(new_label, label, sizeof(new_label));
   strlcpy(s, new_label, len);
   replace_chars(s, '_', ' ');
}

static int fill_title(char *s, const char *title, const char *path, size_t len)
{
   fill_pathname_join_delim(s, title, path, ' ', len);
   return 0;
}

static int action_get_title_action_generic(const char *path, const char *label, 
      unsigned menu_type, char *s, size_t len)
{
   sanitize_to_string(s, label, len);
   return 0;
}

#define default_title_macro(func_name, lbl) \
  static int (func_name)(const char *path, const char *label, unsigned menu_type, char *s, size_t len) \
{ \
   sanitize_to_string(s, msg_hash_to_str(lbl), len); \
   return 0; \
}

#define default_fill_title_macro(func_name, lbl) \
  static int (func_name)(const char *path, const char *label, unsigned menu_type, char *s, size_t len) \
{ \
   return fill_title(s, msg_hash_to_str(lbl), path, len); \
}

#define default_title_copy_macro(func_name, lbl) \
  static int (func_name)(const char *path, const char *label, unsigned menu_type, char *s, size_t len) \
{ \
   strlcpy(s, msg_hash_to_str(lbl), len); \
   return 0; \
}

default_title_macro(action_get_user_accounts_cheevos_list,      MENU_ENUM_LABEL_VALUE_ACCOUNTS_RETRO_ACHIEVEMENTS)
default_title_macro(action_get_download_core_content_list,      MENU_ENUM_LABEL_VALUE_DOWNLOAD_CORE_CONTENT)
default_title_macro(action_get_user_accounts_list,              MENU_ENUM_LABEL_VALUE_ACCOUNTS_LIST)
default_title_macro(action_get_core_information_list,           MENU_ENUM_LABEL_VALUE_CORE_INFORMATION)
default_title_macro(action_get_core_list,                       MENU_ENUM_LABEL_VALUE_CORE_LIST)
default_title_macro(action_get_online_updater_list,             MENU_ENUM_LABEL_VALUE_ONLINE_UPDATER)
default_title_macro(action_get_netplay_list,                    MENU_ENUM_LABEL_VALUE_NETPLAY)
default_title_macro(action_get_online_thumbnails_updater_list,  MENU_ENUM_LABEL_VALUE_THUMBNAILS_UPDATER_LIST)
default_title_macro(action_get_core_updater_list,               MENU_ENUM_LABEL_VALUE_CORE_UPDATER_LIST)
default_title_macro(action_get_add_content_list,                MENU_ENUM_LABEL_VALUE_ADD_CONTENT_LIST)
default_title_macro(action_get_configurations_list,             MENU_ENUM_LABEL_VALUE_CONFIGURATIONS_LIST)
default_title_macro(action_get_core_options_list,               MENU_ENUM_LABEL_VALUE_CORE_OPTIONS)
default_title_macro(action_get_load_recent_list,                MENU_ENUM_LABEL_VALUE_LOAD_CONTENT_HISTORY)
default_title_macro(action_get_quick_menu_list,                 MENU_ENUM_LABEL_VALUE_CONTENT_SETTINGS)
default_title_macro(action_get_input_remapping_options_list,    MENU_ENUM_LABEL_VALUE_CORE_INPUT_REMAPPING_OPTIONS)
default_title_macro(action_get_shader_options_list,             MENU_ENUM_LABEL_VALUE_SHADER_OPTIONS)
default_title_macro(action_get_disk_options_list,               MENU_ENUM_LABEL_VALUE_DISK_OPTIONS)
default_title_macro(action_get_frontend_counters_list,          MENU_ENUM_LABEL_VALUE_FRONTEND_COUNTERS)
default_title_macro(action_get_core_counters_list,              MENU_ENUM_LABEL_VALUE_CORE_COUNTERS)
default_title_macro(action_get_recording_settings_list,         MENU_ENUM_LABEL_VALUE_RECORDING_SETTINGS)
default_title_macro(action_get_playlist_settings_list,          MENU_ENUM_LABEL_VALUE_PLAYLIST_SETTINGS)
default_title_macro(action_get_input_hotkey_binds_settings_list,MENU_ENUM_LABEL_VALUE_INPUT_HOTKEY_BINDS)
default_title_macro(action_get_driver_settings_list,            MENU_ENUM_LABEL_VALUE_DRIVER_SETTINGS)
default_title_macro(action_get_core_settings_list,              MENU_ENUM_LABEL_VALUE_CORE_SETTINGS)
default_title_macro(action_get_video_settings_list,             MENU_ENUM_LABEL_VALUE_VIDEO_SETTINGS)
default_title_macro(action_get_configuration_settings_list,     MENU_ENUM_LABEL_VALUE_CONFIGURATION_SETTINGS)
default_title_macro(action_get_saving_settings_list,            MENU_ENUM_LABEL_VALUE_SAVING_SETTINGS)
default_title_macro(action_get_logging_settings_list,           MENU_ENUM_LABEL_VALUE_LOGGING_SETTINGS)
default_title_macro(action_get_frame_throttle_settings_list,    MENU_ENUM_LABEL_VALUE_FRAME_THROTTLE_SETTINGS)
default_title_macro(action_get_rewind_settings_list,            MENU_ENUM_LABEL_VALUE_REWIND_SETTINGS)
default_title_macro(action_get_onscreen_display_settings_list,  MENU_ENUM_LABEL_VALUE_ONSCREEN_DISPLAY_SETTINGS)
default_title_macro(action_get_onscreen_notifications_settings_list,  MENU_ENUM_LABEL_VALUE_ONSCREEN_NOTIFICATIONS_SETTINGS)
default_title_macro(action_get_onscreen_overlay_settings_list,  MENU_ENUM_LABEL_VALUE_ONSCREEN_OVERLAY_SETTINGS)
default_title_macro(action_get_menu_settings_list,              MENU_ENUM_LABEL_VALUE_MENU_SETTINGS)
default_title_macro(action_get_user_interface_settings_list,    MENU_ENUM_LABEL_VALUE_USER_INTERFACE_SETTINGS)
default_title_macro(action_get_menu_file_browser_settings_list, MENU_ENUM_LABEL_VALUE_MENU_FILE_BROWSER_SETTINGS)
default_title_macro(action_get_retro_achievements_settings_list,MENU_ENUM_LABEL_VALUE_RETRO_ACHIEVEMENTS_SETTINGS)
default_title_macro(action_get_wifi_settings_list,              MENU_ENUM_LABEL_VALUE_WIFI_SETTINGS)
default_title_macro(action_get_network_settings_list,           MENU_ENUM_LABEL_VALUE_NETWORK_SETTINGS)
default_title_macro(action_get_netplay_lan_scan_settings_list,  MENU_ENUM_LABEL_VALUE_NETPLAY_LAN_SCAN_SETTINGS)
default_title_macro(action_get_lakka_services_list,             MENU_ENUM_LABEL_VALUE_LAKKA_SERVICES)
default_title_macro(action_get_user_settings_list,              MENU_ENUM_LABEL_VALUE_USER_SETTINGS)
default_title_macro(action_get_directory_settings_list,         MENU_ENUM_LABEL_VALUE_DIRECTORY_SETTINGS)
default_title_macro(action_get_privacy_settings_list,           MENU_ENUM_LABEL_VALUE_PRIVACY_SETTINGS)
default_title_macro(action_get_updater_settings_list,           MENU_ENUM_LABEL_VALUE_UPDATER_SETTINGS)
default_title_macro(action_get_audio_settings_list,             MENU_ENUM_LABEL_VALUE_AUDIO_SETTINGS)
default_title_macro(action_get_input_settings_list,             MENU_ENUM_LABEL_VALUE_INPUT_SETTINGS)
default_title_macro(action_get_core_cheat_options_list,         MENU_ENUM_LABEL_VALUE_CORE_CHEAT_OPTIONS)
default_title_macro(action_get_load_content_list,               MENU_ENUM_LABEL_VALUE_LOAD_CONTENT_LIST)
default_title_macro(action_get_cursor_manager_list,             MENU_ENUM_LABEL_VALUE_CURSOR_MANAGER)
default_title_macro(action_get_database_manager_list,           MENU_ENUM_LABEL_VALUE_DATABASE_MANAGER)
default_title_macro(action_get_system_information_list,         MENU_ENUM_LABEL_VALUE_SYSTEM_INFORMATION)
default_title_macro(action_get_network_information_list,        MENU_ENUM_LABEL_VALUE_NETWORK_INFORMATION)
default_title_macro(action_get_settings_list,                   MENU_ENUM_LABEL_VALUE_SETTINGS)
default_title_macro(action_get_title_information_list,          MENU_ENUM_LABEL_VALUE_INFORMATION_LIST)

default_fill_title_macro(action_get_title_disk_image_append,    MENU_ENUM_LABEL_VALUE_DISK_IMAGE_APPEND)
default_fill_title_macro(action_get_title_cheat_file_load,      MENU_ENUM_LABEL_VALUE_CHEAT_FILE)
default_fill_title_macro(action_get_title_remap_file_load,      MENU_ENUM_LABEL_VALUE_REMAP_FILE)
default_fill_title_macro(action_get_title_overlay,              MENU_ENUM_LABEL_VALUE_OVERLAY)
default_fill_title_macro(action_get_title_video_filter,         MENU_ENUM_LABEL_VALUE_VIDEO_FILTER)
default_fill_title_macro(action_get_title_cheat_directory,      MENU_ENUM_LABEL_VALUE_CHEAT_DATABASE_PATH)
default_fill_title_macro(action_get_title_core_directory,       MENU_ENUM_LABEL_VALUE_LIBRETRO_DIR_PATH)
default_fill_title_macro(action_get_title_core_info_directory,  MENU_ENUM_LABEL_VALUE_LIBRETRO_INFO_PATH)
default_fill_title_macro(action_get_title_audio_filter,         MENU_ENUM_LABEL_VALUE_AUDIO_FILTER_DIR)
default_fill_title_macro(action_get_title_video_shader_preset,  MENU_ENUM_LABEL_VIDEO_SHADER_PRESET_TWO)
default_fill_title_macro(action_get_title_deferred_core_list,   MENU_ENUM_LABEL_VALUE_SUPPORTED_CORES)
default_fill_title_macro(action_get_title_configurations,       MENU_ENUM_LABEL_VALUE_CONFIG)
default_fill_title_macro(action_get_title_content_database_directory,   MENU_ENUM_LABEL_VALUE_CONTENT_DATABASE_DIRECTORY)
default_fill_title_macro(action_get_title_savestate_directory,          MENU_ENUM_LABEL_VALUE_SAVESTATE_DIRECTORY)
default_fill_title_macro(action_get_title_dynamic_wallpapers_directory, MENU_ENUM_LABEL_VALUE_DYNAMIC_WALLPAPERS_DIRECTORY)
default_fill_title_macro(action_get_title_core_assets_directory, MENU_ENUM_LABEL_VALUE_CORE_ASSETS_DIR)
default_fill_title_macro(action_get_title_config_directory,      MENU_ENUM_LABEL_VALUE_RGUI_CONFIG_DIRECTORY)
default_fill_title_macro(action_get_title_input_remapping_directory,    MENU_ENUM_LABEL_VALUE_INPUT_REMAPPING_DIRECTORY)
default_fill_title_macro(action_get_title_autoconfig_directory,  MENU_ENUM_LABEL_VALUE_JOYPAD_AUTOCONFIG_DIR )
default_fill_title_macro(action_get_title_playlist_directory,    MENU_ENUM_LABEL_VALUE_PLAYLIST_DIRECTORY)
default_fill_title_macro(action_get_title_browser_directory,     MENU_ENUM_LABEL_VALUE_RGUI_BROWSER_DIRECTORY)
default_fill_title_macro(action_get_title_content_directory,     MENU_ENUM_LABEL_VALUE_CONTENT_DIR)
default_fill_title_macro(action_get_title_screenshot_directory,  MENU_ENUM_LABEL_VALUE_SCREENSHOT_DIRECTORY)
default_fill_title_macro(action_get_title_cursor_directory,      MENU_ENUM_LABEL_VALUE_CURSOR_DIRECTORY)
default_fill_title_macro(action_get_title_onscreen_overlay_keyboard_directory, MENU_ENUM_LABEL_VALUE_OSK_OVERLAY_DIRECTORY)
default_fill_title_macro(action_get_title_recording_config_directory, MENU_ENUM_LABEL_VALUE_RECORDING_CONFIG_DIRECTORY)
default_fill_title_macro(action_get_title_recording_output_directory, MENU_ENUM_LABEL_VALUE_RECORDING_OUTPUT_DIRECTORY)
default_fill_title_macro(action_get_title_video_shader_directory, MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_DIR)
default_fill_title_macro(action_get_title_audio_filter_directory, MENU_ENUM_LABEL_VALUE_AUDIO_FILTER_DIR)
default_fill_title_macro(action_get_title_video_filter_directory, MENU_ENUM_LABEL_VALUE_VIDEO_FILTER_DIR)
default_fill_title_macro(action_get_title_savefile_directory,     MENU_ENUM_LABEL_VALUE_SAVEFILE_DIRECTORY)
default_fill_title_macro(action_get_title_overlay_directory,      MENU_ENUM_LABEL_VALUE_OVERLAY_DIRECTORY)
default_fill_title_macro(action_get_title_system_directory,       MENU_ENUM_LABEL_VALUE_SYSTEM_DIRECTORY)
default_fill_title_macro(action_get_title_assets_directory,       MENU_ENUM_LABEL_VALUE_ASSETS_DIRECTORY)
default_fill_title_macro(action_get_title_extraction_directory,   MENU_ENUM_LABEL_VALUE_CACHE_DIRECTORY)
default_fill_title_macro(action_get_title_menu,                   MENU_ENUM_LABEL_VALUE_MENU_SETTINGS)
default_fill_title_macro(action_get_title_font_path,              MENU_ENUM_LABEL_VALUE_XMB_FONT)
default_fill_title_macro(action_get_title_collection,             MENU_ENUM_LABEL_VALUE_SELECT_FROM_COLLECTION)

default_title_copy_macro(action_get_title_help,                   MENU_ENUM_LABEL_VALUE_HELP_LIST)
default_title_copy_macro(action_get_title_input_settings,         MENU_ENUM_LABEL_VALUE_INPUT_SETTINGS)
default_title_copy_macro(action_get_title_cheevos_list,           MENU_ENUM_LABEL_VALUE_ACHIEVEMENT_LIST)
default_title_copy_macro(action_get_title_cheevos_list_hardcore,  MENU_ENUM_LABEL_VALUE_ACHIEVEMENT_LIST_HARDCORE)
default_title_copy_macro(action_get_title_video_shader_parameters,MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_PARAMETERS)
default_title_copy_macro(action_get_title_video_shader_preset_parameters,MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_PRESET_PARAMETERS)

static int action_get_title_generic(char *s, size_t len, const char *path,
      const char *text)
{
   struct string_list *list_path    = NULL;
   
   if (!string_is_empty(path))
      list_path = string_split(path, "|");

   if (list_path)
   {
      char elem0_path[255];

      elem0_path[0] = '\0';

      if (list_path->size > 0)
         strlcpy(elem0_path, list_path->elems[0].data, sizeof(elem0_path));
      string_list_free(list_path);
      snprintf(s, len, "%s - %s", text,
            (string_is_empty(elem0_path)) ? "" : path_basename(elem0_path));
   }
   else
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_NOT_AVAILABLE), len);

   return 0;
}

#define default_title_generic_macro(func_name, lbl) \
   static int (func_name)(const char *path, const char *label, unsigned menu_type, char *s, size_t len) \
  { \
   return action_get_title_generic(s, len, path, msg_hash_to_str(lbl)); \
} \

default_title_generic_macro(action_get_title_deferred_database_manager_list,MENU_ENUM_LABEL_VALUE_DATABASE_SELECTION) 
default_title_generic_macro(action_get_title_deferred_cursor_manager_list,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST) 
default_title_generic_macro(action_get_title_list_rdb_entry_developer,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_DEVELOPER) 
default_title_generic_macro(action_get_title_list_rdb_entry_publisher,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_PUBLISHER) 
default_title_generic_macro(action_get_title_list_rdb_entry_origin,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_ORIGIN) 
default_title_generic_macro(action_get_title_list_rdb_entry_franchise,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_FRANCHISE) 
default_title_generic_macro(action_get_title_list_rdb_entry_edge_magazine_rating,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_EDGE_MAGAZINE_RATING) 
default_title_generic_macro(action_get_title_list_rdb_entry_edge_magazine_issue,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_EDGE_MAGAZINE_ISSUE) 
default_title_generic_macro(action_get_title_list_rdb_entry_releasedate_by_month,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_RELEASEDATE_BY_MONTH) 
default_title_generic_macro(action_get_title_list_rdb_entry_releasedate_by_year,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_RELEASEDATE_BY_YEAR) 
default_title_generic_macro(action_get_title_list_rdb_entry_esrb_rating,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_ESRB_RATING) 
default_title_generic_macro(action_get_title_list_rdb_entry_elspa_rating,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_ELSPA_RATING) 
default_title_generic_macro(action_get_title_list_rdb_entry_pegi_rating,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_PEGI_RATING) 
default_title_generic_macro(action_get_title_list_rdb_entry_cero_rating,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_CERO_RATING) 
default_title_generic_macro(action_get_title_list_rdb_entry_bbfc_rating,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_BBFC_RATING) 
default_title_generic_macro(action_get_title_list_rdb_entry_max_users,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_MAX_USERS) 
default_title_generic_macro(action_get_title_list_rdb_entry_database_info,MENU_ENUM_LABEL_VALUE_DATABASE_CURSOR_LIST_ENTRY_DATABASE_INFO) 

static int action_get_title_default(const char *path, const char *label, 
      unsigned menu_type, char *s, size_t len)
{
   snprintf(s, len, "%s %s", msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SELECT_FILE), path);
   return 0;
}

static int action_get_title_group_settings(const char *path, const char *label, 
      unsigned menu_type, char *s, size_t len)
{
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_MAIN_MENU)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_MAIN_MENU), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_HISTORY_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_HISTORY_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_IMAGES_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_IMAGES_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_MUSIC_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_MUSIC_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_VIDEO_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_VIDEO_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_SETTINGS_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SETTINGS_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_PLAYLISTS_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_PLAYLISTS_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_ADD_TAB)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_ADD_TAB), len);
   else if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_HORIZONTAL_MENU)))
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_HORIZONTAL_MENU), len);
   else
   {
      char elem0[255];
      char elem1[255];
      struct string_list *list_label = string_split(label, "|");

      elem0[0] = elem1[0] = '\0';

      if (list_label)
      {
         if (list_label->size > 0)
         {
            strlcpy(elem0, list_label->elems[0].data, sizeof(elem0));
            if (list_label->size > 1)
               strlcpy(elem1, list_label->elems[1].data, sizeof(elem1));
         }
         string_list_free(list_label);
      }

      strlcpy(s, elem0, len);

      if (!string_is_empty(elem1))
      {
         strlcat(s, " - ", len);
         strlcat(s, elem1, len);
      }
   }

   return 0;
}

static int action_get_title_input_binds_list(const char *path, const char *label, 
      unsigned menu_type, char *s, size_t len)
{
   unsigned val = (((unsigned)path[0]) - 49) + 1;
   snprintf(s, len, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_INPUT_USER_BINDS), val);
   return 0;
}

static int menu_cbs_init_bind_title_compare_label(menu_file_list_cbs_t *cbs,
      const char *label, uint32_t label_hash)
{
   if (cbs->setting)
   {
      const char *parent_group   = cbs->setting->parent_group;

      if (string_is_equal(parent_group, msg_hash_to_str(MENU_ENUM_LABEL_MAIN_MENU)) 
            && setting_get_type(cbs->setting) == ST_GROUP)
      {
         BIND_ACTION_GET_TITLE(cbs, action_get_title_group_settings);
         return 0;
      }
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CORE_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_core_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CONFIGURATION_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_configuration_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_SAVING_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_saving_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_LOGGING_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_logging_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_FRAME_THROTTLE_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_frame_throttle_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_REWIND_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_rewind_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_ONSCREEN_DISPLAY_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_onscreen_display_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_ONSCREEN_NOTIFICATIONS_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_onscreen_notifications_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_ONSCREEN_OVERLAY_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_onscreen_overlay_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_MENU_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_menu_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_USER_INTERFACE_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_user_interface_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_MENU_FILE_BROWSER_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_menu_file_browser_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_RETRO_ACHIEVEMENTS_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_retro_achievements_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_WIFI_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_wifi_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_UPDATER_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_updater_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_NETWORK_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_network_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_NETPLAY_LAN_SCAN_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_netplay_lan_scan_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_LAKKA_SERVICES_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_lakka_services_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_USER_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_user_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_DIRECTORY_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_directory_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_PRIVACY_SETTINGS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_privacy_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_download_core_content_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_SUBDIR_LIST)))
   {
      BIND_ACTION_GET_TITLE(cbs, action_get_download_core_content_list);
      return 0;
   }

   if (cbs->enum_idx != MSG_UNKNOWN)
   {
      switch (cbs->enum_idx)
      {
         case MENU_ENUM_LABEL_DEFERRED_DATABASE_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_deferred_database_manager_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_deferred_cursor_manager_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_DEVELOPER:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_developer);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PUBLISHER:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_publisher);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ORIGIN:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_origin);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_FRANCHISE:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_franchise);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_edge_magazine_rating);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_ISSUE:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_edge_magazine_issue);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEMONTH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_releasedate_by_month);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEYEAR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_releasedate_by_year);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ESRB_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_esrb_rating);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ELSPA_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_elspa_rating);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PEGI_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_pegi_rating);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_CERO_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_cero_rating);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_BBFC_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_bbfc_rating);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_MAX_USERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_max_users);
            break;
         case MENU_ENUM_LABEL_DEFERRED_RDB_ENTRY_DETAIL:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_database_info);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CORE_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_deferred_core_list);
            break;
         case MENU_ENUM_LABEL_CONFIGURATIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_configurations);
            break;
         case MENU_ENUM_LABEL_JOYPAD_AUTOCONFIG_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_autoconfig_directory);
            break;
         case MENU_ENUM_LABEL_CACHE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_extraction_directory);
            break;
         case MENU_ENUM_LABEL_SYSTEM_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_system_directory);
            break;
         case MENU_ENUM_LABEL_ASSETS_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_assets_directory);
            break;
         case MENU_ENUM_LABEL_SAVEFILE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_savefile_directory);
            break;
         case MENU_ENUM_LABEL_OVERLAY_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_overlay_directory);
            break;
         case MENU_ENUM_LABEL_RGUI_BROWSER_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_browser_directory);
            break;
         case MENU_ENUM_LABEL_PLAYLIST_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_playlist_directory);
            break;
         case MENU_ENUM_LABEL_CONTENT_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_content_directory);
            break;
         case MENU_ENUM_LABEL_SCREENSHOT_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_screenshot_directory);
            break;
         case MENU_ENUM_LABEL_VIDEO_SHADER_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_shader_directory);
            break;
         case MENU_ENUM_LABEL_VIDEO_FILTER_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_filter_directory);
            break;
         case MENU_ENUM_LABEL_AUDIO_FILTER_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_audio_filter_directory);
            break;
         case MENU_ENUM_LABEL_CURSOR_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cursor_directory);
            break;
         case MENU_ENUM_LABEL_RECORDING_CONFIG_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_recording_config_directory);
            break;
         case MENU_ENUM_LABEL_RECORDING_OUTPUT_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_recording_output_directory);
            break;
         case MENU_ENUM_LABEL_OSK_OVERLAY_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_onscreen_overlay_keyboard_directory);
            break;
         case MENU_ENUM_LABEL_INPUT_REMAPPING_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_input_remapping_directory);
            break;
         case MENU_ENUM_LABEL_CONTENT_DATABASE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_content_database_directory);
            break;
         case MENU_ENUM_LABEL_SAVESTATE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_savestate_directory);
            break;
         case MENU_ENUM_LABEL_DYNAMIC_WALLPAPERS_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_dynamic_wallpapers_directory);
            break;
         case MENU_ENUM_LABEL_CORE_ASSETS_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_core_assets_directory);
            break;
         case MENU_ENUM_LABEL_RGUI_CONFIG_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_config_directory);
            break;
         case MENU_ENUM_LABEL_INFORMATION_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_information_list);
            break;
         case MENU_ENUM_LABEL_SETTINGS:
            BIND_ACTION_GET_TITLE(cbs, action_get_settings_list);
            break;
         case MENU_ENUM_LABEL_DATABASE_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_database_manager_list);
            break;
         case MENU_ENUM_LABEL_SYSTEM_INFORMATION:
            BIND_ACTION_GET_TITLE(cbs, action_get_system_information_list);
            break;
         case MENU_ENUM_LABEL_NETWORK_INFORMATION:
            BIND_ACTION_GET_TITLE(cbs, action_get_network_information_list);
            break;
         case MENU_ENUM_LABEL_CURSOR_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_cursor_manager_list);
            break;
         case MENU_ENUM_LABEL_CORE_INFORMATION:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_information_list);
            break;
         case MENU_ENUM_LABEL_CORE_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_list);
            break;
         case MENU_ENUM_LABEL_LOAD_CONTENT_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_load_content_list);
            break;
         case MENU_ENUM_LABEL_ONLINE_UPDATER:
            BIND_ACTION_GET_TITLE(cbs, action_get_online_updater_list);
            break;
         case MENU_ENUM_LABEL_NETPLAY:
            BIND_ACTION_GET_TITLE(cbs, action_get_netplay_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_THUMBNAILS_UPDATER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_online_thumbnails_updater_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CORE_UPDATER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_updater_list);
            break;
         case MENU_ENUM_LABEL_ADD_CONTENT_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_add_content_list);
            break;
         case MENU_ENUM_LABEL_CONFIGURATIONS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_configurations_list);
            break;
         case MENU_ENUM_LABEL_CORE_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_options_list);
            break;
         case MENU_ENUM_LABEL_LOAD_CONTENT_HISTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_load_recent_list);
            break;
         case MENU_ENUM_LABEL_CONTENT_SETTINGS:
            BIND_ACTION_GET_TITLE(cbs, action_get_quick_menu_list);
            break;
         case MENU_ENUM_LABEL_CORE_INPUT_REMAPPING_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_input_remapping_options_list);
            break;
         case MENU_ENUM_LABEL_CORE_CHEAT_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_cheat_options_list);
            break;
         case MENU_ENUM_LABEL_SHADER_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_shader_options_list);
            break;
         case MENU_ENUM_LABEL_DISK_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_disk_options_list);
            break;
         case MENU_ENUM_LABEL_FRONTEND_COUNTERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_frontend_counters_list);
            break;
         case MENU_ENUM_LABEL_CORE_COUNTERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_counters_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_USER_BINDS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_input_binds_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_INPUT_HOTKEY_BINDS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_input_hotkey_binds_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_DRIVER_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_driver_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_VIDEO_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_video_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CONFIGURATION_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_configuration_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_LOGGING_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_logging_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_SAVING_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_saving_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_FRAME_THROTTLE_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_frame_throttle_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_REWIND_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_rewind_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_ONSCREEN_DISPLAY_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_onscreen_display_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_ONSCREEN_OVERLAY_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_onscreen_overlay_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CORE_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_AUDIO_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_audio_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_INPUT_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_input_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_RECORDING_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_recording_settings_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_PLAYLIST_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_playlist_settings_list);
            break;
         case MENU_ENUM_LABEL_MANAGEMENT:
         case MENU_ENUM_LABEL_ACHIEVEMENT_LIST:
         case MENU_ENUM_LABEL_ACHIEVEMENT_LIST_HARDCORE:
         case MENU_ENUM_LABEL_VIDEO_SHADER_PARAMETERS:
         case MENU_ENUM_LABEL_VIDEO_SHADER_PRESET_PARAMETERS:
         case MENU_ENUM_LABEL_CONTENT_COLLECTION_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_action_generic);
            break;
         case MENU_ENUM_LABEL_DISK_IMAGE_APPEND:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_disk_image_append);
            break;
         case MENU_ENUM_LABEL_VIDEO_SHADER_PRESET:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_shader_preset);
            break;
         case MENU_ENUM_LABEL_CHEAT_FILE_LOAD:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cheat_file_load);
            break;
         case MENU_ENUM_LABEL_REMAP_FILE_LOAD:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_remap_file_load);
            break;
         case MENU_ENUM_LABEL_DEFERRED_ACCOUNTS_CHEEVOS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_user_accounts_cheevos_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_LIST:
         case MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_SUBDIR_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_download_core_content_list);
            break;
         case MENU_ENUM_LABEL_DEFERRED_ACCOUNTS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_user_accounts_list);
            break;
         case MENU_ENUM_LABEL_HELP_LIST:
         case MENU_ENUM_LABEL_HELP:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_help);
            break;
         case MENU_ENUM_LABEL_INPUT_OVERLAY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_overlay);
            break;
         case MENU_ENUM_LABEL_VIDEO_FONT_PATH:
         case MENU_ENUM_LABEL_XMB_FONT:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_font_path);
            break;
         case MENU_ENUM_LABEL_VIDEO_FILTER:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_filter);
            break;
         case MENU_ENUM_LABEL_AUDIO_DSP_PLUGIN:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_audio_filter);
            break;
         case MENU_ENUM_LABEL_CHEAT_DATABASE_PATH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cheat_directory);
            break;
         case MENU_ENUM_LABEL_LIBRETRO_DIR_PATH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_core_directory);
            break;
         case MENU_ENUM_LABEL_LIBRETRO_INFO_PATH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_core_info_directory);
            break;
         default:
            return -1;
      }
   }
   else
   {
      switch (label_hash)
      {
         case MENU_LABEL_DEFERRED_DATABASE_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_deferred_database_manager_list);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_deferred_cursor_manager_list);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_DEVELOPER:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_developer);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PUBLISHER:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_publisher);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ORIGIN:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_origin);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_FRANCHISE:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_franchise);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_edge_magazine_rating);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_ISSUE:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_edge_magazine_issue);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEMONTH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_releasedate_by_month);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEYEAR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_releasedate_by_year);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ESRB_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_esrb_rating);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ELSPA_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_elspa_rating);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PEGI_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_pegi_rating);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_CERO_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_cero_rating);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_BBFC_RATING:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_bbfc_rating);
            break;
         case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_MAX_USERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_max_users);
            break;
         case MENU_LABEL_DEFERRED_RDB_ENTRY_DETAIL:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_list_rdb_entry_database_info);
            break;
         case MENU_LABEL_DEFERRED_CORE_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_deferred_core_list);
            break;
         case MENU_LABEL_CONFIGURATIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_configurations);
            break;
         case MENU_LABEL_JOYPAD_AUTOCONFIG_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_autoconfig_directory);
            break;
         case MENU_LABEL_CACHE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_extraction_directory);
            break;
         case MENU_LABEL_SYSTEM_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_system_directory);
            break;
         case MENU_LABEL_ASSETS_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_assets_directory);
            break;
         case MENU_LABEL_SAVEFILE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_savefile_directory);
            break;
         case MENU_LABEL_OVERLAY_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_overlay_directory);
            break;
         case MENU_LABEL_RGUI_BROWSER_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_browser_directory);
            break;
         case MENU_LABEL_PLAYLIST_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_playlist_directory);
            break;
         case MENU_LABEL_CONTENT_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_content_directory);
            break;
         case MENU_LABEL_SCREENSHOT_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_screenshot_directory);
            break;
         case MENU_LABEL_VIDEO_SHADER_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_shader_directory);
            break;
         case MENU_LABEL_VIDEO_FILTER_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_filter_directory);
            break;
         case MENU_LABEL_AUDIO_FILTER_DIR:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_audio_filter_directory);
            break;
         case MENU_LABEL_CURSOR_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cursor_directory);
            break;
         case MENU_LABEL_RECORDING_CONFIG_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_recording_config_directory);
            break;
         case MENU_LABEL_RECORDING_OUTPUT_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_recording_output_directory);
            break;
         case MENU_LABEL_OSK_OVERLAY_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_onscreen_overlay_keyboard_directory);
            break;
         case MENU_LABEL_INPUT_REMAPPING_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_input_remapping_directory);
            break;
         case MENU_LABEL_CONTENT_DATABASE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_content_database_directory);
            break;
         case MENU_LABEL_SAVESTATE_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_savestate_directory);
            break;
         case MENU_LABEL_DYNAMIC_WALLPAPERS_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_dynamic_wallpapers_directory);
            break;
         case MENU_LABEL_CORE_ASSETS_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_core_assets_directory);
            break;
         case MENU_LABEL_RGUI_CONFIG_DIRECTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_config_directory);
            break;
         case MENU_LABEL_INFORMATION_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_information_list);
            break;
         case MENU_LABEL_SETTINGS:
            BIND_ACTION_GET_TITLE(cbs, action_get_settings_list);
            break;
         case MENU_LABEL_DATABASE_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_database_manager_list);
            break;
         case MENU_LABEL_SYSTEM_INFORMATION:
            BIND_ACTION_GET_TITLE(cbs, action_get_system_information_list);
            break;
         case MENU_LABEL_NETWORK_INFORMATION:
            BIND_ACTION_GET_TITLE(cbs, action_get_network_information_list);
            break;
         case MENU_LABEL_CURSOR_MANAGER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_cursor_manager_list);
            break;
         case MENU_LABEL_CORE_INFORMATION:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_information_list);
            break;
         case MENU_LABEL_CORE_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_list);
            break;
         case MENU_LABEL_LOAD_CONTENT_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_load_content_list);
            break;
         case MENU_LABEL_ONLINE_UPDATER:
            BIND_ACTION_GET_TITLE(cbs, action_get_online_updater_list);
            break;
         case MENU_LABEL_NETPLAY:
            BIND_ACTION_GET_TITLE(cbs, action_get_netplay_list);
            break;
         case MENU_LABEL_DEFERRED_THUMBNAILS_UPDATER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_online_thumbnails_updater_list);
            break;
         case MENU_LABEL_DEFERRED_CORE_UPDATER_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_updater_list);
            break;
         case MENU_LABEL_DEFERRED_CONFIGURATIONS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_configurations_list);
            break;
         case MENU_LABEL_ADD_CONTENT_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_add_content_list);
            break;
         case MENU_LABEL_CORE_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_options_list);
            break;
         case MENU_LABEL_LOAD_CONTENT_HISTORY:
            BIND_ACTION_GET_TITLE(cbs, action_get_load_recent_list);
            break;
         case MENU_LABEL_CONTENT_SETTINGS:
            BIND_ACTION_GET_TITLE(cbs, action_get_quick_menu_list);
            break;
         case MENU_LABEL_CORE_INPUT_REMAPPING_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_input_remapping_options_list);
            break;
         case MENU_LABEL_CORE_CHEAT_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_cheat_options_list);
            break;
         case MENU_LABEL_SHADER_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_shader_options_list);
            break;
         case MENU_LABEL_DISK_OPTIONS:
            BIND_ACTION_GET_TITLE(cbs, action_get_disk_options_list);
            break;
         case MENU_LABEL_FRONTEND_COUNTERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_frontend_counters_list);
            break;
         case MENU_LABEL_CORE_COUNTERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_core_counters_list);
            break;
         case MENU_LABEL_DEFERRED_USER_BINDS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_input_binds_list);
            break;
         case MENU_LABEL_DEFERRED_INPUT_HOTKEY_BINDS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_input_hotkey_binds_settings_list);
            break;
         case MENU_LABEL_DEFERRED_DRIVER_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_driver_settings_list);
            break;
         case MENU_LABEL_DEFERRED_VIDEO_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_video_settings_list);
            break;
         case MENU_LABEL_DEFERRED_AUDIO_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_audio_settings_list);
            break;
         case MENU_LABEL_DEFERRED_INPUT_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_input_settings_list);
            break;
         case MENU_LABEL_DEFERRED_RECORDING_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_recording_settings_list);
            break;
         case MENU_LABEL_DEFERRED_PLAYLIST_SETTINGS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_playlist_settings_list);
            break;
         case MENU_LABEL_CONTENT_COLLECTION_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_collection);
            break;
         case MENU_LABEL_ACHIEVEMENT_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cheevos_list);
            break;
         case MENU_LABEL_ACHIEVEMENT_LIST_HARDCORE:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cheevos_list_hardcore);
            break;
         case MENU_LABEL_VIDEO_SHADER_PARAMETERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_shader_parameters);
            break;
         case MENU_LABEL_VIDEO_SHADER_PRESET_PARAMETERS:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_shader_preset_parameters);
            break;
         case MENU_LABEL_MANAGEMENT:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_action_generic);
            break;
         case MENU_LABEL_DISK_IMAGE_APPEND:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_disk_image_append);
            break;
         case MENU_LABEL_VIDEO_SHADER_PRESET:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_shader_preset);
            break;
         case MENU_LABEL_CHEAT_FILE_LOAD:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cheat_file_load);
            break;
         case MENU_LABEL_REMAP_FILE_LOAD:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_remap_file_load);
            break;
         case MENU_LABEL_DEFERRED_ACCOUNTS_CHEEVOS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_user_accounts_cheevos_list);
            break;
         case MENU_LABEL_DEFERRED_CORE_CONTENT_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_download_core_content_list);
            break;
         case MENU_LABEL_DEFERRED_ACCOUNTS_LIST:
            BIND_ACTION_GET_TITLE(cbs, action_get_user_accounts_list);
            break;
         case MENU_LABEL_HELP_LIST:
         case MENU_LABEL_HELP:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_help);
            break;
         case MENU_LABEL_INPUT_OVERLAY:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_overlay);
            break;
         case MENU_LABEL_VIDEO_FONT_PATH:
         case MENU_LABEL_XMB_FONT:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_font_path);
            break;
         case MENU_LABEL_VIDEO_FILTER:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_video_filter);
            break;
         case MENU_LABEL_AUDIO_DSP_PLUGIN:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_audio_filter);
            break;
         case MENU_LABEL_CHEAT_DATABASE_PATH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_cheat_directory);
            break;
         case MENU_LABEL_LIBRETRO_DIR_PATH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_core_directory);
            break;
         case MENU_LABEL_LIBRETRO_INFO_PATH:
            BIND_ACTION_GET_TITLE(cbs, action_get_title_core_info_directory);
            break;
         default:
            return -1;
      }
   }

   return 0;
}

static int menu_cbs_init_bind_title_compare_type(menu_file_list_cbs_t *cbs,
      unsigned type)
{
   switch (type)
   {
      case MENU_SETTINGS:
         BIND_ACTION_GET_TITLE(cbs, action_get_title_menu);
         break;
      case MENU_SETTINGS_CUSTOM_BIND:
      case MENU_SETTINGS_CUSTOM_BIND_KEYBOARD:
         BIND_ACTION_GET_TITLE(cbs, action_get_title_input_settings);
         break;
      case MENU_SETTING_ACTION_CORE_DISK_OPTIONS:
         BIND_ACTION_GET_TITLE(cbs, action_get_title_action_generic);
         break;
      default:
         return -1;
   }

   return 0;
}

int menu_cbs_init_bind_title(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx,
      uint32_t label_hash)
{
   if (!cbs)
      return -1;

   BIND_ACTION_GET_TITLE(cbs, action_get_title_default);

   if (menu_cbs_init_bind_title_compare_label(cbs, label, label_hash) == 0)
      return 0;

   if (menu_cbs_init_bind_title_compare_type(cbs, type) == 0)
      return 0;

   return -1;
}
