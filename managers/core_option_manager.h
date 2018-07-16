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

#ifndef CORE_OPTION_MANAGER_H__
#define CORE_OPTION_MANAGER_H__

#include <stddef.h>

#include <boolean.h>
#include <retro_common_api.h>
#include <lists/string_list.h>

RETRO_BEGIN_DECLS

struct core_option
{
  char *desc;
  char *key;
  struct string_list *vals;
  size_t index;
};

struct core_option_manager
{
  config_file_t *conf;
  char conf_path[PATH_MAX_LENGTH];

  struct core_option *opts;
  size_t size;
  bool updated;
};

typedef struct core_option_manager core_option_manager_t;

/**
 * core_option_manager_new:
 * @conf_path        : Filesystem path to write core option config file to.
 * @vars             : Pointer to variable array handle.
 *
 * Creates and initializes a core manager handle.
 *
 * Returns: handle to new core manager handle, otherwise NULL.
 **/
core_option_manager_t *core_option_manager_new(const char *conf_path,
      const void *data);

/**
 * core_option_manager_updated:
 * @opt              : options manager handle
 *
 * Has a core option been updated?
 *
 * Returns: true (1) if a core option has been updated,
 * otherwise false (0).
 **/
bool core_option_manager_updated(core_option_manager_t *opt);

/**
 * core_option_manager_flush:
 * @opt              : options manager handle
 *
 * Writes core option key-pair values to file.
 *
 * Returns: true (1) if core option values could be
 * successfully saved to disk, otherwise false (0).
 **/
bool core_option_manager_flush(core_option_manager_t *opt);

/**
 * core_option_manager_flush_game_specific:
 * @opt              : options manager handle
 * @path             : path for the core options file
 *
 * Writes core option key-pair values to a custom file.
 *
 * Returns: true (1) if core option values could be
 * successfully saved to disk, otherwise false (0).
 **/
bool core_option_manager_flush_game_specific(
      core_option_manager_t *opt, const char* path);

/**
 * core_option_manager_free:
 * @opt              : options manager handle
 *
 * Frees core option manager handle.
 **/
void core_option_manager_free(core_option_manager_t *opt);

void core_option_manager_get(core_option_manager_t *opt,  void *data);

/**
 * core_option_manager_size:
 * @opt              : options manager handle
 *
 * Gets total number of options.
 *
 * Returns: Total number of options.
 **/
size_t core_option_manager_size(core_option_manager_t *opt);

/**
 * core_option_manager_get_desc:
 * @opt              : options manager handle
 * @idx              : idx identifier of the option
 *
 * Gets description for an option.
 *
 * Returns: Description for an option.
 **/
const char *core_option_manager_get_desc(core_option_manager_t *opt, 
      size_t idx);

/**
 * core_option_manager_get_val:
 * @opt              : options manager handle
 * @idx              : idx identifier of the option
 *
 * Gets value for an option.
 *
 * Returns: Value for an option.
 **/
const char *core_option_manager_get_val(core_option_manager_t *opt, 
      size_t idx);

void core_option_manager_set_val(core_option_manager_t *opt,
      size_t idx, size_t val_idx);

/**
 * core_option_manager_next:
 * @opt                   : pointer to core option manager object.
 * @idx                   : idx of core option to be reset to defaults.
 *
 * Get next value for core option specified by @idx.
 * Options wrap around.
 **/
void core_option_manager_next(core_option_manager_t *opt, size_t idx);

/**
 * core_option_manager_prev:
 * @opt                   : pointer to core option manager object.
 * @idx                   : idx of core option to be reset to defaults.
 * Options wrap around.
 *
 * Get previous value for core option specified by @idx.
 * Options wrap around.
 **/
void core_option_manager_prev(core_option_manager_t *opt, size_t idx);

/**
 * core_option_manager_set_default:
 * @opt                   : pointer to core option manager object.
 * @idx                   : idx of core option to be reset to defaults.
 *
 * Reset core option specified by @idx and sets default value for option.
 **/
void core_option_manager_set_default(core_option_manager_t *opt, size_t idx);

RETRO_END_DECLS

#endif
