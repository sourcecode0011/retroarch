/* RetroArch - A frontend for libretro.
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

#include <retro_miscellaneous.h>
#include <windows.h>

#include <boolean.h>
#include <compat/strl.h>
#include <dynamic/dylib.h>
#include <lists/file_list.h>
#include <file/file_path.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif

#include "../frontend_driver.h"
#include "../../configuration.h"
#include "../../defaults.h"
#include "../../runloop.h"
#include "../../verbosity.h"

/* We only load this library once, so we let it be 
 * unloaded at application shutdown, since unloading 
 * it early seems to cause issues on some systems.
 */

static dylib_t dwmlib;

static bool dwm_composition_disabled;

static void gfx_dwm_shutdown(void)
{
   if (dwmlib)
      dylib_close(dwmlib);
   dwmlib = NULL;
}

static bool gfx_init_dwm(void)
{
   static bool inited = false;

   if (inited)
      return true;

   dwmlib = dylib_load("dwmapi.dll");
   if (!dwmlib)
   {
      RARCH_LOG("Did not find dwmapi.dll.\n");
      return false;
   }
   atexit(gfx_dwm_shutdown);

   HRESULT (WINAPI *mmcss)(BOOL) = 
      (HRESULT (WINAPI*)(BOOL))dylib_proc(dwmlib, "DwmEnableMMCSS");
   if (mmcss)
   {
      RARCH_LOG("Setting multimedia scheduling for DWM.\n");
      mmcss(TRUE);
   }

   inited = true;
   return true;
}

static void gfx_set_dwm(void)
{
   HRESULT ret;
   settings_t *settings = config_get_ptr();

   if (!gfx_init_dwm())
      return;

   if (settings->video.disable_composition == dwm_composition_disabled)
      return;

   HRESULT (WINAPI *composition_enable)(UINT) = 
      (HRESULT (WINAPI*)(UINT))dylib_proc(dwmlib, "DwmEnableComposition");
   if (!composition_enable)
   {
      RARCH_ERR("Did not find DwmEnableComposition ...\n");
      return;
   }

   ret = composition_enable(!settings->video.disable_composition);
   if (FAILED(ret))
      RARCH_ERR("Failed to set composition state ...\n");
   dwm_composition_disabled = settings->video.disable_composition;
}

static void frontend_win32_get_os(char *s, size_t len, int *major, int *minor)
{
	uint32_t version = GetVersion();

	*major   = (DWORD)(LOBYTE(LOWORD(version)));
	*minor   = (DWORD)(HIBYTE(LOWORD(version)));

   switch (*major)
   {
      case 10:
         strlcpy(s, "Windows 10", len);
         break;
      case 6:
         switch (*minor)
         {
            case 3:
               strlcpy(s, "Windows 8.1", len);
               break;
            case 2:
               strlcpy(s, "Windows 8", len);
               break;
            case 1:
               strlcpy(s, "Windows 7/2008 R2", len);
               break;
            case 0:
               strlcpy(s, "Windows Vista/2008", len);
               break;
            default:
               break;
         }
         break;
      case 5:
         switch (*minor)
         {
            case 2:
               strlcpy(s, "Windows 2003", len);
               break;
            case 1:
               strlcpy(s, "Windows XP", len);
               break;
            case 0:
               strlcpy(s, "Windows 2000", len);
               break;
         }
         break;
      case 4:
         switch (*minor)
         {
            case 0:
               strlcpy(s, "Windows NT 4.0", len);
               break;
            case 90:
               strlcpy(s, "Windows ME", len);
               break;
            case 10:
               strlcpy(s, "Windows 98", len);
               break;
         }
         break;
      default:
         sprintf(s, "Windows %i.%i", *major, *minor);
         break;
   }
}

static void frontend_win32_init(void *data)
{
	typedef BOOL (WINAPI *isProcessDPIAwareProc)();
	typedef BOOL (WINAPI *setProcessDPIAwareProc)();
	HMODULE handle                         = GetModuleHandle("User32.dll");
	isProcessDPIAwareProc  isDPIAwareProc  = (isProcessDPIAwareProc)dylib_proc(handle, "IsProcessDPIAware");
	setProcessDPIAwareProc setDPIAwareProc = (setProcessDPIAwareProc)dylib_proc(handle, "SetProcessDPIAware");

	if (isDPIAwareProc)
	{
		if (!isDPIAwareProc())
		{
			if (setDPIAwareProc)
				setDPIAwareProc();
		}
	}
   
}

enum frontend_powerstate frontend_win32_get_powerstate(int *seconds, int *percent)
{
   SYSTEM_POWER_STATUS status;
	enum frontend_powerstate ret = FRONTEND_POWERSTATE_NONE;

	if (!GetSystemPowerStatus(&status))
		return ret;

	if (status.BatteryFlag == 0xFF)
		ret = FRONTEND_POWERSTATE_NONE;
	if (status.BatteryFlag & (1 << 7))
		ret = FRONTEND_POWERSTATE_NO_SOURCE;
	else if (status.BatteryFlag & (1 << 3))
		ret = FRONTEND_POWERSTATE_CHARGING;
	else if (status.ACLineStatus == 1)
		ret = FRONTEND_POWERSTATE_CHARGED;
	else
		ret = FRONTEND_POWERSTATE_ON_POWER_SOURCE;

	*percent  = (int)status.BatteryLifePercent;
	*seconds  = (int)status.BatteryLifeTime;

#ifdef _WIN32
      if (*percent == 255)
         *percent = 0;
#endif
	return ret;
}

enum frontend_architecture frontend_win32_get_architecture(void)
{
   /* stub */
   return FRONTEND_ARCH_NONE;
}

static int frontend_win32_parse_drive_list(void *data)
{
#ifdef HAVE_MENU
   size_t i          = 0;
   unsigned drives   = GetLogicalDrives();
   char    drive[]   = " :\\";
   file_list_t *list = (file_list_t*)data;

   for (i = 0; i < 32; i++)
   {
      drive[0] = 'A' + i;
      if (drives & (1 << i))
         menu_entries_append_enum(list,
               drive,
               msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
               MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
               MENU_SETTING_ACTION, 0, 0);
   }
#endif

   return 0;
}

static void frontend_win32_environment_get(int *argc, char *argv[],
      void *args, void *params_data)
{
   gfx_set_dwm();

   fill_pathname_expand_special(g_defaults.dir.assets,
      ":\\assets", sizeof(g_defaults.dir.assets));
   fill_pathname_expand_special(g_defaults.dir.audio_filter,
      ":\\filters\\audio", sizeof(g_defaults.dir.audio_filter));
   fill_pathname_expand_special(g_defaults.dir.video_filter,
      ":\\filters\\video", sizeof(g_defaults.dir.video_filter));
   fill_pathname_expand_special(g_defaults.dir.cheats,
      ":\\cheats", sizeof(g_defaults.dir.cheats));
   fill_pathname_expand_special(g_defaults.dir.database,
      ":\\database\\rdb", sizeof(g_defaults.dir.database));
   fill_pathname_expand_special(g_defaults.dir.cursor,
      ":\\database\\cursors", sizeof(g_defaults.dir.cursor));
   fill_pathname_expand_special(g_defaults.dir.playlist,
      ":\\playlists", sizeof(g_defaults.dir.assets));
   fill_pathname_expand_special(g_defaults.dir.menu_config,
      ":\\config", sizeof(g_defaults.dir.menu_config));
   fill_pathname_expand_special(g_defaults.dir.remap,
      ":\\config\\remaps", sizeof(g_defaults.dir.remap));
   fill_pathname_expand_special(g_defaults.dir.wallpapers,
      ":\\assets\\wallpapers", sizeof(g_defaults.dir.wallpapers));
   fill_pathname_expand_special(g_defaults.dir.thumbnails,
      ":\\thumbnails", sizeof(g_defaults.dir.thumbnails));
   fill_pathname_expand_special(g_defaults.dir.overlay,
      ":\\overlays", sizeof(g_defaults.dir.overlay));
   fill_pathname_expand_special(g_defaults.dir.core,
      ":\\cores", sizeof(g_defaults.dir.core));
   fill_pathname_expand_special(g_defaults.dir.core_info,
      ":\\info", sizeof(g_defaults.dir.core_info));
   fill_pathname_expand_special(g_defaults.dir.autoconfig,
      ":\\autoconfig", sizeof(g_defaults.dir.autoconfig));
   fill_pathname_expand_special(g_defaults.dir.shader,
      ":\\shaders", sizeof(g_defaults.dir.shader));
   fill_pathname_expand_special(g_defaults.dir.core_assets,
      ":\\downloads", sizeof(g_defaults.dir.core_assets));
   fill_pathname_expand_special(g_defaults.dir.screenshot,
      ":\\screenshots", sizeof(g_defaults.dir.screenshot));

/* don't force this in the driver anymore, these will be handled by
   a dummy config file  so they can be reset to content dir

   fill_pathname_expand_special(g_defaults.dir.sram,
      ":\\saves", sizeof(g_defaults.dir.sram));
   fill_pathname_expand_special(g_defaults.dir.savestate,
      ":\\states", sizeof(g_defaults.dir.savestate));
   fill_pathname_expand_special(g_defaults.dir.system,
      ":\\system", sizeof(g_defaults.dir.system));
*/
#ifdef HAVE_MENU
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   snprintf(g_defaults.settings.menu, sizeof(g_defaults.settings.menu), "xmb");
#endif
#endif
}

static uint64_t frontend_win32_get_mem_total(void)
{
   /* OSes below 2000 don't have the Ex version, and non-Ex cannot work with >4GB RAM */
#if _WIN32_WINNT > 0x0400
	MEMORYSTATUSEX mem_info;
	mem_info.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mem_info);
	return mem_info.ullTotalPhys;
#else
	MEMORYSTATUS mem_info;
	mem_info.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&mem_info);
	return mem_info.dwTotalPhys;
#endif
}

static uint64_t frontend_win32_get_mem_used(void)
{
   /* OSes below 2000 don't have the Ex version, and non-Ex cannot work with >4GB RAM */
#if _WIN32_WINNT > 0x0400
	MEMORYSTATUSEX mem_info;
	mem_info.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mem_info);
	return ((frontend_win32_get_mem_total() - mem_info.ullAvailPhys));
#else
	MEMORYSTATUS mem_info;
	mem_info.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&mem_info);
	return ((frontend_win32_get_mem_total() - mem_info.dwAvailPhys));
#endif
}

static void frontend_win32_attach_console(void)
{
#ifdef _WIN32
#ifdef _WIN32_WINNT_WINXP
   if (!AttachConsole(ATTACH_PARENT_PROCESS))
   {
      AllocConsole();
      AttachConsole( GetCurrentProcessId()) ;
      freopen( "CON", "w", stdout );
      freopen( "CON", "w", stderr );
   }
#endif
#endif
}

static void frontend_win32_detach_console(void)
{
#if defined(_WIN32) && !defined(_XBOX)
#ifdef _WIN32_WINNT_WINXP
   if (!AttachConsole(ATTACH_PARENT_PROCESS))
   {
      HWND wnd = GetConsoleWindow();
      FreeConsole();
      PostMessage(wnd, WM_CLOSE, 0, 0);
   }
#endif
#endif
}

frontend_ctx_driver_t frontend_ctx_win32 = {
   frontend_win32_environment_get,
   frontend_win32_init,
   NULL,                           /* deinit */
   NULL,                           /* exitspawn */
   NULL,                           /* process_args */
   NULL,                           /* exec */
   NULL,                           /* set_fork */
   NULL,                           /* shutdown */
   NULL,                           /* get_name */
   frontend_win32_get_os,
   NULL,                           /* get_rating */
   NULL,                           /* load_content */
   frontend_win32_get_architecture,
   frontend_win32_get_powerstate,
   frontend_win32_parse_drive_list,
   frontend_win32_get_mem_total,
   frontend_win32_get_mem_used,
   NULL,                            /* install_signal_handler */
   NULL,                            /* get_sighandler_state */
   NULL,                            /* set_sighandler_state */
   NULL,                            /* destroy_sighandler_state */
   frontend_win32_attach_console,   /* attach_console */
   frontend_win32_detach_console,   /* detach_console */
   "win32"
};
