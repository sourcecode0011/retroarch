/*  RetroArch - A frontend for libretro.
 *  Copyright (c) 2011-2016 - Daniel De Matteis
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

#include <stdlib.h>

#include <retro_assert.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifdef HAVE_OPENGL
#include "gl_common.h"
#endif

#include "egl_common.h"
#include "../../verbosity.h"
#include "../../frontend/frontend_driver.h"

bool g_egl_inited    = false;

unsigned g_egl_major = 0;
unsigned g_egl_minor = 0;

void egl_report_error(void)
{
   EGLint    error = eglGetError();
   const char *str = NULL;
   switch (error)
   {
      case EGL_SUCCESS:
         str = "EGL_SUCCESS";
         break;

      case EGL_BAD_ACCESS:
         str = "EGL_BAD_ACCESS";
         break;

      case EGL_BAD_ALLOC:
         str = "EGL_BAD_ALLOC";
         break;

      case EGL_BAD_ATTRIBUTE:
         str = "EGL_BAD_ATTRIBUTE";
         break;

      case EGL_BAD_CONFIG:
         str = "EGL_BAD_CONFIG";
         break;

      case EGL_BAD_CONTEXT:
         str = "EGL_BAD_CONTEXT";
         break;

      case EGL_BAD_CURRENT_SURFACE:
         str = "EGL_BAD_CURRENT_SURFACE";
         break;

      case EGL_BAD_DISPLAY:
         str = "EGL_BAD_DISPLAY";
         break;

      case EGL_BAD_MATCH:
         str = "EGL_BAD_MATCH";
         break;

      case EGL_BAD_NATIVE_PIXMAP:
         str = "EGL_BAD_NATIVE_PIXMAP";
         break;

      case EGL_BAD_NATIVE_WINDOW:
         str = "EGL_BAD_NATIVE_WINDOW";
         break;

      case EGL_BAD_PARAMETER:
         str = "EGL_BAD_PARAMETER";
         break;

      case EGL_BAD_SURFACE:
         str = "EGL_BAD_SURFACE";
         break;

      default:
         str = "Unknown";
         break;
   }

   RARCH_ERR("[EGL]: #0x%x, %s\n", (unsigned)error, str);
}

gfx_ctx_proc_t egl_get_proc_address(const char *symbol)
{
   return eglGetProcAddress(symbol);
}

void egl_destroy(egl_ctx_data_t *egl)
{
   if (egl->dpy)
   {
#if defined HAVE_OPENGL
#if !defined(RARCH_MOBILE)
      if (egl->ctx != EGL_NO_CONTEXT)
      {
         glFlush();
         glFinish();
      }
#endif
#endif

      eglMakeCurrent(egl->dpy,
            EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (egl->ctx != EGL_NO_CONTEXT)
         eglDestroyContext(egl->dpy, egl->ctx);

      if (egl->hw_ctx != EGL_NO_CONTEXT)
         eglDestroyContext(egl->dpy, egl->hw_ctx);

      if (egl->surf != EGL_NO_SURFACE)
         eglDestroySurface(egl->dpy, egl->surf);
      eglTerminate(egl->dpy);
   }

   /* Be as careful as possible in deinit.
    * If we screw up, any TTY will not restore.
    */

   egl->ctx     = EGL_NO_CONTEXT;
   egl->hw_ctx  = EGL_NO_CONTEXT;
   egl->surf    = EGL_NO_SURFACE;
   egl->dpy     = EGL_NO_DISPLAY;
   egl->config  = 0;
   g_egl_inited  = false;

   frontend_driver_destroy_signal_handler_state();
}

void egl_bind_hw_render(egl_ctx_data_t *egl, bool enable)
{
   egl->use_hw_ctx = enable;

   if (egl->dpy  == EGL_NO_DISPLAY)
      return;
   if (egl->surf == EGL_NO_SURFACE)
      return;

   eglMakeCurrent(egl->dpy, egl->surf,
         egl->surf,
         enable ? egl->hw_ctx : egl->ctx);
}

void egl_swap_buffers(void *data)
{
   egl_ctx_data_t *egl = (egl_ctx_data_t*)data;

   if (egl->dpy  == EGL_NO_DISPLAY)
      return;
   if (egl->surf == EGL_NO_SURFACE)
      return;
   eglSwapBuffers(egl->dpy, egl->surf);
}

void egl_set_swap_interval(egl_ctx_data_t *egl, unsigned interval)
{
   /* Can be called before initialization.
    * Some contexts require that swap interval 
    * is known at startup time.
    */
   egl->interval = interval;

   if (egl->dpy  == EGL_NO_DISPLAY)
      return;
   if (!(eglGetCurrentContext()))
      return;

   RARCH_LOG("[EGL]: eglSwapInterval(%u)\n", interval);
   if (!eglSwapInterval(egl->dpy, interval))
   {
      RARCH_ERR("[EGL]: eglSwapInterval() failed.\n");
      egl_report_error();
   }
}

void egl_get_video_size(egl_ctx_data_t *egl, unsigned *width, unsigned *height)
{
   *width  = 0;
   *height = 0;

   if (egl->dpy != EGL_NO_DISPLAY && egl->surf != EGL_NO_SURFACE)
   {
      EGLint gl_width, gl_height;

      eglQuerySurface(egl->dpy, egl->surf, EGL_WIDTH, &gl_width);
      eglQuerySurface(egl->dpy, egl->surf, EGL_HEIGHT, &gl_height);
      *width  = gl_width;
      *height = gl_height;
   }
}

bool egl_init_context(egl_ctx_data_t *egl,
      void *display_data,
      EGLint *major, EGLint *minor,
     EGLint *n, const EGLint *attrib_ptr)
{
   NativeDisplayType display = (NativeDisplayType)display_data;
   egl->dpy = eglGetDisplay(display);
   if (!egl->dpy)
   {
      RARCH_ERR("[EGL]: Couldn't get EGL display.\n");
      return false;
   }

   if (!eglInitialize(egl->dpy, major, minor))
      return false;

   RARCH_LOG("[EGL]: EGL version: %d.%d\n", *major, *minor);

   if (!eglChooseConfig(egl->dpy, attrib_ptr, &egl->config, 1, n) || *n != 1)
      return false;

   egl->major = g_egl_major;
   egl->minor = g_egl_minor;

   return true;
}

bool egl_create_context(egl_ctx_data_t *egl, const EGLint *egl_attribs)
{
   egl->ctx    = eglCreateContext(egl->dpy, egl->config, EGL_NO_CONTEXT,
         egl_attribs);
   egl->hw_ctx = NULL;

   if (egl->ctx == EGL_NO_CONTEXT)
      return false;

   if (egl->use_hw_ctx)
   {
      egl->hw_ctx = eglCreateContext(egl->dpy, egl->config, egl->ctx,
            egl_attribs);
      RARCH_LOG("[EGL]: Created shared context: %p.\n", (void*)egl->hw_ctx);

      if (egl->hw_ctx == EGL_NO_CONTEXT)
         return false;
   }

   return true;
}

bool egl_create_surface(egl_ctx_data_t *egl, void *native_window)
{
   egl->surf = eglCreateWindowSurface(egl->dpy, egl->config, (NativeWindowType)native_window, NULL);

   if (egl->surf == EGL_NO_SURFACE)
      return false;

   /* Connect the context to the surface. */
   if (!eglMakeCurrent(egl->dpy, egl->surf, egl->surf, egl->ctx))
      return false;

   RARCH_LOG("[EGL]: Current context: %p.\n", (void*)eglGetCurrentContext());

   return true;
}

bool egl_get_native_visual_id(egl_ctx_data_t *egl, EGLint *value)
{
   if (!eglGetConfigAttrib(egl->dpy, egl->config,
         EGL_NATIVE_VISUAL_ID, value))
   {
      RARCH_ERR("[EGL]: egl_get_native_visual_id failed.\n");
      return false;
   }

   return true;
}

bool egl_has_config(egl_ctx_data_t *egl)
{
   if (!egl->config)
   {
      RARCH_ERR("[EGL]: No EGL configurations available.\n");
      return false;
   }
   return true;
}
