/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C) 2014-2016 - Jean-André Santoni
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

#ifndef __RARCH_CONFIGURATION_H__
#define __RARCH_CONFIGURATION_H__

#include <stdint.h>

#include <boolean.h>
#include <retro_common_api.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/video_driver.h"
#include "input/input_defines.h"

enum override_type
{
   OVERRIDE_NONE = 0,
   OVERRIDE_CORE,
   OVERRIDE_GAME
};

RETRO_BEGIN_DECLS

typedef struct settings
{
   video_viewport_t video_viewport_custom;

   char playlist_names[PATH_MAX_LENGTH];
   char playlist_cores[PATH_MAX_LENGTH];

   bool bundle_finished;

   struct
   {
      char driver[32];
      char context_driver[32];
      float scale;
      unsigned window_x;
      unsigned window_y;
      bool fullscreen;
      bool windowed_fullscreen;
      unsigned monitor_index;
      unsigned fullscreen_x;
      unsigned fullscreen_y;
      bool vsync;
      bool hard_sync;
      bool black_frame_insertion;
      unsigned max_swapchain_images;
      unsigned swap_interval;
      unsigned hard_sync_frames;
      unsigned frame_delay;
#ifdef GEKKO
      unsigned viwidth;
      bool vfilter;
#endif
      bool smooth;
      bool force_aspect;
      bool crop_overscan;
      float aspect_ratio;
      bool aspect_ratio_auto;
      bool scale_integer;
      unsigned aspect_ratio_idx;
      unsigned rotation;

      bool shader_enable;

      float refresh_rate;
      bool threaded;


      float font_size;
      bool font_enable;
      float msg_pos_x;
      float msg_pos_y;
      float msg_color_r;
      float msg_color_g;
      float msg_color_b;

      bool disable_composition;

      bool post_filter_record;
      bool gpu_record;
      bool gpu_screenshot;

      bool allow_rotate;
      bool shared_context;
      bool force_srgb_disable;
   } video;

   struct
   {
      char driver[32];
   } record;

   struct
   {
      bool menubar_enable;
      bool suspend_screensaver_enable;
      bool companion_start_on_boot;
      bool companion_enable;
   } ui;

#ifdef HAVE_MENU
   struct
   {
      char driver[32];
      bool pause_libretro;
      bool timedate_enable;
      bool battery_level_enable;
      bool core_enable;
      bool dynamic_wallpaper_enable;
      unsigned thumbnails;
      bool throttle;

      struct
      {
         float opacity;
      } wallpaper;

      struct
      {
         float opacity;
      } footer;

      struct
      {
         float opacity;
      } header;

      struct
      {
         bool enable;
      } mouse;

      struct
      {
         bool enable;
      } pointer;

      struct
      {
         struct
         {
            bool enable;
         } wraparound;
         struct
         {
            struct
            {
               bool supported_extensions_enable;
            } filter;
         } browser;
      } navigation;

      struct
      {
         bool     override_enable;
         unsigned override_value;
      } dpi;

      bool show_advanced_settings;

      unsigned entry_normal_color;
      unsigned entry_hover_color;
      unsigned title_color;
      bool throttle_framerate;
      bool linear_filter;

      struct
      {
         unsigned shader_pipeline;
         char     font[PATH_MAX_LENGTH];
         unsigned scale_factor;
         unsigned alpha_factor;
         unsigned theme;
         unsigned menu_color_theme;
         bool     shadows_enable;
         bool     show_settings;
         bool     show_images;
         bool     show_music;
         bool     show_video;
         bool     show_history;
         bool     show_add;
      } xmb;

      struct
      {
         unsigned menu_color_theme;
      } materialui;

      bool unified_controls;
   } menu;
#endif

#ifdef HAVE_THREADS
   bool threaded_data_runloop_enable;
#endif

   struct
   {
      char driver[32];
      char device[255];
      bool allow;
      unsigned width;
      unsigned height;
   } camera;

   struct
   {
      char driver[32];
      bool allow;
   } wifi;

   struct
   {
      char driver[32];
      bool allow;
      int update_interval_ms;
      int update_interval_distance;
   } location;

   struct
   {
      char driver[32];
      char resampler[32];
      char device[255];
      bool enable;
      bool mute_enable;
      unsigned out_rate;
      unsigned block_frames;
      unsigned latency;
      bool sync;


      bool rate_control;
      float rate_control_delta;
      float max_timing_skew;
      float volume; /* dB scale. */
   } audio;

   struct
   {
      char driver[32];
      char joypad_driver[32];
      char keyboard_layout[64];
      char device_names[MAX_USERS][64];

      unsigned remap_ids[MAX_USERS][RARCH_BIND_LIST_END];
      struct retro_keybind binds[MAX_USERS][RARCH_BIND_LIST_END];
      struct retro_keybind autoconf_binds[MAX_USERS][RARCH_BIND_LIST_END];

      unsigned max_users;

      /* Set by autoconfiguration in joypad_autoconfig_dir.
       * Does not override main binds. */
      bool autoconfigured[MAX_USERS];
      int vid[MAX_USERS];
      int pid[MAX_USERS];

      unsigned libretro_device[MAX_USERS];
      unsigned analog_dpad_mode[MAX_USERS];

      bool remap_binds_enable;
      float axis_threshold;
      unsigned joypad_map[MAX_USERS];
      unsigned device[MAX_USERS];
      unsigned device_name_index[MAX_USERS];
      bool autodetect_enable;

      unsigned turbo_period;
      unsigned turbo_duty_cycle;

      bool overlay_enable;
      bool overlay_enable_autopreferred;
      bool overlay_hide_in_menu;
      float overlay_opacity;
      float overlay_scale;

      unsigned bind_timeout;
      bool input_descriptor_label_show;
      bool input_descriptor_hide_unbound;

      unsigned menu_toggle_gamepad_combo;
      bool all_users_control_menu;

      bool menu_swap_ok_cancel_buttons;
#if defined(VITA)
      bool backtouch_enable;
      bool backtouch_toggle;
#endif
#if TARGET_OS_IPHONE
      bool small_keyboard_enable;
#endif
      bool keyboard_gamepad_enable;
      unsigned keyboard_gamepad_mapping_type;
      unsigned poll_type_behavior;
   } input;

   struct
   {
      unsigned mode;
   } archive;

   struct
   {
      char buildbot_url[255];
      char buildbot_assets_url[255];
      bool buildbot_auto_extract_archive;
   } network;

   bool set_supports_no_game_enable;

   struct
   {
      bool builtin_mediaplayer_enable;
      bool builtin_imageviewer_enable;
   } multimedia;

#ifdef HAVE_CHEEVOS
   struct
   {
      bool enable;
      bool test_unofficial;
      bool hardcore_mode_enable;
      char username[32];
      char password[32];
   } cheevos;
#endif

   char browse_url[4096];

   int state_slot;

   bool bundle_assets_extract_enable;
   unsigned bundle_assets_extract_version_current;
   unsigned bundle_assets_extract_last_version;

   struct
   {
      char cheat_database[PATH_MAX_LENGTH];
      char content_database[PATH_MAX_LENGTH];
      char overlay[PATH_MAX_LENGTH];
      char menu_wallpaper[PATH_MAX_LENGTH];
      char audio_dsp_plugin[PATH_MAX_LENGTH];
      char softfilter_plugin[PATH_MAX_LENGTH];
      char core_options[PATH_MAX_LENGTH];
      char content_history[PATH_MAX_LENGTH];
      char content_music_history[PATH_MAX_LENGTH];
      char content_image_history[PATH_MAX_LENGTH];
      char content_video_history[PATH_MAX_LENGTH];
      char libretro_info[PATH_MAX_LENGTH];
      char cheat_settings[PATH_MAX_LENGTH];
      char bundle_assets_src[PATH_MAX_LENGTH];
      char bundle_assets_dst[PATH_MAX_LENGTH];
      char bundle_assets_dst_subdir[PATH_MAX_LENGTH];
      char shader[PATH_MAX_LENGTH];
      char font[PATH_MAX_LENGTH];
   } path;

   struct
   {
      char audio_filter[PATH_MAX_LENGTH];
      char autoconfig[PATH_MAX_LENGTH];
      char video_filter[PATH_MAX_LENGTH];
      char video_shader[PATH_MAX_LENGTH];
      char content_history[PATH_MAX_LENGTH];
      char libretro[PATH_MAX_LENGTH];
      char cursor[PATH_MAX_LENGTH];
      char input_remapping[PATH_MAX_LENGTH];
      char overlay[PATH_MAX_LENGTH];
      char resampler[PATH_MAX_LENGTH];
      char screenshot[PATH_MAX_LENGTH];
      char system[PATH_MAX_LENGTH];
      char cache[PATH_MAX_LENGTH];
      char playlist[PATH_MAX_LENGTH];
      char core_assets[PATH_MAX_LENGTH];
      char assets[PATH_MAX_LENGTH];
      char dynamic_wallpapers[PATH_MAX_LENGTH];
      char thumbnails[PATH_MAX_LENGTH];
      char menu_config[PATH_MAX_LENGTH];
      char menu_content[PATH_MAX_LENGTH];
   } directory;

#ifdef HAVE_NETWORKING
   struct
   {
      char server[255];
      unsigned port;
      bool stateless_mode;
      int check_frames;
      bool swap_input;
      bool nat_traversal;
      char password[128];
      char spectate_password[128];
   } netplay;
#endif

   unsigned content_history_size;

   unsigned libretro_log_level;

   bool auto_screenshot_filename;

   bool history_list_enable;
   bool playlist_entry_remove;
   bool rewind_enable;
   size_t rewind_buffer_size;
   unsigned rewind_granularity;

   float slowmotion_ratio;
   float fastforward_ratio;

   bool pause_nonactive;
   unsigned autosave_interval;

   bool block_sram_overwrite;
   bool savestate_auto_index;
   bool savestate_auto_save;
   bool savestate_auto_load;
   bool savestate_thumbnail_enable;

   bool network_cmd_enable;
   unsigned network_cmd_port;
   bool stdin_cmd_enable;
   bool network_remote_enable;
   bool network_remote_enable_user[MAX_USERS];
   unsigned network_remote_base_port;

#if defined(HAVE_MENU)
   bool menu_show_start_screen;
#endif
   bool fps_show;
   bool load_dummy_on_core_shutdown;
   bool check_firmware_before_loading;

   bool game_specific_options;
   bool auto_overrides_enable;
   bool auto_remaps_enable;
   bool auto_shaders_enable;

   bool sort_savefiles_enable;
   bool sort_savestates_enable;

   char username[32];
#ifdef HAVE_LANGEXTRA
   unsigned int user_language;
#endif

   bool config_save_on_exit;
   bool show_hidden_files;

#ifdef HAVE_LAKKA
   bool ssh_enable;
   bool samba_enable;
   bool bluetooth_enable;
#endif

} settings_t;

/**
 * config_get_default_camera:
 *
 * Gets default camera driver.
 *
 * Returns: Default camera driver.
 **/
const char *config_get_default_camera(void);

/**
 * config_get_default_wifi:
 *
 * Gets default wifi driver.
 *
 * Returns: Default wifi driver.
 **/
const char *config_get_default_wifi(void);

/**
 * config_get_default_location:
 *
 * Gets default location driver.
 *
 * Returns: Default location driver.
 **/
const char *config_get_default_location(void);

/**
 * config_get_default_video:
 *
 * Gets default video driver.
 *
 * Returns: Default video driver.
 **/
const char *config_get_default_video(void);

/**
 * config_get_default_audio:
 *
 * Gets default audio driver.
 *
 * Returns: Default audio driver.
 **/
const char *config_get_default_audio(void);

/**
 * config_get_default_audio_resampler:
 *
 * Gets default audio resampler driver.
 *
 * Returns: Default audio resampler driver.
 **/
const char *config_get_default_audio_resampler(void);

/**
 * config_get_default_input:
 *
 * Gets default input driver.
 *
 * Returns: Default input driver.
 **/
const char *config_get_default_input(void);

/**
 * config_get_default_joypad:
 *
 * Gets default input joypad driver.
 *
 * Returns: Default input joypad driver.
 **/
const char *config_get_default_joypad(void);

#ifdef HAVE_MENU
/**
 * config_get_default_menu:
 *
 * Gets default menu driver.
 *
 * Returns: Default menu driver.
 **/
const char *config_get_default_menu(void);
#endif

const char *config_get_default_record(void);

/**
 * config_load:
 *
 * Loads a config file and reads all the values into memory.
 *
 */
void config_load(void);

/**
 * config_load_override:
 *
 * Tries to append game-specific and core-specific configuration.
 * These settings will always have precedence, thus this feature
 * can be used to enforce overrides.
 *
 * Returns: false if there was an error or no action was performed.
 *
 */
bool config_load_override(void);

/**
 * config_unload_override:
 *
 * Unloads configuration overrides if overrides are active.
 *
 *
 * Returns: false if there was an error.
 */
bool config_unload_override(void);

/**
 * config_load_remap:
 *
 * Tries to append game-specific and core-specific remap files.
 *
 * Returns: false if there was an error or no action was performed.
 *
 */
bool config_load_remap(void);

/**
 * config_load_shader_preset:
 *
 * Tries to append game-specific and core-specific shader presets.
 *
 * Returns: false if there was an error or no action was performed.
 *
 */
bool config_load_shader_preset(void);

/**
 * config_save_autoconf_profile:
 * @path            : Path that shall be written to.
 * @user              : Controller number to save
 * Writes a controller autoconf file to disk.
 **/
bool config_save_autoconf_profile(const char *path, unsigned user);

/**
 * config_save_file:
 * @path            : Path that shall be written to.
 *
 * Writes a config file to disk.
 *
 * Returns: true (1) on success, otherwise returns false (0).
 **/
bool config_save_file(const char *path);

/**
 * config_save_overrides:
 * @path            : Path that shall be written to.
 *
 * Writes a config file override to disk.
 *
 * Returns: true (1) on success, otherwise returns false (0).
 **/
bool config_save_overrides(int override_type);

/* Replaces currently loaded configuration file with
 * another one. Will load a dummy core to flush state
 * properly. */
bool config_replace(char *path);

bool config_init(void);

bool config_overlay_enable_default(void);

void config_free(void);

settings_t *config_get_ptr(void);

RETRO_END_DECLS

#endif
