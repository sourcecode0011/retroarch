/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2016 - Hans-Kristian Arntzen
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

#ifndef GLSLANG_UTIL_HPP
#define GLSLANG_UTIL_HPP

#include <stdint.h>
#include <vector>
#include <string>

enum glslang_format
{
   // 8-bit
   SLANG_FORMAT_R8_UNORM = 0,
   SLANG_FORMAT_R8_UINT,
   SLANG_FORMAT_R8_SINT,
   SLANG_FORMAT_R8G8_UNORM,
   SLANG_FORMAT_R8G8_UINT,
   SLANG_FORMAT_R8G8_SINT,
   SLANG_FORMAT_R8G8B8A8_UNORM,
   SLANG_FORMAT_R8G8B8A8_UINT,
   SLANG_FORMAT_R8G8B8A8_SINT,
   SLANG_FORMAT_R8G8B8A8_SRGB,

   // 10-bit
   SLANG_FORMAT_A2B10G10R10_UNORM_PACK32,
   SLANG_FORMAT_A2B10G10R10_UINT_PACK32,

   // 16-bit
   SLANG_FORMAT_R16_UINT,
   SLANG_FORMAT_R16_SINT,
   SLANG_FORMAT_R16_SFLOAT,
   SLANG_FORMAT_R16G16_UINT,
   SLANG_FORMAT_R16G16_SINT,
   SLANG_FORMAT_R16G16_SFLOAT,
   SLANG_FORMAT_R16G16B16A16_UINT,
   SLANG_FORMAT_R16G16B16A16_SINT,
   SLANG_FORMAT_R16G16B16A16_SFLOAT,

   // 32-bit
   SLANG_FORMAT_R32_UINT,
   SLANG_FORMAT_R32_SINT,
   SLANG_FORMAT_R32_SFLOAT,
   SLANG_FORMAT_R32G32_UINT,
   SLANG_FORMAT_R32G32_SINT,
   SLANG_FORMAT_R32G32_SFLOAT,
   SLANG_FORMAT_R32G32B32A32_UINT,
   SLANG_FORMAT_R32G32B32A32_SINT,
   SLANG_FORMAT_R32G32B32A32_SFLOAT,

   SLANG_FORMAT_UNKNOWN
};

struct glslang_parameter
{
   std::string id;
   std::string desc;
   float initial;
   float minimum;
   float maximum;
   float step;
};

struct glslang_meta
{
   std::vector<glslang_parameter> parameters;
   std::string name;
   glslang_format rt_format = SLANG_FORMAT_UNKNOWN;
};

struct glslang_output
{
   std::vector<uint32_t> vertex;
   std::vector<uint32_t> fragment;
   glslang_meta meta;
};

bool glslang_compile_shader(const char *shader_path, glslang_output *output);
const char *glslang_format_to_string(enum glslang_format fmt);

// Helpers for internal use.
bool glslang_read_shader_file(const char *path, std::vector<std::string> *output, bool root_file);
bool glslang_parse_meta(const std::vector<std::string> &lines, glslang_meta *meta);

#endif

