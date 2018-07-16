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

#ifndef _FILE_PATH_SPECIAL_H
#define _FILE_PATH_SPECIAL_H

#include <stdint.h>
#include <stddef.h>

#include <boolean.h>

enum file_path_enum
{
   FILE_PATH_UNKNOWN = 0,
   FILE_PATH_CONTENT_BASENAME,
   FILE_PATH_PROGRAM_NAME,
   FILE_PATH_DETECT,
   FILE_PATH_NUL,
   FILE_PATH_LUTRO_PLAYLIST,
   FILE_PATH_LOG_WARN,
   FILE_PATH_LOG_ERROR,
   FILE_PATH_LOG_INFO,
   FILE_PATH_CONTENT_HISTORY,
   FILE_PATH_CONTENT_MUSIC_HISTORY,
   FILE_PATH_CONTENT_VIDEO_HISTORY,
   FILE_PATH_CONTENT_IMAGE_HISTORY,
   FILE_PATH_BACKGROUND_IMAGE,
   FILE_PATH_TTF_FONT,
   FILE_PATH_MAIN_CONFIG,
   FILE_PATH_CORE_OPTIONS_CONFIG,
   FILE_PATH_ASSETS_ZIP,
   FILE_PATH_AUTOCONFIG_ZIP,
   FILE_PATH_CORE_INFO_ZIP,
   FILE_PATH_OVERLAYS_ZIP,
   FILE_PATH_DATABASE_RDB_ZIP,
   FILE_PATH_SHADERS_SLANG_ZIP,
   FILE_PATH_SHADERS_GLSL_ZIP,
   FILE_PATH_SHADERS_CG_ZIP,
   FILE_PATH_CHEATS_ZIP,
   FILE_PATH_LAKKA_URL,
   FILE_PATH_CORE_THUMBNAILS_URL,
   FILE_PATH_INDEX_DIRS_URL,
   FILE_PATH_INDEX_URL,
   FILE_PATH_INDEX_EXTENDED_URL,
   FILE_PATH_CGP_EXTENSION,
   FILE_PATH_GLSLP_EXTENSION,
   FILE_PATH_SLANGP_EXTENSION,
   FILE_PATH_SRM_EXTENSION,
   FILE_PATH_PNG_EXTENSION,
   FILE_PATH_BMP_EXTENSION,
   FILE_PATH_TGA_EXTENSION,
   FILE_PATH_JPEG_EXTENSION,
   FILE_PATH_JPG_EXTENSION,
   FILE_PATH_UPS_EXTENSION,
   FILE_PATH_OPT_EXTENSION,
   FILE_PATH_IPS_EXTENSION,
   FILE_PATH_BPS_EXTENSION,
   FILE_PATH_STATE_EXTENSION,
   FILE_PATH_RTC_EXTENSION,
   FILE_PATH_REMAP_EXTENSION,
   FILE_PATH_CHT_EXTENSION,
   FILE_PATH_LPL_EXTENSION,
   FILE_PATH_LPL_EXTENSION_NO_DOT,
   FILE_PATH_RDB_EXTENSION,
   FILE_PATH_BSV_EXTENSION,
   FILE_PATH_AUTO_EXTENSION,
   FILE_PATH_ZIP_EXTENSION,
   FILE_PATH_7Z_EXTENSION,
   FILE_PATH_CONFIG_EXTENSION,
   FILE_PATH_CORE_INFO_EXTENSION
};

enum application_special_type
{
   APPLICATION_SPECIAL_NONE = 0,
   APPLICATION_SPECIAL_DIRECTORY_AUTOCONFIG,
   APPLICATION_SPECIAL_DIRECTORY_CONFIG,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_MATERIALUI,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_MATERIALUI_FONT,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_MATERIALUI_ICONS,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_XMB,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_XMB_BG,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_XMB_ICONS,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_XMB_FONT,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_ZARCH,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_ZARCH_FONT,
   APPLICATION_SPECIAL_DIRECTORY_ASSETS_ZARCH_ICONS
};

/**
 * fill_short_pathname_representation:
 * @out_rep            : output representation
 * @in_path            : input path
 * @size               : size of output representation
 *
 * Generates a short representation of path. It should only
 * be used for displaying the result; the output representation is not
 * binding in any meaningful way (for a normal path, this is the same as basename)
 * In case of more complex URLs, this should cut everything except for
 * the main image file.
 *
 * E.g.: "/path/to/game.img" -> game.img
 *       "/path/to/myarchive.7z#folder/to/game.img" -> game.img
 */
void fill_short_pathname_representation_wrapper(char* out_rep,
      const char *in_path, size_t size);

/**
 * path_basedir:
 * @path               : path
 *
 * Extracts base directory by mutating path.
 * Keeps trailing '/'.
 **/
void path_basedir_wrapper(char *path);

const char *file_path_str(enum file_path_enum enum_idx);

bool fill_pathname_application_data(char *s, size_t len);

void fill_pathname_application_special(char *s, size_t len, enum application_special_type type);

#endif
