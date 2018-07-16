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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <file/config_file.h>
#include <file/file_path.h>
#include <compat/strl.h>
#include <compat/posix_string.h>
#include <string/stdstring.h>
#include <retro_miscellaneous.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#ifdef HAVE_CHEEVOS
#include "../cheevos.h"
#endif

#include "cheat_manager.h"

#include "../msg_hash.h"
#include "../runloop.h"
#include "../dynamic.h"
#include "../core.h"
#include "../verbosity.h"

struct item_cheat
{
   char *desc;
   bool state;
   char *code;
};

struct cheat_manager
{
   struct item_cheat *cheats;
   unsigned ptr;
   unsigned size;
   unsigned buf_size;
};

static cheat_manager_t *cheat_manager_state;

unsigned cheat_manager_get_buf_size(void)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return 0;
   return handle->buf_size;
}

unsigned cheat_manager_get_size(void)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return 0;
   return handle->size;
}

void cheat_manager_apply_cheats(void)
{
#ifdef HAVE_CHEEVOS
   bool data_bool  = false;
#endif
   unsigned i, idx = 0;
   cheat_manager_t *handle = cheat_manager_state;

   if (!handle)
      return;

   core_reset_cheat();

   for (i = 0; i < handle->size; i++)
   {
      if (handle->cheats[i].state)
      {
         retro_ctx_cheat_info_t cheat_info;

         cheat_info.index   = idx++;
         cheat_info.enabled = true;
         cheat_info.code    = handle->cheats[i].code;

         core_set_cheat(&cheat_info);
      }
   }
    runloop_msg_queue_push(msg_hash_to_str(MSG_APPLYING_CHEAT), 1, 180, true);
    RARCH_LOG("%s\n", msg_hash_to_str(MSG_APPLYING_CHEAT));

#ifdef HAVE_CHEEVOS
   data_bool = idx != 0;
   cheevos_apply_cheats(&data_bool);
#endif
}

void cheat_manager_set_code(unsigned i, const char *str)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return;

   if (!string_is_empty(str))
      handle->cheats[i].code  = strdup(str);

   handle->cheats[i].state    = true;
}

/**
 * cheat_manager_save:
 * @path                      : Path to cheats file (relative path).
 *
 * Saves cheats to file on disk.
 *
 * Returns: true (1) if successful, otherwise false (0).
 **/
bool cheat_manager_save(const char *path, const char *cheat_database)
{
   bool ret;
   unsigned i;
   char buf[PATH_MAX_LENGTH];
   char cheats_file[PATH_MAX_LENGTH];
   config_file_t *conf               = NULL;
   cheat_manager_t *handle           = cheat_manager_state;

   buf[0] = cheats_file[0] = '\0';

   fill_pathname_join(buf, cheat_database, path, sizeof(buf));

   fill_pathname_noext(cheats_file, buf, ".cht", sizeof(cheats_file));

   conf = config_file_new(cheats_file);

   if (!conf)
      conf = config_file_new(NULL);

   if (!conf)
      return false;

   if (!handle)
   {
      config_file_free(conf);
      return false;
   }

   config_set_int(conf, "cheats", handle->size);

   for (i = 0; i < handle->size; i++)
   {
      char key[64];
      char desc_key[256];
      char code_key[256];
      char enable_key[256];

      key[0] = desc_key[0] = code_key[0] = enable_key[0] = '\0';

      snprintf(key,        sizeof(key),        "cheat%u",        i);
      snprintf(desc_key,   sizeof(desc_key),   "cheat%u_desc",   i);
      snprintf(code_key,   sizeof(code_key),   "cheat%u_code",   i);
      snprintf(enable_key, sizeof(enable_key), "cheat%u_enable", i);

      if (handle->cheats[i].desc)
         config_set_string(conf, desc_key,   handle->cheats[i].desc);
      else
         config_set_string(conf, desc_key,   handle->cheats[i].code);
      config_set_string(conf,    code_key,   handle->cheats[i].code);
      config_set_bool(conf,      enable_key, handle->cheats[i].state);
   }

   ret = config_file_write(conf, cheats_file);
   config_file_free(conf);

   return ret;
}

static cheat_manager_t *cheat_manager_new(unsigned size)
{
   unsigned i;
   cheat_manager_t *handle = (cheat_manager_t*)
      calloc(1, sizeof(struct cheat_manager));

   if (!handle)
      return NULL;

   handle->buf_size = size;
   handle->size     = size;
   handle->cheats   = (struct item_cheat*)
      calloc(handle->buf_size, sizeof(struct item_cheat));

   if (!handle->cheats)
   {
      handle->buf_size = 0;
      handle->size = 0;
      handle->cheats = NULL;
      return handle;
   }

   for (i = 0; i < handle->size; i++)
   {
      handle->cheats[i].desc   = NULL;
      handle->cheats[i].code   = NULL;
      handle->cheats[i].state  = false;
   }

   return handle;
}

bool cheat_manager_load(const char *path)
{
   unsigned cheats = 0, i;
   cheat_manager_t *cheat;
   config_file_t *conf    = config_file_new(path);

   if (!conf)
      return false;

   config_get_uint(conf, "cheats", &cheats);

   if (cheats == 0)
      goto error;

   cheat = cheat_manager_new(cheats);

   if (!cheat)
      goto error;

   for (i = 0; i < cheats; i++)
   {
      char key[64];
      char desc_key[256];
      char code_key[256];
      char enable_key[256];
      char *tmp            = NULL;
      bool tmp_bool        = false;

      key[0] = desc_key[0] = code_key[0] = enable_key[0] = '\0';

      snprintf(key,        sizeof(key),        "cheat%u",        i);
      snprintf(desc_key,   sizeof(desc_key),   "cheat%u_desc",   i);
      snprintf(code_key,   sizeof(code_key),   "cheat%u_code",   i);
      snprintf(enable_key, sizeof(enable_key), "cheat%u_enable", i);

      if (config_get_string(conf, desc_key, &tmp) && !string_is_empty(tmp))
         cheat->cheats[i].desc   = strdup(tmp);

      if (config_get_string(conf, code_key, &tmp) && !string_is_empty(tmp))
         cheat->cheats[i].code   = strdup(tmp);

      if (config_get_bool(conf, enable_key, &tmp_bool))
         cheat->cheats[i].state  = tmp_bool;

      if (tmp)
         free(tmp);
   }

   config_file_free(conf);

   cheat_manager_state = cheat;

   return true;

error:
   config_file_free(conf);
   return false;
}


bool cheat_manager_realloc(unsigned new_size)
{
   unsigned i;
   cheat_manager_t *handle = cheat_manager_state;

   if (!handle)
      return false;

   if (!handle->cheats)
      handle->cheats = (struct item_cheat*)
         calloc(new_size, sizeof(struct item_cheat));
   else
      handle->cheats = (struct item_cheat*)
         realloc(handle->cheats, new_size * sizeof(struct item_cheat));

   if (!handle->cheats)
   {
      handle->buf_size = handle->size = 0;
      handle->cheats = NULL;
      return false;
   }

   handle->buf_size = new_size;
   handle->size     = new_size;

   for (i = 0; i < handle->size; i++)
   {
      handle->cheats[i].desc    = NULL;
      handle->cheats[i].code    = NULL;
      handle->cheats[i].state   = false;
   }

   return true;
}

void cheat_manager_free(void)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return;

   if (handle->cheats)
   {
      unsigned i;

      for (i = 0; i < handle->size; i++)
      {
         free(handle->cheats[i].desc);
         free(handle->cheats[i].code);
      }

      free(handle->cheats);
   }

   free(handle);
}

void cheat_manager_update(cheat_manager_t *handle, unsigned handle_idx)
{
   char msg[256];

   if (!handle)
      return;

   snprintf(msg, sizeof(msg), "Cheat: #%u [%s]: %s",
         handle_idx, handle->cheats[handle_idx].state ? "ON" : "OFF",
         (handle->cheats[handle_idx].desc) ?
         (handle->cheats[handle_idx].desc) : (handle->cheats[handle_idx].code)
         );
   runloop_msg_queue_push(msg, 1, 180, true);
   RARCH_LOG("%s\n", msg);
}

void cheat_manager_toggle_index(unsigned i)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return;

   handle->cheats[i].state = !handle->cheats[i].state;
   cheat_manager_update(handle, i);
}

void cheat_manager_toggle(cheat_manager_t *handle)
{
   if (!handle)
      return;

   handle->cheats[handle->ptr].state ^= true;
   cheat_manager_apply_cheats();
   cheat_manager_update(handle, handle->ptr);
}

void cheat_manager_index_next(cheat_manager_t *handle)
{
   if (!handle)
      return;

   handle->ptr = (handle->ptr + 1) % handle->size;
   cheat_manager_update(handle, handle->ptr);
}

void cheat_manager_index_prev(cheat_manager_t *handle)
{
   if (!handle)
      return;

   if (handle->ptr == 0)
      handle->ptr = handle->size - 1;
   else
      handle->ptr--;

   cheat_manager_update(handle, handle->ptr);
}

const char *cheat_manager_get_code(unsigned i)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return NULL;
   return handle->cheats[i].code;
}

const char *cheat_manager_get_desc(unsigned i)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return NULL;
   return handle->cheats[i].desc;
}

bool cheat_manager_get_code_state(unsigned i)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return false;
   return handle->cheats[i].state;
}

void cheat_manager_state_checks(
      bool cheat_index_plus_pressed,
      bool cheat_index_minus_pressed,
      bool cheat_toggle_pressed)
{
   cheat_manager_t *handle = cheat_manager_state;
   if (!handle)
      return;
   if (cheat_index_plus_pressed)
      cheat_manager_index_next(handle);
   else if (cheat_index_minus_pressed)
      cheat_manager_index_prev(handle);
   else if (cheat_toggle_pressed)
      cheat_manager_toggle(handle);
}

void cheat_manager_state_free(void)
{
   cheat_manager_free();

   cheat_manager_state = NULL;
}

bool cheat_manager_alloc_if_empty(void)
{
   cheat_manager_t *handle = cheat_manager_state;

   if (!handle)
   {
      cheat_manager_t *tmp    = cheat_manager_new(0);

      if (!tmp)
         return false;
      cheat_manager_state = tmp;
   }

   return true;
}
