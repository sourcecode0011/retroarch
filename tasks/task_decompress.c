/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C) 2016 - Brad Parker
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


#include <lists/string_list.h>
#include <string/stdstring.h>
#include <file/file_path.h>
#include <file/archive_file.h>
#include <retro_miscellaneous.h>
#include <retro_stat.h>
#include <compat/strl.h>

#include "tasks_internal.h"
#include "../file_path_special.h"
#include "../verbosity.h"
#include "../msg_hash.h"

static int file_decompressed_target_file(const char *name,
      const char *valid_exts,
      const uint8_t *cdata,
      unsigned cmode, uint32_t csize, uint32_t size,
      uint32_t crc32, struct archive_extract_userdata *userdata)
{
   /* TODO/FIXME */
   return 0;
}

static int file_decompressed_subdir(const char *name,
      const char *valid_exts,
      const uint8_t *cdata,
      unsigned cmode, uint32_t csize,uint32_t size,
      uint32_t crc32, struct archive_extract_userdata *userdata)
{
   char path_dir[PATH_MAX_LENGTH];
   char path[PATH_MAX_LENGTH];

   path_dir[0] = path[0] = '\0';

   /* Ignore directories. */
   if (name[strlen(name) - 1] == '/' || name[strlen(name) - 1] == '\\')
      goto next_file;

   if (strstr(name, userdata->dec->subdir) != name)
      return 1;

   name += strlen(userdata->dec->subdir) + 1;

   fill_pathname_join(path, userdata->dec->target_dir, name, sizeof(path));
   fill_pathname_basedir(path_dir, path, sizeof(path_dir));

   /* Make directory */
   if (!path_mkdir(path_dir))
      goto error;

   if (!file_archive_perform_mode(path, valid_exts,
            cdata, cmode, csize, size, crc32, userdata))
      goto error;

#if 0
   RARCH_LOG("[deflate subdir] Path: %s, CRC32: 0x%x\n", name, crc32);
#endif

next_file:
   return 1;

error:
   userdata->dec->callback_error = (char*)malloc(PATH_MAX_LENGTH);
   snprintf(userdata->dec->callback_error,
         PATH_MAX_LENGTH, "Failed to deflate %s.\n", path);

   return 0;
}

static int file_decompressed(const char *name, const char *valid_exts,
   const uint8_t *cdata, unsigned cmode, uint32_t csize, uint32_t size,
   uint32_t crc32, struct archive_extract_userdata *userdata)
{
   char path[PATH_MAX_LENGTH];
   decompress_state_t    *dec = userdata->dec;

   path[0] = '\0';

   /* Ignore directories. */
   if (name[strlen(name) - 1] == '/' || name[strlen(name) - 1] == '\\')
      goto next_file;

   /* Make directory */
   fill_pathname_join(path, dec->target_dir, name, sizeof(path));
   path_basedir_wrapper(path);

   if (!path_mkdir(path))
      goto error;

   fill_pathname_join(path, dec->target_dir, name, sizeof(path));

   if (!file_archive_perform_mode(path, valid_exts,
            cdata, cmode, csize, size, crc32, userdata))
      goto error;

#if 0
   RARCH_LOG("[deflate] Path: %s, CRC32: 0x%x\n", name, crc32);
#endif

next_file:
   return 1;

error:
   dec->callback_error = (char*)malloc(PATH_MAX_LENGTH);
   snprintf(dec->callback_error, PATH_MAX_LENGTH,
         "Failed to deflate %s.\n", path);

   return 0;
}

static void task_decompress_handler_finished(retro_task_t *task,
      decompress_state_t *dec)
{
   task_set_finished(task, true);

   if (!task_get_error(task) && task_get_cancelled(task))
      task_set_error(task, strdup("Task canceled"));

   if (task_get_error(task))
      free(dec->source_file);
   else
   {
      decompress_task_data_t *data =
         (decompress_task_data_t*)calloc(1, sizeof(*data));

      data->source_file = dec->source_file;
      task_set_data(task, data);
   }

   if (dec->subdir)
      free(dec->subdir);
   if (dec->valid_ext)
      free(dec->valid_ext);
   free(dec->target_dir);
   free(dec);
}

static void task_decompress_handler(retro_task_t *task)
{
   int ret;
   bool retdec                              = false;
   struct archive_extract_userdata userdata = {{0}};
   decompress_state_t *dec                  = (decompress_state_t*)task->state;

   userdata.dec            = dec;
   strlcpy(userdata.archive_path, dec->source_file, sizeof(userdata.archive_path));

   ret                     = file_archive_parse_file_iterate(&dec->archive,
         &retdec, dec->source_file,
         dec->valid_ext, file_decompressed, &userdata);

   task_set_progress(task, file_archive_parse_file_progress(&dec->archive));

   if (task_get_cancelled(task) || ret != 0)
   {
      task_set_error(task, dec->callback_error);
      file_archive_parse_file_iterate_stop(&dec->archive);

      task_decompress_handler_finished(task, dec);
   }
}

static void task_decompress_handler_target_file(retro_task_t *task)
{
   bool retdec;
   int ret;
   struct archive_extract_userdata userdata = {{0}};
   decompress_state_t *dec                  = (decompress_state_t*)task->state;

   strlcpy(userdata.archive_path,
         dec->source_file, sizeof(userdata.archive_path));

   ret = file_archive_parse_file_iterate(&dec->archive,
         &retdec, dec->source_file,
         dec->valid_ext, file_decompressed_target_file, &userdata);

   task_set_progress(task, file_archive_parse_file_progress(&dec->archive));

   if (task_get_cancelled(task) || ret != 0)
   {
      task_set_error(task, dec->callback_error);
      file_archive_parse_file_iterate_stop(&dec->archive);

      task_decompress_handler_finished(task, dec);
   }
}

static void task_decompress_handler_subdir(retro_task_t *task)
{
   int ret;
   bool retdec;
   decompress_state_t *dec = (decompress_state_t*)task->state;
   struct archive_extract_userdata userdata = {{0}};

   userdata.dec            = dec;
   strlcpy(userdata.archive_path, dec->source_file, sizeof(userdata.archive_path));

   ret                     = file_archive_parse_file_iterate(&dec->archive,
         &retdec, dec->source_file,
         dec->valid_ext, file_decompressed_subdir, &userdata);

   task_set_progress(task, file_archive_parse_file_progress(&dec->archive));

   if (task_get_cancelled(task) || ret != 0)
   {
      task_set_error(task, dec->callback_error);
      file_archive_parse_file_iterate_stop(&dec->archive);

      task_decompress_handler_finished(task, dec);
   }
}

static bool task_decompress_finder(
      retro_task_t *task, void *user_data)
{
   decompress_state_t *dec = (decompress_state_t*)task->state;

   if (task->handler != task_decompress_handler)
      return false;

   return string_is_equal(dec->source_file, (const char*)user_data);
}

bool task_check_decompress(const char *source_file)
{
   task_finder_data_t find_data;

   /* Prepare find parameters */
   find_data.func = task_decompress_finder;
   find_data.userdata = (void *)source_file;

   /* Return whether decompressing is in progress or not */
   return task_queue_ctl(TASK_QUEUE_CTL_FIND, &find_data);
}

bool task_push_decompress(
      const char *source_file,
      const char *target_dir,
      const char *target_file,
      const char *subdir,
      const char *valid_ext,
      retro_task_callback_t cb,
      void *user_data)
{
   char tmp[PATH_MAX_LENGTH];
   decompress_state_t *s      = NULL;
   retro_task_t *t            = NULL;

   tmp[0] = '\0';

   if (string_is_empty(target_dir) || string_is_empty(source_file))
   {
      RARCH_WARN("[decompress] Empty or null source file or"
            " target directory arguments.\n");
      return false;
   }

   /* ZIP or APK only */
   if (!path_file_exists(source_file) ||
         msg_hash_to_file_type(msg_hash_calculate(path_get_extension(source_file)))
         != FILE_TYPE_COMPRESSED)
   {
      RARCH_WARN("[decompress] File '%s' does not exist or is not a compressed file.\n",
            source_file);
      return false;
   }

   if (!valid_ext || !valid_ext[0])
      valid_ext   = NULL;

   if (task_check_decompress(source_file))
   {
      RARCH_LOG("[decompress] File '%s' already being decompressed.\n",
            source_file);
      return false;
   }

   RARCH_LOG("[decompress] File '%s.\n", source_file);

   s              = (decompress_state_t*)calloc(1, sizeof(*s));

   if (!s)
      goto error;

   s->source_file = strdup(source_file);
   s->target_dir  = strdup(target_dir);

   s->valid_ext   = valid_ext ? strdup(valid_ext) : NULL;
   s->archive.type   = ARCHIVE_TRANSFER_INIT;

   t              = (retro_task_t*)calloc(1, sizeof(*t));

   if (!t)
      goto error;

   t->state       = s;
   t->handler     = task_decompress_handler;

   if (!string_is_empty(subdir))
   {
      s->subdir        = strdup(subdir);
      t->handler       = task_decompress_handler_subdir;
   }
   else if (!string_is_empty(target_file))
   {
      s->target_file   = strdup(target_file);
      t->handler       = task_decompress_handler_target_file;
   }

   t->callback    = cb;
   t->user_data   = user_data;

   snprintf(tmp, sizeof(tmp), "%s '%s'",
         msg_hash_to_str(MSG_EXTRACTING), path_basename(source_file));

   t->title       = strdup(tmp);

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, t);

   return true;

error:
   if (s)
      free(s);
   return false;
}
