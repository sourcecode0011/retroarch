/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2014-2016 - Ali Bouhlel
 *  Copyright (C)      2016 - FIX94
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
#include <malloc.h>
#include <stdint.h>

#include <sndcore2/core.h>
#include <sndcore2/device.h>
#include <sndcore2/drcvs.h>
#include <sndcore2/result.h>
#include <sndcore2/voice.h>
#include <coreinit/time.h>
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/spinlock.h>

#include "wiiu/wiiu_dbg.h"
#include "wiiu/system/memory.h"
#include "wiiu/multivoice.h"

#include "audio/audio_driver.h"
#include "performance_counters.h"
#include "runloop.h"

typedef struct
{
   AXMVoice* mvoice;
   uint16_t* buffer_l;
   uint16_t* buffer_r;
   bool nonblocking;

   uint32_t pos;
   uint32_t written;
   OSSpinLock spinlock;
} ax_audio_t;

/* 3072 samples main buffer, 64ms total */
#define AX_AUDIO_COUNT              3072
#define AX_AUDIO_SIZE               (AX_AUDIO_COUNT << 1u)

#define AX_AUDIO_SAMPLE_COUNT       144 //3ms
#define AX_AUDIO_SAMPLE_MIN         (AX_AUDIO_SAMPLE_COUNT * 3) //9ms
#define AX_AUDIO_SAMPLE_LOAD        (AX_AUDIO_SAMPLE_COUNT * 10) //30ms
#define AX_AUDIO_MAX_FREE           (AX_AUDIO_COUNT - (AX_AUDIO_SAMPLE_COUNT * 2))
#define AX_AUDIO_RATE               48000
//#define ax_audio_ticks_to_samples(ticks)     (((ticks) * 64) / 82875)
//#define ax_audio_samples_to_ticks(samples)   (((samples) * 82875) / 64)

static volatile ax_audio_t *wiiu_cb_ax = NULL;
void wiiu_ax_callback(void)
{
   //possibly called before unregister
   if(wiiu_cb_ax == NULL)
      return;

   ax_audio_t *ax = (ax_audio_t*)wiiu_cb_ax;
   if(AXIsMultiVoiceRunning(ax->mvoice))
   {
      if(OSUninterruptibleSpinLock_Acquire(&ax->spinlock))
      {
         //buffer underrun, stop playback to let it fill up
         if(ax->written < AX_AUDIO_SAMPLE_MIN)
            AXSetMultiVoiceState(ax->mvoice, AX_VOICE_STATE_STOPPED);
         ax->written -= AX_AUDIO_SAMPLE_COUNT;
         OSUninterruptibleSpinLock_Release(&ax->spinlock);
      }
   }
}

extern void AXRegisterFrameCallback(void *cb);

static void* ax_audio_init(const char* device, unsigned rate, unsigned latency,
      unsigned block_frames,
      unsigned *new_rate)
{
   AXVoiceOffsets offsets[2];
   u16 setup_buf[0x30] = {0};
   setup_buf[0x25]     = 2; /* we request 2 channels */
   AXInitParams init   = {AX_INIT_RENDERER_48KHZ, 0, 0};
   AXVoiceVeData ve    = {0xF000, 0};
   ax_audio_t* ax      = (ax_audio_t*)calloc(1, sizeof(ax_audio_t));

   if (!ax)
      return NULL;

   AXInitWithParams(&init);

   AXAcquireMultiVoice(31, NULL, 0, setup_buf, &ax->mvoice);

   if (!ax->mvoice || ax->mvoice->channels != 2)
   {
      free(ax);
      return NULL;
   }

   ax->buffer_l               = MEM1_alloc(AX_AUDIO_SIZE, 0x100);
   ax->buffer_r              = MEM1_alloc(AX_AUDIO_SIZE, 0x100);
   memset(ax->buffer_l,0,AX_AUDIO_SIZE);
   memset(ax->buffer_r,0,AX_AUDIO_SIZE);
   DCFlushRange(ax->buffer_l,AX_AUDIO_SIZE);
   DCFlushRange(ax->buffer_r,AX_AUDIO_SIZE);

   /* shared by both voices */
   offsets[0].currentOffset  = 0;
   offsets[0].loopOffset     = 0;
   offsets[0].endOffset      = AX_AUDIO_COUNT - 1;
   offsets[0].loopingEnabled = AX_VOICE_LOOP_ENABLED;
   offsets[0].dataType       = AX_VOICE_FORMAT_LPCM16;
   memcpy(&offsets[1], &offsets[0], sizeof(AXVoiceOffsets));

   /* different buffers per voice */
   offsets[0].data           = ax->buffer_l;
   offsets[1].data           = ax->buffer_r;
   AXSetMultiVoiceOffsets(ax->mvoice, offsets);

   AXSetMultiVoiceSrcType(ax->mvoice, AX_VOICE_SRC_TYPE_NONE);
   AXSetMultiVoiceSrcRatio(ax->mvoice, 1.0f);

   AXSetMultiVoiceVe(ax->mvoice, &ve);

   AXSetMultiVoiceDeviceMix(ax->mvoice, AX_DEVICE_TYPE_DRC, 0, 0, 0x8000, 0);
   AXSetMultiVoiceDeviceMix(ax->mvoice, AX_DEVICE_TYPE_TV, 0, 0, 0x8000, 0);

   AXSetMultiVoiceState(ax->mvoice, AX_VOICE_STATE_STOPPED);

   ax->pos                   = 0;
   ax->written               = 0;
   *new_rate                 = AX_AUDIO_RATE;

   OSInitSpinLock(&ax->spinlock);

   wiiu_cb_ax                = ax;
   AXRegisterFrameCallback(wiiu_ax_callback);

   return ax;
}

static void ax_audio_free(void* data)
{
   ax_audio_t* ax = (ax_audio_t*)data;
   wiiu_cb_ax     = NULL;

   AXRegisterFrameCallback(NULL);
   AXFreeMultiVoice(ax->mvoice);
   AXQuit();

   MEM1_free(ax->buffer_l);
   MEM1_free(ax->buffer_r);
   free(ax);
}

static bool ax_audio_stop(void* data)
{
   ax_audio_t* ax = (ax_audio_t*)data;

   AXSetMultiVoiceState(ax->mvoice, AX_VOICE_STATE_STOPPED);
   return true;
}

static int ax_audio_limit(int in)
{
	if(in < 0)
		in += AX_AUDIO_COUNT;
	else if(in >= AX_AUDIO_COUNT)
		in -= AX_AUDIO_COUNT;
	return in;
}

static bool ax_audio_start(void* data)
{
   ax_audio_t* ax = (ax_audio_t*)data;

   /* Prevents restarting audio when the menu
    * is toggled off on shutdown */

   if (runloop_ctl(RUNLOOP_CTL_IS_SHUTDOWN, NULL))
      return true;

   //set back to playing on enough buffered data
   if(ax->written > AX_AUDIO_SAMPLE_LOAD)
   {
      AXSetMultiVoiceCurrentOffset(ax->mvoice, ax_audio_limit(ax->pos - ax->written));
      AXSetMultiVoiceState(ax->mvoice, AX_VOICE_STATE_PLAYING);
   }
   return true;
}

static ssize_t ax_audio_write(void* data, const void* buf, size_t size)
{
   int i;
   static struct retro_perf_counter ax_audio_write_perf = {0};
   size_t countAvail   = 0;
   ax_audio_t* ax      = (ax_audio_t*)data;
   const uint16_t* src = buf;
   int count           = size >> 2;

   if(!size || (size & 0x3))
      return 0;

   //measure copy performance from here
   performance_counter_init(&ax_audio_write_perf, "ax_audio_write");
   performance_counter_start(&ax_audio_write_perf);

   if(count > AX_AUDIO_MAX_FREE)
      count = AX_AUDIO_MAX_FREE;

   countAvail = ((ax->written > AX_AUDIO_MAX_FREE) ? 0 : (AX_AUDIO_MAX_FREE - ax->written));

   if (ax->nonblocking)
   {
      //not enough available for 3ms of data
      if(countAvail < AX_AUDIO_SAMPLE_COUNT)
         count = 0;
   }
   else if(countAvail < count)
   {
      //sync, wait for free memory
      while(AXIsMultiVoiceRunning(ax->mvoice) && (countAvail < count))
      {
         OSYieldThread(); //gives threads with same priority time to run
         countAvail = (ax->written > AX_AUDIO_MAX_FREE ? 0 : (AX_AUDIO_MAX_FREE - ax->written));
      }
   }
   //over available space, do as much as possible
   if(count > countAvail)
      count = countAvail;
   //make sure we have input size
   if(count > 0)
   {
      //write in new data
      size_t startPos = ax->pos;
      int flushP2needed = 0;
      int flushP2 = 0;
      for (i = 0; i < (count << 1); i += 2)
      {
         ax->buffer_l[ax->pos] = src[i];
         ax->buffer_r[ax->pos] = src[i + 1];
         ax->pos = ax_audio_limit(ax->pos + 1);
         //wrapped around, make sure to store cache
         if(ax->pos == 0)
         {
            flushP2needed = 1;
            flushP2 = ((count << 1) - i);
            DCStoreRangeNoSync(ax->buffer_l+startPos, (AX_AUDIO_COUNT-startPos) << 1);
            DCStoreRangeNoSync(ax->buffer_r+startPos, (AX_AUDIO_COUNT-startPos) << 1);
         }
      }
      //standard cache store case
      if(!flushP2needed)
      {
         DCStoreRangeNoSync(ax->buffer_l+startPos, count << 1);
         DCStoreRange(ax->buffer_r+startPos, count << 1);
      } //store the rest after wrap
      else if(flushP2 > 0)
      {
         DCStoreRangeNoSync(ax->buffer_l, flushP2);
         DCStoreRange(ax->buffer_r, flushP2);
      }
      //add in new audio data
      if(OSUninterruptibleSpinLock_Acquire(&ax->spinlock))
      {
         ax->written += count;
         OSUninterruptibleSpinLock_Release(&ax->spinlock);
      }
   }
   //possibly buffer underrun
   if(!AXIsMultiVoiceRunning(ax->mvoice))
   {
      //checks if it can be started
      ax_audio_start(ax);
   }
   //done copying new data
   performance_counter_stop(&ax_audio_write_perf);
   //return what was actually copied
   return (count << 2);
}

static bool ax_audio_alive(void* data)
{
   ax_audio_t* ax = (ax_audio_t*)data;
   return AXIsMultiVoiceRunning(ax->mvoice);
}

static void ax_audio_set_nonblock_state(void* data, bool state)
{
   ax_audio_t* ax = (ax_audio_t*)data;

   if (ax)
      ax->nonblocking = state;
}

static bool ax_audio_use_float(void* data)
{
   (void)data;
   return false;
}

static size_t ax_audio_write_avail(void* data)
{
   ax_audio_t* ax = (ax_audio_t*)data;

   size_t ret = AX_AUDIO_COUNT - ax->written;

   return (ret < AX_AUDIO_SAMPLE_COUNT ? 0 : ret);
}

static size_t ax_audio_buffer_size(void* data)
{
   (void)data;
   return AX_AUDIO_COUNT;
}


audio_driver_t audio_ax =
{
   ax_audio_init,
   ax_audio_write,
   ax_audio_stop,
   ax_audio_start,
   ax_audio_alive,
   ax_audio_set_nonblock_state,
   ax_audio_free,
   ax_audio_use_float,
   "AX",
   NULL,
   NULL,
//   ax_audio_write_avail,
//   ax_audio_buffer_size
};
