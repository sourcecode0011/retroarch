/*  RetroArch - A frontend for libretro.
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

#include <net/net_http.h>
#include <queues/message_queue.h>
#include <lists/string_list.h>
#include <string/stdstring.h>
#include <compat/strl.h>
#include <file/file_path.h>
#include <file/archive_file.h>
#include <net/net_compat.h>
#include <retro_stat.h>

#include "../msg_hash.h"
#include "../verbosity.h"
#include "tasks_internal.h"

enum http_status_enum
{
   HTTP_STATUS_POLL = 0,
   HTTP_STATUS_CONNECTION_TRANSFER,
   HTTP_STATUS_CONNECTION_TRANSFER_PARSE,
   HTTP_STATUS_TRANSFER,
   HTTP_STATUS_TRANSFER_PARSE,
   HTTP_STATUS_TRANSFER_PARSE_FREE
};

typedef struct http_transfer_info
{
   char url[255];
   int progress;
} http_transfer_info_t;

typedef struct http_handle
{
   struct
   {
      struct http_connection_t *handle;
      transfer_cb_t  cb;
      char elem1[255];
      char url[255];
   } connection;
   struct http_t *handle;
   transfer_cb_t  cb;
   unsigned status;
   bool error;
} http_handle_t;

static int task_http_con_iterate_transfer(http_handle_t *http)
{
   if (!net_http_connection_iterate(http->connection.handle))
      return -1;
   return 0;
}

static int task_http_conn_iterate_transfer_parse(
      http_handle_t *http)
{
   if (net_http_connection_done(http->connection.handle))
   {
      if (http->connection.handle && http->connection.cb)
         http->connection.cb(http, 0);
   }
   
   net_http_connection_free(http->connection.handle);

   http->connection.handle = NULL;

   return 0;
}

static int cb_http_conn_default(void *data_, size_t len)
{
   http_handle_t *http = (http_handle_t*)data_;

   if (!http)
      return -1;

   if (!network_init())
      return -1;

   http->handle = net_http_new(http->connection.handle);

   if (!http->handle)
   {
      RARCH_ERR("[http] Could not create new HTTP session handle.\n");
      http->error = true;
      return -1;
   }

   http->cb     = NULL;

   return 0;
}

/**
 * task_http_iterate_transfer:
 *
 * Resumes HTTP transfer update.
 *
 * Returns: 0 when finished, -1 when we should continue
 * with the transfer on the next frame.
 **/
static int task_http_iterate_transfer(retro_task_t *task)
{
   http_handle_t *http  = (http_handle_t*)task->state;
   size_t pos  = 0, tot = 0;

   /* FIXME: This wouldn't be needed if we could wait for a timeout */
   if (task_queue_is_threaded())
      retro_sleep(1);

   if (!net_http_update(http->handle, &pos, &tot))
   {
      task_set_progress(task, (tot == 0) ? -1 : (signed)(pos * 100 / tot));
      return -1;
   }

   return 0;
}

static void task_http_transfer_handler(retro_task_t *task)
{
   http_transfer_data_t *data = NULL;
   http_handle_t        *http = (http_handle_t*)task->state;

   if (task_get_cancelled(task))
      goto task_finished;

   switch (http->status)
   {
      case HTTP_STATUS_CONNECTION_TRANSFER_PARSE:
         task_http_conn_iterate_transfer_parse(http);
         http->status = HTTP_STATUS_TRANSFER;
         break;
      case HTTP_STATUS_CONNECTION_TRANSFER:
         if (!task_http_con_iterate_transfer(http))
            http->status = HTTP_STATUS_CONNECTION_TRANSFER_PARSE;
         break;
      case HTTP_STATUS_TRANSFER:
         if (!task_http_iterate_transfer(task))
            goto task_finished;
         break;
      case HTTP_STATUS_TRANSFER_PARSE:
      case HTTP_STATUS_POLL:
         goto task_finished;
      default:
         break;
   }

   if (http->error)
      goto task_finished;

   return;
task_finished:
   task_set_finished(task, true);

   if (http->handle)
   {
      size_t len = 0;
      char  *tmp = (char*)net_http_data(http->handle, &len, false);

      if (tmp && http->cb)
         http->cb(tmp, len);

      if (net_http_error(http->handle) || task_get_cancelled(task))
      {
         tmp = (char*)net_http_data(http->handle, &len, true);

         if (tmp)
            free(tmp);

         if (task_get_cancelled(task))
            task_set_error(task, strdup("Task cancelled."));
         else
            task_set_error(task, strdup("Download failed."));
      }
      else
      {
         data = (http_transfer_data_t*)calloc(1, sizeof(*data));
         data->data = tmp;
         data->len  = len;

         task_set_data(task, data);
      }

      net_http_delete(http->handle);
   } else if (http->error)
      task_set_error(task, strdup("Internal error."));

   free(http);
}

static bool task_http_finder(retro_task_t *task, void *user_data)
{
   http_handle_t *http = NULL;

   if (!task || (task->handler != task_http_transfer_handler))
      return false;

   if (!user_data)
      return false;

   http = (http_handle_t*)task->state;
   if (!http)
      return false;

   return string_is_equal(http->connection.url, (const char*)user_data);
}

static bool task_http_retriever(retro_task_t *task, void *data)
{
   http_transfer_info_t *info = (http_transfer_info_t*)data;

   /* Extract HTTP handle and return already if invalid */
   http_handle_t        *http = (http_handle_t *)task->state;
   if (!http)
      return false;

   /* Fill HTTP info link */
   strlcpy(info->url, http->connection.url, sizeof(info->url));
   info->progress = task_get_progress(task);
   return true;
}

void *task_push_http_transfer(const char *url, bool mute, const char *type,
      retro_task_callback_t cb, void *user_data)
{
   task_finder_data_t find_data;
   char tmp[255];
   struct http_connection_t *conn = NULL;
   retro_task_t  *t               = NULL;
   http_handle_t *http            = NULL;

   if (string_is_empty(url))
      return NULL;

   tmp[0]             = '\0';

   find_data.func     = task_http_finder;
   find_data.userdata = (void*)url;

   /* Concurrent download of the same file is not allowed */
   if (task_queue_ctl(TASK_QUEUE_CTL_FIND, &find_data))
   {
      RARCH_LOG("[http] '%s'' is already being downloaded.\n", url);
      return NULL;
   }
   WERROR_A("current download url:%s",url);
   conn = net_http_connection_new(url);

   if (!conn)
      return NULL;

   http                    = (http_handle_t*)calloc(1, sizeof(*http));

   if (!http)
      goto error;

   http->connection.handle = conn;
   http->connection.cb     = &cb_http_conn_default;

   if (type)
      strlcpy(http->connection.elem1, type, sizeof(http->connection.elem1));

   strlcpy(http->connection.url, url, sizeof(http->connection.url));

   http->status            = HTTP_STATUS_CONNECTION_TRANSFER;
   t                       = (retro_task_t*)calloc(1, sizeof(*t));

   if (!t)
      goto error;

   t->handler              = task_http_transfer_handler;
   t->state                = http;
   t->mute                 = mute;
   t->callback             = cb;
   t->user_data            = user_data;
   t->progress             = -1;

   snprintf(tmp, sizeof(tmp), "%s '%s'",
         msg_hash_to_str(MSG_DOWNLOADING), path_basename(url));

   t->title                = strdup(tmp);

   task_queue_ctl(TASK_QUEUE_CTL_PUSH, t);

   return t;

error:
   if (conn)
      net_http_connection_free(conn);
   if (http)
      free(http);

   return NULL;
}

task_retriever_info_t *http_task_get_transfer_list(void)
{
   task_retriever_data_t retrieve_data;

   /* Fill retrieve data */
   retrieve_data.handler      = task_http_transfer_handler;
   retrieve_data.element_size = sizeof(http_transfer_info_t);
   retrieve_data.func         = task_http_retriever;

   /* Build list of current HTTP transfers and return it */
   task_queue_ctl(TASK_QUEUE_CTL_RETRIEVE, &retrieve_data);
   return retrieve_data.list;
}
