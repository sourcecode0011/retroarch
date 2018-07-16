/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C) 2014-2015 - Alfred Agrell
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

#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <string.h>

#include <retro_inline.h>
#include <algorithms/mismatch.h>

#include "state_manager.h"
#include "../msg_hash.h"
#include "../movie.h"
#include "../core.h"
#include "../runloop.h"
#include "../performance_counters.h"
#include "../verbosity.h"
#include "../audio/audio_driver.h"

/* This makes Valgrind throw errors if a core overflows its savestate size. */
/* Keep it off unless you're chasing a core bug, it slows things down. */
#define STRICT_BUF_SIZE 0

#ifndef UINT16_MAX
#define UINT16_MAX 0xffff
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffffu
#endif

struct state_manager
{
   uint8_t *data;
   size_t capacity;
   /* Reading and writing is done here here. */
   uint8_t *head;
   /* If head comes close to this, discard a frame. */
   uint8_t *tail;

   uint8_t *thisblock;
   uint8_t *nextblock;

   /* This one is rounded up from reset::blocksize. */
   size_t blocksize;

   /* size_t + (blocksize + 131071) / 131072 * 
    * (blocksize + u16 + u16) + u16 + u32 + size_t
    * (yes, the math is a bit ugly). */
   size_t maxcompsize;

   unsigned entries;
   bool thisblock_valid;
#if STRICT_BUF_SIZE
   size_t debugsize;
   uint8_t *debugblock;
#endif
};

/* Format per frame (pseudocode): */
#if 0
size nextstart;
repeat {
   uint16 numchanged; /* everything is counted in units of uint16 */
   if (numchanged)
   {
      uint16 numunchanged; /* skip these before handling numchanged */
      uint16[numchanged] changeddata;
   }
   else
   {
      uint32 numunchanged;
      if (!numunchanged)
         break;
   }
}
size thisstart;
#endif

struct state_manager_rewind_state
{
   /* Rewind support. */
   state_manager_t *state;
   size_t size;
};

static struct state_manager_rewind_state rewind_state;
static bool frame_is_reversed;

/* Returns the maximum compressed size of a savestate. 
 * It is very likely to compress to far less. */
static size_t state_manager_raw_maxsize(size_t uncomp)
{
   /* bytes covered by a compressed block */
   const int maxcblkcover = UINT16_MAX * sizeof(uint16_t);
   /* uncompressed size, rounded to 16 bits */
   size_t uncomp16        = (uncomp + sizeof(uint16_t) - 1) & -sizeof(uint16_t);
   /* number of blocks */
   size_t maxcblks        = (uncomp + maxcblkcover - 1) / maxcblkcover;
   return uncomp16 + maxcblks * sizeof(uint16_t) * 2 /* two u16 overhead per block */ + sizeof(uint16_t) *
      3; /* three u16 to end it */
}

/*
 * See state_manager_raw_compress for information about this.
 * When you're done with it, send it to free().
 */
static void *state_manager_raw_alloc(size_t len, uint16_t uniq)
{
   size_t  len16 = (len + sizeof(uint16_t) - 1) & -sizeof(uint16_t);
   uint16_t *ret = (uint16_t*)calloc(len16 + sizeof(uint16_t) * 4 + 16, 1);

   /* Force in a different byte at the end, so we don't need to check 
    * bounds in the innermost loop (it's expensive).
    *
    * There is also a large amount of data that's the same, to stop 
    * the other scan.
    *
    * There is also some padding at the end. This is so we don't 
    * read outside the buffer end if we're reading in large blocks;
    *
    * It doesn't make any difference to us, but sacrificing 16 bytes to get 
    * Valgrind happy is worth it. */
   ret[len16/sizeof(uint16_t) + 3] = uniq;

   return ret;
}

/*
 * Takes two savestates and creates a patch that turns 'src' into 'dst'.
 * Both 'src' and 'dst' must be returned from state_manager_raw_alloc(), 
 * with the same 'len', and different 'uniq'.
 *
 * 'patch' must be size 'state_manager_raw_maxsize(len)' or more.
 * Returns the number of bytes actually written to 'patch'.
 */
static size_t state_manager_raw_compress(const void *src,
      const void *dst, size_t len, void *patch)
{
   const uint16_t  *old16 = (const uint16_t*)src;
   const uint16_t  *new16 = (const uint16_t*)dst;
   uint16_t *compressed16 = (uint16_t*)patch;
   size_t          num16s = (len + sizeof(uint16_t) - 1) 
      / sizeof(uint16_t);
   
   while (num16s)
   {
      size_t i, changed;
      size_t skip = find_change(old16, new16);
   
      if (skip >= num16s)
         break;
   
      old16  += skip;
      new16  += skip;
      num16s -= skip;
   
      if (skip > UINT16_MAX)
      {
         if (skip > UINT32_MAX)
         {
            /* This will make it scan the entire thing again, 
             * but it only hits on 8GB unchanged data anyways,
             * and if you're doing that, you've got bigger problems. */
            skip = UINT32_MAX;
         }
         *compressed16++ = 0;
         *compressed16++ = skip;
         *compressed16++ = skip >> 16;
         continue;
      }
   
      changed = find_same(old16, new16);
      if (changed > UINT16_MAX)
         changed = UINT16_MAX;
   
      *compressed16++ = changed;
      *compressed16++ = skip;
   
      for (i = 0; i < changed; i++)
         compressed16[i] = old16[i];
   
      old16 += changed;
      new16 += changed;
      num16s -= changed;
      compressed16 += changed;
   }
   
   compressed16[0] = 0;
   compressed16[1] = 0;
   compressed16[2] = 0;
   
   return (uint8_t*)(compressed16+3) - (uint8_t*)patch;
}

/*
 * Takes 'patch' from a previous call to 'state_manager_raw_compress' 
 * and applies it to 'data' ('src' from that call), 
 * yielding 'dst' in that call.
 *
 * If the given arguments do not match a previous call to 
 * state_manager_raw_compress(), anything at all can happen.
 */
static void state_manager_raw_decompress(const void *patch,
      size_t patchlen, void *data, size_t datalen)
{
   uint16_t         *out16 = (uint16_t*)data;
   const uint16_t *patch16 = (const uint16_t*)patch;
   
   (void)patchlen;
   (void)datalen;
   
   for (;;)
   {
      uint16_t numchanged = *(patch16++);

      if (numchanged)
      {
         uint16_t i;

         out16 += *patch16++;

         /* We could do memcpy, but it seems that memcpy has a 
          * constant-per-call overhead that actually shows up.
          *
          * Our average size in here seems to be 8 or something.
          * Therefore, we do something with lower overhead. */
         for (i = 0; i < numchanged; i++)
            out16[i] = patch16[i];

         patch16 += numchanged;
         out16 += numchanged;
      }
      else
      {
         uint32_t numunchanged = patch16[0] | (patch16[1] << 16);

         if (!numunchanged)
            break;
         patch16 += 2;
         out16 += numunchanged;
      }
   }
}

/* The start offsets point to 'nextstart' of any given compressed frame.
 * Each uint16 is stored native endian; anything that claims any other 
 * endianness refers to the endianness of this specific item.
 * The uint32 is stored little endian.
 *
 * Each size value is stored native endian if alignment is not enforced; 
 * if it is, they're little endian.
 *
 * The start of the buffer contains a size pointing to the end of the 
 * buffer; the end points to its start.
 *
 * Wrapping is handled by returning to the start of the buffer if the 
 * compressed data could potentially hit the edge;
 *
 * if the compressed data could potentially overwrite the tail pointer, 
 * the tail retreats until it can no longer collide.
 *
 * This means that on average, ~2 * maxcompsize is 
 * unused at any given moment. */


/* These are called very few constant times per frame, 
 * keep it as simple as possible. */
static INLINE void write_size_t(void *ptr, size_t val)
{
   memcpy(ptr, &val, sizeof(val));
}

static INLINE size_t read_size_t(const void *ptr)
{
   size_t ret;

   memcpy(&ret, ptr, sizeof(ret));
   return ret;
}

static void state_manager_free(state_manager_t *state)
{
   if (!state)
      return;

   if (state->data)
      free(state->data);
   if (state->thisblock)
      free(state->thisblock);
   if (state->nextblock)
      free(state->nextblock);
#if STRICT_BUF_SIZE
   if (state->debugblock)
      free(state->debugblock);
   state->debugblock = NULL;
#endif
   state->data       = NULL;
   state->thisblock  = NULL;
   state->nextblock  = NULL;
}

static state_manager_t *state_manager_new(size_t state_size, size_t buffer_size)
{
   size_t max_comp_size, block_size;
   uint8_t *next_block    = NULL;
   uint8_t *this_block    = NULL;
   uint8_t *state_data    = NULL;
   state_manager_t *state = (state_manager_t*)calloc(1, sizeof(*state));

   if (!state)
      return NULL;

   block_size         = (state_size + sizeof(uint16_t) - 1) & -sizeof(uint16_t);

   /* the compressed data is surrounded by pointers to the other side */
   max_comp_size      = state_manager_raw_maxsize(state_size) + sizeof(size_t) * 2;
   state_data         = (uint8_t*)malloc(buffer_size);

   if (!state_data)
      goto error;

   this_block         = (uint8_t*)state_manager_raw_alloc(state_size, 0);
   next_block         = (uint8_t*)state_manager_raw_alloc(state_size, 1);

   if (!this_block || !next_block)
      goto error;

   state->blocksize   = block_size;
   state->maxcompsize = max_comp_size;
   state->data        = state_data;
   state->thisblock   = this_block;
   state->nextblock   = next_block;
   state->capacity    = buffer_size;

   state->head        = state->data + sizeof(size_t);
   state->tail        = state->data + sizeof(size_t);

#if STRICT_BUF_SIZE
   state->debugsize   = state_size;
   state->debugblock  = (uint8_t*)malloc(state_size);
#endif

   return state;

error:
   if (state_data)
      free(state_data);
   state_manager_free(state);
   free(state);

   return NULL;
}

static bool state_manager_pop(state_manager_t *state, const void **data)
{
   size_t start;
   uint8_t *out                 = NULL;
   const uint8_t *compressed    = NULL;

   *data = NULL;

   if (state->thisblock_valid)
   {
      state->thisblock_valid = false;
      state->entries--;
      *data = state->thisblock;
      return true;
   }

   *data = state->thisblock;
   if (state->head == state->tail)
      return false;

   start = read_size_t(state->head - sizeof(size_t));
   state->head = state->data + start;

   compressed = state->data + start + sizeof(size_t);
   out = state->thisblock;

   state_manager_raw_decompress(compressed,
         state->maxcompsize, out, state->blocksize);

   state->entries--;
   return true;
}

static void state_manager_push_where(state_manager_t *state, void **data)
{
   /* We need to ensure we have an uncompressed copy of the last
    * pushed state, or we could end up applying a 'patch' to wrong 
    * savestate, and that'd blow up rather quickly. */

   if (!state->thisblock_valid) 
   {
      const void *ignored;
      if (state_manager_pop(state, &ignored))
      {
         state->thisblock_valid = true;
         state->entries++;
      }
   }
   
   *data = state->nextblock;
#if STRICT_BUF_SIZE
   *data = state->debugblock;
#endif
}

static void state_manager_push_do(state_manager_t *state)
{
#if STRICT_BUF_SIZE
   memcpy(state->nextblock, state->debugblock, state->debugsize);
#endif

   static struct retro_perf_counter gen_deltas = {0};
   uint8_t *swap = NULL;

   if (state->thisblock_valid)
   {
      const uint8_t *oldb, *newb;
      uint8_t *compressed;
      size_t headpos, tailpos, remaining;
      if (state->capacity < sizeof(size_t) + state->maxcompsize)
         return;

recheckcapacity:;

      headpos = state->head - state->data;
      tailpos = state->tail - state->data;
      remaining = (tailpos + state->capacity -
            sizeof(size_t) - headpos - 1) % state->capacity + 1;

      if (remaining <= state->maxcompsize)
      {
         state->tail = state->data + read_size_t(state->tail);
         state->entries--;
         goto recheckcapacity;
      }

      performance_counter_init(&gen_deltas, "gen_deltas");
      performance_counter_start(&gen_deltas);

      oldb        = state->thisblock;
      newb        = state->nextblock;
      compressed  = state->head + sizeof(size_t);

      compressed += state_manager_raw_compress(oldb, newb,
            state->blocksize, compressed);

      if (compressed - state->data + state->maxcompsize > state->capacity)
      {
         compressed = state->data;
         if (state->tail == state->data + sizeof(size_t))
            state->tail = state->data + read_size_t(state->tail);
      }
      write_size_t(compressed, state->head-state->data);
      compressed += sizeof(size_t);
      write_size_t(state->head, compressed-state->data);
      state->head = compressed;

      performance_counter_stop(&gen_deltas);
   }
   else
      state->thisblock_valid = true;

   swap             = state->thisblock;
   state->thisblock = state->nextblock;
   state->nextblock = swap;

   state->entries++;
}

#if 0
static void state_manager_capacity(state_manager_t *state,
      unsigned *entries, size_t *bytes, bool *full)
{
   size_t headpos   = state->head - state->data;
   size_t tailpos   = state->tail - state->data;
   size_t remaining = (tailpos + state->capacity -
         sizeof(size_t) - headpos - 1) % state->capacity + 1;

   if (entries)
      *entries = state->entries;
   if (bytes)
      *bytes = state->capacity-remaining;
   if (full)
      *full = remaining <= state->maxcompsize * 2;
}
#endif

void state_manager_event_init(unsigned rewind_buffer_size)
{
   retro_ctx_serialize_info_t serial_info;
   retro_ctx_size_info_t info;
   void *state          = NULL;

   if (rewind_state.state)
      return;

   if (audio_driver_has_callback())
   {
      RARCH_ERR("%s.\n", msg_hash_to_str(MSG_REWIND_INIT_FAILED));
      return;
   }

   core_serialize_size(&info);

   rewind_state.size = info.size;

   if (!rewind_state.size)
   {
      RARCH_ERR("%s.\n",
            msg_hash_to_str(MSG_REWIND_INIT_FAILED_THREADED_AUDIO));
      return;
   }

   RARCH_LOG("%s: %u MB\n",
         msg_hash_to_str(MSG_REWIND_INIT),
         (unsigned)(rewind_buffer_size / 1000000));

   rewind_state.state = state_manager_new(rewind_state.size,
         rewind_buffer_size);

   if (!rewind_state.state)
      RARCH_WARN("%s.\n", msg_hash_to_str(MSG_REWIND_INIT_FAILED));

   state_manager_push_where(rewind_state.state, &state);

   serial_info.data = state;
   serial_info.size = rewind_state.size;

   core_serialize(&serial_info);

   state_manager_push_do(rewind_state.state);
}


bool state_manager_frame_is_reversed(void)
{
   return frame_is_reversed;
}

static void state_manager_set_frame_is_reversed(bool value)
{
   frame_is_reversed = value;
}

void state_manager_event_deinit(void)
{
   if (rewind_state.state)
   {
      state_manager_free(rewind_state.state);
      free(rewind_state.state);
   }
   rewind_state.state = NULL;
   rewind_state.size  = 0;
}

/**
 * check_rewind:
 * @pressed              : was rewind key pressed or held?
 *
 * Checks if rewind toggle/hold was being pressed and/or held.
 **/
void state_manager_check_rewind(bool pressed,
      unsigned rewind_granularity)
{
   static bool first    = true;

   if (state_manager_frame_is_reversed())
   {
      audio_driver_frame_is_reverse();
      state_manager_set_frame_is_reversed(false);
   }

   if (first)
   {
      first = false;
      return;
   }

   if (!rewind_state.state)
      return;

   if (pressed)
   {
      const void *buf    = NULL;

      if (state_manager_pop(rewind_state.state, &buf))
      {
         retro_ctx_serialize_info_t serial_info;

         state_manager_set_frame_is_reversed(true);

         audio_driver_setup_rewind();

         runloop_msg_queue_push(
               msg_hash_to_str(MSG_REWINDING), 0,
               runloop_ctl(RUNLOOP_CTL_IS_PAUSED, NULL) 
               ? 1 : 30, true);

         serial_info.data_const = buf;
         serial_info.size       = rewind_state.size;

         core_unserialize(&serial_info);

         if (bsv_movie_ctl(BSV_MOVIE_CTL_IS_INITED, NULL))
            bsv_movie_ctl(BSV_MOVIE_CTL_FRAME_REWIND, NULL);
      }
      else
      {
         retro_ctx_serialize_info_t serial_info;
         serial_info.data_const = buf;
         serial_info.size       = rewind_state.size;
         core_unserialize(&serial_info);
         runloop_msg_queue_push(
               msg_hash_to_str(MSG_REWIND_REACHED_END),
               0, 30, true);
      }
   }
   else
   {
      static unsigned cnt      = 0;

      cnt = (cnt + 1) % (rewind_granularity ?
            rewind_granularity : 1); /* Avoid possible SIGFPE. */

      if ((cnt == 0) || bsv_movie_ctl(BSV_MOVIE_CTL_IS_INITED, NULL))
      {
         retro_ctx_serialize_info_t serial_info;
         static struct retro_perf_counter rewind_serialize = {0};
         void *state = NULL;

         state_manager_push_where(rewind_state.state, &state);

         performance_counter_init(&rewind_serialize, "rewind_serialize");
         performance_counter_start(&rewind_serialize);

         serial_info.data = state;
         serial_info.size = rewind_state.size;

         core_serialize(&serial_info);

         performance_counter_stop(&rewind_serialize);

         state_manager_push_do(rewind_state.state);
      }
   }

   core_set_rewind_callbacks();
}
