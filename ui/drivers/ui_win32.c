/* RetroArch - A frontend for libretro.
 *  Copyright (C) 2015-2016 - Ali Bouhlel
 *  Copyright (C) 2011-2016 - Daniel De Matteis
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
#include <boolean.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment( lib, "comctl32" )
#endif

#define TITLE_MAX PATH_MAX
#define FULLPATH_MAX 32768

#define IDI_ICON 1

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 //_WIN32_WINNT_WIN2K
#endif

#include "../../gfx/common/win32_common.h"
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>

#include <retro_inline.h>
#include <retro_miscellaneous.h>
#include <file/file_path.h>
#include <string/stdstring.h>
#include <compat/strl.h>

#include "../ui_companion_driver.h"
#include "../../msg_hash.h"
#include "../../configuration.h"
#include "../../driver.h"
#include "../../paths.h"
#include "../../runloop.h"
#include "../../gfx/video_context_driver.h"
#include "../../gfx/video_shader_driver.h"
#include "../../tasks/tasks_internal.h"

#include "../../gfx/common/gl_common.h"
#include "ui_win32.h"

#define SHADER_DLG_WIDTH                  220
#define SHADER_DLG_MIN_HEIGHT             200
#define SHADER_DLG_MAX_HEIGHT             800
#define SHADER_DLG_CTRL_MARGIN            8
#define SHADER_DLG_CTRL_X                 10
#define SHADER_DLG_CHECKBOX_HEIGHT        15
#define SHADER_DLG_SEPARATOR_HEIGHT       10
#define SHADER_DLG_LABEL_HEIGHT           14
#define SHADER_DLG_TRACKBAR_HEIGHT        22
#define SHADER_DLG_TRACKBAR_LABEL_WIDTH   30

#define SHADER_DLG_CTRL_WIDTH      (SHADER_DLG_WIDTH - 2 * SHADER_DLG_CTRL_X)
#define SHADER_DLG_TRACKBAR_WIDTH  (SHADER_DLG_CTRL_WIDTH - SHADER_DLG_TRACKBAR_LABEL_WIDTH)

typedef struct ui_companion_win32
{
   void *empty;
} ui_companion_win32_t;

enum shader_param_ctrl_type
{
   SHADER_PARAM_CTRL_NONE = 0,
   SHADER_PARAM_CTRL_CHECKBOX,
   SHADER_PARAM_CTRL_TRACKBAR
};

enum
{
   SHADER_DLG_CHECKBOX_ONTOP_ID = GFX_MAX_PARAMETERS,
   SHADER_DLG_CHECKBOX_BUTTON1_ID,
   SHADER_DLG_CHECKBOX_BUTTON2_ID
};

typedef struct
{
   enum shader_param_ctrl_type type;
   union
   {
      ui_window_win32_t checkbox;
      struct
      {
         HWND hwnd;
         HWND label_title;
         HWND label_val;
      } trackbar;
   };
} shader_param_ctrl_t;

typedef struct
{
   ui_window_win32_t window;
   ui_window_win32_t separator;
   ui_window_win32_t on_top_checkbox;
   shader_param_ctrl_t controls[GFX_MAX_PARAMETERS];
   int parameters_start_y;
} shader_dlg_t;

static shader_dlg_t g_shader_dlg = {{0}};

static bool shader_dlg_refresh_trackbar_label(int index, 
      video_shader_ctx_t *shader_info)
{
   char val_buffer[32]         = {0};

   if (floorf(shader_info->data->parameters[index].current) 
         == shader_info->data->parameters[index].current)
      snprintf(val_buffer, sizeof(val_buffer), "%.0f",
            shader_info->data->parameters[index].current);
   else
      snprintf(val_buffer, sizeof(val_buffer), "%.2f",
            shader_info->data->parameters[index].current);

   SendMessage(g_shader_dlg.controls[index].trackbar.label_val,
         WM_SETTEXT, 0, (LPARAM)val_buffer);

   return true;
}

static void shader_dlg_params_refresh(void)
{
   int i;

   for (i = 0; i < GFX_MAX_PARAMETERS; i++)
   {
      shader_param_ctrl_t*control = &g_shader_dlg.controls[i];

      if (control->type == SHADER_PARAM_CTRL_NONE)
         break;

      switch (control->type)
      {
         case SHADER_PARAM_CTRL_CHECKBOX:
            {
               video_shader_ctx_t shader_info;
               video_shader_driver_get_current_shader(&shader_info);

               bool checked = shader_info.data ?
                  (shader_info.data->parameters[i].current == 
                   shader_info.data->parameters[i].maximum) : false;
               SendMessage(control->checkbox.hwnd, BM_SETCHECK, checked, 0);
            }
            break;
         case SHADER_PARAM_CTRL_TRACKBAR:
            {
               video_shader_ctx_t shader_info;
               video_shader_driver_get_current_shader(&shader_info);
               if (shader_info.data && !shader_dlg_refresh_trackbar_label(i, &shader_info))
                  break;

               if (shader_info.data)
               {
                  SendMessage(control->trackbar.hwnd,
                        TBM_SETRANGEMIN, (WPARAM)TRUE, (LPARAM)0);
                  SendMessage(control->trackbar.hwnd,
                        TBM_SETRANGEMAX, (WPARAM)TRUE,
                        (LPARAM)((shader_info.data->parameters[i].maximum - 
                              shader_info.data->parameters[i].minimum) 
                           / shader_info.data->parameters[i].step));
                  SendMessage(control->trackbar.hwnd, TBM_SETPOS, (WPARAM)TRUE,
                        (LPARAM)((shader_info.data->parameters[i].current - 
                              shader_info.data->parameters[i].minimum) / 
                           shader_info.data->parameters[i].step));
               }
            }
            break;
         case SHADER_PARAM_CTRL_NONE:
         default:
            break;
      }
   }
}

static void shader_dlg_params_clear(void)
{
   unsigned i;

   for (i = 0; i < GFX_MAX_PARAMETERS; i++)
   {
      shader_param_ctrl_t*control = &g_shader_dlg.controls[i];

      if (!control || control->type == SHADER_PARAM_CTRL_NONE)
         break;

      switch (control->type)
      {
         case SHADER_PARAM_CTRL_NONE:
            break;
         case SHADER_PARAM_CTRL_CHECKBOX:
            {
               const ui_window_t *window = ui_companion_driver_get_window_ptr();
               if (window)
                  window->destroy(&control->checkbox);
            }
            break;
         case SHADER_PARAM_CTRL_TRACKBAR:
            DestroyWindow(control->trackbar.label_title);
            DestroyWindow(control->trackbar.label_val);
            DestroyWindow(control->trackbar.hwnd);
            break;
      }

      control->type = SHADER_PARAM_CTRL_NONE;
   }
}

void shader_dlg_params_reload(void)
{
#ifdef HAVE_SHADERPIPELINE
   HFONT hFont;
   RECT parent_rect;
   int i, pos_x, pos_y;
   video_shader_ctx_t shader_info;
   const ui_window_t *window = NULL;
   
   shader_dlg_params_clear();

   video_shader_driver_get_current_shader(&shader_info);

   if (!shader_info.data || shader_info.data->num_parameters > GFX_MAX_PARAMETERS)
      return;

   window = ui_companion_driver_get_window_ptr();
   hFont  = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
   pos_y  = g_shader_dlg.parameters_start_y;
   pos_x  = SHADER_DLG_CTRL_X;

   for (i = 0; i < (int)shader_info.data->num_parameters; i++)
   {
      shader_param_ctrl_t*control = &g_shader_dlg.controls[i];

      if ((shader_info.data->parameters[i].minimum == 0.0)
            && (shader_info.data->parameters[i].maximum 
               == (shader_info.data->parameters[i].minimum 
                  + shader_info.data->parameters[i].step)))
      {
         if ((pos_y + SHADER_DLG_CHECKBOX_HEIGHT 
                    + SHADER_DLG_CTRL_MARGIN + 20) 
               > SHADER_DLG_MAX_HEIGHT)
         {
            pos_y  = g_shader_dlg.parameters_start_y;
            pos_x += SHADER_DLG_WIDTH;
         }

         control->type          = SHADER_PARAM_CTRL_CHECKBOX;
         control->checkbox.hwnd = CreateWindowEx(0, "BUTTON",
               shader_info.data->parameters[i].desc,
               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, pos_x, pos_y,
               SHADER_DLG_CTRL_WIDTH, SHADER_DLG_CHECKBOX_HEIGHT,
               g_shader_dlg.window.hwnd, (HMENU)(size_t)i, NULL, NULL);
         SendMessage(control->checkbox.hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
         pos_y += SHADER_DLG_CHECKBOX_HEIGHT + SHADER_DLG_CTRL_MARGIN;
      }
      else
      {
         if ((pos_y + SHADER_DLG_LABEL_HEIGHT + SHADER_DLG_TRACKBAR_HEIGHT +
                  SHADER_DLG_CTRL_MARGIN + 20) > SHADER_DLG_MAX_HEIGHT)
         {
            pos_y = g_shader_dlg.parameters_start_y;
            pos_x += SHADER_DLG_WIDTH;
         }

         control->type                 = SHADER_PARAM_CTRL_TRACKBAR;
         control->trackbar.label_title = CreateWindowEx(0, "STATIC",
               shader_info.data->parameters[i].desc,
               WS_CHILD | WS_VISIBLE | SS_LEFT, pos_x, pos_y,
               SHADER_DLG_CTRL_WIDTH, SHADER_DLG_LABEL_HEIGHT, g_shader_dlg.window.hwnd,
               (HMENU)(size_t)i, NULL, NULL);
         SendMessage(control->trackbar.label_title, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

         pos_y += SHADER_DLG_LABEL_HEIGHT;
         control->trackbar.hwnd = CreateWindowEx(0, TRACKBAR_CLASS, "",
               WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
               pos_x + SHADER_DLG_TRACKBAR_LABEL_WIDTH, pos_y,
               SHADER_DLG_TRACKBAR_WIDTH, SHADER_DLG_TRACKBAR_HEIGHT,
               g_shader_dlg.window.hwnd, (HMENU)(size_t)i, NULL, NULL);

         control->trackbar.label_val = CreateWindowEx(0, "STATIC", "",
               WS_CHILD | WS_VISIBLE | SS_LEFT, pos_x,
               pos_y, SHADER_DLG_TRACKBAR_LABEL_WIDTH, SHADER_DLG_LABEL_HEIGHT,
               g_shader_dlg.window.hwnd, (HMENU)(size_t)i, NULL, NULL);
         SendMessage(control->trackbar.label_val, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

         SendMessage(control->trackbar.hwnd, TBM_SETBUDDY, (WPARAM)TRUE,
               (LPARAM)control->trackbar.label_val);

         pos_y += SHADER_DLG_TRACKBAR_HEIGHT + SHADER_DLG_CTRL_MARGIN;

      }

   }

   if (window && g_shader_dlg.separator.hwnd)
      window->destroy(&g_shader_dlg.separator);

   g_shader_dlg.separator.hwnd = CreateWindowEx(0, "STATIC", "",
         SS_ETCHEDHORZ | WS_VISIBLE | WS_CHILD, SHADER_DLG_CTRL_X,
         g_shader_dlg.parameters_start_y - SHADER_DLG_CTRL_MARGIN - SHADER_DLG_SEPARATOR_HEIGHT / 2,
         (pos_x - SHADER_DLG_CTRL_X) + SHADER_DLG_CTRL_WIDTH,
         SHADER_DLG_SEPARATOR_HEIGHT / 2,
         g_shader_dlg.window.hwnd, NULL, NULL,
         NULL);

   shader_dlg_params_refresh();

   GetWindowRect(g_shader_dlg.window.hwnd, &parent_rect);
   SetWindowPos(g_shader_dlg.window.hwnd, NULL, 0, 0,
         (pos_x - SHADER_DLG_CTRL_X) + SHADER_DLG_WIDTH,
         (pos_x == SHADER_DLG_CTRL_X) ? pos_y + 30 : SHADER_DLG_MAX_HEIGHT,
         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

static void shader_dlg_update_on_top_state(void)
{
   bool on_top = SendMessage(g_shader_dlg.on_top_checkbox.hwnd,
         BM_GETCHECK, 0, 0) == BST_CHECKED;

   SetWindowPos(g_shader_dlg.window.hwnd, on_top 
         ? HWND_TOPMOST : HWND_NOTOPMOST , 0, 0, 0, 0,
         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void shader_dlg_show(HWND parent_hwnd)
{
   const ui_window_t *window = ui_companion_driver_get_window_ptr();

   if (!IsWindowVisible(g_shader_dlg.window.hwnd))
   {
      if (parent_hwnd)
      {
         RECT parent_rect;
         GetWindowRect(parent_hwnd, &parent_rect);
         SetWindowPos(g_shader_dlg.window.hwnd, HWND_TOP,
               parent_rect.right, parent_rect.top,
               0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
      }
      else
         window->set_visible(&g_shader_dlg.window, true);

      shader_dlg_update_on_top_state();

      shader_dlg_params_reload();

   }

   window->set_focused(&g_shader_dlg.window);
}

#if defined(HAVE_OPENGL) || defined(HAVE_VULKAN)
static LRESULT CALLBACK ShaderDlgWndProc(HWND hwnd, UINT message,
      WPARAM wparam, LPARAM lparam)
{
   int i, pos;
   const ui_window_t *window = ui_companion_driver_get_window_ptr();

   switch (message)
   {
      case WM_CREATE:
         break;

      case WM_CLOSE:
      case WM_DESTROY:
      case WM_QUIT:
         if (window)
            window->set_visible(&g_shader_dlg.window, false);
         return 0;

      case WM_COMMAND:
         i = LOWORD(wparam);

         if (i == SHADER_DLG_CHECKBOX_ONTOP_ID)
         {
            shader_dlg_update_on_top_state();
            break;
         }

         if (i >= GFX_MAX_PARAMETERS)
            break;

         if (g_shader_dlg.controls[i].type != SHADER_PARAM_CTRL_CHECKBOX)
            break;

         {
            video_shader_ctx_t shader_info;
            video_shader_driver_get_current_shader(&shader_info);

            if (SendMessage(g_shader_dlg.controls[i].checkbox.hwnd,
                     BM_GETCHECK, 0, 0) == BST_CHECKED)
               shader_info.data->parameters[i].current = 
                  shader_info.data->parameters[i].maximum;
            else
               shader_info.data->parameters[i].current = 
                  shader_info.data->parameters[i].minimum;
         }
         break;

      case WM_HSCROLL:
         {
            video_shader_ctx_t shader_info;
            video_shader_driver_get_current_shader(&shader_info);
            i = GetWindowLong((HWND)lparam, GWL_ID);

            if (i >= GFX_MAX_PARAMETERS)
               break;

            if (g_shader_dlg.controls[i].type != SHADER_PARAM_CTRL_TRACKBAR)
               break;

            pos = (int)SendMessage(g_shader_dlg.controls[i].trackbar.hwnd, TBM_GETPOS, 0, 0);

            {

               shader_info.data->parameters[i].current = 
                  shader_info.data->parameters[i].minimum + pos * shader_info.data->parameters[i].step;
            }

            if (shader_info.data)
               shader_dlg_refresh_trackbar_label(i, &shader_info);
         }
         break;

   }

   return DefWindowProc(hwnd, message, wparam, lparam);
}
#endif

bool win32_window_init(WNDCLASSEX *wndclass,
      bool fullscreen, const char *class_name)
{
   wndclass->cbSize        = sizeof(WNDCLASSEX);
   wndclass->style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wndclass->hInstance     = GetModuleHandle(NULL);
   wndclass->hCursor       = LoadCursor(NULL, IDC_ARROW);
   wndclass->lpszClassName = (class_name != NULL) ? class_name : "RetroArch";
   wndclass->hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
   wndclass->hIconSm       = (HICON)LoadImage(GetModuleHandle(NULL),
         MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, 0);
   if (!fullscreen)
      wndclass->hbrBackground = (HBRUSH)COLOR_WINDOW;

   if (class_name != NULL)
      wndclass->style         |= CS_CLASSDC;

   if (!RegisterClassEx(wndclass))
      return false;

   /* This is non-NULL when we want a window for shader dialogs, 
    * therefore early return here */
   /* TODO/FIXME - this is ugly. Find a better way */
   if (class_name != NULL) 
      return true;

   /* Shader dialog is disabled for now, until video_threaded issues are fixed.
   if (!win32_shader_dlg_init())
      RARCH_ERR("[WGL]: wgl_shader_dlg_init() failed.\n");*/
   return true;
}

bool win32_shader_dlg_init(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_VULKAN)
   static bool inited = false;
   int pos_y;
   HFONT hFont;

   if (g_shader_dlg.window.hwnd)
      return true;

   if (!inited)
   {
      WNDCLASSEX wc_shader_dlg = {0};
      INITCOMMONCONTROLSEX comm_ctrl_init = {0};

      comm_ctrl_init.dwSize = sizeof(comm_ctrl_init);
      comm_ctrl_init.dwICC  = ICC_BAR_CLASSES;

      if (!InitCommonControlsEx(&comm_ctrl_init))
         return false;

      wc_shader_dlg.lpfnWndProc = ShaderDlgWndProc;
      wc_shader_dlg.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

      if (!win32_window_init(&wc_shader_dlg, true, "Shader Dialog"))
         return false;

      inited = true;
   }

   hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

   g_shader_dlg.window.hwnd = CreateWindowEx(0, "Shader Dialog", "Shader Parameters",
         WS_POPUPWINDOW | WS_CAPTION, 100, 100,
         SHADER_DLG_WIDTH, SHADER_DLG_MIN_HEIGHT, NULL, NULL, NULL, NULL);

   pos_y = SHADER_DLG_CTRL_MARGIN;
   g_shader_dlg.on_top_checkbox.hwnd = CreateWindowEx(0, "BUTTON", "Always on Top",
         BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
         SHADER_DLG_CTRL_X, pos_y, SHADER_DLG_CTRL_WIDTH,
         SHADER_DLG_CHECKBOX_HEIGHT, g_shader_dlg.window.hwnd,
         (HMENU)SHADER_DLG_CHECKBOX_ONTOP_ID, NULL, NULL);
   pos_y +=  SHADER_DLG_CHECKBOX_HEIGHT + SHADER_DLG_CTRL_MARGIN;

   SendMessage(g_shader_dlg.on_top_checkbox.hwnd,
         WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

   pos_y +=  SHADER_DLG_SEPARATOR_HEIGHT + SHADER_DLG_CTRL_MARGIN;

   g_shader_dlg.parameters_start_y = pos_y;
#endif
   return true;
}

static bool win32_browser(
      HWND owner,
      char *filename,
      const char *extensions,
      const char *title,
      const char *initial_dir)
{
   bool result = false;
   const ui_browser_window_t *browser = 
      ui_companion_driver_get_browser_window_ptr();

   if (browser)
   {
      ui_browser_window_state_t browser_state;
      /* OPENFILENAME.lpstrFilter requires a null separated list of name/ext pairs terminated by a second null at the end. */
      char *all_files[] = {"All Files (*.*)", "*.*", ""};

      /* These need to be big enough to hold the path/name of any file the user may select.
       * FIXME: We should really handle the error case when this isn't big enough. */
      char new_title[TITLE_MAX];
      char new_file[FULLPATH_MAX];

      new_title[0] = '\0';
      new_file[0] = '\0';

      if (!string_is_empty(title))
         strlcpy(new_title, title, sizeof(new_title));

      if (filename && *filename)
         strlcpy(new_file, filename, sizeof(new_file));

      browser_state.filters  = all_files[0];
      browser_state.title    = new_title;
      browser_state.startdir = strdup("");
      browser_state.path     = new_file;
      browser_state.window   = owner;

      result = browser->open(&browser_state);

      free(browser_state.startdir);
   }

   return result;
}

LRESULT win32_menu_loop(HWND owner, WPARAM wparam)
{
   WPARAM mode         = wparam & 0xffff;
   enum event_command cmd         = CMD_EVENT_NONE;
   bool do_wm_close     = false;
   settings_t *settings = config_get_ptr();

   switch (mode)
   {
      case ID_M_LOAD_CORE:
      case ID_M_LOAD_CONTENT:
         {
            char win32_file[PATH_MAX_LENGTH] = {0};
            wchar_t title_wide[PATH_MAX];
            char title_cp[PATH_MAX];
            const char *extensions  = NULL;
            const char *title       = NULL;
            const char *initial_dir = NULL;
            size_t converted        = 0;

            switch (mode)
            {
               case ID_M_LOAD_CORE:
                  extensions  = "Libretro core (.dll)\0*.dll\0All Files\0*.*\0";
                  title       = msg_hash_to_str(MENU_ENUM_LABEL_VALUE_CORE_LIST);
                  initial_dir = settings->directory.libretro;
                  break;
               case ID_M_LOAD_CONTENT:
                  extensions  = "All Files (*.*)\0*.*\0";
                  title       = msg_hash_to_str(
                        MENU_ENUM_LABEL_VALUE_LOAD_CONTENT_LIST);
                  initial_dir = settings->directory.menu_content;
                  break;
            }

            /* Convert UTF8 to UTF16, then back to the local code page.
             * This is needed for proper multi-byte string display until Unicode is fully supported.
             */
            MultiByteToWideChar(CP_UTF8, 0, title, -1, title_wide, sizeof(title_wide) / sizeof(title_wide[0]));
            wcstombs(title_cp, title_wide, sizeof(title_cp) - 1);

            if (!win32_browser(owner, win32_file,
                     extensions, title_cp, initial_dir))
               break;

            switch (mode)
            {
               case ID_M_LOAD_CORE:
                  runloop_ctl(RUNLOOP_CTL_SET_LIBRETRO_PATH, win32_file);
                  cmd         = CMD_EVENT_LOAD_CORE;
                  break;
               case ID_M_LOAD_CONTENT:
                  {
                     content_ctx_info_t content_info = {0};

                     path_set(RARCH_PATH_CONTENT, win32_file);

                     do_wm_close = true;
                     task_push_content_load_default(
                           NULL, NULL,
                           &content_info,
                           CORE_TYPE_PLAIN,
                           CONTENT_MODE_LOAD_CONTENT_WITH_CURRENT_CORE_FROM_COMPANION_UI,
                           NULL, NULL);
                  }
                  break;
            }
         }
         break;
      case ID_M_RESET:
         cmd = CMD_EVENT_RESET;
         break;
      case ID_M_MUTE_TOGGLE:
         cmd = CMD_EVENT_AUDIO_MUTE_TOGGLE;
         break;
      case ID_M_MENU_TOGGLE:
         cmd = CMD_EVENT_MENU_TOGGLE;
         break;
      case ID_M_PAUSE_TOGGLE:
         cmd = CMD_EVENT_PAUSE_TOGGLE;
         break;
      case ID_M_LOAD_STATE:
         cmd = CMD_EVENT_LOAD_STATE;
         break;
      case ID_M_SAVE_STATE:
         cmd = CMD_EVENT_SAVE_STATE;
         break;
      case ID_M_DISK_CYCLE:
         cmd = CMD_EVENT_DISK_EJECT_TOGGLE;
         break;
      case ID_M_DISK_NEXT:
         cmd = CMD_EVENT_DISK_NEXT;
         break;
      case ID_M_DISK_PREV:
         cmd = CMD_EVENT_DISK_PREV;
         break;
      case ID_M_FULL_SCREEN:
         cmd = CMD_EVENT_FULLSCREEN_TOGGLE;
         break;
#ifndef _XBOX
      case ID_M_SHADER_PARAMETERS:
         shader_dlg_show(owner);
         break;
#endif
      case ID_M_MOUSE_GRAB:
         cmd = CMD_EVENT_GRAB_MOUSE_TOGGLE;
         break;
      case ID_M_TAKE_SCREENSHOT:
         cmd = CMD_EVENT_TAKE_SCREENSHOT;
         break;
      case ID_M_QUIT:
         do_wm_close = true;
         break;
      default:
         if (mode >= ID_M_WINDOW_SCALE_1X && mode <= ID_M_WINDOW_SCALE_10X)
         {
            unsigned idx = (mode - (ID_M_WINDOW_SCALE_1X-1));
            runloop_ctl(RUNLOOP_CTL_SET_WINDOWED_SCALE, &idx);
            cmd = CMD_EVENT_RESIZE_WINDOWED_SCALE;
         }
         else if (mode == ID_M_STATE_INDEX_AUTO)
         {
            signed idx = -1;
            settings->state_slot = idx;
         }
         else if (mode >= (ID_M_STATE_INDEX_AUTO+1) 
               && mode <= (ID_M_STATE_INDEX_AUTO+10))
         {
            signed idx = (mode - (ID_M_STATE_INDEX_AUTO+1));
            settings->state_slot = idx;
         }
         break;
   }

   if (cmd != CMD_EVENT_NONE)
      command_event(cmd, NULL);

   if (do_wm_close)
      PostMessage(owner, WM_CLOSE, 0, 0);
   
   return 0L;
}

static void ui_companion_win32_deinit(void *data)
{
   ui_companion_win32_t *handle = (ui_companion_win32_t*)data;

   if (handle)
      free(handle);
}

static void *ui_companion_win32_init(void)
{
   ui_companion_win32_t *handle = (ui_companion_win32_t*)
      calloc(1, sizeof(*handle));

   if (!handle)
      return NULL;

   return handle;
}

static int ui_companion_win32_iterate(void *data, unsigned action)
{
   (void)data;
   (void)action;

   return 0;
}

static void ui_companion_win32_notify_content_loaded(void *data)
{
   (void)data;
}

static void ui_companion_win32_toggle(void *data)
{
   (void)data;
}

static void ui_companion_win32_event_command(
      void *data, enum event_command cmd)
{
   (void)data;
   (void)cmd;
}

static void ui_companion_win32_notify_list_pushed(void *data,
        file_list_t *list, file_list_t *menu_list)
{
    (void)data;
    (void)list;
    (void)menu_list;
}

const ui_companion_driver_t ui_companion_win32 = {
   ui_companion_win32_init,
   ui_companion_win32_deinit,
   ui_companion_win32_iterate,
   ui_companion_win32_toggle,
   ui_companion_win32_event_command,
   ui_companion_win32_notify_content_loaded,
   ui_companion_win32_notify_list_pushed,
   NULL,
   NULL,
   NULL,
   NULL,
   &ui_browser_window_win32,
   &ui_msg_window_win32,
   &ui_window_win32,
   &ui_application_win32,
   "win32",
};
