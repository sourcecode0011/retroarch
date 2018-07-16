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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <errno.h>

#include <compat/strl.h>
#include <retro_assert.h>
#include <lists/string_list.h>
#include <streams/file_stream.h>
#include <rthreads/rthreads.h>
#include <file/file_path.h>
#include <retro_miscellaneous.h>

#ifdef HAVE_CONFIG_H
#include "../core.h"
#endif

#ifdef HAVE_NETWORKING
#include "../network/netplay/netplay.h"
#endif

#include "../core.h"
#include "../file_path_special.h"
#include "../configuration.h"
#include "../msg_hash.h"
#include "../retroarch.h"
#include "../runloop.h"
#include "../verbosity.h"
#include "tasks_internal.h"

#define SAVE_STATE_CHUNK 4096

static struct string_list *task_save_files = NULL;

struct ram_type
{
   const char *path;
   int type;
};

struct save_state_buf
{
   void* data;
   char path[PATH_MAX_LENGTH];
   size_t size;
};

struct sram_block
{
   unsigned type;
   void *data;
   size_t size;
};

typedef struct
{
   RFILE *file;
   char path[PATH_MAX_LENGTH];
   void *data;
   void *undo_data;
   ssize_t size;
   ssize_t undo_size;
   ssize_t written;
   ssize_t bytes_read;
   bool load_to_backup_buffer;
   bool autoload;
   bool autosave;
   bool undo_save;
   bool mute;
   int state_slot;
   bool thumbnail_enable;
} save_task_state_t;

typedef save_task_state_t load_task_data_t;

/* Holds the previous saved state
 * Can be restored to disk with undo_save_state(). */
static struct save_state_buf undo_save_buf;

/* Holds the data from before a load_state() operation
 * Can be restored with undo_load_state(). */
static struct save_state_buf undo_load_buf;

#ifdef HAVE_THREADS
typedef struct autosave autosave_t;

/* Autosave support. */
struct autosave_st
{
   autosave_t **list;
   unsigned num;
};

struct autosave
{
   volatile bool quit;
   slock_t *lock;

   slock_t *cond_lock;
   scond_t *cond;
   sthread_t *thread;

   void *buffer;
   const void *retro_buffer;
   const char *path;
   size_t bufsize;
   unsigned interval;
};

static struct autosave_st autosave_state;

/**
 * autosave_thread:
 * @data            : pointer to autosave object
 *
 * Callback function for (threaded) autosave.
 **/
static void autosave_thread(void *data)
{
   bool first_log   = true;
   autosave_t *save = (autosave_t*)data;

   while (!save->quit)
   {
      bool differ;

      slock_lock(save->lock);
      differ = memcmp(save->buffer, save->retro_buffer,
            save->bufsize) != 0;
      if (differ)
         memcpy(save->buffer, save->retro_buffer, save->bufsize);
      slock_unlock(save->lock);

      if (differ)
      {
         /* Should probably deal with this more elegantly. */
         FILE *file = fopen(save->path, "wb");

         if (file)
         {
            bool failed = false;

            /* Avoid spamming down stderr ... */
            if (first_log)
            {
               RARCH_LOG("Autosaving SRAM to \"%s\", will continue to check every %u seconds ...\n",
                     save->path, save->interval);
               first_log = false;
            }
            else
               RARCH_LOG("SRAM changed ... autosaving ...\n");

            failed |= fwrite(save->buffer, 1, save->bufsize, file)
               != save->bufsize;
            failed |= fflush(file) != 0;
            failed |= fclose(file) != 0;
            if (failed)
               RARCH_WARN("Failed to autosave SRAM. Disk might be full.\n");
         }
      }

      slock_lock(save->cond_lock);

      if (!save->quit)
         scond_wait_timeout(save->cond, save->cond_lock,
               save->interval * 1000000LL);

      slock_unlock(save->cond_lock);
   }
}

/**
 * autosave_new:
 * @path            : path to autosave file
 * @data            : pointer to buffer
 * @size            : size of @data buffer
 * @interval        : interval at which saves should be performed.
 *
 * Create and initialize autosave object.
 *
 * Returns: pointer to new autosave_t object if successful, otherwise
 * NULL.
 **/
static autosave_t *autosave_new(const char *path,
      const void *data, size_t size,
      unsigned interval)
{
   autosave_t *handle   = (autosave_t*)calloc(1, sizeof(*handle));
   if (!handle)
      goto error;

   handle->bufsize      = size;
   handle->interval     = interval;
   handle->path         = path;
   handle->buffer       = malloc(size);
   handle->retro_buffer = data;

   if (!handle->buffer)
      goto error;

   memcpy(handle->buffer, handle->retro_buffer, handle->bufsize);

   handle->lock         = slock_new();
   handle->cond_lock    = slock_new();
   handle->cond         = scond_new();

   handle->thread       = sthread_create(autosave_thread, handle);

   return handle;

error:
   if (handle)
      free(handle);
   return NULL;
}

/**
 * autosave_free:
 * @handle          : pointer to autosave object
 *
 * Frees autosave object.
 **/
static void autosave_free(autosave_t *handle)
{
   slock_lock(handle->cond_lock);
   handle->quit = true;
   slock_unlock(handle->cond_lock);
   scond_signal(handle->cond);
   sthread_join(handle->thread);

   slock_free(handle->lock);
   slock_free(handle->cond_lock);
   scond_free(handle->cond);

   if (handle->buffer)
      free(handle->buffer);
   handle->buffer = NULL;
}


void autosave_init(void)
{
   unsigned i;
   autosave_t **list    = NULL;
   settings_t *settings = config_get_ptr();

   if (settings->autosave_interval < 1 || !task_save_files)
      return;

   list = (autosave_t**)calloc(task_save_files->size,
               sizeof(*autosave_state.list));
   if (!list)
      return;

   autosave_state.list = list;
   autosave_state.num  = task_save_files->size;

   for (i = 0; i < task_save_files->size; i++)
   {
      retro_ctx_memory_info_t mem_info;
      const char *path = task_save_files->elems[i].data;
      unsigned    type = task_save_files->elems[i].attr.i;

      mem_info.id = type;

      core_get_memory(&mem_info);

      if (mem_info.size <= 0)
         continue;

      autosave_state.list[i] = autosave_new(path,
            mem_info.data,
            mem_info.size,
            settings->autosave_interval);

      if (!autosave_state.list[i])
         RARCH_WARN("%s\n", msg_hash_to_str(MSG_AUTOSAVE_FAILED));
   }
}

void autosave_deinit(void)
{
   unsigned i;

   for (i = 0; i < autosave_state.num; i++)
   {
      autosave_t *handle = autosave_state.list[i];
      if (handle)
      {
         autosave_free(handle);
         free(autosave_state.list[i]);
      }
      autosave_state.list[i] = NULL;
   }

   free(autosave_state.list);

   autosave_state.list     = NULL;
   autosave_state.num      = 0;
}
#endif

/**
 * autosave_lock:
 *
 * Lock autosave.
 **/
void autosave_lock(void)
{
#ifdef HAVE_THREADS
   unsigned i;

   for (i = 0; i < autosave_state.num; i++)
   {
      autosave_t *handle = autosave_state.list[i];
      if (handle)
         slock_lock(handle->lock);
   }
#endif
}

/**
 * autosave_unlock:
 *
 * Unlocks autosave.
 **/
void autosave_unlock(void)
{
#ifdef HAVE_THREADS
   unsigned i;

   for (i = 0; i < autosave_state.num; i++)
   {
      autosave_t *handle = autosave_state.list[i];
      if (handle)
         slock_unlock(handle->lock);
   }
#endif
}

/**
 * undo_load_state:
 * Revert to the state before a state was loaded.
 *
 * Returns: true if successful, false otherwise.
 **/
bool content_undo_load_state(void)
{
   unsigned i;
   retro_ctx_serialize_info_t serial_info;
   size_t temp_data_size;
   bool ret                  = false;
   unsigned num_blocks       = 0;
   void* temp_data           = NULL;
   struct sram_block *blocks = NULL;
   settings_t *settings      = config_get_ptr();

   RARCH_LOG("%s: \"%s\".\n",
         msg_hash_to_str(MSG_LOADING_STATE),
         undo_load_buf.path);

   RARCH_LOG("%s: %u %s.\n",
         msg_hash_to_str(MSG_STATE_SIZE),
         undo_load_buf.size,
         msg_hash_to_str(MSG_BYTES));

   /* TODO/FIXME - This checking of SRAM overwrite, 
    * the backing up of it and
    * its flushing could all be in their 
    * own functions... */
   if (settings->block_sram_overwrite && task_save_files
         && task_save_files->size)
   {
      RARCH_LOG("%s.\n",
            msg_hash_to_str(MSG_BLOCKING_SRAM_OVERWRITE));
      blocks = (struct sram_block*)
         calloc(task_save_files->size, sizeof(*blocks));

      if (blocks)
      {
         num_blocks = task_save_files->size;
         for (i = 0; i < num_blocks; i++)
            blocks[i].type = task_save_files->elems[i].attr.i;
      }
   }

   for (i = 0; i < num_blocks; i++)
   {
      retro_ctx_memory_info_t    mem_info;

      mem_info.id = blocks[i].type;
      core_get_memory(&mem_info);

      blocks[i].size = mem_info.size;
   }

   for (i = 0; i < num_blocks; i++)
      if (blocks[i].size)
         blocks[i].data = malloc(blocks[i].size);

   /* Backup current SRAM which is overwritten by unserialize. */
   for (i = 0; i < num_blocks; i++)
   {
      if (blocks[i].data)
      {
         retro_ctx_memory_info_t    mem_info;
         const void *ptr = NULL;

         mem_info.id = blocks[i].type;

         core_get_memory(&mem_info);

         ptr = mem_info.data;
         if (ptr)
            memcpy(blocks[i].data, ptr, blocks[i].size);
      }
   }

   /* We need to make a temporary copy of the buffer, to allow the swap below */
   temp_data              = malloc(undo_load_buf.size);
   temp_data_size         = undo_load_buf.size;
   memcpy(temp_data, undo_load_buf.data, undo_load_buf.size);

   serial_info.data_const = temp_data;
   serial_info.size       = temp_data_size;

   /* Swap the current state with the backup state. This way, we can undo
   what we're undoing */
   content_save_state("RAM", false, false);

   ret                    = core_unserialize(&serial_info);

   /* Clean up the temporary copy */
   free(temp_data);
   temp_data              = NULL;

    /* Flush back. */
   for (i = 0; i < num_blocks; i++)
   {
      if (blocks[i].data)
      {
         retro_ctx_memory_info_t    mem_info;
         void *ptr   = NULL;

         mem_info.id = blocks[i].type;

         core_get_memory(&mem_info);

         ptr = mem_info.data;
         if (ptr)
            memcpy(ptr, blocks[i].data, blocks[i].size);
      }
   }

   for (i = 0; i < num_blocks; i++)
      free(blocks[i].data);
   free(blocks);

   if (!ret)
   {
      RARCH_ERR("%s \"%s\".\n",
         msg_hash_to_str(MSG_FAILED_TO_UNDO_LOAD_STATE),
         undo_load_buf.path);
   }

   return ret;
}

static void undo_save_state_cb(void *task_data,
                               void *user_data, const char *error)
{
   /* Wipe the save file buffer as it's intended to be one use only */
   undo_save_buf.path[0] = '\0';
   undo_save_buf.size    = 0;
   if (undo_save_buf.data)
   {
      free(undo_save_buf.data);
      undo_save_buf.data = NULL;
   }
}

/**
 * task_save_handler_finished:
 * @task : the task to finish
 * @state : the state associated with this task
 *
 * Close the save state file and finish the task.
 **/
#ifdef ANDROID
 #define RETRO_EVENT_SAVEING_STATE 0x1001
 #define RETRO_EVENT_SAVED_STATE 0x1002
 #define RETRO_EVENT_LOADING_STATE 0x1003
 #define RETRO_EVENT_LOADED_STATE 0x1004
void tl_android_event(int event,int val,int other);
#endif

static void task_save_handler_finished(retro_task_t *task,
      save_task_state_t *state)
{
   save_task_state_t *task_data = NULL;

   task_set_finished(task, true);

   filestream_close(state->file);


#ifdef ANDROID
   tl_android_event(RETRO_EVENT_SAVED_STATE,task->error!=NULL,task->cancelled);
#endif
   if (!task_get_error(task) && task_get_cancelled(task))
      task_set_error(task, strdup("Task canceled"));

   task_data = (save_task_state_t*)calloc(1, sizeof(*task_data));
   memcpy(task_data, state, sizeof(*state));

   task_set_data(task, task_data);

   if (state->data)
   {
      if (state->undo_save && state->data == undo_save_buf.data)
         undo_save_buf.data = NULL;
      free(state->data);
      state->data = NULL;
   }

   free(state);
}

/**
 * task_save_handler:
 * @task : the task being worked on
 *
 * Write a chunk of data to the save state file.
 **/


static void task_save_handler(retro_task_t *task)
{
   int written;
   ssize_t remaining;
   save_task_state_t *state = (save_task_state_t*)task->state;

   if (!state->file)
   {
      state->file = filestream_open(state->path, RFILE_MODE_WRITE, -1);

      if (!state->file)
         return;
   }

   remaining       = MIN(state->size - state->written, SAVE_STATE_CHUNK);
   written         = filestream_write(state->file,
         (uint8_t*)state->data + state->written, remaining);

   state->written += written;

   task_set_progress(task, (state->written / (float)state->size) * 100);


#ifdef ANDROID
   tl_android_event(RETRO_EVENT_SAVEING_STATE,task->progress,0);
#endif


   if (task_get_cancelled(task) || written != remaining)
   {
      char err[PATH_MAX_LENGTH];

      err[0] = '\0';

      if (state->undo_save)
      {
         RARCH_ERR("%s \"%s\".\n",
            msg_hash_to_str(MSG_FAILED_TO_UNDO_SAVE_STATE),
            undo_save_buf.path);

         snprintf(err, sizeof(err), "%s \"%s\".",
                  msg_hash_to_str(MSG_FAILED_TO_UNDO_SAVE_STATE),
                  "RAM");
      }
      else
         snprintf(err, sizeof(err), "%s %s", msg_hash_to_str(MSG_FAILED_TO_SAVE_STATE_TO), state->path);

      task_set_error(task, strdup(err));
      task_save_handler_finished(task, state);
      return;
   }

   if (state->written == state->size)
   {
      char       *msg      = NULL;

      task_free_title(task);

      if (state->undo_save)
         msg = strdup(msg_hash_to_str(MSG_RESTORED_OLD_SAVE_STATE));
      else if (state->state_slot < 0)
         msg = strdup(msg_hash_to_str(MSG_SAVED_STATE_TO_SLOT_AUTO));
      else
      {
         char new_msg[128];
         new_msg[0] = '\0';

         snprintf(new_msg, sizeof(new_msg), msg_hash_to_str(MSG_SAVED_STATE_TO_SLOT),
               state->state_slot);
         msg = strdup(new_msg);
      }

      if (!task_get_mute(task) && msg)
         task_set_title(task, msg);

      task_save_handler_finished(task, state);

      return;
   }
}

/**
 * task_push_undo_save_state:
 * @path : file path of the save state
 * @data : the save state data to write
 * @size : the total size of the save state
 *
 * Create a new task to undo the last save of the content state.
 **/
static bool task_push_undo_save_state(const char *path, void *data, size_t size)
{
   retro_task_t       *task = (retro_task_t*)calloc(1, sizeof(*task));
   save_task_state_t *state = (save_task_state_t*)calloc(1, sizeof(*state));
   settings_t     *settings = config_get_ptr();

   if (!task || !state)
      goto error;

   strlcpy(state->path, path, sizeof(state->path));
   state->data       = data;
   state->size       = size;
   state->undo_save  = true;
   state->state_slot = settings->state_slot;

   task->type        = TASK_TYPE_BLOCKING;
   task->state       = state;
   task->handler     = task_save_handler;
   task->callback    = undo_save_state_cb;
   task->title       = strdup(msg_hash_to_str(MSG_UNDOING_SAVE_STATE));

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, task);

   return true;

error:
   if (data)
      free(data);
   if (state)
      free(state);
   if (task)
      free(task);

   return false;
}

/**
 * undo_save_state:
 * Reverts the last save operation
 *
 * Returns: true if successful, false otherwise.
 **/
bool content_undo_save_state(void)
{
   return task_push_undo_save_state(undo_save_buf.path,
                             undo_save_buf.data,
                             undo_save_buf.size);
}

/**
 * task_load_handler_finished:
 * @task : the task to finish
 * @state : the state associated with this task
 *
 * Close the loaded state file and finish the task.
 **/
static void task_load_handler_finished(retro_task_t *task,
      save_task_state_t *state)
{
   load_task_data_t *task_data = NULL;

   task_set_finished(task, true);

   if (state->file)
      filestream_close(state->file);


#ifdef ANDROID
   tl_android_event(RETRO_EVENT_LOADED_STATE,task->error!=NULL,task->cancelled);
#endif
   
   if (!task_get_error(task) && task_get_cancelled(task))
      task_set_error(task, strdup("Task canceled"));


   task_data = (load_task_data_t*)calloc(1, sizeof(*task_data));
   memcpy(task_data, state, sizeof(*task_data));

   task_set_data(task, task_data);

   free(state);
}

/**
 * task_load_handler:
 * @task : the task being worked on
 *
 * Load a chunk of data from the save state file.
 **/
static void task_load_handler(retro_task_t *task)
{
   ssize_t remaining, bytes_read;
   save_task_state_t *state = (save_task_state_t*)task->state;

   if (!state->file)
   {
      state->file = filestream_open(state->path, RFILE_MODE_READ, -1);

      if (!state->file)
         goto error;

      if (filestream_seek(state->file, 0, SEEK_END) != 0)
         goto error;

      state->size = filestream_tell(state->file);

      if (state->size < 0)
         goto error;

      filestream_rewind(state->file);

      state->data = malloc(state->size + 1);

      if (!state->data)
         goto error;
   }

   remaining          = MIN(state->size - state->bytes_read, SAVE_STATE_CHUNK);
   bytes_read         = filestream_read(state->file,
         (uint8_t*)state->data + state->bytes_read, remaining);
   state->bytes_read += bytes_read;

   if (state->size > 0){
      task_set_progress(task, (state->bytes_read / (float)state->size) * 100);
#ifdef ANDROID
   tl_android_event(RETRO_EVENT_LOADING_STATE,task->progress,0);
#endif
   }


   if (task_get_cancelled(task) || bytes_read != remaining)
   {
      if (state->autoload)
      {
         char msg[1024];

         msg[0] = '\0';

         snprintf(msg, sizeof(msg), "%s \"%s\" %s.",
               msg_hash_to_str(MSG_AUTOLOADING_SAVESTATE_FROM),
               state->path,
               msg_hash_to_str(MSG_FAILED));
         task_set_error(task, strdup(msg));
      }
      else
         task_set_error(task, strdup(msg_hash_to_str(MSG_FAILED_TO_LOAD_STATE)));

      free(state->data);
      state->data = NULL;
      task_load_handler_finished(task, state);
      return;
   }

   if (state->bytes_read == state->size)
   {
      char msg[1024];

      msg[0] = '\0';

      task_free_title(task);

      if (state->autoload)
      {
         snprintf(msg, sizeof(msg), "%s \"%s\" %s.",
               msg_hash_to_str(MSG_AUTOLOADING_SAVESTATE_FROM),
               state->path,
               msg_hash_to_str(MSG_SUCCEEDED));
      }
      else
      {
         if (state->state_slot < 0)
            strlcpy(msg, msg_hash_to_str(MSG_LOADED_STATE_FROM_SLOT_AUTO), sizeof(msg));
         else
            snprintf(msg, sizeof(msg), msg_hash_to_str(MSG_LOADED_STATE_FROM_SLOT),
                  state->state_slot);

      }

      if (!task_get_mute(task))
         task_set_title(task, strdup(msg));

      task_load_handler_finished(task, state);

      return;
   }

   return;

error:
   task_load_handler_finished(task, state);
}

/**
 * content_load_state_cb:
 * @path      : path that state will be loaded from.
 * Load a state from disk to memory.
 *
 **/
static void content_load_state_cb(void *task_data,
                           void *user_data, const char *error)
{
   retro_ctx_serialize_info_t serial_info;
   unsigned i;
   bool ret;
   load_task_data_t *load_data = (load_task_data_t*)task_data;
   ssize_t size                = load_data->size;
   unsigned num_blocks         = 0;
   void *buf                   = load_data->data;
   struct sram_block *blocks   = NULL;
   settings_t *settings        = config_get_ptr();

   RARCH_LOG("%s: \"%s\".\n",
         msg_hash_to_str(MSG_LOADING_STATE),
         load_data->path);

   if (size < 0 || !buf)
      goto error;

   RARCH_LOG("%s: %u %s.\n",
         msg_hash_to_str(MSG_STATE_SIZE),
         (unsigned)size,
         msg_hash_to_str(MSG_BYTES));

   /* This means we're backing up the file in memory, so content_undo_save_state()
   can restore it */
   if (load_data->load_to_backup_buffer)
   {
      /* If we were previously backing up a file, let go of it first */
      if (undo_save_buf.data)
      {
         free(undo_save_buf.data);
         undo_save_buf.data = NULL;
      }

      undo_save_buf.data = malloc(size);
      if (!undo_save_buf.data)
         goto error;

      memcpy(undo_save_buf.data, buf, size);
      undo_save_buf.size = size;
      strlcpy(undo_save_buf.path, load_data->path, sizeof(undo_save_buf.path));

      free(buf);
      free(load_data);
      return;
   }

   if (settings->block_sram_overwrite && task_save_files
         && task_save_files->size)
   {
      RARCH_LOG("%s.\n",
            msg_hash_to_str(MSG_BLOCKING_SRAM_OVERWRITE));
      blocks = (struct sram_block*)
         calloc(task_save_files->size, sizeof(*blocks));

      if (blocks)
      {
         num_blocks = task_save_files->size;
         for (i = 0; i < num_blocks; i++)
            blocks[i].type = task_save_files->elems[i].attr.i;
      }
   }

   for (i = 0; i < num_blocks; i++)
   {
      retro_ctx_memory_info_t    mem_info;

      mem_info.id = blocks[i].type;
      core_get_memory(&mem_info);

      blocks[i].size = mem_info.size;
   }

   for (i = 0; i < num_blocks; i++)
      if (blocks[i].size)
         blocks[i].data = malloc(blocks[i].size);

   /* Backup current SRAM which is overwritten by unserialize. */
   for (i = 0; i < num_blocks; i++)
   {
      if (blocks[i].data)
      {
         retro_ctx_memory_info_t    mem_info;
         const void *ptr = NULL;

         mem_info.id = blocks[i].type;

         core_get_memory(&mem_info);

         ptr = mem_info.data;
         if (ptr)
            memcpy(blocks[i].data, ptr, blocks[i].size);
      }
   }

   serial_info.data_const = buf;
   serial_info.size       = size;

   /* Backup the current state so we can undo this load */
   content_save_state("RAM", false, false);

   ret                    = core_unserialize(&serial_info);

    /* Flush back. */
   for (i = 0; i < num_blocks; i++)
   {
      if (blocks[i].data)
      {
         retro_ctx_memory_info_t    mem_info;
         void *ptr = NULL;

         mem_info.id = blocks[i].type;

         core_get_memory(&mem_info);

         ptr = mem_info.data;
         if (ptr)
            memcpy(ptr, blocks[i].data, blocks[i].size);
      }
   }

   for (i = 0; i < num_blocks; i++)
      free(blocks[i].data);
   free(blocks);

   if (!ret)
      goto error;

   free(buf);
   free(load_data);

   return;

error:
   RARCH_ERR("%s \"%s\".\n",
         msg_hash_to_str(MSG_FAILED_TO_LOAD_STATE),
         load_data->path);
   if (buf)
      free(buf);
   free(load_data);
}

/**
 * save_state_cb:
 *
 * Called after the save state is done. Takes a screenshot if needed.
 **/
static void save_state_cb(void *task_data,
                           void *user_data, const char *error)
{
   save_task_state_t *state = (save_task_state_t*)task_data;
   char               *path = strdup(state->path);

   if (state->thumbnail_enable)
      take_screenshot(path, true);

   free(path);
}

/**
 * task_push_save_state:
 * @path : file path of the save state
 * @data : the save state data to write
 * @size : the total size of the save state
 *
 * Create a new task to save the content state.
 **/
static void task_push_save_state(const char *path, void *data, size_t size, bool autosave)
{
   retro_task_t       *task = (retro_task_t*)calloc(1, sizeof(*task));
   save_task_state_t *state = (save_task_state_t*)calloc(1, sizeof(*state));
   settings_t     *settings = config_get_ptr();

   if (!task || !state)
      goto error;

   strlcpy(state->path, path, sizeof(state->path));
   state->data             = data;
   state->size             = size;
   state->autosave         = autosave;
   state->mute             = autosave; /* don't show OSD messages if we are auto-saving */
   state->thumbnail_enable = settings->savestate_thumbnail_enable;

   task->type              = TASK_TYPE_BLOCKING;
   task->state             = state;
   task->handler           = task_save_handler;
   task->callback          = save_state_cb;
   task->title             = strdup(msg_hash_to_str(MSG_SAVING_STATE));
   task->mute              = state->mute;

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, task);

   return;

error:
   if (data)
      free(data);
   if (state)
      free(state);
   if (task)
      free(task);
}

/**
 * content_load_and_save_state_cb:
 * @path      : path that state will be loaded from.
 * Load then save a state.
 *
 **/
static void content_load_and_save_state_cb(void *task_data,
                           void *user_data, const char *error)
{
   load_task_data_t *load_data = (load_task_data_t*)task_data;
   char                  *path = strdup(load_data->path);
   void                  *data = load_data->undo_data;
   size_t                 size = load_data->undo_size;
   bool               autosave = load_data->autosave;

   content_load_state_cb(task_data, user_data, error);

   task_push_save_state(path, data, size, autosave);

   free(path);
}

/**
 * task_push_load_and_save_state:
 * @path : file path of the save state
 * @data : the save state data to write
 * @size : the total size of the save state
 * @load_to_backup_buffer : If true, the state will be loaded into undo_save_buf.
 *
 * Create a new task to load current state first into a backup buffer (for undo)
 * and then save the content state.
 **/
static void task_push_load_and_save_state(const char *path, void *data,
      size_t size, bool load_to_backup_buffer, bool autosave)
{
   retro_task_t       *task = (retro_task_t*)calloc(1, sizeof(*task));
   save_task_state_t *state = (save_task_state_t*)calloc(1, sizeof(*state));

   if (!task || !state)
      goto error;

   strlcpy(state->path, path, sizeof(state->path));
   state->load_to_backup_buffer = load_to_backup_buffer;
   state->undo_size = size;
   state->undo_data = data;
   state->autosave  = autosave;
   state->mute      = autosave; /* don't show OSD messages if we are auto-saving */

   task->state      = state;
   task->type       = TASK_TYPE_BLOCKING;
   task->handler    = task_load_handler;
   task->callback   = content_load_and_save_state_cb;
   task->title      = strdup(msg_hash_to_str(MSG_LOADING_STATE));
   task->mute       = state->mute;

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, task);

   return;

error:
   if (data)
      free(data);
   if (state)
      free(state);
   if (task)
      free(task);
}

/**
 * content_save_state:
 * @path      : path of saved state that shall be written to.
 * @save_to_disk: If false, saves the state onto undo_load_buf.
 * Save a state from memory to disk.
 *
 * Returns: true if successful, false otherwise.
 **/
bool content_save_state(const char *path, bool save_to_disk, bool autosave)
{
   retro_ctx_serialize_info_t serial_info;
   retro_ctx_size_info_t info;
   bool ret    = false;
   void *data  = NULL;

   core_serialize_size(&info);

   RARCH_LOG("%s: \"%s\".\n",
         msg_hash_to_str(MSG_SAVING_STATE),
         path);
   WERROR_A("%s: \"%s\".\n",
         msg_hash_to_str(MSG_SAVING_STATE),
         path);

   if (info.size == 0)
      return false;

   data = malloc(info.size);

   if (!data)
      return false;

   RARCH_LOG("%s: %d %s.\n",
         msg_hash_to_str(MSG_STATE_SIZE),
         (int)info.size,
         msg_hash_to_str(MSG_BYTES));
   WERROR_A("%s: %d %s.\n",
         msg_hash_to_str(MSG_STATE_SIZE),
         (int)info.size,
         msg_hash_to_str(MSG_BYTES));

   serial_info.data = data;
   serial_info.size = info.size;
   ret              = core_serialize(&serial_info);

   if (ret)
   {
      if (save_to_disk)
      {
         if (path_file_exists(path))
         {
            /* Before overwritting the savestate file, load it into a buffer
            to allow undo_save_state() to work */
            /* TODO/FIXME - Use msg_hash_to_str here */
            RARCH_LOG("%s ...\n",
                  msg_hash_to_str(MSG_FILE_ALREADY_EXISTS_SAVING_TO_BACKUP_BUFFER));

            task_push_load_and_save_state(path, data, info.size, true, autosave);
         }
         else
            task_push_save_state(path, data, info.size, autosave);
      }
      else
      {
         /* save_to_disk is false, which means we are saving the state
         in undo_load_buf to allow content_undo_load_state() to restore it */

         /* If we were holding onto an old state already, clean it up first */
         if (undo_load_buf.data)
         {
            free(undo_load_buf.data);
            undo_load_buf.data = NULL;
         }

         undo_load_buf.data = malloc(info.size);
         if (!undo_load_buf.data)
         {
            free(data);
            return false;
         }

         memcpy(undo_load_buf.data, data, info.size);
         free(data);
         undo_load_buf.size = info.size;
         strlcpy(undo_load_buf.path, path, sizeof(undo_load_buf.path));
      }
   }
   else
   {
      free(data);
      RARCH_ERR("%s \"%s\".\n",
            msg_hash_to_str(MSG_FAILED_TO_SAVE_STATE_TO),
            path);
   }

   return ret;
}

/**
 * content_load_state:
 * @path      : path that state will be loaded from.
 * @load_to_backup_buffer: If true, the state will be loaded into undo_save_buf.
 * Load a state from disk to memory.
 *
 * Returns: true if successful, false otherwise.
 *
 *
 **/
bool content_load_state(const char *path,
      bool load_to_backup_buffer, bool autoload)
{
   retro_task_t       *task = (retro_task_t*)calloc(1, sizeof(*task));
   save_task_state_t *state = (save_task_state_t*)calloc(1, sizeof(*state));

   if (!task || !state)
      goto error;

   strlcpy(state->path, path, sizeof(state->path));
   state->load_to_backup_buffer = load_to_backup_buffer;
   state->autoload              = autoload;

   task->type     = TASK_TYPE_BLOCKING;
   task->state    = state;
   task->handler  = task_load_handler;
   task->callback = content_load_state_cb;
   task->title    = strdup(msg_hash_to_str(MSG_LOADING_STATE));

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, task);

   return true;

error:
   if (state)
      free(state);
   if (task)
      free(task);

   return false;
}

bool content_rename_state(const char *origin, const char *dest)
{
   int ret = 0;
   if (path_file_exists(dest))
      unlink(dest);

   ret = rename (origin, dest);
   if (!ret)
      return true;

   RARCH_LOG("Error %d renaming file %s\n", ret, origin);
   return false;
}

/*
*
* TODO/FIXME: Figure out when and where this should be called.
* As it is, when e.g. closing Gambatte, we get the same printf message 4 times.
*
*/
bool content_reset_savestate_backups(void)
{
   RARCH_LOG("Resetting undo buffers.\n");

   if (undo_save_buf.data)
   {
      free(undo_save_buf.data);
      undo_save_buf.data = NULL;
   }

   undo_save_buf.path[0] = '\0';
   undo_save_buf.size    = 0;

   if (undo_load_buf.data)
   {
      free(undo_load_buf.data);
      undo_load_buf.data = NULL;
   }

   undo_load_buf.path[0] = '\0';
   undo_load_buf.size    = 0;

   return true;
}

bool content_undo_load_buf_is_empty(void)
{
   return undo_load_buf.data == NULL || undo_load_buf.size == 0;
}

bool content_undo_save_buf_is_empty(void)
{
   return undo_save_buf.data == NULL || undo_save_buf.size == 0;
}

static bool content_get_memory(retro_ctx_memory_info_t *mem_info,
      struct ram_type *ram, unsigned slot)
{
   ram->type = task_save_files->elems[slot].attr.i;
   ram->path = task_save_files->elems[slot].data;

   mem_info->id  = ram->type;

   core_get_memory(mem_info);

   if (!mem_info->data || mem_info->size == 0)
      return false;

   return true;
}

/**
 * content_load_ram_file:
 * @path             : path of RAM state that will be loaded from.
 * @type             : type of memory
 *
 * Load a RAM state from disk to memory.
 */
bool content_load_ram_file(unsigned slot)
{
   ssize_t rc;
   struct ram_type ram;
   retro_ctx_memory_info_t mem_info;
   void *buf        = NULL;

   if (!content_get_memory(&mem_info, &ram, slot))
      return false;

   if (!filestream_read_file(ram.path, &buf, &rc))
      return false;

   if (rc > 0)
   {
      if (rc > (ssize_t)mem_info.size)
      {
         RARCH_WARN("SRAM is larger than implementation expects, "
               "doing partial load (truncating %u %s %s %u).\n",
               (unsigned)rc,
               msg_hash_to_str(MSG_BYTES),
               msg_hash_to_str(MSG_TO),
               (unsigned)mem_info.size);
         rc = mem_info.size;
      }
      memcpy(mem_info.data, buf, rc);
   }

   if (buf)
      free(buf);

   return true;
}

/**
 * dump_to_file_desperate:
 * @data         : pointer to data buffer.
 * @size         : size of @data.
 * @type         : type of file to be saved.
 *
 * Attempt to save valuable RAM data somewhere.
 **/
static bool dump_to_file_desperate(const void *data,
      size_t size, unsigned type)
{
   time_t time_;
   char timebuf[256];
   char application_data[PATH_MAX_LENGTH];
   char path[PATH_MAX_LENGTH];

   timebuf[0] = application_data[0] = path[0] = '\0';

   if (!fill_pathname_application_data(application_data,
            sizeof(application_data)))
      return false;

   snprintf(path, sizeof(path), "%s/RetroArch-recovery-%u",
      application_data, type);

   time(&time_);

   strftime(timebuf, sizeof(timebuf), "%Y-%m-%d-%H-%M-%S", localtime(&time_));
   strlcat(path, timebuf, sizeof(path));

   if (!filestream_write_file(path, data, size))
      return false;

   RARCH_WARN("Succeeded in saving RAM data to \"%s\".\n", path);
   return true;
}

/**
 * content_save_ram_file:
 * @path             : path of RAM state that shall be written to.
 * @type             : type of memory
 *
 * Save a RAM state from memory to disk.
 *
 */
bool content_save_ram_file(unsigned slot)
{
   struct ram_type ram;
   retro_ctx_memory_info_t mem_info;

   if (!content_get_memory(&mem_info, &ram, slot))
      return false;

   RARCH_LOG("%s #%u %s \"%s\".\n",
         msg_hash_to_str(MSG_SAVING_RAM_TYPE),
         ram.type,
         msg_hash_to_str(MSG_TO),
         ram.path);

   if (!filestream_write_file(ram.path, mem_info.data, mem_info.size))
   {
      RARCH_ERR("%s.\n",
            msg_hash_to_str(MSG_FAILED_TO_SAVE_SRAM));
      RARCH_WARN("Attempting to recover ...\n");

      /* In case the file could not be written to,
       * the fallback function 'dump_to_file_desperate'
       * will be called. */
      if (!dump_to_file_desperate(mem_info.data, mem_info.size, ram.type))
      {
         RARCH_WARN("Failed ... Cannot recover save file.\n");
      }
      return false;
   }

   RARCH_LOG("%s \"%s\".\n",
         msg_hash_to_str(MSG_SAVED_SUCCESSFULLY_TO),
         ram.path);

   return true;
}

bool event_save_files(void)
{
   unsigned i;

   if (!task_save_files || !rarch_ctl(RARCH_CTL_IS_SRAM_USED, NULL))
      return false;

   for (i = 0; i < task_save_files->size; i++)
      content_save_ram_file(i);

   return true;
}

bool event_load_save_files(void)
{
   unsigned i;

   if (!task_save_files || rarch_ctl(RARCH_CTL_IS_SRAM_LOAD_DISABLED, NULL))
      return false;

   for (i = 0; i < task_save_files->size; i++)
      content_load_ram_file(i);

   return true;
}

void path_init_savefile_rtc(void)
{
   union string_list_elem_attr attr;
   char savefile_name_rtc[PATH_MAX_LENGTH];
   global_t                        *global = global_get_ptr();

   savefile_name_rtc[0] = '\0';

   attr.i = RETRO_MEMORY_SAVE_RAM;
   string_list_append(task_save_files, global->name.savefile, attr);

   /* Infer .rtc save path from save ram path. */
   attr.i = RETRO_MEMORY_RTC;
   fill_pathname(savefile_name_rtc,
         global->name.savefile,
         file_path_str(FILE_PATH_RTC_EXTENSION),
         sizeof(savefile_name_rtc));
   string_list_append(task_save_files, savefile_name_rtc, attr);
}

void path_deinit_savefile(void)
{
   if (task_save_files)
      string_list_free(task_save_files);
   task_save_files = NULL;
}

void path_init_savefile_new(void)
{
   task_save_files = string_list_new();
   retro_assert(task_save_files);
}

void *savefile_ptr_get(void)
{
   return task_save_files;
}
