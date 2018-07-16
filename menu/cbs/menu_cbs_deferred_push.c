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
#include <file/file_path.h>
#include <string/stdstring.h>
#include <lists/string_list.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../menu_driver.h"
#include "../menu_cbs.h"
#include "../../msg_hash.h"

#include "../../database_info.h"

#include "../../cores/internal_cores.h"

#include "../../configuration.h"
#include "../../core.h"
#include "../../core_info.h"
#include "../../runloop.h"
#include "../../verbosity.h"

#ifndef BIND_ACTION_DEFERRED_PUSH
#define BIND_ACTION_DEFERRED_PUSH(cbs, name) \
   cbs->action_deferred_push = name; \
   cbs->action_deferred_push_ident = #name;
#endif

enum
{
   PUSH_ARCHIVE_OPEN_DETECT_CORE = 0,
   PUSH_ARCHIVE_OPEN,
   PUSH_DEFAULT,
   PUSH_DETECT_CORE_LIST
};

static int deferred_push_dlist(menu_displaylist_info_t *info, enum menu_displaylist_ctl_state state)
{
   if (!menu_displaylist_ctl(state, info))
      return menu_cbs_exit();
   menu_displaylist_ctl(DISPLAYLIST_PROCESS, info);
   return 0;
}

static int deferred_push_core_information(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORE_INFO);
}

static int deferred_push_system_information(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SYSTEM_INFO);
}

static int deferred_push_network_information(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_NETWORK_INFO);
}

static int deferred_push_achievement_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ACHIEVEMENT_LIST);
}

static int deferred_push_achievement_list_hardcore(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ACHIEVEMENT_LIST_HARDCORE);
}

static int deferred_push_rdb_collection(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_PLAYLIST_COLLECTION);
}

static int deferred_main_menu_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_MAIN_MENU);
}

static int deferred_user_binds_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_USER_BINDS_LIST);
}

static int deferred_push_accounts_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ACCOUNTS_LIST);
}

static int deferred_push_driver_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_DRIVER_SETTINGS_LIST);
}

static int deferred_push_core_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORE_SETTINGS_LIST);
}

static int deferred_push_video_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_VIDEO_SETTINGS_LIST);
}

static int deferred_push_configuration_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CONFIGURATION_SETTINGS_LIST);
}

static int deferred_push_saving_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SAVING_SETTINGS_LIST);
}

static int deferred_push_logging_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_LOGGING_SETTINGS_LIST);
}

static int deferred_push_frame_throttle_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_FRAME_THROTTLE_SETTINGS_LIST);
}

static int deferred_push_rewind_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_REWIND_SETTINGS_LIST);
}

static int deferred_push_onscreen_display_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ONSCREEN_DISPLAY_SETTINGS_LIST);
}

static int deferred_push_onscreen_notifications_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ONSCREEN_NOTIFICATIONS_SETTINGS_LIST);
}

static int deferred_push_onscreen_overlay_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ONSCREEN_OVERLAY_SETTINGS_LIST);
}

static int deferred_push_menu_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_MENU_SETTINGS_LIST);
}

static int deferred_push_user_interface_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_USER_INTERFACE_SETTINGS_LIST);
}

static int deferred_push_retro_achievements_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_RETRO_ACHIEVEMENTS_SETTINGS_LIST);
}

static int deferred_push_updater_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_UPDATER_SETTINGS_LIST);
}

static int deferred_push_wifi_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_WIFI_SETTINGS_LIST);
}

static int deferred_push_network_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_NETWORK_SETTINGS_LIST);
}

static int deferred_push_netplay_lan_scan_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_NETPLAY_LAN_SCAN_SETTINGS_LIST);
}

static int deferred_push_lakka_services_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_LAKKA_SERVICES_LIST);
}

static int deferred_push_user_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_USER_SETTINGS_LIST);
}

static int deferred_push_directory_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_DIRECTORY_SETTINGS_LIST);
}

static int deferred_push_privacy_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_PRIVACY_SETTINGS_LIST);
}

static int deferred_push_audio_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_AUDIO_SETTINGS_LIST);
}

static int deferred_push_input_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_INPUT_SETTINGS_LIST);
}

static int deferred_push_recording_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_RECORDING_SETTINGS_LIST);
}

static int deferred_push_playlist_settings_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_PLAYLIST_SETTINGS_LIST);
}

static int deferred_push_input_hotkey_binds_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_INPUT_HOTKEY_BINDS_LIST);
}

static int deferred_push_accounts_cheevos_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ACCOUNTS_CHEEVOS_LIST);
}

static int deferred_push_help(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_HELP_SCREEN_LIST);
}

static int deferred_push_rdb_entry_detail(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_DATABASE_ENTRY);
}

static int deferred_push_rpl_entry_actions(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_HORIZONTAL_CONTENT_ACTIONS);
}

static int deferred_push_core_list_deferred(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORES_SUPPORTED);
}

static int deferred_push_core_collection_list_deferred(
      menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORES_COLLECTION_SUPPORTED);
}

static int deferred_push_database_manager_list_deferred(
      menu_displaylist_info_t *info)
{
   strlcpy(info->path_b,    info->path, sizeof(info->path_b));
   info->path_c[0] = '\0';

   return deferred_push_dlist(info, DISPLAYLIST_DATABASE_QUERY);
}

static int deferred_push_video_shader_preset_parameters(
      menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SHADER_PARAMETERS_PRESET);
}

static int deferred_push_video_shader_parameters(
      menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SHADER_PARAMETERS);
}

static int deferred_push_settings(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SETTINGS_ALL);
}

static int deferred_push_shader_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OPTIONS_SHADERS);
}

static int deferred_push_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OPTIONS);
}

static int deferred_push_netplay(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_NETPLAY);
}

static int deferred_push_content_settings(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CONTENT_SETTINGS);
}

static int deferred_push_add_content_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ADD_CONTENT_LIST);
}

static int deferred_push_configurations_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CONFIGURATIONS_LIST);
}

static int deferred_push_load_content_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_LOAD_CONTENT_LIST);
}

static int deferred_push_information_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_INFORMATION_LIST);
}

static int deferred_push_management_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OPTIONS_MANAGEMENT);
}

static int deferred_push_core_counters(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_PERFCOUNTERS_CORE);
}

static int deferred_push_frontend_counters(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_PERFCOUNTERS_FRONTEND);
}

static int deferred_push_core_cheat_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OPTIONS_CHEATS);
}

static int deferred_push_core_input_remapping_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OPTIONS_REMAPPINGS);
}

static int deferred_push_core_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORE_OPTIONS);
}

static int deferred_push_disk_options(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OPTIONS_DISK);
}

#ifdef HAVE_NETWORKING
static int deferred_push_thumbnails_updater_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_THUMBNAILS_UPDATER);
}

static int deferred_push_core_updater_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORES_UPDATER);
}

static int deferred_push_core_content_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORE_CONTENT);
}

static int deferred_push_core_content_dirs_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORE_CONTENT_DIRS);
}

static int deferred_push_core_content_dirs_subdir_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORE_CONTENT_DIRS_SUBDIR);
}

static int deferred_push_lakka_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_LAKKA);
}

#endif

static int deferred_archive_action_detect_core(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ARCHIVE_ACTION_DETECT_CORE);
}

static int deferred_archive_action(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_ARCHIVE_ACTION);
}

static int deferred_push_cursor_manager_list_deferred(
      menu_displaylist_info_t *info)
{
   char rdb_path[PATH_MAX_LENGTH];
   int ret                        = -1;
   char *query                    = NULL;
   char *rdb                      = NULL;
   settings_t *settings           = config_get_ptr();
   config_file_t *conf            = config_file_new(info->path);

   if (!conf || !settings)
      goto end;

   if (!config_get_string(conf, "query", &query))
      goto end;

   if (!config_get_string(conf, "rdb", &rdb))
      goto end;

   rdb_path[0] = '\0';

   fill_pathname_join(rdb_path, settings->path.content_database,
         rdb, sizeof(rdb_path));

   strlcpy(info->path_b, info->path, sizeof(info->path_b));
   strlcpy(info->path,   rdb_path,   sizeof(info->path));
   strlcpy(info->path_c,    query,   sizeof(info->path_c));

   ret = deferred_push_dlist(info, DISPLAYLIST_DATABASE_QUERY);

end:
   if (conf)
      config_file_free(conf);
   free(rdb);
   free(query);
   return ret;
}


#ifdef HAVE_LIBRETRODB
static int deferred_push_cursor_manager_list_generic(
      menu_displaylist_info_t *info, enum database_query_type type)
{
   int ret                       = -1;
   char query[PATH_MAX_LENGTH];
   struct string_list *str_list  = string_split(info->path, "|"); 

   query[0] = '\0';

   database_info_build_query_enum(query, sizeof(query), type, str_list->elems[0].data);

   if (string_is_empty(query))
      goto end;

   strlcpy(info->path,   str_list->elems[1].data, sizeof(info->path));
   strlcpy(info->path_b, str_list->elems[0].data, sizeof(info->path_b));
   strlcpy(info->path_c, query, sizeof(info->path_c));

   ret = deferred_push_dlist(info, DISPLAYLIST_DATABASE_QUERY);

end:
   string_list_free(str_list);
   return ret;
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_max_users(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_MAX_USERS);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_famitsu_magazine_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_FAMITSU_MAGAZINE_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_edge_magazine_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_EDGE_MAGAZINE_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_edge_magazine_issue(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_EDGE_MAGAZINE_ISSUE);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_elspa_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_ELSPA_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_cero_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_CERO_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_pegi_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_PEGI_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_bbfc_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_BBFC_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_esrb_rating(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_ESRB_RATING);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_enhancement_hw(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_ENHANCEMENT_HW);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_franchise(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_FRANCHISE);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_publisher(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_PUBLISHER);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_developer(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_DEVELOPER);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_origin(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_ORIGIN);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_releasemonth(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_RELEASEDATE_MONTH);
}

static int deferred_push_cursor_manager_list_deferred_query_rdb_entry_releaseyear(
      menu_displaylist_info_t *info)
{
   return deferred_push_cursor_manager_list_generic(info, DATABASE_QUERY_ENTRY_RELEASEDATE_YEAR);
}
#endif

#if 0
static int deferred_push_cursor_manager_list_deferred_query_subsearch(
      menu_displaylist_info_t *info)
{
   int ret                       = -1;
#ifdef HAVE_LIBRETRODB
   char query[PATH_MAX_LENGTH];
   struct string_list *str_list  = string_split(info->path, "|"); 

   query[0] = '\0';

   database_info_build_query(query, sizeof(query),
         info->label, str_list->elems[0].data);

   if (string_is_empty(query))
      goto end;

   strlcpy(info->path,   str_list->elems[1].data, sizeof(info->path));
   strlcpy(info->path_b, str_list->elems[0].data, sizeof(info->path_b));
   strlcpy(info->path_c, query, sizeof(info->path_c));

   ret = deferred_push_dlist(info, DISPLAYLIST_DATABASE_QUERY);

end:
   string_list_free(str_list);
#endif
   return ret;
}
#endif

static int general_push(menu_displaylist_info_t *info,
      unsigned id, enum menu_displaylist_ctl_state state)
{
   struct retro_system_info *system_menu = NULL;
   settings_t        *settings = config_get_ptr();
   rarch_system_info_t *system = NULL;
   core_info_list_t *list      = NULL;
   menu_handle_t        *menu  = NULL;

   if (!menu_driver_ctl(RARCH_MENU_CTL_DRIVER_DATA_GET, &menu))
      return menu_cbs_exit();

   core_info_get_list(&list);

   menu_driver_ctl(RARCH_MENU_CTL_SYSTEM_INFO_GET, &system_menu);
   runloop_ctl(RUNLOOP_CTL_SYSTEM_INFO_GET, &system);

   switch (id)
   {
      case PUSH_DEFAULT:
      case PUSH_DETECT_CORE_LIST:
         break;
      default:
         fill_pathname_join(info->path, menu->scratch2_buf,
               menu->scratch_buf, sizeof(info->path));
         fill_pathname_join(info->label, menu->scratch2_buf,
               menu->scratch_buf, sizeof(info->label));
         break;
   }

   info->type_default = FILE_TYPE_PLAIN;

   switch (id)
   {
      case PUSH_ARCHIVE_OPEN_DETECT_CORE:
      case PUSH_ARCHIVE_OPEN:
      case PUSH_DEFAULT:
         info->setting      = menu_setting_find_enum(info->enum_idx);
         break;
      default:
         break;
   }

   switch (id)
   {
      case PUSH_ARCHIVE_OPEN_DETECT_CORE:
         if (list && !string_is_empty(list->all_ext))
            strlcpy(info->exts, list->all_ext, sizeof(info->exts));
         else if (system_menu->valid_extensions)
         {
            if (!string_is_empty(system_menu->valid_extensions))
               strlcpy(info->exts, system_menu->valid_extensions,
                     sizeof(info->exts));
         }
         else
            strlcpy(info->exts, system->valid_extensions, sizeof(info->exts));

         break;
      case PUSH_ARCHIVE_OPEN:
         if (system_menu->valid_extensions)
         {
            if (*system_menu->valid_extensions)
               strlcpy(info->exts, system_menu->valid_extensions,
                     sizeof(info->exts));
         }
         else
            strlcpy(info->exts, system->valid_extensions, sizeof(info->exts));
         break;
      case PUSH_DEFAULT:
         if (menu_setting_get_browser_selection_type(info->setting) == ST_DIR)
         {
         }
         else if (system_menu->valid_extensions)
         {
            if (*system_menu->valid_extensions)
               strlcpy(info->exts, system_menu->valid_extensions,
                     sizeof(info->exts));
         }
         else
         {
            if (!string_is_empty(system->valid_extensions))
               strlcpy(info->exts, system->valid_extensions, sizeof(info->exts));
         }
         break;
      case PUSH_DETECT_CORE_LIST:
         if (list && !string_is_empty(list->all_ext))
            strlcpy(info->exts, list->all_ext, sizeof(info->exts));
         else if (system_menu->valid_extensions)
         {
            if (!string_is_empty(system_menu->valid_extensions))
               strlcpy(info->exts, system_menu->valid_extensions,
                     sizeof(info->exts));
         }
         break;
   }

   (void)settings;

   if (settings->multimedia.builtin_mediaplayer_enable ||
         settings->multimedia.builtin_imageviewer_enable)
   {
      struct retro_system_info sysinfo = {0};

      (void)sysinfo;
#ifdef HAVE_FFMPEG
      if (settings->multimedia.builtin_mediaplayer_enable)
      {
         libretro_ffmpeg_retro_get_system_info(&sysinfo);
         strlcat(info->exts, "|", sizeof(info->exts));
         strlcat(info->exts, sysinfo.valid_extensions, sizeof(info->exts));
      }
#endif
#ifdef HAVE_IMAGEVIEWER
      if (settings->multimedia.builtin_imageviewer_enable)
      {
         libretro_imageviewer_retro_get_system_info(&sysinfo);
         strlcat(info->exts, "|", sizeof(info->exts));
         strlcat(info->exts, sysinfo.valid_extensions, sizeof(info->exts));
      }
#endif
   }

   return deferred_push_dlist(info, state);
}

static int deferred_push_detect_core_list(menu_displaylist_info_t *info)
{
   return general_push(info, PUSH_DETECT_CORE_LIST,
         DISPLAYLIST_CORES_DETECTED);
}

static int deferred_archive_open_detect_core(menu_displaylist_info_t *info)
{
   return general_push(info, PUSH_ARCHIVE_OPEN_DETECT_CORE,
         DISPLAYLIST_DEFAULT);
}

static int deferred_archive_open(menu_displaylist_info_t *info)
{
   return general_push(info, PUSH_ARCHIVE_OPEN, DISPLAYLIST_DEFAULT);
}

static int deferred_push_default(menu_displaylist_info_t *info)
{
   return general_push(info, PUSH_DEFAULT, DISPLAYLIST_DEFAULT);
}

static int deferred_push_history_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_HISTORY);
}

static int deferred_push_database_manager_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_DATABASES);
}

static int deferred_push_cursor_manager_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_DATABASE_CURSORS);
}

static int deferred_push_content_collection_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_DATABASE_PLAYLISTS);
}

static int deferred_push_browse_url_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_BROWSE_URL_LIST);
}

static int deferred_push_browse_url_start(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_BROWSE_URL_START);
}

static int deferred_push_core_list(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CORES);
}

static int deferred_push_configurations(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CONFIG_FILES);
}

static int deferred_push_video_shader_preset(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SHADER_PRESET);
}

static int deferred_push_video_shader_pass(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_SHADER_PASS);
}

static int deferred_push_video_filter(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_VIDEO_FILTERS);
}

static int deferred_push_images(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_IMAGES);
}

static int deferred_push_audio_dsp_plugin(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_AUDIO_FILTERS);
}

static int deferred_push_cheat_file_load(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CHEAT_FILES);
}

static int deferred_push_remap_file_load(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_REMAP_FILES);
}

static int deferred_push_record_configfile(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_RECORD_CONFIG_FILES);
}

static int deferred_push_input_overlay(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_OVERLAYS);
}

static int deferred_push_video_font_path(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_FONTS);
}

static int deferred_push_xmb_font_path(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_FONTS);
}

static int deferred_push_content_history_path(menu_displaylist_info_t *info)
{
   return deferred_push_dlist(info, DISPLAYLIST_CONTENT_HISTORY);
}

static int menu_cbs_init_bind_deferred_push_compare_label(
      menu_file_list_cbs_t *cbs, 
      const char *label, uint32_t label_hash)
{
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_BROWSE_URL_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_browse_url_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_BROWSE_URL_START)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_browse_url_start);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CORE_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CONFIGURATION_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_configuration_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_SAVING_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_saving_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_LOGGING_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_logging_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_FRAME_THROTTLE_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_frame_throttle_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_REWIND_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_rewind_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_ONSCREEN_DISPLAY_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_onscreen_display_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_ONSCREEN_NOTIFICATIONS_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_onscreen_notifications_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_ONSCREEN_OVERLAY_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_onscreen_overlay_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_MENU_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_menu_settings_list);
      return 0;
   }
   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_USER_INTERFACE_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_user_interface_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_RETRO_ACHIEVEMENTS_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_retro_achievements_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_UPDATER_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_updater_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_NETWORK_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_network_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_WIFI_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_wifi_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_NETPLAY_LAN_SCAN_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_netplay_lan_scan_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_LAKKA_SERVICES_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_lakka_services_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_USER_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_user_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_DIRECTORY_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_directory_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_PRIVACY_SETTINGS_LIST)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_privacy_settings_list);
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_LIST)))
   {
#ifdef HAVE_NETWORKING
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_content_dirs_list);
#endif
      return 0;
   }

   if (string_is_equal(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_SUBDIR_LIST)))
   {
#ifdef HAVE_NETWORKING
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_content_dirs_subdir_list);
#endif
      return 0;
   }

   if (strstr(label, msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_RDB_ENTRY_DETAIL)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_rdb_entry_detail);
   }
   else if (strstr(label,
            msg_hash_to_str(MENU_ENUM_LABEL_DEFERRED_RPL_ENTRY_ACTIONS)))
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_rpl_entry_actions);
   }
   else
   {
      if (cbs->enum_idx != MSG_UNKNOWN)
      {
         switch (cbs->enum_idx)
         {
            case MENU_ENUM_LABEL_MAIN_MENU:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_main_menu_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_USER_BINDS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_user_binds_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ACCOUNTS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_accounts_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_INPUT_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_input_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_PLAYLIST_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_playlist_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_RECORDING_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_recording_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_INPUT_HOTKEY_BINDS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_input_hotkey_binds_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ACCOUNTS_CHEEVOS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_accounts_cheevos_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ARCHIVE_ACTION_DETECT_CORE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_action_detect_core);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ARCHIVE_ACTION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_action);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ARCHIVE_OPEN_DETECT_CORE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_open_detect_core);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ARCHIVE_OPEN:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_open);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_content_list);
#endif
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_content_dirs_list);
#endif
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_CONTENT_DIRS_SUBDIR_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_content_dirs_subdir_list);
#endif
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_UPDATER_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_updater_list);
#endif
               break;
            case MENU_ENUM_LABEL_DEFERRED_THUMBNAILS_UPDATER_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_thumbnails_updater_list);
#endif
               break;
            case MENU_ENUM_LABEL_DEFERRED_LAKKA_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_lakka_list);
#endif
               break;
            case MENU_ENUM_LABEL_LOAD_CONTENT_HISTORY:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_history_list);
               break;
            case MENU_ENUM_LABEL_DATABASE_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_database_manager_list);
               break;
            case MENU_ENUM_LABEL_CURSOR_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list);
               break;
            case MENU_ENUM_LABEL_CHEAT_FILE_LOAD:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cheat_file_load);
               break;
            case MENU_ENUM_LABEL_REMAP_FILE_LOAD:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_remap_file_load);
               break;
            case MENU_ENUM_LABEL_RECORD_CONFIG:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_record_configfile);
               break;
            case MENU_ENUM_LABEL_SHADER_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_shader_options);
               break;
            case MENU_ENUM_LABEL_ONLINE_UPDATER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_options);
               break;
            case MENU_ENUM_LABEL_NETPLAY:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_netplay);
               break;
            case MENU_ENUM_LABEL_CONTENT_SETTINGS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_content_settings);
               break;
            case MENU_ENUM_LABEL_ADD_CONTENT_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_add_content_list);
               break;
            case MENU_ENUM_LABEL_CONFIGURATIONS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_configurations_list);
               break;
            case MENU_ENUM_LABEL_LOAD_CONTENT_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_load_content_list);
               break;
            case MENU_ENUM_LABEL_INFORMATION_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_information_list);
               break;
            case MENU_ENUM_LABEL_MANAGEMENT:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_management_options);
               break;
            case MENU_ENUM_LABEL_HELP_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_help);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_list_deferred);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_LIST_SET:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_collection_list_deferred);
               break;
            case MENU_ENUM_LABEL_DEFERRED_VIDEO_FILTER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_filter);
               break;
            case MENU_ENUM_LABEL_DEFERRED_DATABASE_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_database_manager_list_deferred);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred);
               break;
#ifdef HAVE_LIBRETRODB
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PUBLISHER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_publisher);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_DEVELOPER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_developer);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ORIGIN:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_origin);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_FRANCHISE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_franchise);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ENHANCEMENT_HW:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_enhancement_hw);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ESRB_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_esrb_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_BBFC_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_bbfc_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ELSPA_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_elspa_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PEGI_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_pegi_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_CERO_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_cero_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_edge_magazine_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_ISSUE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_edge_magazine_issue);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_FAMITSU_MAGAZINE_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_famitsu_magazine_rating);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_MAX_USERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_max_users);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEMONTH:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_releasemonth);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEYEAR:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_releaseyear);
               break;
#endif
            case MENU_ENUM_LABEL_CORE_INFORMATION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_information);
               break;
            case MENU_ENUM_LABEL_SYSTEM_INFORMATION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_system_information);
               break;
            case MENU_ENUM_LABEL_NETWORK_INFORMATION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_network_information);
               break;
            case MENU_ENUM_LABEL_ACHIEVEMENT_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_achievement_list);
               break;
            case MENU_ENUM_LABEL_ACHIEVEMENT_LIST_HARDCORE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_achievement_list_hardcore);
               break;
            case MENU_ENUM_LABEL_CORE_COUNTERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_counters);
               break;
            case MENU_ENUM_LABEL_FRONTEND_COUNTERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_frontend_counters);
               break;
            case MENU_ENUM_LABEL_VIDEO_SHADER_PRESET_PARAMETERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_preset_parameters);
               break;
            case MENU_ENUM_LABEL_VIDEO_SHADER_PARAMETERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_parameters);
               break;
            case MENU_ENUM_LABEL_SETTINGS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_settings);
               break;
            case MENU_ENUM_LABEL_CORE_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_options);
               break;
            case MENU_ENUM_LABEL_CORE_CHEAT_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_cheat_options);
               break;
            case MENU_ENUM_LABEL_CORE_INPUT_REMAPPING_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_input_remapping_options);
               break;
            case MENU_ENUM_LABEL_CORE_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_list);
               break;
            case MENU_ENUM_LABEL_CONTENT_COLLECTION_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_content_collection_list);
               break;
            case MENU_ENUM_LABEL_CONFIGURATIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_configurations);
               break;
            case MENU_ENUM_LABEL_VIDEO_SHADER_PRESET:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_preset);
               break;
            case MENU_ENUM_LABEL_VIDEO_SHADER_PASS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_pass);
               break;
            case MENU_ENUM_LABEL_VIDEO_FILTER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_filter);
               break;
            case MENU_ENUM_LABEL_MENU_WALLPAPER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_images);
               break;
            case MENU_ENUM_LABEL_AUDIO_DSP_PLUGIN:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_audio_dsp_plugin);
               break;
            case MENU_ENUM_LABEL_INPUT_OVERLAY:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_input_overlay);
               break;
            case MENU_ENUM_LABEL_VIDEO_FONT_PATH:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_font_path);
               break;
            case MENU_ENUM_LABEL_XMB_FONT:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_xmb_font_path);
               break;
            case MENU_ENUM_LABEL_CONTENT_HISTORY_PATH:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_content_history_path);
               break;
            case MENU_ENUM_LABEL_DEFERRED_VIDEO_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CONFIGURATION_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_configuration_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_SAVING_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_saving_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_LOGGING_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_saving_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_FRAME_THROTTLE_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_frame_throttle_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_REWIND_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_rewind_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ONSCREEN_DISPLAY_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_onscreen_display_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_ONSCREEN_OVERLAY_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_onscreen_overlay_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_AUDIO_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_audio_settings_list);
               break;
            case MENU_ENUM_LABEL_DEFERRED_CORE_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_settings_list);
               break;
            case MENU_ENUM_LABEL_DOWNLOADED_FILE_DETECT_CORE_LIST:
            case MENU_ENUM_LABEL_FAVORITES:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_detect_core_list);
               break;
            default:
               return -1;
         }
      }
      else
      {
         switch (label_hash)
         {
            case MENU_LABEL_DEFERRED_CONFIGURATIONS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_configurations_list);
               break;
            case MENU_LABEL_DEFERRED_USER_BINDS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_user_binds_list);
               break;
            case MENU_LABEL_DEFERRED_ACCOUNTS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_accounts_list);
               break;
            case MENU_LABEL_DEFERRED_DRIVER_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_driver_settings_list);
               break;
            case MENU_LABEL_DEFERRED_VIDEO_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_settings_list);
               break;
            case MENU_LABEL_DEFERRED_AUDIO_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_audio_settings_list);
               break;
            case MENU_LABEL_DEFERRED_INPUT_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_input_settings_list);
               break;
            case MENU_LABEL_DEFERRED_PLAYLIST_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_playlist_settings_list);
               break;
            case MENU_LABEL_DEFERRED_RECORDING_SETTINGS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_recording_settings_list);
               break;
            case MENU_LABEL_DEFERRED_INPUT_HOTKEY_BINDS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_input_hotkey_binds_list);
               break;
            case MENU_LABEL_DEFERRED_ACCOUNTS_CHEEVOS_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_accounts_cheevos_list);
               break;
            case MENU_LABEL_DEFERRED_ARCHIVE_ACTION_DETECT_CORE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_action_detect_core);
               break;
            case MENU_LABEL_DEFERRED_ARCHIVE_ACTION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_action);
               break;
            case MENU_LABEL_DEFERRED_ARCHIVE_OPEN_DETECT_CORE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_open_detect_core);
               break;
            case MENU_LABEL_DEFERRED_ARCHIVE_OPEN:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_archive_open);
               break;
            case MENU_LABEL_DEFERRED_CORE_CONTENT_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_content_list);
#endif
               break;
            case MENU_LABEL_DEFERRED_CORE_UPDATER_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_updater_list);
#endif
               break;
            case MENU_LABEL_DEFERRED_THUMBNAILS_UPDATER_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_thumbnails_updater_list);
#endif
               break;
            case MENU_LABEL_DEFERRED_LAKKA_LIST:
#ifdef HAVE_NETWORKING
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_lakka_list);
#endif
               break;
            case MENU_LABEL_LOAD_CONTENT_HISTORY:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_history_list);
               break;
            case MENU_LABEL_DATABASE_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_database_manager_list);
               break;
            case MENU_LABEL_CURSOR_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list);
               break;
            case MENU_LABEL_CHEAT_FILE_LOAD:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cheat_file_load);
               break;
            case MENU_LABEL_REMAP_FILE_LOAD:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_remap_file_load);
               break;
            case MENU_LABEL_RECORD_CONFIG:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_record_configfile);
               break;
            case MENU_LABEL_SHADER_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_shader_options);
               break;
            case MENU_LABEL_ONLINE_UPDATER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_options);
               break;
            case MENU_LABEL_NETPLAY:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_netplay);
               break;
            case MENU_LABEL_CONTENT_SETTINGS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_content_settings);
               break;
            case MENU_LABEL_ADD_CONTENT_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_add_content_list);
               break;
            case MENU_LABEL_LOAD_CONTENT_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_load_content_list);
               break;
            case MENU_LABEL_INFORMATION_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_information_list);
               break;
            case MENU_LABEL_MANAGEMENT:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_management_options);
               break;
            case MENU_LABEL_HELP_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_help);
               break;
            case MENU_LABEL_DEFERRED_CORE_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_list_deferred);
               break;
            case MENU_LABEL_DEFERRED_CORE_LIST_SET:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_collection_list_deferred);
               break;
            case MENU_LABEL_DEFERRED_VIDEO_FILTER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_filter);
               break;
            case MENU_LABEL_DEFERRED_DATABASE_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_database_manager_list_deferred);
               break;
#ifdef HAVE_LIBRETRODB
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PUBLISHER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_publisher);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_DEVELOPER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_developer);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ORIGIN:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_origin);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_FRANCHISE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_franchise);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ENHANCEMENT_HW:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_enhancement_hw);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ESRB_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_esrb_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_BBFC_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_bbfc_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_ELSPA_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_elspa_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_PEGI_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_pegi_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_CERO_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_cero_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_edge_magazine_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_EDGE_MAGAZINE_ISSUE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_edge_magazine_issue);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_FAMITSU_MAGAZINE_RATING:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_famitsu_magazine_rating);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_MAX_USERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_max_users);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEMONTH:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_releasemonth);
               break;
            case MENU_LABEL_DEFERRED_CURSOR_MANAGER_LIST_RDB_ENTRY_RELEASEYEAR:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_cursor_manager_list_deferred_query_rdb_entry_releaseyear);
               break;
#endif
            case MENU_LABEL_CORE_INFORMATION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_information);
               break;
            case MENU_LABEL_SYSTEM_INFORMATION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_system_information);
               break;
            case MENU_LABEL_NETWORK_INFORMATION:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_network_information);
               break;
            case MENU_LABEL_ACHIEVEMENT_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_achievement_list);
               break;
            case MENU_LABEL_ACHIEVEMENT_LIST_HARDCORE:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_achievement_list_hardcore);
               break;
            case MENU_LABEL_CORE_COUNTERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_counters);
               break;
            case MENU_LABEL_FRONTEND_COUNTERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_frontend_counters);
               break;
            case MENU_LABEL_VIDEO_SHADER_PRESET_PARAMETERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_preset_parameters);
               break;
            case MENU_LABEL_VIDEO_SHADER_PARAMETERS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_parameters);
               break;
            case MENU_LABEL_SETTINGS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_settings);
               break;
            case MENU_LABEL_CORE_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_options);
               break;
            case MENU_LABEL_CORE_CHEAT_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_cheat_options);
               break;
            case MENU_LABEL_CORE_INPUT_REMAPPING_OPTIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_input_remapping_options);
               break;
            case MENU_LABEL_CORE_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_core_list);
               break;
            case MENU_LABEL_CONTENT_COLLECTION_LIST:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_content_collection_list);
               break;
            case MENU_LABEL_CONFIGURATIONS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_configurations);
               break;
            case MENU_LABEL_VIDEO_SHADER_PRESET:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_preset);
               break;
            case MENU_LABEL_VIDEO_SHADER_PASS:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_shader_pass);
               break;
            case MENU_LABEL_VIDEO_FILTER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_filter);
               break;
            case MENU_LABEL_MENU_WALLPAPER:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_images);
               break;
            case MENU_LABEL_AUDIO_DSP_PLUGIN:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_audio_dsp_plugin);
               break;
            case MENU_LABEL_INPUT_OVERLAY:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_input_overlay);
               break;
            case MENU_LABEL_VIDEO_FONT_PATH:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_video_font_path);
               break;
            case MENU_LABEL_XMB_FONT:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_xmb_font_path);
               break;
            case MENU_LABEL_CONTENT_HISTORY_PATH:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_content_history_path);
               break;
            case MENU_LABEL_DOWNLOADED_FILE_DETECT_CORE_LIST:
            case MENU_LABEL_FAVORITES:
               BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_detect_core_list);
               break;
            default:
               return -1;
         }
      }
   }

   return 0;
}

static int menu_cbs_init_bind_deferred_push_compare_type(
      menu_file_list_cbs_t *cbs, unsigned type)
{
   if (type == FILE_TYPE_PLAYLIST_COLLECTION)
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_rdb_collection);
   }
   else if (type == MENU_SETTING_ACTION_CORE_DISK_OPTIONS)
   {
      BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_disk_options);
   }
   else
      return -1;

   return 0;
}

int menu_cbs_init_bind_deferred_push(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx,
      uint32_t label_hash)
{
   if (!cbs)
      return -1;

   BIND_ACTION_DEFERRED_PUSH(cbs, deferred_push_default);

   if (menu_cbs_init_bind_deferred_push_compare_label(
            cbs, label, label_hash) == 0)
      return 0;

   if (menu_cbs_init_bind_deferred_push_compare_type(
            cbs, type) == 0)
      return 0;

   return -1;
}
