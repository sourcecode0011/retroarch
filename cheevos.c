/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2015-2016 - Andre Leiradella
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

#include <string.h>
#include <ctype.h>

#include <formats/jsonsax.h>
#include <streams/file_stream.h>
#include <compat/strl.h>
#include <rhash.h>
#include <libretro.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MENU
#include "menu/menu_driver.h"
#include "menu/menu_entries.h"
#endif

#include "cheevos.h"
#include "command.h"
#include "dynamic.h"
#include "network/net_http_special.h"
#include "tasks/tasks_internal.h"
#include "configuration.h"
#include "performance_counters.h"
#include "msg_hash.h"
#include "runloop.h"
#include "core.h"

#include "verbosity.h"

/* Define this macro to prevent cheevos from being deactivated. */
#undef CHEEVOS_DONT_DEACTIVATE

/* Define this macro to log URLs (will log the user token). */
#undef CHEEVOS_LOG_URLS

/* Define this macro to dump all cheevos' addresses. */
#undef CHEEVOS_DUMP_ADDRS

/* Define this macro to remove HTTP timeouts. */
#undef CHEEVOS_NO_TIMEOUT

/* Define this macro to get extra-verbose log for cheevos. */
#undef CHEEVOS_VERBOSE

/* Define this macro to load a JSON file from disk instead of downloading
 * from retroachievements.org. */
#undef CHEEVOS_JSON_OVERRIDE

/* Define this macro to have the password and token logged. THIS WILL DISCLOSE
 * THE USER'S PASSWORD, TAKE CARE! */
#undef CHEEVOS_LOG_PASSWORD

/* C89 wants only int values in enums. */
#define CHEEVOS_JSON_KEY_GAMEID       0xb4960eecU
#define CHEEVOS_JSON_KEY_ACHIEVEMENTS 0x69749ae1U
#define CHEEVOS_JSON_KEY_ID           0x005973f2U
#define CHEEVOS_JSON_KEY_MEMADDR      0x1e76b53fU
#define CHEEVOS_JSON_KEY_TITLE        0x0e2a9a07U
#define CHEEVOS_JSON_KEY_DESCRIPTION  0xe61a1f69U
#define CHEEVOS_JSON_KEY_POINTS       0xca8fce22U
#define CHEEVOS_JSON_KEY_AUTHOR       0xa804edb8U
#define CHEEVOS_JSON_KEY_MODIFIED     0xdcea4fe6U
#define CHEEVOS_JSON_KEY_CREATED      0x3a84721dU
#define CHEEVOS_JSON_KEY_BADGENAME    0x887685d9U
#define CHEEVOS_JSON_KEY_CONSOLE_ID   0x071656e5U
#define CHEEVOS_JSON_KEY_TOKEN        0x0e2dbd26U
#define CHEEVOS_JSON_KEY_FLAGS        0x0d2e96b2U

enum
{
   /* Don't change those, the values match the console IDs
    * at retroachievements.org. */
   CHEEVOS_CONSOLE_MEGA_DRIVE       = 1,
   CHEEVOS_CONSOLE_NINTENDO_64      = 2,
   CHEEVOS_CONSOLE_SUPER_NINTENDO   = 3,
   CHEEVOS_CONSOLE_GAMEBOY          = 4,
   CHEEVOS_CONSOLE_GAMEBOY_ADVANCE  = 5,
   CHEEVOS_CONSOLE_GAMEBOY_COLOR    = 6,
   CHEEVOS_CONSOLE_NINTENDO         = 7,
   CHEEVOS_CONSOLE_PC_ENGINE        = 8,
   CHEEVOS_CONSOLE_SEGA_CD          = 9,
   CHEEVOS_CONSOLE_SEGA_32X         = 10,
   CHEEVOS_CONSOLE_MASTER_SYSTEM    = 11
};

enum
{
   CHEEVOS_VAR_SIZE_BIT_0 = 0,
   CHEEVOS_VAR_SIZE_BIT_1,
   CHEEVOS_VAR_SIZE_BIT_2,
   CHEEVOS_VAR_SIZE_BIT_3,
   CHEEVOS_VAR_SIZE_BIT_4,
   CHEEVOS_VAR_SIZE_BIT_5,
   CHEEVOS_VAR_SIZE_BIT_6,
   CHEEVOS_VAR_SIZE_BIT_7,
   CHEEVOS_VAR_SIZE_NIBBLE_LOWER,
   CHEEVOS_VAR_SIZE_NIBBLE_UPPER,
   /* Byte, */
   CHEEVOS_VAR_SIZE_EIGHT_BITS, /* =Byte, */
   CHEEVOS_VAR_SIZE_SIXTEEN_BITS,
   CHEEVOS_VAR_SIZE_THIRTYTWO_BITS,

   CHEEVOS_VAR_SIZE_LAST
}; /* cheevos_var_t.size */

enum
{
   /* compare to the value of a live address in RAM */
   CHEEVOS_VAR_TYPE_ADDRESS = 0,

   /* a number. assume 32 bit */
   CHEEVOS_VAR_TYPE_VALUE_COMP,

   /* the value last known at this address. */
   CHEEVOS_VAR_TYPE_DELTA_MEM,

   /* a custom user-set variable */
   CHEEVOS_VAR_TYPE_DYNAMIC_VAR,

   CHEEVOS_VAR_TYPE_LAST
}; /* cheevos_var_t.type */

enum
{
   CHEEVOS_COND_OP_EQUALS = 0,
   CHEEVOS_COND_OP_LESS_THAN,
   CHEEVOS_COND_OP_LESS_THAN_OR_EQUAL,
   CHEEVOS_COND_OP_GREATER_THAN,
   CHEEVOS_COND_OP_GREATER_THAN_OR_EQUAL,
   CHEEVOS_COND_OP_NOT_EQUAL_TO,

   CHEEVOS_COND_OP_LAST
}; /* cheevos_cond_t.op */

enum
{
   CHEEVOS_COND_TYPE_STANDARD = 0,
   CHEEVOS_COND_TYPE_PAUSE_IF,
   CHEEVOS_COND_TYPE_RESET_IF,

   CHEEVOS_COND_TYPE_LAST
}; /* cheevos_cond_t.type */

enum
{
   CHEEVOS_DIRTY_TITLE       = 1 << 0,
   CHEEVOS_DIRTY_DESC        = 1 << 1,
   CHEEVOS_DIRTY_POINTS      = 1 << 2,
   CHEEVOS_DIRTY_AUTHOR      = 1 << 3,
   CHEEVOS_DIRTY_ID          = 1 << 4,
   CHEEVOS_DIRTY_BADGE       = 1 << 5,
   CHEEVOS_DIRTY_CONDITIONS  = 1 << 6,
   CHEEVOS_DIRTY_VOTES       = 1 << 7,
   CHEEVOS_DIRTY_DESCRIPTION = 1 << 8,

   CHEEVOS_DIRTY_ALL         = (1 << 9) - 1
};

enum
{
   CHEEVOS_ACTIVE_SOFTCORE = 1 << 0,
   CHEEVOS_ACTIVE_HARDCORE = 1 << 1
};

typedef struct
{
   unsigned type;
   unsigned req_hits;
   unsigned curr_hits;

   cheevos_var_t source;
   unsigned      op;
   cheevos_var_t target;
} cheevos_cond_t;

typedef struct
{
   cheevos_cond_t *conds;
   unsigned        count;
} cheevos_condset_t;

typedef struct
{
   cheevos_condset_t *condsets;
   unsigned count;
} cheevos_condition_t;

typedef struct
{
   unsigned    id;
   const char *title;
   const char *description;
   const char *author;
   const char *badge;
   unsigned    points;
   unsigned    dirty;
   int         active;
   int         last;
   int         modified;

   cheevos_condition_t condition;
} cheevo_t;

typedef struct
{
   cheevo_t *cheevos;
   unsigned  count;
} cheevoset_t;

typedef struct
{
   int is_element;
   int mode;
} cheevos_deactivate_t;

typedef struct
{
   unsigned    key_hash;
   int         is_key;
   const char *value;
   size_t      length;
} cheevos_getvalueud_t;

typedef struct
{
   int      in_cheevos;
   uint32_t field_hash;
   unsigned core_count;
   unsigned unofficial_count;
} cheevos_countud_t;

typedef struct
{
   const char *string;
   size_t      length;
} cheevos_field_t;

typedef struct
{
   int      in_cheevos;
   int      is_console_id;
   unsigned core_count;
   unsigned unofficial_count;

   cheevos_field_t *field;
   cheevos_field_t  id, memaddr, title, desc, points, author;
   cheevos_field_t  modified, created, badge, flags;
} cheevos_readud_t;

typedef struct
{
   unsigned (*finder)(const struct retro_game_info *, retro_time_t);
   const char *name;
   const uint32_t *ext_hashes;
} cheevos_finder_t;

typedef struct
{
   int  console_id;
   bool core_supports;

   cheevoset_t core;
   cheevoset_t unofficial;

   char token[32];

   retro_ctx_memory_info_t meminfo[4];
} cheevos_locals_t;


static cheevos_locals_t cheevos_locals =
{
   0,
   true,
   {NULL, 0},
   {NULL, 0},
   {0}
};

bool cheevos_loaded     = false;
int cheats_are_enabled  = 0;
int cheats_were_enabled = 0;

/*****************************************************************************
Supporting functions.
*****************************************************************************/

#ifdef CHEEVOS_LOG_URLS
static void cheevos_log_url(const char* format, const char* url)
{
#ifdef CHEEVOS_LOG_PASSWORD
   RARCH_LOG(format, url);
#else
   char copy[256];
   char* aux;
   char* next;
   
   strlcpy(copy, url, sizeof(copy));
   aux = strstr(copy, "?p=");
   
   if (aux == NULL)
      aux = strstr(copy, "&p=");
   
   if (aux != NULL)
   {
      aux += 3;
      next = strchr(aux, '&');
      
      if (next != NULL)
      {
         do
         {
            *aux++ = *next++;
         }
         while (next[-1] != 0);
      }
      else
         *aux = 0;
   }
   
   aux = strstr(copy, "?t=");
   
   if (aux == NULL)
      aux = strstr(copy, "&t=");
   
   if (aux != NULL)
   {
      aux += 3;
      next = strchr(aux, '&');
      
      if (next != NULL)
      {
         do
         {
            *aux++ = *next++;
         }
         while (next[-1] != 0);
      }
      else
         *aux = 0;
   }
   
   RARCH_LOG(format, copy);
#endif
}
#endif

#ifdef CHEEVOS_VERBOSE
static void cheevos_add_char(char** aux, size_t* left, char k)
{
   if (*left >= 1)
   {
      **aux = k;
      (*aux)++;
      (*left)--;
   }
}

static void cheevos_add_string(char** aux, size_t* left, const char* s)
{
   size_t len = strlen(s);

   if (*left >= len)
   {
      strcpy(*aux, s);
      *aux += len;
      *left -= len;
   }
}

static void cheevos_add_hex(char** aux, size_t* left, unsigned v)
{
   char buffer[32];

   snprintf(buffer, sizeof(buffer), "%06x", v);
   buffer[sizeof(buffer) - 1] = 0;

   cheevos_add_string(aux, left, buffer);
}

static void cheevos_add_uint(char** aux, size_t* left, unsigned v)
{
   char buffer[32];

   snprintf(buffer, sizeof(buffer), "%u", v);
   buffer[sizeof(buffer) - 1] = 0;

   cheevos_add_string(aux, left, buffer);
}

static void cheevos_log_var(const cheevos_var_t* var)
{
   RARCH_LOG("CHEEVOS         size: %s\n",
      var->size == CHEEVOS_VAR_SIZE_BIT_0 ? "bit 0" :
      var->size == CHEEVOS_VAR_SIZE_BIT_1 ? "bit 1" :
      var->size == CHEEVOS_VAR_SIZE_BIT_2 ? "bit 2" :
      var->size == CHEEVOS_VAR_SIZE_BIT_3 ? "bit 3" :
      var->size == CHEEVOS_VAR_SIZE_BIT_4 ? "bit 4" :
      var->size == CHEEVOS_VAR_SIZE_BIT_5 ? "bit 5" :
      var->size == CHEEVOS_VAR_SIZE_BIT_6 ? "bit 6" :
      var->size == CHEEVOS_VAR_SIZE_BIT_7 ? "bit 7" :
      var->size == CHEEVOS_VAR_SIZE_NIBBLE_LOWER ? "low nibble" :
      var->size == CHEEVOS_VAR_SIZE_NIBBLE_UPPER ? "high nibble" :
      var->size == CHEEVOS_VAR_SIZE_EIGHT_BITS ? "byte" :
      var->size == CHEEVOS_VAR_SIZE_SIXTEEN_BITS ? "word" :
      var->size == CHEEVOS_VAR_SIZE_THIRTYTWO_BITS ? "dword" :
      "?"
   );
   RARCH_LOG("CHEEVOS         type: %s\n",
      var->type == CHEEVOS_VAR_TYPE_ADDRESS ? "address" :
      var->type == CHEEVOS_VAR_TYPE_VALUE_COMP ? "value" :
      var->type == CHEEVOS_VAR_TYPE_DELTA_MEM ? "delta" :
      var->type == CHEEVOS_VAR_TYPE_DYNAMIC_VAR ? "dynamic" :
      "?"
   );
   RARCH_LOG("CHEEVOS         value: %u\n", var->value);
}

static void cheevos_log_cond(const cheevos_cond_t* cond)
{
   RARCH_LOG("CHEEVOS     condition %p\n", cond);
   RARCH_LOG("CHEEVOS       type:     %s\n",
      cond->type == CHEEVOS_COND_TYPE_STANDARD ? "standard" :
      cond->type == CHEEVOS_COND_TYPE_PAUSE_IF ? "pause" :
      cond->type == CHEEVOS_COND_TYPE_RESET_IF ? "reset" :
      "?"
   );
   RARCH_LOG("CHEEVOS       req_hits: %u\n", cond->req_hits);
   RARCH_LOG("CHEEVOS       source:\n");
   cheevos_log_var(&cond->source);
   RARCH_LOG("CHEEVOS       op: %s\n",
      cond->op == CHEEVOS_COND_OP_EQUALS ? "==" :
      cond->op == CHEEVOS_COND_OP_LESS_THAN ? "<" :
      cond->op == CHEEVOS_COND_OP_LESS_THAN_OR_EQUAL ? "<=" :
      cond->op == CHEEVOS_COND_OP_GREATER_THAN ? ">" :
      cond->op == CHEEVOS_COND_OP_GREATER_THAN_OR_EQUAL ? ">=" :
      cond->op == CHEEVOS_COND_OP_NOT_EQUAL_TO ? "!=" :
      "?"
   );
   RARCH_LOG("CHEEVOS       target:\n");
   cheevos_log_var(&cond->target);
}

static void cheevos_log_cheevo(const cheevo_t* cheevo,
      const cheevos_field_t* memaddr_ud)
{
   RARCH_LOG("CHEEVOS cheevo %p\n", cheevo);
   RARCH_LOG("CHEEVOS   id:      %u\n", cheevo->id);
   RARCH_LOG("CHEEVOS   title:   %s\n", cheevo->title);
   RARCH_LOG("CHEEVOS   desc:    %s\n", cheevo->description);
   RARCH_LOG("CHEEVOS   author:  %s\n", cheevo->author);
   RARCH_LOG("CHEEVOS   badge:   %s\n", cheevo->badge);
   RARCH_LOG("CHEEVOS   points:  %u\n", cheevo->points);
   RARCH_LOG("CHEEVOS   sets:    TBD\n");
   RARCH_LOG("CHEEVOS   memaddr: %.*s\n", (int)memaddr_ud->length, memaddr_ud->string);
}

static void cheevos_add_var_size(char** aux, size_t* left,
      const cheevos_var_t* var)
{
   switch( var->size )
   {
      case CHEEVOS_VAR_SIZE_BIT_0:
         cheevos_add_char(aux, left, 'M');
         break;
      case CHEEVOS_VAR_SIZE_BIT_1:
         cheevos_add_char(aux, left, 'N');
         break;
      case CHEEVOS_VAR_SIZE_BIT_2:
         cheevos_add_char(aux, left, 'O');
         break;
      case CHEEVOS_VAR_SIZE_BIT_3:
         cheevos_add_char(aux, left, 'P');
         break;
      case CHEEVOS_VAR_SIZE_BIT_4:
         cheevos_add_char(aux, left, 'Q');
         break;
      case CHEEVOS_VAR_SIZE_BIT_5:
         cheevos_add_char(aux, left, 'R');
         break;
      case CHEEVOS_VAR_SIZE_BIT_6:
         cheevos_add_char(aux, left, 'S');
         break;
      case CHEEVOS_VAR_SIZE_BIT_7:
         cheevos_add_char(aux, left, 'T');
         break;
      case CHEEVOS_VAR_SIZE_NIBBLE_LOWER:
         cheevos_add_char(aux, left, 'L');
         break;
      case CHEEVOS_VAR_SIZE_NIBBLE_UPPER:
         cheevos_add_char(aux, left, 'U');
         break;
      case CHEEVOS_VAR_SIZE_EIGHT_BITS:
         cheevos_add_char(aux, left, 'H');
         break;
      case CHEEVOS_VAR_SIZE_THIRTYTWO_BITS:
         cheevos_add_char(aux, left, 'X');
         break;
      case CHEEVOS_VAR_SIZE_SIXTEEN_BITS:
      default:
         cheevos_add_char(aux, left, ' ');
         break;
   }
}

static void cheevos_add_var(const cheevos_var_t* var, char** memaddr,
      size_t *left)
{
   if (   var->type == CHEEVOS_VAR_TYPE_ADDRESS
       || var->type == CHEEVOS_VAR_TYPE_DELTA_MEM)
   {
      if (var->type == CHEEVOS_VAR_TYPE_DELTA_MEM)
         cheevos_add_char(memaddr, left, 'd');

      cheevos_add_string(memaddr, left, "0x");
      cheevos_add_var_size(memaddr, left, var);
      cheevos_add_hex(memaddr, left, var->value);
   }
   else if (var->type == CHEEVOS_VAR_TYPE_VALUE_COMP)
   {
      cheevos_add_uint(memaddr, left, var->value);
   }
}

static void cheevos_build_memaddr(const cheevos_condition_t* condition,
      char* memaddr, size_t left)
{
   char *aux = memaddr;
   const cheevos_condset_t* condset;
   const cheevos_cond_t* cond;
   size_t i, j;

   left--; /* reserve one char for the null terminator */

   for (i = 0, condset = condition->condsets; i < condition->count; i++, condset++)
   {
      if (i != 0)
         cheevos_add_char(&aux, &left, 'S');

      for (j = 0, cond = condset->conds; j < condset->count; j++, cond++)
      {
         if (j != 0)
            cheevos_add_char(&aux, &left, '_');

         if (cond->type == CHEEVOS_COND_TYPE_RESET_IF)
            cheevos_add_string(&aux, &left, "R:");
         else if (cond->type == CHEEVOS_COND_TYPE_PAUSE_IF)
            cheevos_add_string(&aux, &left, "P:");

         cheevos_add_var(&cond->source, &aux, &left);

         switch (cond->op)
         {
            case CHEEVOS_COND_OP_EQUALS:
               cheevos_add_char(&aux, &left, '=');
               break;
      		case CHEEVOS_COND_OP_GREATER_THAN:
               cheevos_add_char(&aux, &left, '>');
               break;
      		case CHEEVOS_COND_OP_GREATER_THAN_OR_EQUAL:
               cheevos_add_string(&aux, &left, ">=");
               break;
      		case CHEEVOS_COND_OP_LESS_THAN:
               cheevos_add_char(&aux, &left, '<');
               break;
      		case CHEEVOS_COND_OP_LESS_THAN_OR_EQUAL:
               cheevos_add_string(&aux, &left, "<=");
               break;
      		case CHEEVOS_COND_OP_NOT_EQUAL_TO:
               cheevos_add_string(&aux, &left, "!=");
               break;
         }

         cheevos_add_var(&cond->target, &aux, &left);

         if (cond->req_hits > 0)
         {
            cheevos_add_char(&aux, &left, '.');
            cheevos_add_uint(&aux, &left, cond->req_hits);
            cheevos_add_char(&aux, &left, '.');
         }
      }
   }

   *aux = 0;
}

static void cheevos_post_log_cheevo(const cheevo_t* cheevo)
{
   char memaddr[256];
   cheevos_build_memaddr(&cheevo->condition, memaddr, sizeof(memaddr));
   RARCH_LOG("CHEEVOS   memaddr (computed): %s\n", memaddr);
}
#endif

static uint32_t cheevos_djb2(const char* str, size_t length)
{
   const unsigned char *aux = (const unsigned char*)str;
   const unsigned char *end = aux + length;
   uint32_t            hash = 5381;

   while (aux < end)
      hash = (hash << 5) + hash + *aux++;

   return hash;
}

static int cheevos_http_get(const char **result, size_t *size,
      const char *url, retro_time_t *timeout)
{
   const char *msg = NULL;

#ifdef CHEEVOS_NO_TIMEOUT
   int ret         = net_http_get(result, size, url, NULL);
#else
   int ret         = net_http_get(result, size, url, timeout);
#endif

   switch (ret)
   {
      case NET_HTTP_GET_OK:
         return ret;

      case NET_HTTP_GET_MALFORMED_URL:
         msg = "malformed url";
         break;

      case NET_HTTP_GET_CONNECT_ERROR:
         msg = "connect error";
         break;

      case NET_HTTP_GET_TIMEOUT:
         msg = "timeout";
         break;

      default:
         msg = "?";
         break;
   }

   RARCH_ERR("CHEEVOS error getting %s: %s.\n", url, msg);
   return ret;
}

static int cheevos_getvalue__json_key(void *userdata,
      const char *name, size_t length)
{
   cheevos_getvalueud_t* ud = (cheevos_getvalueud_t*)userdata;

   ud->is_key = cheevos_djb2(name, length) == ud->key_hash;
   return 0;
}

static int cheevos_getvalue__json_string(void *userdata,
      const char *string, size_t length)
{
   cheevos_getvalueud_t* ud = (cheevos_getvalueud_t*)userdata;

   if (ud->is_key)
   {
      ud->value = string;
      ud->length = length;
      ud->is_key = 0;
   }

   return 0;
}

static int cheevos_getvalue__json_boolean(void *userdata, int istrue)
{
   cheevos_getvalueud_t* ud = (cheevos_getvalueud_t*)userdata;

   if ( ud->is_key )
   {
      ud->value  = istrue ? "true" : "false";
      ud->length = istrue ? 4 : 5;
      ud->is_key = 0;
   }

   return 0;
}

static int cheevos_getvalue__json_null(void *userdata)
{
   cheevos_getvalueud_t* ud = (cheevos_getvalueud_t*)userdata;

   if ( ud->is_key )
   {
      ud->value = "null";
      ud->length = 4;
      ud->is_key = 0;
   }

   return 0;
}

static int cheevos_get_value(const char *json, unsigned key_hash,
      char *value, size_t length)
{
   static const jsonsax_handlers_t handlers =
   {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      cheevos_getvalue__json_key,
      NULL,
      cheevos_getvalue__json_string,
      cheevos_getvalue__json_string, /* number */
      cheevos_getvalue__json_boolean,
      cheevos_getvalue__json_null
   };

   cheevos_getvalueud_t ud;

   ud.key_hash = key_hash;
   ud.is_key   = 0;
   ud.value    = NULL;
   ud.length   = 0;
   *value      = 0;

   if ((jsonsax_parse(json, &handlers, (void*)&ud) == JSONSAX_OK)
         && ud.value && ud.length < length)
   {
      strlcpy(value, ud.value, ud.length + 1);
      return 0;
   }

   return -1;
}

/*****************************************************************************
Count number of achievements in a JSON file.
*****************************************************************************/

static int cheevos_count__json_end_array(void *userdata)
{
  cheevos_countud_t* ud = (cheevos_countud_t*)userdata;
  ud->in_cheevos = 0;
  return 0;
}

static int cheevos_count__json_key(void *userdata,
      const char *name, size_t length)
{
   cheevos_countud_t* ud = (cheevos_countud_t*)userdata;
   ud->field_hash        = cheevos_djb2(name, length);

   if (ud->field_hash == CHEEVOS_JSON_KEY_ACHIEVEMENTS)
      ud->in_cheevos = 1;

   return 0;
}

static int cheevos_count__json_number(void *userdata,
      const char *number, size_t length)
{
   cheevos_countud_t* ud = (cheevos_countud_t*)userdata;

   if (ud->in_cheevos && ud->field_hash == CHEEVOS_JSON_KEY_FLAGS)
   {
      long flags = strtol(number, NULL, 10);

      if (flags == 3)
         ud->core_count++; /* Core achievements */
      else if (flags == 5)
         ud->unofficial_count++; /* Unofficial achievements */
   }

   return 0;
}

static int cheevos_count_cheevos(const char *json,
      unsigned *core_count, unsigned *unofficial_count)
{
   static const jsonsax_handlers_t handlers =
   {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      cheevos_count__json_end_array,
      cheevos_count__json_key,
      NULL,
      NULL,
      cheevos_count__json_number,
      NULL,
      NULL
   };

   int res;
   cheevos_countud_t ud;
   ud.in_cheevos       = 0;
   ud.core_count       = 0;
   ud.unofficial_count = 0;

   res                 = jsonsax_parse(json, &handlers, (void*)&ud);

   *core_count         = ud.core_count;
   *unofficial_count   = ud.unofficial_count;

   return res;
}

/*****************************************************************************
Parse the MemAddr field.
*****************************************************************************/

static unsigned cheevos_prefix_to_comp_size(char prefix)
{
   /* Careful not to use ABCDEF here, this denotes part of an actual variable! */

   switch( toupper( prefix ) )
   {
      case 'M':
         return CHEEVOS_VAR_SIZE_BIT_0;
      case 'N':
         return CHEEVOS_VAR_SIZE_BIT_1;
      case 'O':
         return CHEEVOS_VAR_SIZE_BIT_2;
      case 'P':
         return CHEEVOS_VAR_SIZE_BIT_3;
      case 'Q':
         return CHEEVOS_VAR_SIZE_BIT_4;
      case 'R':
         return CHEEVOS_VAR_SIZE_BIT_5;
      case 'S':
         return CHEEVOS_VAR_SIZE_BIT_6;
      case 'T':
         return CHEEVOS_VAR_SIZE_BIT_7;
      case 'L':
         return CHEEVOS_VAR_SIZE_NIBBLE_LOWER;
      case 'U':
         return CHEEVOS_VAR_SIZE_NIBBLE_UPPER;
      case 'H':
         return CHEEVOS_VAR_SIZE_EIGHT_BITS;
      case 'X':
         return CHEEVOS_VAR_SIZE_THIRTYTWO_BITS;
      default:
      case ' ':
         break;
   }

   return CHEEVOS_VAR_SIZE_SIXTEEN_BITS;
}

static unsigned cheevos_read_hits(const char **memaddr)
{
   char *end         = NULL;
   const char *str   = *memaddr;
   unsigned num_hits = 0;

   if (*str == '(' || *str == '.')
   {
      num_hits = strtol(str + 1, &end, 10);
      str = end + 1;
   }

   *memaddr = str;
   return num_hits;
}

static unsigned cheevos_parse_operator(const char **memaddr)
{
   unsigned char op;
   const char *str = *memaddr;

   if (*str == '=' && str[1] == '=')
   {
      op = CHEEVOS_COND_OP_EQUALS;
      str += 2;
   }
   else if (*str == '=')
   {
      op = CHEEVOS_COND_OP_EQUALS;
      str++;
   }
   else if (*str == '!' && str[1] == '=')
   {
      op = CHEEVOS_COND_OP_NOT_EQUAL_TO;
      str += 2;
   }
   else if (*str == '<' && str[1] == '=')
   {
      op = CHEEVOS_COND_OP_LESS_THAN_OR_EQUAL;
      str += 2;
   }
   else if (*str == '<')
   {
      op = CHEEVOS_COND_OP_LESS_THAN;
      str++;
   }
   else if (*str == '>' && str[1] == '=')
   {
      op = CHEEVOS_COND_OP_GREATER_THAN_OR_EQUAL;
      str += 2;
   }
   else if (*str == '>')
   {
      op = CHEEVOS_COND_OP_GREATER_THAN;
      str++;
   }
   else
   {
      RARCH_ERR("CHEEVOS Unknown operator %c\n.", *str);
      op = CHEEVOS_COND_OP_EQUALS;
   }

   *memaddr = str;
   return op;
}

static size_t cheevos_reduce(size_t addr, size_t mask)
{
   while (mask)
   {
      size_t tmp = (mask - 1) & ~mask;
      addr = (addr & tmp) | ((addr >> 1) & ~tmp);
      mask = (mask & (mask - 1)) >> 1;
   }

   return addr;
}

static size_t cheevos_highest_bit(size_t n)
{
   n |= n >>  1;
   n |= n >>  2;
   n |= n >>  4;
   n |= n >>  8;
   n |= n >> 16;

   return n ^ (n >> 1);
}

void cheevos_parse_guest_addr(cheevos_var_t *var, unsigned value)
{
   rarch_system_info_t *system;
   runloop_ctl(RUNLOOP_CTL_SYSTEM_INFO_GET, &system);

   var->bank_id = -1;
   var->value   = value;

   if (   cheevos_locals.console_id == CHEEVOS_CONSOLE_NINTENDO
       && var->value < 0x2000)
      var->value &= 0x07ff;

   if (system->mmaps.num_descriptors != 0)
   {
      const rarch_memory_descriptor_t *desc = NULL;
      const rarch_memory_descriptor_t *end  = NULL;

      switch (cheevos_locals.console_id)
      {
         /* Patch the address to correctly map it to the mmaps */
         case CHEEVOS_CONSOLE_GAMEBOY_ADVANCE:
            if (var->value < 0x8000) /* Internal RAM */
               var->value += 0x3000000;
            else                     /* Work RAM */
               var->value += 0x2000000 - 0x8000;
            break;
         case CHEEVOS_CONSOLE_PC_ENGINE:
            var->value += 0x1f0000;
            break;
         case CHEEVOS_CONSOLE_SUPER_NINTENDO:
            if (var->value < 0x020000) /* Work RAM */
               var->value += 0x7e0000;
            else                       /* Save RAM */
            {
               var->value -= 0x020000;
               var->value += 0x006000;
            }
            break;
         default:
            break;
      }

      desc = system->mmaps.descriptors;
      end  = desc + system->mmaps.num_descriptors;

      for (; desc < end; desc++)
      {
         if (((desc->core.start ^ var->value) & desc->core.select) == 0)
         {
            var->bank_id = desc - system->mmaps.descriptors;
            var->value = cheevos_reduce(
               (var->value - desc->core.start) & desc->disconnect_mask,
               desc->core.disconnect);

   			if (var->value >= desc->core.len)
               var->value -= cheevos_highest_bit(var->value);

   			var->value += desc->core.offset;
            break;
         }
      }
   }
   else
   {
      unsigned i;

      for (i = 0; i < ARRAY_SIZE(cheevos_locals.meminfo); i++)
      {
         if (var->value < cheevos_locals.meminfo[i].size)
         {
            var->bank_id = i;
            break;
         }

         /* HACK subtract the correct amount of bytes to reach the save RAM */
         if (i == 0 && cheevos_locals.console_id == CHEEVOS_CONSOLE_NINTENDO)
            var->value -= 0x6000;
         else
            var->value -= cheevos_locals.meminfo[i].size;
      }
   }
}

static void cheevos_parse_var(cheevos_var_t *var, const char **memaddr)
{
   char *end       = NULL;
   const char *str = *memaddr;
   unsigned base   = 16;

   if (toupper(*str) == 'D' && str[1] == '0' && toupper(str[2]) == 'X')
   {
      /* d0x + 4 hex digits */
      str += 3;
      var->type = CHEEVOS_VAR_TYPE_DELTA_MEM;
   }
   else if (*str == '0' && toupper(str[1]) == 'X')
   {
      /* 0x + 4 hex digits */
      str += 2;
      var->type = CHEEVOS_VAR_TYPE_ADDRESS;
   }
   else
   {
      var->type = CHEEVOS_VAR_TYPE_VALUE_COMP;

      if (toupper(*str) == 'H')
         str++;
      else
         base = 10;
   }

   if (var->type != CHEEVOS_VAR_TYPE_VALUE_COMP)
   {
      var->size = cheevos_prefix_to_comp_size(*str);

      if (var->size != CHEEVOS_VAR_SIZE_SIXTEEN_BITS)
         str++;
   }

   var->value = strtol(str, &end, base);
   *memaddr   = end;

   switch (var->type)
   {
      case CHEEVOS_VAR_TYPE_ADDRESS:
      case CHEEVOS_VAR_TYPE_DELTA_MEM:
         cheevos_parse_guest_addr(var, var->value);
#ifdef CHEEVOS_DUMP_ADDRS
         RARCH_LOG("CHEEVOS var %03d:%08X\n", var->bank_id + 1, var->value);
#endif
         break;
      default:
         break;
   }
}

static void cheevos_parse_cond(cheevos_cond_t *cond, const char **memaddr)
{
   const char* str = *memaddr;

   if (*str == 'R' && str[1] == ':')
   {
      cond->type = CHEEVOS_COND_TYPE_RESET_IF;
      str += 2;
   }
   else if (*str == 'P' && str[1] == ':')
   {
      cond->type = CHEEVOS_COND_TYPE_PAUSE_IF;
      str += 2;
   }
   else
      cond->type = CHEEVOS_COND_TYPE_STANDARD;

   cheevos_parse_var(&cond->source, &str);
   cond->op = cheevos_parse_operator(&str);
   cheevos_parse_var(&cond->target, &str);
   cond->curr_hits = 0;
   cond->req_hits = cheevos_read_hits(&str);

   *memaddr = str;
}

static unsigned cheevos_count_cond_sets(const char *memaddr)
{
   cheevos_cond_t cond;
   unsigned count = 0;

   for (;;)
   {
      count++;

      for (;;)
      {
         cheevos_parse_cond(&cond, &memaddr);

         if (*memaddr != '_')
            break;

         memaddr++;
      }

      if (*memaddr != 'S')
         break;

      memaddr++;
   }

   return count;
}

static unsigned cheevos_count_conds_in_set(const char *memaddr, unsigned set)
{
   cheevos_cond_t cond;
   unsigned index = 0;
   unsigned count = 0;

   for (;;)
   {
      for (;;)
      {
         cheevos_parse_cond(&cond, &memaddr);

         if (index == set)
            count++;

         if (*memaddr != '_')
            break;

         memaddr++;
      }

      index++;

      if (*memaddr != 'S')
         break;

      memaddr++;
   }

   return count;
}

static void cheevos_parse_memaddr(cheevos_cond_t *cond, const char *memaddr, unsigned set)
{
   cheevos_cond_t dummy;
   unsigned index = 0;

   for (;;)
   {
      for (;;)
      {
         if (index == set)
         {
            cheevos_parse_cond(cond, &memaddr);
#ifdef CHEEVOS_VERBOSE
            cheevos_log_cond(cond);
#endif
            cond++;
         }
         else
            cheevos_parse_cond(&dummy, &memaddr);

         if (*memaddr != '_')
            break;

         memaddr++;
      }

      index++;

      if (*memaddr != 'S')
         break;

      memaddr++;
   }
}

static int cheevos_parse_condition(cheevos_condition_t *condition, const char* memaddr)
{
   condition->count = cheevos_count_cond_sets(memaddr);

   if (condition->count)
   {
      unsigned set                 = 0;
      cheevos_condset_t *condset   = NULL;
      cheevos_condset_t *conds     = (cheevos_condset_t*)
         calloc(condition->count, sizeof(cheevos_condset_t));
      const cheevos_condset_t* end;

      if (!conds)
         return -1;

      condition->condsets = conds;
      end                 = condition->condsets + condition->count;

      for (condset = condition->condsets; condset < end; condset++, set++)
      {
         condset->count =
            cheevos_count_conds_in_set(memaddr, set);
         condset->conds = NULL;

#ifdef CHEEVOS_VERBOSE
         RARCH_LOG("CHEEVOS   set %p (index=%u)\n", condset, set);
         RARCH_LOG("CHEEVOS     conds: %u\n", condset->count);
#endif

         if (condset->count)
         {
            cheevos_cond_t *conds = (cheevos_cond_t*)
               calloc(condset->count, sizeof(cheevos_cond_t));

            if (!conds)
               return -1;

            condset->conds = conds;
            cheevos_parse_memaddr(condset->conds, memaddr, set);
         }
      }
   }

   return 0;
}

/*****************************************************************************
Load achievements from a JSON string.
*****************************************************************************/

static INLINE const char *cheevos_dupstr(const cheevos_field_t *field)
{
   char *string = (char*)malloc(field->length + 1);

   if (!string)
      return NULL;

   memcpy ((void*)string, (void*)field->string, field->length);
   string[field->length] = 0;

   return string;
}

static int cheevos_new_cheevo(cheevos_readud_t *ud)
{
   cheevo_t *cheevo = NULL;

   if (strtol(ud->flags.string, NULL, 10) == 3)
      cheevo = cheevos_locals.core.cheevos + ud->core_count++;
   else
      cheevo = cheevos_locals.unofficial.cheevos + ud->unofficial_count++;

   cheevo->id          = strtol(ud->id.string, NULL, 10);
   cheevo->title       = cheevos_dupstr(&ud->title);
   cheevo->description = cheevos_dupstr(&ud->desc);
   cheevo->author      = cheevos_dupstr(&ud->author);
   cheevo->badge       = cheevos_dupstr(&ud->badge);
   cheevo->points      = strtol(ud->points.string, NULL, 10);
   cheevo->dirty       = 0;
   cheevo->active      = CHEEVOS_ACTIVE_SOFTCORE | CHEEVOS_ACTIVE_HARDCORE;
   cheevo->last        = 1;
   cheevo->modified    = 0;

   if (!cheevo->title || !cheevo->description || !cheevo->author || !cheevo->badge)
      goto error;

#ifdef CHEEVOS_VERBOSE
   cheevos_log_cheevo(cheevo, &ud->memaddr);
#endif

   if (cheevos_parse_condition(&cheevo->condition, ud->memaddr.string))
      goto error;

#ifdef CHEEVOS_VERBOSE
   cheevos_post_log_cheevo(cheevo);
#endif

   return 0;

error:
   free((void*)cheevo->title);
   free((void*)cheevo->description);
   free((void*)cheevo->author);
   free((void*)cheevo->badge);
   return -1;
}

static int cheevos_read__json_key( void *userdata,
      const char *name, size_t length)
{
   cheevos_readud_t *ud = (cheevos_readud_t*)userdata;
   uint32_t        hash = cheevos_djb2(name, length);

   ud->field = NULL;

   switch (hash)
   {
      case CHEEVOS_JSON_KEY_ACHIEVEMENTS:
         ud->in_cheevos = 1;
         break;
      case CHEEVOS_JSON_KEY_CONSOLE_ID:
         ud->is_console_id = 1;
         break;
      case CHEEVOS_JSON_KEY_ID:
         if (ud->in_cheevos)
            ud->field = &ud->id;
         break;
      case CHEEVOS_JSON_KEY_MEMADDR:
         if (ud->in_cheevos)
            ud->field = &ud->memaddr;
         break;
      case CHEEVOS_JSON_KEY_TITLE:
         if (ud->in_cheevos)
            ud->field = &ud->title;
         break;
      case CHEEVOS_JSON_KEY_DESCRIPTION:
         if (ud->in_cheevos)
            ud->field = &ud->desc;
         break;
      case CHEEVOS_JSON_KEY_POINTS:
         if (ud->in_cheevos)
            ud->field = &ud->points;
         break;
      case CHEEVOS_JSON_KEY_AUTHOR:
         if (ud->in_cheevos)
            ud->field = &ud->author;
         break;
      case CHEEVOS_JSON_KEY_MODIFIED:
         if (ud->in_cheevos)
            ud->field = &ud->modified;
         break;
      case CHEEVOS_JSON_KEY_CREATED:
         if (ud->in_cheevos)
            ud->field = &ud->created;
         break;
      case CHEEVOS_JSON_KEY_BADGENAME:
         if (ud->in_cheevos)
            ud->field = &ud->badge;
         break;
      case CHEEVOS_JSON_KEY_FLAGS:
         if (ud->in_cheevos)
            ud->field = &ud->flags;
         break;
      default:
         break;
   }

   return 0;
}

static int cheevos_read__json_string(void *userdata,
      const char *string, size_t length)
{
   cheevos_readud_t *ud = (cheevos_readud_t*)userdata;

   if (ud->field)
   {
      ud->field->string = string;
      ud->field->length = length;
   }

   return 0;
}

static int cheevos_read__json_number(void *userdata,
      const char *number, size_t length)
{
   cheevos_readud_t *ud = (cheevos_readud_t*)userdata;

   if (ud->field)
   {
      ud->field->string = number;
      ud->field->length = length;
   }
   else if (ud->is_console_id)
   {
      cheevos_locals.console_id = strtol(number, NULL, 10);
      ud->is_console_id = 0;
   }

   return 0;
}

static int cheevos_read__json_end_object(void *userdata)
{
   cheevos_readud_t *ud = (cheevos_readud_t*)userdata;

   if (ud->in_cheevos)
      return cheevos_new_cheevo(ud);

   return 0;
}

static int cheevos_read__json_end_array(void *userdata)
{
   cheevos_readud_t *ud = (cheevos_readud_t*)userdata;
   ud->in_cheevos = 0;
   return 0;
}

static int cheevos_parse(const char *json)
{
   static const jsonsax_handlers_t handlers =
   {
      NULL,
      NULL,
      NULL,
      cheevos_read__json_end_object,
      NULL,
      cheevos_read__json_end_array,
      cheevos_read__json_key,
      NULL,
      cheevos_read__json_string,
      cheevos_read__json_number,
      NULL,
      NULL
   };

   unsigned core_count, unofficial_count;
   int res;
   cheevos_readud_t ud;
   settings_t *settings = config_get_ptr();

   /* Just return OK if cheevos are disabled. */
   if (!settings->cheevos.enable)
      return 0;

   /* Count the number of achievements in the JSON file. */
   res = cheevos_count_cheevos(json, &core_count, &unofficial_count);

   if (res != JSONSAX_OK)
      return -1;

   /* Allocate the achievements. */

   cheevos_locals.core.cheevos = (cheevo_t*)
      calloc(core_count, sizeof(cheevo_t));
   cheevos_locals.core.count = core_count;

   cheevos_locals.unofficial.cheevos = (cheevo_t*)
      calloc(unofficial_count, sizeof(cheevo_t));
   cheevos_locals.unofficial.count = unofficial_count;

   if (!cheevos_locals.core.cheevos || !cheevos_locals.unofficial.cheevos)
   {
      free((void*)cheevos_locals.core.cheevos);
      free((void*)cheevos_locals.unofficial.cheevos);
      cheevos_locals.core.count = cheevos_locals.unofficial.count = 0;

      return -1;
   }

   /* Load the achievements. */
   ud.in_cheevos       = 0;
   ud.is_console_id    = 0;
   ud.field            = NULL;
   ud.core_count       = 0;
   ud.unofficial_count = 0;

   if (jsonsax_parse(json, &handlers, (void*)&ud) != JSONSAX_OK)
      goto error;

   return 0;

error:
   cheevos_unload();

   return -1;
}

/*****************************************************************************
Test all the achievements (call once per frame).
*****************************************************************************/

uint8_t *cheevos_get_memory(const cheevos_var_t *var)
{
   uint8_t *memory;
   
   if (var->bank_id >= 0)
   {
      rarch_system_info_t *system;
      runloop_ctl(RUNLOOP_CTL_SYSTEM_INFO_GET, &system);

      if (system->mmaps.num_descriptors != 0)
         memory = (uint8_t *)system->mmaps.descriptors[var->bank_id].core.ptr;
      else
         memory = (uint8_t *)cheevos_locals.meminfo[var->bank_id].data;
      
      if (memory)
         memory += var->value;
   }
   else
      memory = NULL;
   
   return memory;
}

static unsigned cheevos_get_var_value(cheevos_var_t *var)
{
   if (var->type == CHEEVOS_VAR_TYPE_VALUE_COMP)
      return var->value;

   if (     var->type == CHEEVOS_VAR_TYPE_ADDRESS
         || var->type == CHEEVOS_VAR_TYPE_DELTA_MEM)
   {
      /* TODO Check with Scott if the bank id is needed */
      const uint8_t *memory = cheevos_get_memory(var);
      unsigned live_val     = 0;

      if (memory)
      {
         live_val = memory[0];

         switch (var->size)
         {
            case CHEEVOS_VAR_SIZE_BIT_0:
               live_val &= 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_1:
               live_val = (live_val >> 1) & 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_2:
               live_val = (live_val >> 2) & 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_3:
               live_val = (live_val >> 3) & 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_4:
               live_val = (live_val >> 4) & 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_5:
               live_val = (live_val >> 5) & 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_6:
               live_val = (live_val >> 6) & 1;
               break;
            case CHEEVOS_VAR_SIZE_BIT_7:
               live_val = (live_val >> 7) & 1;
               break;
            case CHEEVOS_VAR_SIZE_NIBBLE_LOWER:
               live_val &= 0x0f;
               break;
            case CHEEVOS_VAR_SIZE_NIBBLE_UPPER:
               live_val = (live_val >> 4) & 0x0f;
               break;
            case CHEEVOS_VAR_SIZE_EIGHT_BITS:
               break;
            case CHEEVOS_VAR_SIZE_SIXTEEN_BITS:
               live_val |= memory[1] << 8;
               break;
            case CHEEVOS_VAR_SIZE_THIRTYTWO_BITS:
               live_val |= memory[1] << 8;
               live_val |= memory[2] << 16;
               live_val |= memory[3] << 24;
               break;
         }
      }

      if (var->type == CHEEVOS_VAR_TYPE_DELTA_MEM)
      {
         unsigned previous = var->previous;
         var->previous     = live_val;
         return previous;
      }

      return live_val;
   }

   /* We shouldn't get here... */
   return 0;
}

static int cheevos_test_condition(cheevos_cond_t *cond)
{
   unsigned sval = cheevos_get_var_value(&cond->source);
   unsigned tval = cheevos_get_var_value(&cond->target);

   switch (cond->op)
   {
      case CHEEVOS_COND_OP_EQUALS:
         return sval == tval;
      case CHEEVOS_COND_OP_LESS_THAN:
         return sval < tval;
      case CHEEVOS_COND_OP_LESS_THAN_OR_EQUAL:
         return sval <= tval;
      case CHEEVOS_COND_OP_GREATER_THAN:
         return sval > tval;
      case CHEEVOS_COND_OP_GREATER_THAN_OR_EQUAL:
         return sval >= tval;
      case CHEEVOS_COND_OP_NOT_EQUAL_TO:
         return sval != tval;
      default:
         break;
   }

   return 0;
}

static int cheevos_test_cond_set(const cheevos_condset_t *condset,
      int *dirty_conds, int *reset_conds, int match_any)
{
   int cond_valid            = 0;
   int set_valid             = 1;
   const cheevos_cond_t *end = condset->conds + condset->count;
   cheevos_cond_t *cond      = NULL;

   /* Now, read all Pause conditions, and if any are true,
    * do not process further (retain old state). */

   for (cond = condset->conds; cond < end; cond++)
   {
      if (cond->type != CHEEVOS_COND_TYPE_PAUSE_IF)
         continue;

      /* Reset by default, set to 1 if hit! */
      cond->curr_hits = 0;

      if (cheevos_test_condition(cond))
      {
         cond->curr_hits = 1;
         *dirty_conds = 1;

         /* Early out: this achievement is paused,
          * do not process any further! */
         return 0;
      }
   }

   /* Read all standard conditions, and process as normal: */
   for (cond = condset->conds; cond < end; cond++)
   {
      if (cond->type != CHEEVOS_COND_TYPE_STANDARD)
         continue;

      if (cond->req_hits != 0 && cond->curr_hits >= cond->req_hits)
         continue;

      cond_valid = cheevos_test_condition(cond);

      if (cond_valid)
      {
         cond->curr_hits++;
         *dirty_conds = 1;

         /* Process this logic, if this condition is true: */
         if (cond->req_hits == 0)
            ; /* Not a hit-based requirement: ignore any additional logic! */
         else if (cond->curr_hits < cond->req_hits)
            cond_valid = 0; /* Not entirely valid yet! */

         if (match_any)
            break;
      }

      /* Sequential or non-sequential? */
      set_valid &= cond_valid;
   }

   /* Now, ONLY read reset conditions! */
   for (cond = condset->conds; cond < end; cond++)
   {
      if (cond->type != CHEEVOS_COND_TYPE_RESET_IF)
         continue;

      cond_valid = cheevos_test_condition(cond);

      if (cond_valid)
      {
         *reset_conds = 1; /* Resets all hits found so far */
         set_valid = 0;    /* Cannot be valid if we've hit a reset condition. */
         break;            /* No point processing any further reset conditions. */
      }
   }

   return set_valid;
}

static int cheevos_reset_cond_set(cheevos_condset_t *condset, int deltas)
{
   int dirty                 = 0;
   const cheevos_cond_t *end = condset->conds + condset->count;
   cheevos_cond_t *cond      = NULL;

   if (deltas)
   {
      for (cond = condset->conds; cond < end; cond++)
      {
         dirty |= cond->curr_hits != 0;
         cond->curr_hits = 0;

         cond->source.previous = cond->source.value;
         cond->target.previous = cond->target.value;
      }
   }
   else
   {
      for (cond = condset->conds; cond < end; cond++)
      {
         dirty |= cond->curr_hits != 0;
         cond->curr_hits = 0;
      }
   }

   return dirty;
}

static int cheevos_test_cheevo(cheevo_t *cheevo)
{
   int dirty;
   int dirty_conds              = 0;
   int reset_conds              = 0;
   int ret_val                  = 0;
   int ret_val_sub_cond         = cheevo->condition.count == 1;
   cheevos_condset_t *condset   = cheevo->condition.condsets;
   const cheevos_condset_t *end = condset + cheevo->condition.count;

   if (condset < end)
   {
      ret_val = cheevos_test_cond_set(condset, &dirty_conds, &reset_conds, 0);
      condset++;
   }

   while (condset < end)
   {
      ret_val_sub_cond |= cheevos_test_cond_set(condset, &dirty_conds, &reset_conds, 0);
      condset++;
   }

   if (dirty_conds)
      cheevo->dirty |= CHEEVOS_DIRTY_CONDITIONS;

   if (reset_conds)
   {
      dirty = 0;

      for (condset = cheevo->condition.condsets; condset < end; condset++)
         dirty |= cheevos_reset_cond_set(condset, 0);

      if (dirty)
         cheevo->dirty |= CHEEVOS_DIRTY_CONDITIONS;
   }

   return ret_val && ret_val_sub_cond;
}

static void cheevos_url_encode(const char *str, char *encoded, size_t len)
{
   while (*str)
   {
      if (     isalnum(*str) || *str == '-'
            || *str == '_' || *str == '.'
            || *str == '~')
      {
         if (len >= 2)
         {
            *encoded++ = *str++;
            len--;
         }
         else
            break;
      }
      else
      {
         if (len >= 4)
         {
            snprintf(encoded, len, "%%%02x", (uint8_t)*str);
            encoded += 3;
            str++;
            len -= 3;
         }
         else
            break;
      }
   }

   *encoded = 0;
}

static int cheevos_login(retro_time_t *timeout)
{
   int res;
   char urle_user[64];
   char urle_pwd[64];
   char request[256];
   const char *json             = NULL;
   const char *username         = NULL;
   const char *password         = NULL;
   settings_t *settings         = config_get_ptr();

   if (cheevos_locals.token[0])
      return 0;

   urle_user[0]                 = '\0';
   urle_pwd[0]                  = '\0';
   request[0]                   = '\0';

   username = settings->cheevos.username;
   password = settings->cheevos.password;

   if (!username || !*username || !password || !*password)
   {
      runloop_msg_queue_push("Missing Retro Achievements account information.", 0, 5 * 60, false);
      runloop_msg_queue_push("Please fill in your account information in Settings.", 0, 5 * 60, false);
      RARCH_ERR("CHEEVOS username and/or password not informed.\n");
      return -1;
   }

   cheevos_url_encode(username, urle_user, sizeof(urle_user));
   cheevos_url_encode(password, urle_pwd, sizeof(urle_pwd));

   snprintf(
      request, sizeof(request),
      "http://retroachievements.org/dorequest.php?r=login&u=%s&p=%s",
      urle_user, urle_pwd
   );

   request[sizeof(request) - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
   cheevos_log_url("CHEEVOS url to login: %s\n", request);
#endif

   if (!cheevos_http_get(&json, NULL, request, timeout))
   {
      res = cheevos_get_value(json, CHEEVOS_JSON_KEY_TOKEN,
            cheevos_locals.token, sizeof(cheevos_locals.token));

      free((void*)json);

      if (!res)
         return 0;
   }

   runloop_msg_queue_push("Retro Achievements login error.",
         0, 5 * 60, false);
   RARCH_ERR("CHEEVOS error getting user token.\n");
   return -1;
}

static void cheevos_make_unlock_url(const cheevo_t *cheevo, char* url, size_t url_size)
{
   settings_t *settings = config_get_ptr();

   snprintf(
      url, url_size,
      "http://retroachievements.org/dorequest.php?r=awardachievement&u=%s&t=%s&a=%u&h=%d",
      settings->cheevos.username, cheevos_locals.token, cheevo->id,
      settings->cheevos.hardcore_mode_enable ? 1 : 0
   );

   url[url_size - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
   cheevos_log_url("CHEEVOS url to award the cheevo: %s\n", url);
#endif
}

static void cheevos_unlocked(void *task_data, void *user_data, const char *error)
{
   cheevo_t *cheevo = (cheevo_t *)user_data;

   if (error == NULL)
   {
      RARCH_LOG("CHEEVOS awarded achievement %u.\n", cheevo->id);
   }
   else
   {
      char url[256];
      url[0] = '\0';

      RARCH_ERR("CHEEVOS error awarding achievement %u, retrying...\n", cheevo->id);

      cheevos_make_unlock_url(cheevo, url, sizeof(url));
      task_push_http_transfer(url, true, NULL, cheevos_unlocked, cheevo);
   }
}

static void cheevos_test_cheevo_set(const cheevoset_t *set)
{
   settings_t *settings = config_get_ptr();
   cheevo_t *cheevo    = NULL;
   const cheevo_t *end = set->cheevos + set->count;
   int mode, valid;

   if (settings->cheevos.hardcore_mode_enable)
      mode = CHEEVOS_ACTIVE_HARDCORE;
   else
      mode = CHEEVOS_ACTIVE_SOFTCORE;

   for (cheevo = set->cheevos; cheevo < end; cheevo++)
   {
      if (cheevo->active & mode)
      {
         valid = cheevos_test_cheevo(cheevo);

         if (valid && !cheevo->last)
         {
            char url[256];
            url[0] = '\0';

            cheevo->active &= ~mode;

            if (mode == CHEEVOS_ACTIVE_HARDCORE)
               cheevo->active &= ~CHEEVOS_ACTIVE_SOFTCORE;

            RARCH_LOG("CHEEVOS awarding cheevo %u: %s (%s).\n",
                  cheevo->id, cheevo->title, cheevo->description);

            runloop_msg_queue_push(cheevo->title, 0, 3 * 60, false);
            runloop_msg_queue_push(cheevo->description, 0, 5 * 60, false);

            cheevos_make_unlock_url(cheevo, url, sizeof(url));
            task_push_http_transfer(url, true, NULL, cheevos_unlocked, cheevo);
         }

         cheevo->last = valid;
      }
   }
}

/*****************************************************************************
Free the loaded achievements.
*****************************************************************************/

static void cheevos_free_condset(const cheevos_condset_t *set)
{
   free((void*)set->conds);
}

static void cheevos_free_cheevo(const cheevo_t *cheevo)
{
   free((void*)cheevo->title);
   free((void*)cheevo->description);
   free((void*)cheevo->author);
   free((void*)cheevo->badge);
   cheevos_free_condset(cheevo->condition.condsets);
}

static void cheevos_free_cheevo_set(const cheevoset_t *set)
{
   const cheevo_t *cheevo = set->cheevos;
   const cheevo_t *end = cheevo + set->count;

   while (cheevo < end)
      cheevos_free_cheevo(cheevo++);

   free((void*)set->cheevos);
}

/*****************************************************************************
Load achievements from retroachievements.org.
*****************************************************************************/

static int cheevos_get_by_game_id(const char **json,
      unsigned game_id, retro_time_t *timeout)
{
   settings_t *settings = config_get_ptr();

   /* Just return OK if cheevos are disabled. */
   if (!settings->cheevos.enable)
      return 0;

   if (!cheevos_login(timeout))
   {
      char request[256];

      request[0] = '\0';

      snprintf(
         request, sizeof(request),
         "http://retroachievements.org/dorequest.php?r=patch&u=%s&g=%u&f=3&l=1&t=%s",
         settings->cheevos.username, game_id, cheevos_locals.token
      );

      request[sizeof(request) - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
      cheevos_log_url("CHEEVOS url to get the list of cheevos: %s\n", request);
#endif

      if (!cheevos_http_get(json, NULL, request, timeout))
      {
         RARCH_LOG("CHEEVOS got achievements for game id %u.\n", game_id);
         return 0;
      }

      RARCH_ERR("CHEEVOS error getting achievements for game id %u.\n", game_id);
   }

   return -1;
}

static unsigned cheevos_get_game_id(unsigned char *hash, retro_time_t *timeout)
{
   int res;
   char request[256];
   char game_id[16];
   const char* json  = NULL;

   request[0] = '\0';
   game_id[0] = '\0';

   RARCH_LOG(
      "CHEEVOS getting game id for hash %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
      hash[ 0], hash[ 1], hash[ 2], hash[ 3],
      hash[ 4], hash[ 5], hash[ 6], hash[ 7],
      hash[ 8], hash[ 9], hash[10], hash[11],
      hash[12], hash[13], hash[14], hash[15]
   );

   snprintf(
      request, sizeof(request),
      "http://retroachievements.org/dorequest.php?r=gameid&m=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
      hash[ 0], hash[ 1], hash[ 2], hash[ 3],
      hash[ 4], hash[ 5], hash[ 6], hash[ 7],
      hash[ 8], hash[ 9], hash[10], hash[11],
      hash[12], hash[13], hash[14], hash[15]
   );

   request[sizeof(request) - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
   cheevos_log_url("CHEEVOS url to get the game's id: %s\n", request);
#endif

   if (!cheevos_http_get(&json, NULL, request, timeout))
   {
      res = cheevos_get_value(json, CHEEVOS_JSON_KEY_GAMEID,
            game_id, sizeof(game_id));

      free((void*)json);

      if (!res)
      {
         RARCH_LOG("CHEEVOS got game id %s.\n", game_id);
         return strtoul(game_id, NULL, 10);
      }
   }

   RARCH_ERR("CHEEVOS error getting game_id.\n");
   return 0;
}

static void cheevos_make_playing_url(unsigned game_id, char* url, size_t url_size)
{
   settings_t *settings = config_get_ptr();

   snprintf(
      url, url_size,
      "http://retroachievements.org/dorequest.php?r=postactivity&u=%s&t=%s&a=3&m=%u",
      settings->cheevos.username, cheevos_locals.token, game_id
   );

   url[url_size - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
   cheevos_log_url("CHEEVOS url to post the 'playing' activity: %s\n", url);
#endif
}

static void cheevos_playing(void *task_data, void *user_data, const char *error)
{
   unsigned game_id = (unsigned)(uintptr_t)user_data;

   if (error == NULL)
   {
      RARCH_LOG("CHEEVOS posted playing game %u activity.\n", game_id);
   }
   else
   {
      char url[256];

      url[0] = '\0';

      RARCH_ERR("CHEEVOS error posting playing game %u activity, will retry...\n", game_id);

      cheevos_make_playing_url(game_id, url, sizeof(url));
      task_push_http_transfer(url, true, NULL, cheevos_playing, (void*)(uintptr_t)game_id);
   }
}

#ifndef CHEEVOS_DONT_DEACTIVATE
static int cheevos_deactivate__json_index(void *userdata, unsigned int index)
{
   cheevos_deactivate_t *ud = (cheevos_deactivate_t*)userdata;
   ud->is_element = 1;
   return 0;
}

static int cheevos_deactivate__json_number(void *userdata,
      const char *number, size_t length)
{
   long id;
   int found;
   cheevo_t* cheevo         = NULL;
   const cheevo_t* end      = NULL;
   cheevos_deactivate_t *ud = (cheevos_deactivate_t*)userdata;

   if (ud->is_element)
   {
      ud->is_element = 0;
      id             = strtol(number, NULL, 10);
      found          = 0;
      cheevo         = cheevos_locals.core.cheevos;
      end            = cheevo + cheevos_locals.core.count;

      for (; cheevo < end; cheevo++)
      {
         if (cheevo->id == (unsigned)id)
         {
            cheevo->active &= ~ud->mode;
            found = 1;
            break;
         }
      }

      if (!found)
      {
         cheevo = cheevos_locals.unofficial.cheevos;
         end    = cheevo + cheevos_locals.unofficial.count;

         for (; cheevo < end; cheevo++)
         {
            if (cheevo->id == (unsigned)id)
            {
               cheevo->active &= ~ud->mode;
               break;
            }
         }
      }

      if (found)
         RARCH_LOG("CHEEVOS deactivated unlocked cheevo %u (%s).\n", cheevo->id, cheevo->title);
      else
         RARCH_ERR("CHEEVOS unknown cheevo to deactivate: %u.\n", id);
   }

   return 0;
}
#endif

static int cheevos_deactivate_unlocks(unsigned game_id, retro_time_t *timeout)
{
   /* Only call this function after the cheevos have been loaded. */

#ifndef CHEEVOS_DONT_DEACTIVATE
   static const jsonsax_handlers_t handlers =
   {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      cheevos_deactivate__json_index,
      NULL,
      cheevos_deactivate__json_number,
      NULL,
      NULL
   };

   int res = 0;
   cheevos_deactivate_t ud;
   const char* json             = NULL;

   if (!cheevos_login(timeout))
   {
      char request[256];
      settings_t *settings = config_get_ptr();

      request[0] = '\0';

      /* Deactivate achievements in softcore mode. */
      snprintf(
         request, sizeof(request),
         "http://retroachievements.org/dorequest.php?r=unlocks&u=%s&t=%s&g=%u&h=0",
         settings->cheevos.username, cheevos_locals.token, game_id
      );

      request[sizeof(request) - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
      cheevos_log_url("CHEEVOS url to get the list of unlocked cheevos in softcore: %s\n", request);
#endif

      if (!cheevos_http_get(&json, NULL, request, timeout))
      {
         ud.is_element = 0;
         ud.mode = CHEEVOS_ACTIVE_SOFTCORE;
         res = jsonsax_parse(json, &handlers, (void*)&ud);
         free((void*)json);

         if (res == JSONSAX_OK)
            RARCH_LOG("CHEEVOS deactivated unlocked achievements in softcore mode.\n");
         else
         {
            RARCH_ERR("CHEEVOS error deactivating unlocked achievements in softcore mode.\n");
            res = -1;
         }
      }

      /* Deactivate achievements in hardcore mode. */
      snprintf(
         request, sizeof(request),
         "http://retroachievements.org/dorequest.php?r=unlocks&u=%s&t=%s&g=%u&h=1",
         settings->cheevos.username, cheevos_locals.token, game_id
      );

      request[sizeof(request) - 1] = 0;

#ifdef CHEEVOS_LOG_URLS
      cheevos_log_url("CHEEVOS url to get the list of unlocked cheevos in hardcore: %s\n", request);
#endif

      if (!cheevos_http_get(&json, NULL, request, timeout))
      {
         ud.is_element = 0;
         ud.mode = CHEEVOS_ACTIVE_HARDCORE;
         res = jsonsax_parse(json, &handlers, (void*)&ud);
         free((void*)json);

         if (res == JSONSAX_OK)
            RARCH_LOG("CHEEVOS deactivated unlocked achievements in hardcore mode.\n");
         else
         {
            RARCH_ERR("CHEEVOS error deactivating unlocked achievements in hardcore mode.\n");
            res = -1;
         }
      }
   }

   return res;
#else
   RARCH_LOG("CHEEVOS cheevo deactivation is disabled\n");
   return 0;
#endif
}

#define CHEEVOS_SIX_MB   (6 * 1024 * 1024)
#define CHEEVOS_EIGHT_MB (8 * 1024 * 1024)

static INLINE unsigned cheevos_next_power_of_2(unsigned n)
{
   n--;

   n |= n >> 1;
   n |= n >> 2;
   n |= n >> 4;
   n |= n >> 8;
   n |= n >> 16;

   return n + 1;
}

static size_t cheevos_eval_md5(
      const struct retro_game_info *info,
      size_t offset,
      size_t max_size,
      MD5_CTX *ctx)
{
   MD5_Init(ctx);

   if (info->data)
   {
      if (max_size == 0)
         max_size = info->size;

      if (info->size - offset < max_size)
         max_size = info->size - offset;

      MD5_Update(ctx, (void*)((uint8_t*)info->data + offset), max_size);
      return max_size;
   }
   else
   {
      size_t size = 0;
      RFILE *file = filestream_open(info->path, RFILE_MODE_READ, 0);

      if (!file)
         return 0;

      filestream_seek(file, offset, SEEK_SET);

      for (;;)
      {
         uint8_t buffer[4096];
         ssize_t num_read;
         size_t to_read = sizeof(buffer);

         if (max_size != 0 && to_read > max_size)
            to_read = max_size;

         num_read = filestream_read(file, (void*)buffer, to_read);

         if (num_read <= 0)
            break;

         MD5_Update(ctx, (void*)buffer, num_read);
         size += num_read;
         
         if (max_size != 0)
         {
            max_size -= num_read;
            
            if (max_size == 0)
               break;
         }
      }

      filestream_close(file);
      return size;
   }
}

static void cheevos_fill_md5(size_t size, char fill, MD5_CTX *ctx)
{
   char buffer[4096];

   memset((void*)buffer, fill, sizeof(buffer));

   while (size > 0)
   {
      size_t len = sizeof(buffer);

      if (len > size)
         len = size;

      MD5_Update(ctx, (void*)buffer, len);
      size -= len;
   }
}

static unsigned cheevos_find_game_id_generic(
      const struct retro_game_info *info,
      retro_time_t timeout)
{
   MD5_CTX ctx;
   retro_time_t to;
   uint8_t hash[16];
   size_t size      = cheevos_eval_md5(info, 0, 0, &ctx);

   hash[0] = '\0';

   MD5_Final(hash, &ctx);

   if (!size)
      return 0;

   to = timeout;
   return cheevos_get_game_id(hash, &to);
}

static unsigned cheevos_find_game_id_snes(
      const struct retro_game_info *info,
      retro_time_t timeout)
{
   MD5_CTX ctx;
   uint8_t hash[16];
   size_t count = cheevos_eval_md5(info, 0, 0, &ctx);

   if (count == 0)
   {
      MD5_Final(hash, &ctx);
      return 0;
   }

   if (count < CHEEVOS_EIGHT_MB)
      cheevos_fill_md5(CHEEVOS_EIGHT_MB - count, 0, &ctx);
   
   MD5_Final(hash, &ctx);
   return cheevos_get_game_id(hash, &timeout);
}

static unsigned cheevos_find_game_id_genesis(
      const struct retro_game_info *info, retro_time_t timeout)
{
   MD5_CTX ctx;
   uint8_t hash[16];
   size_t count = cheevos_eval_md5(info, 0, 0, &ctx);

   if (count == 0)
   {
      MD5_Final(hash, &ctx);
      return 0;
   }

   if (count < CHEEVOS_SIX_MB)
      cheevos_fill_md5(CHEEVOS_SIX_MB - count, 0, &ctx);
   
   MD5_Final(hash, &ctx);
   return cheevos_get_game_id(hash, &timeout);
}

static unsigned cheevos_find_game_id_nes(
      const struct retro_game_info *info,
      retro_time_t timeout)
{
   /* Note about the references to the FCEU emulator below. There is no
    * core-specific code in this function, it's rather Retro Achievements
    * specific code that must be followed to the letter so we compute
    * the correct ROM hash. Retro Achievements does indeed use some
    * FCEU related method to compute the hash, since its NES emulator
    * is based on it. */
   struct
   {
      uint8_t id[4]; /* NES^Z */
      uint8_t rom_size;
      uint8_t vrom_size;
      uint8_t rom_type;
      uint8_t rom_type2;
      uint8_t reserve[8];
   } header;

   size_t rom_size, offset, count;
   MD5_CTX ctx;
   uint8_t hash[16];

   size_t bytes;
   RFILE *file;
   ssize_t num_read;
   int mapper_no;
   bool round;

   if (info->data)
   {
      if (info->size < sizeof(header))
         return 0;

      memcpy((void*)&header, info->data, sizeof(header));
   }
   else
   {
      file = filestream_open(info->path, RFILE_MODE_READ, 0);

      if (!file)
         return 0;

      num_read = filestream_read(file, (void*)&header, sizeof(header));
      filestream_close(file);

      if (num_read < (ssize_t)sizeof(header))
         return 0;
   }

   if (     header.id[0] != 'N'
         || header.id[1] != 'E'
         || header.id[2] != 'S'
         || header.id[3] != 0x1a)
      return 0;

   if (header.rom_size)
      rom_size = cheevos_next_power_of_2(header.rom_size);
   else
      rom_size = 256;

   /* from FCEU core - compute size using the cart mapper */
   mapper_no = (header.rom_type >> 4) | (header.rom_type2 & 0xF0);

   /* for games not to the power of 2, so we just read enough
    * PRG rom from it, but we have to keep ROM_size to the power of 2
    * since PRGCartMapping wants ROM_size to be to the power of 2
    * so instead if not to power of 2, we just use head.ROM_size when
    * we use FCEU_read. */
   round = mapper_no != 53 && mapper_no != 198 && mapper_no != 228;
   bytes = (round) ? rom_size : header.rom_size;

   /* from FCEU core - check if Trainer included in ROM data */
   offset = sizeof(header) + (header.rom_type & 4 ? sizeof(header) : 0);

   MD5_Init(&ctx);
   count = cheevos_eval_md5(info, offset, 0x4000 * bytes, &ctx);
   count = 0x4000 * bytes - count;
   cheevos_fill_md5(count, (char)0xff, &ctx);
   MD5_Final(hash, &ctx);

   return cheevos_get_game_id(hash, &timeout);
}

bool cheevos_load(const void *data)
{
   static const uint32_t genesis_exts[] =
   {
      0x0b888feeU, /* mdx */
      0x005978b6U, /* md  */
      0x0b88aa89U, /* smd */
      0x0b88767fU, /* gen */
      0x0b8861beU, /* bin */
      0x0b886782U, /* cue */
      0x0b8880d0U, /* iso */
      0x0b88aa98U, /* sms */
      0x005977f3U, /* gg  */
      0x0059797fU, /* sg  */
      0
   };

   static const uint32_t snes_exts[] =
   {
      0x0b88aa88U, /* smc */
      0x0b8872bbU, /* fig */
      0x0b88a9a1U, /* sfc */
      0x0b887623U, /* gd3 */
      0x0b887627U, /* gd7 */
      0x0b886bf3U, /* dx2 */
      0x0b886312U, /* bsx */
      0x0b88abd2U, /* swc */
      0
   };

   static cheevos_finder_t finders[] =
   {
      {cheevos_find_game_id_snes,    "SNES (8Mb padding)",      snes_exts},
      {cheevos_find_game_id_genesis, "Genesis (6Mb padding)",   genesis_exts},
      {cheevos_find_game_id_nes,     "NES (discards VROM)",     NULL},
      {cheevos_find_game_id_generic, "Generic (plain content)", NULL},
   };

   struct retro_system_info sysinfo;
   unsigned i;
   char url[256];
   const char *json     = NULL;
   retro_time_t timeout = 5000000;
   unsigned game_id     = 0;
   settings_t *settings = config_get_ptr();
   const struct retro_game_info *info = (const struct retro_game_info*)data;

   url[0]                = '\0';

   cheevos_loaded        = 0;

   /* Just return OK if the core doesn't support cheevos, or info is NULL. */
   if (!cheevos_locals.core_supports || !info)
      return true;

   cheevos_locals.meminfo[0].id = RETRO_MEMORY_SYSTEM_RAM;
   core_get_memory(&cheevos_locals.meminfo[0]);

   cheevos_locals.meminfo[1].id = RETRO_MEMORY_SAVE_RAM;
   core_get_memory(&cheevos_locals.meminfo[1]);

   cheevos_locals.meminfo[2].id = RETRO_MEMORY_VIDEO_RAM;
   core_get_memory(&cheevos_locals.meminfo[2]);

   cheevos_locals.meminfo[3].id = RETRO_MEMORY_RTC;
   core_get_memory(&cheevos_locals.meminfo[3]);

   RARCH_LOG("CHEEVOS system RAM: %p %u\n",
      cheevos_locals.meminfo[0].data, cheevos_locals.meminfo[0].size);
   RARCH_LOG("CHEEVOS save RAM:   %p %u\n",
      cheevos_locals.meminfo[1].data, cheevos_locals.meminfo[1].size);
   RARCH_LOG("CHEEVOS video RAM:  %p %u\n",
      cheevos_locals.meminfo[2].data, cheevos_locals.meminfo[2].size);
   RARCH_LOG("CHEEVOS RTC:        %p %u\n",
      cheevos_locals.meminfo[3].data, cheevos_locals.meminfo[3].size);

   /* Bail out if cheevos are disabled.
    * But set the above anyways, command_read_ram needs it. */
   if (!settings->cheevos.enable)
      return true;

   /* Use the supported extensions as a hint
    * to what method we should use. */
   core_get_system_info(&sysinfo);

   for (i = 0; i < ARRAY_SIZE(finders); i++)
   {
      if (finders[i].ext_hashes)
      {
         const char *ext = sysinfo.valid_extensions;

         while (ext)
         {
            int j;
            unsigned hash;
            const char *end = strchr(ext, '|');

            if (end)
            {
               hash = cheevos_djb2(ext, end - ext);
               ext = end + 1;
            }
            else
            {
               hash = cheevos_djb2(ext, strlen(ext));
               ext = NULL;
            }

            for (j = 0; finders[i].ext_hashes[j]; j++)
            {
               if (finders[i].ext_hashes[j] == hash)
               {
                  RARCH_LOG("CHEEVOS testing %s.\n", finders[i].name);

                  game_id = finders[i].finder(info, 5000000);

                  if (game_id)
                     goto found;

                  ext = NULL; /* force next finder */
                  break;
               }
            }
         }
      }
   }

   for (i = 0; i < ARRAY_SIZE(finders); i++)
   {
      if (finders[i].ext_hashes)
         continue;

      RARCH_LOG("CHEEVOS testing %s.\n", finders[i].name);

      game_id = finders[i].finder(info, 5000000);

      if (game_id)
         goto found;
   }

   RARCH_LOG("CHEEVOS this game doesn't feature achievements.\n");
   return false;

found:
#ifdef CHEEVOS_JSON_OVERRIDE
   {
      FILE* file;
      size_t size;

      file = fopen(CHEEVOS_JSON_OVERRIDE, "rb");
      fseek(file, 0, SEEK_END);
      size = ftell(file);
      fseek(file, 0, SEEK_SET);

      json = (const char*)malloc(size);
      fread((void*)json, 1, size, file);

      fclose(file);
   }
#else
   if (cheevos_get_by_game_id(&json, game_id, &timeout) == 0 && json != NULL)
#endif
   {
      if (!cheevos_parse(json))
      {
         cheevos_deactivate_unlocks(game_id, &timeout);
         free((void*)json);
         cheevos_loaded = true;

         cheevos_make_playing_url(game_id, url, sizeof(url));
         task_push_http_transfer(url, true, NULL,
               cheevos_playing, (void*)(uintptr_t)game_id);
         return true;
      }

      free((void*)json);
   }

   runloop_msg_queue_push("Error loading achievements.", 0, 5 * 60, false);
   RARCH_ERR("CHEEVOS error loading achievements.\n");
   return false;
}

void cheevos_reset_game(void)
{
   cheevo_t *cheevo    = cheevos_locals.core.cheevos;
   const cheevo_t *end = cheevo + cheevos_locals.core.count;

   for (; cheevo < end; cheevo++)
      cheevo->last = 1;

   cheevo = cheevos_locals.unofficial.cheevos;
   end    = cheevo + cheevos_locals.unofficial.count;

   for (; cheevo < end; cheevo++)
      cheevo->last = 1;
}

void cheevos_populate_menu(void *data, bool hardcore)
{
#ifdef HAVE_MENU
   unsigned i;
   unsigned items_found          = 0;
   settings_t *settings          = config_get_ptr();
   menu_displaylist_info_t *info = (menu_displaylist_info_t*)data;
   cheevo_t *cheevo              = cheevos_locals.core.cheevos;
   const cheevo_t *end           = cheevos_locals.core.cheevos +
                                   cheevos_locals.core.count;

   for (i = 0; cheevo < end; i++, cheevo++)
   {
      if (!hardcore)
      {
         if (!(cheevo->active & CHEEVOS_ACTIVE_SOFTCORE))
         {
            menu_entries_append_enum(info->list, cheevo->title,
                  cheevo->description, MENU_ENUM_LABEL_CHEEVOS_UNLOCKED_ENTRY,
                  MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
            items_found++;
         }
         else
         {
            menu_entries_append_enum(info->list, cheevo->title,
                  cheevo->description, MENU_ENUM_LABEL_CHEEVOS_LOCKED_ENTRY,
                  MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
            items_found++;
         }
      }
      else
      {
         if (!(cheevo->active & CHEEVOS_ACTIVE_HARDCORE))
         {
            menu_entries_append_enum(info->list, cheevo->title,
                  cheevo->description, MENU_ENUM_LABEL_CHEEVOS_UNLOCKED_ENTRY,
                  MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
            items_found++;
         }
         else
         {
            menu_entries_append_enum(info->list, cheevo->title,
                  cheevo->description, MENU_ENUM_LABEL_CHEEVOS_LOCKED_ENTRY,
                  MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
            items_found++;
         }
      }
   }

   if (settings->cheevos.test_unofficial)
   {
      cheevo = cheevos_locals.unofficial.cheevos;
      end    = cheevos_locals.unofficial.cheevos
         + cheevos_locals.unofficial.count;

      for (i = cheevos_locals.core.count; cheevo < end; i++, cheevo++)
      {
         if (!hardcore)
         {
            if (!(cheevo->active & CHEEVOS_ACTIVE_SOFTCORE))
            {
               menu_entries_append_enum(info->list, cheevo->title,
                     cheevo->description, MENU_ENUM_LABEL_CHEEVOS_UNLOCKED_ENTRY,
                     MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
               items_found++;
            }
            else
            {
               menu_entries_append_enum(info->list, cheevo->title,
                     cheevo->description, MENU_ENUM_LABEL_CHEEVOS_LOCKED_ENTRY,
                     MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
               items_found++;
            }
         }
         else
         {
            if (!(cheevo->active & CHEEVOS_ACTIVE_HARDCORE))
            {
               menu_entries_append_enum(info->list, cheevo->title,
                     cheevo->description, MENU_ENUM_LABEL_CHEEVOS_UNLOCKED_ENTRY,
                     MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
               items_found++;
            }
            else
            {
               menu_entries_append_enum(info->list, cheevo->title,
                     cheevo->description, MENU_ENUM_LABEL_CHEEVOS_LOCKED_ENTRY,
                     MENU_SETTINGS_CHEEVOS_START + i, 0, 0);
               items_found++;
            }
         }
      }
   }

   if (items_found == 0)
   {
      menu_entries_append_enum(info->list,
            msg_hash_to_str(MENU_ENUM_LABEL_VALUE_NO_ACHIEVEMENTS_TO_DISPLAY),
            msg_hash_to_str(MENU_ENUM_LABEL_NO_ACHIEVEMENTS_TO_DISPLAY),
            MENU_ENUM_LABEL_NO_ACHIEVEMENTS_TO_DISPLAY,
            FILE_TYPE_NONE, 0, 0);
   }
#endif
}

bool cheevos_get_description(cheevos_ctx_desc_t *desc)
{
   cheevo_t *cheevos        = cheevos_locals.core.cheevos;

   if (desc->idx >= cheevos_locals.core.count)
   {
      cheevos       = cheevos_locals.unofficial.cheevos;
      desc->idx    -= cheevos_locals.unofficial.count;
   }

   strlcpy(desc->s, cheevos[desc->idx].description, desc->len);
   return true;
}

bool cheevos_apply_cheats(bool *data_bool)
{
   cheats_are_enabled   = *data_bool;
   cheats_were_enabled |= cheats_are_enabled;

   return true;
}

bool cheevos_unload(void)
{
   if (!cheevos_loaded)
      return false;

   cheevos_free_cheevo_set(&cheevos_locals.core);
   cheevos_free_cheevo_set(&cheevos_locals.unofficial);

   cheevos_loaded = false;

   return true;
}

bool cheevos_toggle_hardcore_mode(void)
{
   settings_t *settings = config_get_ptr();

   /* reset and deinit rewind to avoid cheat the score */
   if (settings->cheevos.hardcore_mode_enable)
   {
      /* send reset core cmd to avoid any user savestate previusly loaded */
      command_event(CMD_EVENT_RESET, NULL);
      if (settings->rewind_enable)
         command_event(CMD_EVENT_REWIND_DEINIT, NULL);

      RARCH_LOG("%s\n", msg_hash_to_str(MSG_CHEEVOS_HARDCORE_MODE_ENABLE));
      runloop_msg_queue_push(
            msg_hash_to_str(MSG_CHEEVOS_HARDCORE_MODE_ENABLE), 0, 3 * 60, true);
   }
   else
   {
      if (settings->rewind_enable)
         command_event(CMD_EVENT_REWIND_INIT, NULL);
   }

   return true;
}

void cheevos_test(void)
{
   settings_t *settings = config_get_ptr();

   cheevos_test_cheevo_set(&cheevos_locals.core);

   if (settings->cheevos.test_unofficial)
      cheevos_test_cheevo_set(&cheevos_locals.unofficial);

#if 0
   cheevos_test_leaderboards();
#endif
}

bool cheevos_set_cheats(void)
{
   cheats_were_enabled = cheats_are_enabled;

   return true;
}

void cheevos_set_support_cheevos(bool state)
{
   cheevos_locals.core_supports = state;
}

bool cheevos_get_support_cheevos(void)
{
  return cheevos_locals.core_supports;
}
