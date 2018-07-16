/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
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

#include <gfx/math/matrix_4x4.h>
#include <gfx/gl_capabilities.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../drivers/gl_symlinks.h"
#include "../video_coord_array.h"

void gl_ff_vertex(const struct video_coords *coords)
{
#ifndef NO_GL_FF_VERTEX
   /* Fall back to fixed function-style if needed and possible. */
   glClientActiveTexture(GL_TEXTURE1);
   glTexCoordPointer(2, GL_FLOAT, 0, coords->lut_tex_coord);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glClientActiveTexture(GL_TEXTURE0);
   glVertexPointer(2, GL_FLOAT, 0, coords->vertex);
   glEnableClientState(GL_VERTEX_ARRAY);
   glColorPointer(4, GL_FLOAT, 0, coords->color);
   glEnableClientState(GL_COLOR_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, 0, coords->tex_coord);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}

void gl_ff_matrix(const math_matrix_4x4 *mat)
{
#ifndef NO_GL_FF_MATRIX
   math_matrix_4x4 ident;

   /* Fall back to fixed function-style if needed and possible. */
   glMatrixMode(GL_PROJECTION);
   glLoadMatrixf(mat->data);
   glMatrixMode(GL_MODELVIEW);
   matrix_4x4_identity(&ident);
   glLoadMatrixf(ident.data);
#endif
}

static void gl_size_format(GLint* internalFormat)
{
#ifndef HAVE_PSGL
   switch (*internalFormat)
   {
      case GL_RGB:
         /* FIXME: PS3 does not support this, neither does it have GL_RGB565_OES. */
         *internalFormat = GL_RGB565;
         break;
      case GL_RGBA:
#ifdef HAVE_OPENGLES2
         *internalFormat = GL_RGBA8_OES;
#else
         *internalFormat = GL_RGBA8;
#endif
         break;
   }
#endif
}

/* This function should only be used without mipmaps
   and when data == NULL */
void gl_load_texture_image(GLenum target,
      GLint level,
      GLint internalFormat,
      GLsizei width,
      GLsizei height,
      GLint border,
      GLenum format,
      GLenum type,
      const GLvoid * data)
{
#ifndef HAVE_PSGL
#ifdef HAVE_OPENGLES2
   if (gl_check_capability(GL_CAPS_TEX_STORAGE_EXT) && internalFormat != GL_BGRA_EXT)
   {
      gl_size_format(&internalFormat);
      glTexStorage2DEXT(target, 1, internalFormat, width, height);
   }
#else
   if (gl_check_capability(GL_CAPS_TEX_STORAGE) && internalFormat != GL_BGRA_EXT)
   {
      gl_size_format(&internalFormat);
      glTexStorage2D(target, 1, internalFormat, width, height);
   }
#endif
   else
#endif
   {
#ifdef HAVE_OPENGLES
      if (gl_check_capability(GL_CAPS_GLES3_SUPPORTED))
#endif
         gl_size_format(&internalFormat);
      glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
   }
}
