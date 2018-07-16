/* RetroArch - A frontend for libretro.
 *  Copyright (C) 2014-2016 - Ali Bouhlel
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
#include <ctype.h>
#include <boolean.h>

#include <3ds.h>

#include <file/file_path.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifndef IS_SALAMANDER
#include <lists/file_list.h>
#endif

#include "../frontend_driver.h"
#include "../../verbosity.h"
#include "../../defaults.h"
#include "../../paths.h"
#include "retroarch.h"
#include "file_path_special.h"
#include "audio/audio_driver.h"

#include "ctr/ctr_debug.h"

#ifndef IS_SALAMANDER
#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif
#endif

static enum frontend_fork ctr_fork_mode = FRONTEND_FORK_NONE;
static const char* elf_path_cst = "sdmc:/retroarch/test.3dsx";

static void frontend_ctr_get_environment_settings(int *argc, char *argv[],
      void *args, void *params_data)
{
   (void)args;

#ifndef IS_SALAMANDER
#if defined(HAVE_LOGGER)
   logger_init();
#elif defined(HAVE_FILE_LOGGER)
   retro_main_log_file_init("sdmc:/retroarch/retroarch-log.txt");
#endif
#endif

   fill_pathname_basedir(g_defaults.dir.port, elf_path_cst, sizeof(g_defaults.dir.port));
   RARCH_LOG("port dir: [%s]\n", g_defaults.dir.port);

   fill_pathname_join(g_defaults.dir.core_assets, g_defaults.dir.port,
         "downloads", sizeof(g_defaults.dir.core_assets));
   fill_pathname_join(g_defaults.dir.assets, g_defaults.dir.port,
         "media", sizeof(g_defaults.dir.assets));
   fill_pathname_join(g_defaults.dir.core, g_defaults.dir.port,
         "cores", sizeof(g_defaults.dir.core));
   fill_pathname_join(g_defaults.dir.core_info, g_defaults.dir.core,
         "info", sizeof(g_defaults.dir.core_info));
   fill_pathname_join(g_defaults.dir.savestate, g_defaults.dir.core,
         "savestates", sizeof(g_defaults.dir.savestate));
   fill_pathname_join(g_defaults.dir.sram, g_defaults.dir.core,
         "savefiles", sizeof(g_defaults.dir.sram));
   fill_pathname_join(g_defaults.dir.system, g_defaults.dir.core,
         "system", sizeof(g_defaults.dir.system));
   fill_pathname_join(g_defaults.dir.playlist, g_defaults.dir.core,
         "playlists", sizeof(g_defaults.dir.playlist));
   fill_pathname_join(g_defaults.dir.menu_config, g_defaults.dir.port,
         "config", sizeof(g_defaults.dir.menu_config));
   fill_pathname_join(g_defaults.dir.remap, g_defaults.dir.port,
         "config/remaps", sizeof(g_defaults.dir.remap));
   fill_pathname_join(g_defaults.dir.video_filter, g_defaults.dir.port,
         "filters", sizeof(g_defaults.dir.remap));
   fill_pathname_join(g_defaults.dir.database, g_defaults.dir.port,
         "database/rdb", sizeof(g_defaults.dir.database));
   fill_pathname_join(g_defaults.dir.cursor, g_defaults.dir.port,
         "database/cursors", sizeof(g_defaults.dir.cursor));
   fill_pathname_join(g_defaults.path.config, g_defaults.dir.port,
         file_path_str(FILE_PATH_MAIN_CONFIG), sizeof(g_defaults.path.config));
}

static void frontend_ctr_deinit(void *data)
{
   extern PrintConsole* currentConsole;
   Handle lcd_handle;
   u8 not_2DS;
   (void)data;
#ifndef IS_SALAMANDER
   verbosity_enable();

#ifdef HAVE_FILE_LOGGER
   command_event(CMD_EVENT_LOG_FILE_DEINIT, NULL);
#endif

   if((gfxBottomFramebuffers[0] == (u8*)currentConsole->frameBuffer)
      && (ctr_fork_mode == FRONTEND_FORK_NONE))
      wait_for_input();

   CFGU_GetModelNintendo2DS(&not_2DS);
   if(not_2DS && srvGetServiceHandle(&lcd_handle, "gsp::Lcd") >= 0)
   {
      u32 *cmdbuf = getThreadCommandBuffer();
      cmdbuf[0] = 0x00110040;
      cmdbuf[1] = 2;
      svcSendSyncRequest(lcd_handle);
      svcCloseHandle(lcd_handle);
   }

   u32 parallax_layer_reg_state = (*(float*)0x1FF81080 == 0.0)? 0x0 : 0x00010001;
   GSPGPU_WriteHWRegs(0x202000, &parallax_layer_reg_state, 4);

   cfguExit();
   ndspExit();
   csndExit();   
   gfxTopRightFramebuffers[0] = NULL;
   gfxTopRightFramebuffers[1] = NULL;
   gfxExit();
#endif
}

static void frontend_ctr_exec(const char *path, bool should_load_game)
{
   struct
   {
      u32 argc;
      char args[0x300 - 0x4];
   }param;
   int len;
   Result res;
   extern char __argv_hmac[0x20];

   DEBUG_VAR(path);
   DEBUG_STR(path);

   strlcpy(param.args, elf_path_cst, sizeof(param.args));
   len = strlen(param.args) + 1;
   param.argc = 1;

   RARCH_LOG("Attempt to load core: [%s].\n", path);
#ifndef IS_SALAMANDER
   if (should_load_game && !path_is_empty(RARCH_PATH_CONTENT))
   {
      strlcpy(param.args + len, path_get(RARCH_PATH_CONTENT), sizeof(param.args) - len);
      len += strlen(param.args + len) + 1;
      param.argc++;
      RARCH_LOG("content path: [%s].\n", path_get(RARCH_PATH_CONTENT));
   }
#endif
   uint64_t app_ID;
   if(!path || !*path)
   {
      APT_GetProgramID(&app_ID);
      RARCH_LOG("APP_ID 0x%016llX.\n", app_ID);
   }
   else
   {
      u32 app_ID_low;
      char app_ID_str[11];
      FILE* fp = fopen(path, "rb");
      size_t bytes_read = fread(app_ID_str, 1, sizeof(app_ID_str), fp);
      fclose(fp);
      if(bytes_read <= 0)
      {
         RARCH_LOG("error reading APP_ID from: [%s].\n", path);
         return;
      }
      app_ID_str[bytes_read] = '\0';
      sscanf(app_ID_str, "0x%x", &app_ID_low);
      app_ID_low <<= 8;
      app_ID = 0x0004000000000000ULL | app_ID_low;
      RARCH_LOG("APP_ID [%s] -- > 0x%016llX.\n", app_ID_str, app_ID);
   }

   if(R_SUCCEEDED(res = APT_PrepareToDoApplicationJump(0, app_ID, 0x1)))
        res = APT_DoApplicationJump(&param, sizeof(param.argc) + len, __argv_hmac);

   if(res)
   {
      RARCH_LOG("Failed to load core\n");
      dump_result_value(res);
   }

   svcSleepThread(INT64_MAX);
}

#ifndef IS_SALAMANDER
static bool frontend_ctr_set_fork(enum frontend_fork fork_mode)
{
   switch (fork_mode)
   {
      case FRONTEND_FORK_CORE:
         RARCH_LOG("FRONTEND_FORK_CORE\n");
         ctr_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_CORE_WITH_ARGS:
         RARCH_LOG("FRONTEND_FORK_CORE_WITH_ARGS\n");
         ctr_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_RESTART:
         RARCH_LOG("FRONTEND_FORK_RESTART\n");
         /* NOTE: We don't implement Salamander, so just turn
          * this into FRONTEND_FORK_CORE. */
         ctr_fork_mode  = FRONTEND_FORK_CORE;
         break;
      case FRONTEND_FORK_NONE:
      default:
         return false;
   }

   return true;
}
#endif

static void frontend_ctr_exitspawn(char *s, size_t len)
{
   bool should_load_game = false;
#ifndef IS_SALAMANDER
   if (ctr_fork_mode == FRONTEND_FORK_NONE)
      return;

   switch (ctr_fork_mode)
   {
      case FRONTEND_FORK_CORE_WITH_ARGS:
         should_load_game = true;
         break;
      default:
         break;
   }
#endif
   frontend_ctr_exec(s, should_load_game);
}

static void frontend_ctr_shutdown(bool unused)
{
   (void)unused;
}

static void ctr_check_dspfirm(void)
{
   FILE* dsp_fp = fopen("sdmc:/3ds/dspfirm.cdc", "rb");

   if(dsp_fp)
      fclose(dsp_fp);
   else
   {
      size_t code_size;
      uint32_t* code_buffer     = NULL;
      uint32_t* ptr             = NULL;
      const uint32_t dsp1_magic = 0x31505344; /* "DSP1" */
      FILE             *code_fp = fopen("sdmc:/3ds/code.bin", "rb");

      if(code_fp)
      {
         fseek(code_fp, 0, SEEK_END);
         code_size = ftell(code_fp);
         fseek(code_fp, 0, SEEK_SET);

         code_buffer = (uint32_t*) malloc(code_size);
         if(code_buffer)
         {
            fread(code_buffer, 1, code_size, code_fp);

            for (ptr = code_buffer + 0x40; ptr < (code_buffer + (code_size >> 2)); ptr++)
            {
               if (*ptr == dsp1_magic)
               {
                  size_t dspfirm_size = ptr[1];
                  ptr -= 0x40;
                  if ((ptr + (dspfirm_size >> 2)) > (code_buffer + (code_size >> 2)))
                     break;

                  dsp_fp = fopen("sdmc:/3ds/dspfirm.cdc", "wb");
                  if(!dsp_fp)
                     break;
                  fwrite(ptr, 1, dspfirm_size, dsp_fp);
                  fclose(dsp_fp);
                  break;
               }
            }
            free(code_buffer);
         }
         fclose(code_fp);
      }
   }
}

__attribute__((weak)) Result svchax_init(bool patch_srv);
__attribute__((weak)) u32 __ctr_patch_services;

void gfxSetFramebufferInfo(gfxScreen_t screen, u8 id);

static void frontend_ctr_init(void *data)
{
#ifndef IS_SALAMANDER
   (void)data;

   extern void* __service_ptr;
   if (__service_ptr)
   {
      frontend_ctx_ctr.exec = NULL;
      frontend_ctx_ctr.exitspawn = NULL;
      frontend_ctx_ctr.set_fork = NULL;
   }

   verbosity_enable();

   gfxInit(GSP_BGR8_OES,GSP_RGB565_OES,false);   

   u32 topSize = 400 * 240 * 3;
	u32 bottomSize = 320 * 240 * 2;
   linearFree(gfxTopLeftFramebuffers[0]);
	linearFree(gfxTopLeftFramebuffers[1]);
	linearFree(gfxBottomFramebuffers[0]);
	linearFree(gfxBottomFramebuffers[1]);
	linearFree(gfxTopRightFramebuffers[0]);
	linearFree(gfxTopRightFramebuffers[1]);

	gfxTopLeftFramebuffers[0]=linearAlloc(topSize * 2);
	gfxTopRightFramebuffers[0] = gfxTopLeftFramebuffers[0] + topSize;

   gfxTopLeftFramebuffers[1]=linearAlloc(topSize * 2);
   gfxTopRightFramebuffers[1] = gfxTopLeftFramebuffers[1] + topSize;

   gfxBottomFramebuffers[0]=linearAlloc(bottomSize);
	gfxBottomFramebuffers[1]=linearAlloc(bottomSize);

   gfxSetFramebufferInfo(GFX_TOP, 0);
	gfxSetFramebufferInfo(GFX_BOTTOM, 0);

   gfxSet3D(true);
   consoleInit(GFX_BOTTOM, NULL);

   /* enable access to all service calls when possible. */
   if(svchax_init)
   {
      osSetSpeedupEnable(false);
      svchax_init(__ctr_patch_services);
   }
   osSetSpeedupEnable(true);

   audio_driver_t* dsp_audio_driver = &audio_ctr_dsp;
   if(csndInit() != 0)
   {
      dsp_audio_driver = &audio_ctr_csnd;
      audio_ctr_csnd = audio_ctr_dsp;
      audio_ctr_dsp  = audio_null;
   }
   ctr_check_dspfirm();
   if(ndspInit() != 0)
      *dsp_audio_driver = audio_null;
   cfguInit();
#endif
}


static int frontend_ctr_get_rating(void)
{
   return 3;
}

enum frontend_architecture frontend_ctr_get_architecture(void)
{
   return FRONTEND_ARCH_ARM;
}

static int frontend_ctr_parse_drive_list(void *data)
{
#ifndef IS_SALAMANDER
   file_list_t *list = (file_list_t*)data;

   if (!list)
      return -1;

   menu_entries_append_enum(list,
         "sdmc:/",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR,
         MENU_SETTING_ACTION, 0, 0);
#endif

   return 0;
}

frontend_ctx_driver_t frontend_ctx_ctr = {
   frontend_ctr_get_environment_settings,
   frontend_ctr_init,
   frontend_ctr_deinit,
   frontend_ctr_exitspawn,
   NULL,                         /* process_args */
   frontend_ctr_exec,
#ifdef IS_SALAMANDER
   NULL,
#else
   frontend_ctr_set_fork,
#endif
   frontend_ctr_shutdown,
   NULL,                         /* get_name */
   NULL,                         /* get_os */
   frontend_ctr_get_rating,
   NULL,                         /* load_content */
   frontend_ctr_get_architecture,
   NULL,                         /* get_powerstate */
   frontend_ctr_parse_drive_list,
   NULL,                         /* get_mem_total */
   NULL,                         /* get_mem_free */
   NULL,                         /* install_signal_handler */
   NULL,                         /* get_signal_handler_state */
   NULL,                         /* set_signal_handler_state */
   NULL,                         /* destroy_signal_handler_state */
   NULL,                         /* attach_console */
   NULL,                         /* detach_console */
   "ctr",
};
