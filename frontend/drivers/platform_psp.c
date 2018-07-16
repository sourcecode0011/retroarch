/* RetroArch - A frontend for libretro.
 * Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 * Copyright (C) 2011-2016 - Daniel De Matteis
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifdef VITA
#include <psp2/power.h>
#include <psp2/sysmodule.h>
#include <psp2/appmgr.h>
#include <pthread.h>

#include "../../bootstrap/vita/sbrk.c"

#else
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspfpu.h>
#include <psppower.h>
#include <pspsdk.h>
#endif

#include <string/stdstring.h>
#include <boolean.h>
#include <file/file_path.h>
#ifndef IS_SALAMANDER
#include <lists/file_list.h>
#endif

#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif

#include "../frontend_driver.h"
#include "../../defaults.h"
#include "../../file_path_special.h"
#include "../../defines/psp_defines.h"
#include "../../retroarch.h"
#include "../../paths.h"
#include "../../verbosity.h"

#if defined(HAVE_KERNEL_PRX) || defined(IS_SALAMANDER)
#ifndef VITA
#include "../../bootstrap/psp1/kernel_functions.h"
#endif
#endif

#ifndef VITA
PSP_MODULE_INFO("RetroArch", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
#ifdef BIG_STACK
PSP_MAIN_THREAD_STACK_SIZE_KB(4*1024);
#endif
PSP_HEAP_SIZE_MAX();
#endif

char eboot_path[512];
char user_path[512];

static enum frontend_fork psp_fork_mode = FRONTEND_FORK_NONE;

static void frontend_psp_get_environment_settings(int *argc, char *argv[],
      void *args, void *params_data)
{
   struct rarch_main_wrap *params = NULL;

   (void)args;

#ifndef IS_SALAMANDER
#if defined(HAVE_LOGGER)
   logger_init();
#elif defined(HAVE_FILE_LOGGER)
#ifndef VITA
   retro_main_log_file_init("ms0:/temp/retroarch-log.txt");
#else
   retro_main_log_file_init("ux0:/temp/retroarch-log.txt");
#endif
#endif
#endif

#ifdef VITA
   strlcpy(eboot_path, "app0:/", sizeof(eboot_path));
   strlcpy(g_defaults.dir.port, eboot_path, sizeof(g_defaults.dir.port));
   strlcpy(user_path, "ux0:/data/retroarch/", sizeof(user_path));
#else
   strlcpy(eboot_path, argv[0], sizeof(eboot_path));
   /* for PSP, use uppercase directories, and no trailing slashes
      otherwise mkdir fails */
   strlcpy(user_path, "ms0:/PSP/RETROARCH", sizeof(user_path));
   fill_pathname_basedir(g_defaults.dir.port, argv[0], sizeof(g_defaults.dir.port));
#endif
   RARCH_LOG("port dir: [%s]\n", g_defaults.dir.port);

#ifdef VITA
   /* bundle data*/
   fill_pathname_join(g_defaults.dir.core, g_defaults.dir.port,
         "", sizeof(g_defaults.dir.core));
   fill_pathname_join(g_defaults.dir.assets, g_defaults.dir.port,
         "assets", sizeof(g_defaults.dir.assets));
   fill_pathname_join(g_defaults.dir.core_info, g_defaults.dir.core,
         "info", sizeof(g_defaults.dir.core_info));
   fill_pathname_join(g_defaults.dir.database, g_defaults.dir.port,
         "database/rdb", sizeof(g_defaults.dir.database));
   fill_pathname_join(g_defaults.dir.cursor, g_defaults.dir.port,
         "database/cursors", sizeof(g_defaults.dir.cursor));
   /* user data*/
   fill_pathname_join(g_defaults.dir.cheats, user_path,
         "cheats", sizeof(g_defaults.dir.cheats));
   fill_pathname_join(g_defaults.dir.menu_config, user_path,
         "config", sizeof(g_defaults.dir.menu_config));
   fill_pathname_join(g_defaults.dir.core_assets, user_path,
         "downloads", sizeof(g_defaults.dir.core_assets));
   fill_pathname_join(g_defaults.dir.playlist, user_path,
         "playlists", sizeof(g_defaults.dir.playlist));
   fill_pathname_join(g_defaults.dir.remap, user_path,
         "remaps", sizeof(g_defaults.dir.remap));
   fill_pathname_join(g_defaults.dir.sram, user_path,
         "savefiles", sizeof(g_defaults.dir.sram));
   fill_pathname_join(g_defaults.dir.savestate, user_path,
         "savestates", sizeof(g_defaults.dir.savestate));
   fill_pathname_join(g_defaults.dir.system, user_path,
         "system", sizeof(g_defaults.dir.system));
   fill_pathname_join(g_defaults.dir.cache, user_path,
         "temp", sizeof(g_defaults.dir.cache));
   fill_pathname_join(g_defaults.dir.overlay, user_path,
         "overlays", sizeof(g_defaults.dir.overlay));
   fill_pathname_join(g_defaults.dir.thumbnails, user_path,
         "thumbnails", sizeof(g_defaults.dir.thumbnails));
   strlcpy(g_defaults.dir.content_history,
         user_path, sizeof(g_defaults.dir.content_history));
   fill_pathname_join(g_defaults.path.config, user_path,
         file_path_str(FILE_PATH_MAIN_CONFIG), sizeof(g_defaults.path.config));
#else

   fill_pathname_join(g_defaults.dir.core, g_defaults.dir.port,
         "CORES", sizeof(g_defaults.dir.core));
   fill_pathname_join(g_defaults.dir.core_info, g_defaults.dir.port,
         "INFO", sizeof(g_defaults.dir.core_info));

   /* bundle data */
   /*
   fill_pathname_join(g_defaults.dir.assets, g_defaults.dir.port,
         "BUNDLE/ASSETS", sizeof(g_defaults.dir.assets));
   fill_pathname_join(g_defaults.dir.autoconfig, g_defaults.dir.port,
         "BUNDLE/AUTOCONFIG", sizeof(g_defaults.dir.autoconfig));
   fill_pathname_join(g_defaults.dir.cursor, base_path,
         "BUNDLE/DATABASE/CURSORS", sizeof(g_defaults.dir.cursor));
   fill_pathname_join(g_defaults.dir.database, base_path,
         "BUNDLE/DATABASE/RDB", sizeof(g_defaults.dir.database));
   fill_pathname_join(g_defaults.dir.overlay, base_path,
         "BUNDLE/OVERLAYS", sizeof(g_defaults.dir.overlay));
   fill_pathname_join(g_defaults.dir.shader, base_path,
         "BUNDLE/SHADERS", sizeof(g_defaults.dir.shader));
   */

   /* user data */
   fill_pathname_join(g_defaults.dir.cheats, user_path,
         "CHEATS", sizeof(g_defaults.dir.cheats));
   fill_pathname_join(g_defaults.dir.menu_config, user_path,
         "CONFIG", sizeof(g_defaults.dir.menu_config));
   fill_pathname_join(g_defaults.dir.core_assets, user_path,
         "DOWNLOADS", sizeof(g_defaults.dir.core_assets));
   fill_pathname_join(g_defaults.dir.playlist, user_path,
         "PLAYLISTS", sizeof(g_defaults.dir.playlist));
   fill_pathname_join(g_defaults.dir.remap, g_defaults.dir.menu_config,
         "REMAPS", sizeof(g_defaults.dir.remap));
   fill_pathname_join(g_defaults.dir.sram, user_path,
         "SAVEFILES", sizeof(g_defaults.dir.sram));
   fill_pathname_join(g_defaults.dir.savestate, user_path,
         "SAVESTATES", sizeof(g_defaults.dir.savestate));
   fill_pathname_join(g_defaults.dir.screenshot, user_path,
         "SCREENSHOTS", sizeof(g_defaults.dir.screenshot));
   fill_pathname_join(g_defaults.dir.system, user_path,
         "SYSTEM", sizeof(g_defaults.dir.system));
   /* fill_pathname_join(g_defaults.dir.thumbnails, user_path,
         "THUMBNAILS", sizeof(g_defaults.dir.thumbnails)); */

   /* cache dir */
   fill_pathname_join(g_defaults.dir.cache, user_path,
         "TEMP", sizeof(g_defaults.dir.cache));

   /* history and main config */
   strlcpy(g_defaults.dir.content_history,
         user_path, sizeof(g_defaults.dir.content_history));
   fill_pathname_join(g_defaults.path.config, user_path,
         file_path_str(FILE_PATH_MAIN_CONFIG), sizeof(g_defaults.path.config));
#endif
   /* create user data dirs */
   path_mkdir(g_defaults.dir.cheats);
   path_mkdir(g_defaults.dir.core_assets);
   path_mkdir(g_defaults.dir.playlist);
   path_mkdir(g_defaults.dir.menu_config);
   path_mkdir(g_defaults.dir.playlist);
   path_mkdir(g_defaults.dir.remap);
   path_mkdir(g_defaults.dir.savestate);
   path_mkdir(g_defaults.dir.screenshot);
   path_mkdir(g_defaults.dir.sram);
   path_mkdir(g_defaults.dir.system);
#ifdef VITA
   path_mkdir(g_defaults.dir.thumbnails);
#endif
   /* create cache dir */
   path_mkdir(g_defaults.dir.cache);



#ifndef IS_SALAMANDER
#ifdef VITA
   params          = (struct rarch_main_wrap*)params_data;
   params->verbose = true;
#endif
   if (!string_is_empty(argv[1]))
   {
      static char path[PATH_MAX_LENGTH] = {0};
      struct rarch_main_wrap      *args =
         (struct rarch_main_wrap*)params_data;

      if (args)
      {
         strlcpy(path, argv[1], sizeof(path));

         args->touched        = true;
         args->no_content     = false;
         args->verbose        = false;
         args->config_path    = NULL;
         args->sram_path      = NULL;
         args->state_path     = NULL;
         args->content_path   = path;
         args->libretro_path  = NULL;

         RARCH_LOG("argv[0]: %s\n", argv[0]);
         RARCH_LOG("argv[1]: %s\n", argv[1]);
         RARCH_LOG("argv[2]: %s\n", argv[2]);

         RARCH_LOG("Auto-start game %s.\n", argv[1]);
      }
   }
#endif
}

static void frontend_psp_deinit(void *data)
{
   (void)data;
#ifndef IS_SALAMANDER
   verbosity_disable();
#ifdef HAVE_FILE_LOGGER
   command_event(CMD_EVENT_LOG_FILE_DEINIT, NULL);
#endif

#endif
}

static void frontend_psp_shutdown(bool unused)
{
   (void)unused;
#ifdef VITA
   //sceKernelExitProcess(0);
   return;
#else
   sceKernelExitGame();
#endif
}

#ifndef VITA
static int exit_callback(int arg1, int arg2, void *common)
{
   frontend_psp_deinit(NULL);
   frontend_psp_shutdown(false);
   return 0;
}

static int callback_thread(SceSize args, void *argp)
{
   int cbid = sceKernelCreateCallback("Exit callback", exit_callback, NULL);
   sceKernelRegisterExitCallback(cbid);
   sceKernelSleepThreadCB();

   return 0;
}

static int setup_callback(void)
{
   int thread_id = sceKernelCreateThread("update_thread",
         callback_thread, 0x11, 0xFA0, 0, 0);

   if (thread_id >= 0)
      sceKernelStartThread(thread_id, 0, 0);

   return thread_id;
}
#endif

static void frontend_psp_init(void *data)
{
#ifndef IS_SALAMANDER

#ifdef VITA
   scePowerSetArmClockFrequency(444);
   sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
   pthread_init();
#else
   (void)data;
   /* initialize debug screen */
   pspDebugScreenInit();
   pspDebugScreenClear();

   setup_callback();

   pspFpuSetEnable(0); /* disable FPU exceptions */
   scePowerSetClockFrequency(333,333,166);
#endif

#endif

#if defined(HAVE_KERNEL_PRX) || defined(IS_SALAMANDER)
#ifndef VITA
   pspSdkLoadStartModule("kernel_functions.prx", PSP_MEMORY_PARTITION_KERNEL);
#endif
#endif
}

static void frontend_psp_exec(const char *path, bool should_load_game)
{
#if defined(HAVE_KERNEL_PRX) || defined(IS_SALAMANDER) || defined(VITA)
   char argp[512] = {0};
   SceSize   args = 0;

#if !defined(VITA)
   strlcpy(argp, eboot_path, sizeof(argp));
   args = strlen(argp) + 1;
#endif

#ifndef IS_SALAMANDER
   if (should_load_game && !path_is_empty(RARCH_PATH_CONTENT))
   {
      argp[args] = '\0';
      strlcat(argp + args, path_get(RARCH_PATH_CONTENT), sizeof(argp) - args);
      args += strlen(argp + args) + 1;
   }
#endif

   RARCH_LOG("Attempt to load executable: [%s].\n", path);
#if defined(VITA)
   RARCH_LOG("Attempt to load executable: %d [%s].\n", args, argp);
   int ret =  sceAppMgrLoadExec(path, args==0? NULL : (char * const*)((const char*[]){argp, 0}), NULL);
   RARCH_LOG("Attempt to load executable: [%d].\n", ret);
#else
   exitspawn_kernel(path, args, argp);
#endif
#endif
}

#ifndef IS_SALAMANDER
static bool frontend_psp_set_fork(enum frontend_fork fork_mode)
{
   switch (fork_mode)
   {
      case FRONTEND_FORK_CORE:
         RARCH_LOG("FRONTEND_FORK_CORE\n");
         psp_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_CORE_WITH_ARGS:
         RARCH_LOG("FRONTEND_FORK_CORE_WITH_ARGS\n");
         psp_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_RESTART:
         RARCH_LOG("FRONTEND_FORK_RESTART\n");
         /* NOTE: We don't implement Salamander, so just turn
          * this into FRONTEND_FORK_CORE. */
         psp_fork_mode  = FRONTEND_FORK_CORE;
         break;
      case FRONTEND_FORK_NONE:
      default:
         return false;
   }

   return true;
}
#endif

static void frontend_psp_exitspawn(char *s, size_t len)
{
   bool should_load_game = false;
#ifndef IS_SALAMANDER
   if (psp_fork_mode == FRONTEND_FORK_NONE)
      return;

   switch (psp_fork_mode)
   {
      case FRONTEND_FORK_CORE_WITH_ARGS:
         should_load_game = true;
         break;
      case FRONTEND_FORK_NONE:
      default:
         break;
   }
#endif
   frontend_psp_exec(s, should_load_game);
}

static int frontend_psp_get_rating(void)
{
#ifdef VITA
   return 6; /* Go with a conservative figure for now. */
#else
   return 4;
#endif
}

static enum frontend_powerstate frontend_psp_get_powerstate(int *seconds, int *percent)
{
   enum frontend_powerstate ret = FRONTEND_POWERSTATE_NONE;
#ifndef VITA
   int battery                  = scePowerIsBatteryExist();
#endif
   int plugged                  = scePowerIsPowerOnline();
   int charging                 = scePowerIsBatteryCharging();

   *percent = scePowerGetBatteryLifePercent();
   *seconds = scePowerGetBatteryLifeTime() * 60;

#ifndef VITA
   if (!battery)
   {
      ret = FRONTEND_POWERSTATE_NO_SOURCE;
      *seconds = -1;
      *percent = -1;
   }
   else
#endif
   if (charging)
      ret = FRONTEND_POWERSTATE_CHARGING;
   else if (plugged)
      ret = FRONTEND_POWERSTATE_CHARGED;
   else
      ret = FRONTEND_POWERSTATE_ON_POWER_SOURCE;

   return ret;
}

enum frontend_architecture frontend_psp_get_architecture(void)
{
#ifdef VITA
   return FRONTEND_ARCH_ARMV7;
#else
   return FRONTEND_ARCH_MIPS;
#endif
}

static int frontend_psp_parse_drive_list(void *data)
{
#ifndef IS_SALAMANDER
   file_list_t *list = (file_list_t*)data;

#ifdef VITA
   menu_entries_append_enum(list,
         "app0:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
   menu_entries_append_enum(list,
         "ur0:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
   menu_entries_append_enum(list,
         "ux0:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
#else
   menu_entries_append_enum(list,
         "ms0:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
   menu_entries_append_enum(list,
         "ef0:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
   menu_entries_append_enum(list,
         "host0:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
#endif
#endif

   return 0;
}

frontend_ctx_driver_t frontend_ctx_psp = {
   frontend_psp_get_environment_settings,
   frontend_psp_init,
   frontend_psp_deinit,
   frontend_psp_exitspawn,
   NULL,                         /* process_args */
   frontend_psp_exec,
#ifdef IS_SALAMANDER
   NULL,
#else
   frontend_psp_set_fork,
#endif
   frontend_psp_shutdown,
   NULL,                         /* get_name */
   NULL,                         /* get_os */
   frontend_psp_get_rating,
   NULL,                         /* load_content */
   frontend_psp_get_architecture,
   frontend_psp_get_powerstate,
   frontend_psp_parse_drive_list,
   NULL,                         /* get_mem_total */
   NULL,                         /* get_mem_free */
   NULL,                         /* install_signal_handler */
   NULL,                         /* get_sighandler_state */
   NULL,                         /* set_sighandler_state */
   NULL,                         /* destroy_sighandler_state */
   NULL,                         /* attach_console */
   NULL,                         /* detach_console */
#ifdef VITA
   "vita",
#else
   "psp",
#endif
};
