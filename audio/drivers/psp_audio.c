/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *  Copyright (C) 2014-2016 - Ali Bouhlel
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

#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#ifdef VITA
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/audioout.h>
#else
#include <pspkernel.h>
#include <pspaudio.h>
#endif

#include "../audio_driver.h"

typedef struct psp_audio
{
   bool nonblocking;

   uint32_t* buffer;
   uint32_t* zeroBuffer;

   SceUID thread;
   int rate;

   volatile bool running;
   volatile uint16_t read_pos;
   volatile uint16_t write_pos;

#ifdef VITA
   char lock[32] __attribute__ ((aligned (8)));
   char cond_lock[32] __attribute__ ((aligned (8)));
   char cond[32] __attribute__ ((aligned (8)));
#endif
} psp_audio_t;

#define AUDIO_OUT_COUNT 512u
#define AUDIO_BUFFER_SIZE (1u<<13u)
#define AUDIO_BUFFER_SIZE_MASK (AUDIO_BUFFER_SIZE-1)

#ifdef VITA
#define PSP_THREAD_STOPPED SCE_THREAD_STOPPED
#else
#define SceKernelThreadInfo SceKernelThreadRunStatus
#define sceKernelGetThreadInfo sceKernelReferThreadRunStatus
#endif

static int audioMainLoop(SceSize args, void* argp)
{
   psp_audio_t* psp = *((psp_audio_t**)argp);
#ifdef VITA
   int port         = sceAudioOutOpenPort(
         SCE_AUDIO_OUT_PORT_TYPE_MAIN, AUDIO_OUT_COUNT,
         psp->rate, SCE_AUDIO_OUT_MODE_STEREO);
#else
   sceAudioSRCChReserve(AUDIO_OUT_COUNT, psp->rate, 2);
#endif

   while (psp->running)
   {
      bool cond           = false;
      uint16_t read_pos   = psp->read_pos;
      uint16_t read_pos_2 = psp->read_pos;

#ifdef VITA
      sceKernelLockLwMutex((struct SceKernelLwMutexWork*)&psp->lock, 1, 0);
#endif

      cond                = ((uint16_t)(psp->write_pos - read_pos) & AUDIO_BUFFER_SIZE_MASK)
            < (AUDIO_OUT_COUNT * 2);

#ifndef VITA
      sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, cond ? (psp->zeroBuffer)
            : (psp->buffer + read_pos));
#endif
      if (!cond)
      {
         read_pos      += AUDIO_OUT_COUNT;
         read_pos      &= AUDIO_BUFFER_SIZE_MASK;
         psp->read_pos  = read_pos;
      }

#ifdef VITA
      sceKernelUnlockLwMutex((struct SceKernelLwMutexWork*)&psp->lock, 1);
      sceKernelSignalLwCond((struct SceKernelLwCondWork*)&psp->cond);

      sceAudioOutOutput(port,
        cond ? (psp->zeroBuffer)
              : (psp->buffer + read_pos_2));
#endif
   }

#ifdef VITA
   sceAudioOutReleasePort(port);
#else
   sceAudioSRCChRelease();
   sceKernelExitThread(0);
#endif
   return 0;
}

static void *psp_audio_init(const char *device,
      unsigned rate, unsigned latency,
      unsigned block_frames,
      unsigned *new_rate)
{
   psp_audio_t *psp = (psp_audio_t*)calloc(1, sizeof(psp_audio_t));

   if (!psp)
      return NULL;

   (void)device;
   (void)latency;


   /* Cache aligned, not necessary but helpful. */
   psp->buffer      = (uint32_t*)
      memalign(64, AUDIO_BUFFER_SIZE * sizeof(uint32_t));
   memset(psp->buffer, 0, AUDIO_BUFFER_SIZE * sizeof(uint32_t));
   psp->zeroBuffer  = (uint32_t*)
      memalign(64, AUDIO_OUT_COUNT   * sizeof(uint32_t));
   memset(psp->zeroBuffer, 0, AUDIO_OUT_COUNT * sizeof(uint32_t));

   psp->read_pos    = 0;
   psp->write_pos   = 0;
   psp->rate        = rate;
#if defined(VITA)

   sceKernelCreateLwMutex((struct SceKernelLwMutexWork*)&psp->lock, "audio_get_lock", 0, 0, 0);
   sceKernelCreateLwMutex((struct SceKernelLwMutexWork*)&psp->cond_lock, "audio_get_cond_lock", 0, 0, 0);
   sceKernelCreateLwCond((struct SceKernelLwCondWork*)&psp->cond, "audio_get_cond", 0, (struct SceKernelLwMutexWork*)&psp->cond_lock, 0);
   psp->thread      = sceKernelCreateThread
      ("audioMainLoop", audioMainLoop, 0x10000100, 0x10000, 0, 0, NULL);
#else
   psp->thread      = sceKernelCreateThread
      ("audioMainLoop", audioMainLoop, 0x08, 0x10000, 0, NULL);
#endif

   psp->nonblocking = false;
   psp->running     = true;
   sceKernelStartThread(psp->thread, sizeof(psp_audio_t*), &psp);

   return psp;
}

static void psp_audio_free(void *data)
{
   SceUInt timeout = 100000;
   psp_audio_t* psp = (psp_audio_t*)data;
   if(!psp)
      return;

   psp->running    = false;
#if defined(VITA)
   sceKernelWaitThreadEnd(psp->thread, NULL, &timeout);
   sceKernelDeleteLwMutex((struct SceKernelLwMutexWork*)&psp->lock);
   sceKernelDeleteLwMutex((struct SceKernelLwMutexWork*)&psp->cond_lock);
   sceKernelDeleteLwCond((struct SceKernelLwCondWork*)&psp->cond);
#else
   sceKernelWaitThreadEnd(psp->thread, &timeout);
#endif
   free(psp->buffer);
   sceKernelDeleteThread(psp->thread);
   free(psp->zeroBuffer);
   free(psp);
}

static ssize_t psp_audio_write(void *data, const void *buf, size_t size)
{
   psp_audio_t* psp     = (psp_audio_t*)data;
   uint16_t write_pos   = psp->write_pos;
   uint16_t sampleCount = size / sizeof(uint32_t);

#ifdef VITA
   if (psp->nonblocking)
   {
      if (AUDIO_BUFFER_SIZE - ((uint16_t)
            (psp->write_pos - psp->read_pos) & AUDIO_BUFFER_SIZE_MASK) < size)
         return 0;
   }

   while (AUDIO_BUFFER_SIZE - ((uint16_t)
         (psp->write_pos - psp->read_pos) & AUDIO_BUFFER_SIZE_MASK) < size){
      sceKernelWaitLwCond((struct SceKernelLwCondWork*)&psp->cond, 0);
   }

   sceKernelLockLwMutex((struct SceKernelLwMutexWork*)&psp->lock, 1, 0);

   if((write_pos + sampleCount) > AUDIO_BUFFER_SIZE)
   {
      memcpy(psp->buffer + write_pos, buf,
            (AUDIO_BUFFER_SIZE - write_pos) * sizeof(uint32_t));
      memcpy(psp->buffer, (uint32_t*) buf +
            (AUDIO_BUFFER_SIZE - write_pos),
            (write_pos + sampleCount - AUDIO_BUFFER_SIZE) * sizeof(uint32_t));
   }
   else
      memcpy(psp->buffer + write_pos, buf, size);

   write_pos      += sampleCount;
   write_pos      &= AUDIO_BUFFER_SIZE_MASK;
   psp->write_pos  = write_pos;
   sceKernelUnlockLwMutex((struct SceKernelLwMutexWork*)&psp->lock, 1);

  return size;
#else



#if 0
   if (psp->nonblocking)
   {
      /* TODO */
   }
#endif

   if((write_pos + sampleCount) > AUDIO_BUFFER_SIZE)
   {
      memcpy(psp->buffer + write_pos, buf,
            (AUDIO_BUFFER_SIZE - write_pos) * sizeof(uint32_t));
      memcpy(psp->buffer, (uint32_t*) buf +
            (AUDIO_BUFFER_SIZE - write_pos),
            (write_pos + sampleCount - AUDIO_BUFFER_SIZE) * sizeof(uint32_t));
   }
   else
      memcpy(psp->buffer + write_pos, buf, size);

   write_pos  += sampleCount;
   write_pos  &= AUDIO_BUFFER_SIZE_MASK;
   psp->write_pos = write_pos;

   return sampleCount;
#endif
}

static bool psp_audio_alive(void *data)
{
   psp_audio_t* psp = (psp_audio_t*)data;
   if (!psp)
      return false;
   return psp->running;
}


static bool psp_audio_stop(void *data)
{
   SceKernelThreadInfo info;
   SceUInt timeout   = 100000;
   psp_audio_t* psp = (psp_audio_t*)data;

   info.size = sizeof(SceKernelThreadInfo);

   if (sceKernelGetThreadInfo(
            psp->thread, &info) < 0) /* Error */
      return false;

   if (info.status == PSP_THREAD_STOPPED)
      return false;

   psp->running = false;
#if defined(VITA)
   sceKernelWaitThreadEnd(psp->thread, NULL, &timeout);
#else
   sceKernelWaitThreadEnd(psp->thread, &timeout);
#endif
   return true;
}

static bool psp_audio_start(void *data)
{
   SceKernelThreadInfo info;
   psp_audio_t* psp = (psp_audio_t*)data;

   info.size = sizeof(SceKernelThreadInfo);

   if (sceKernelGetThreadInfo(
            psp->thread, &info) < 0) /* Error */
      return false;

   if (info.status != PSP_THREAD_STOPPED)
      return false;

   psp->running = true;

   sceKernelStartThread(psp->thread, sizeof(psp_audio_t*), &psp);

   return true;
}

static void psp_audio_set_nonblock_state(void *data, bool toggle)
{
   psp_audio_t* psp = (psp_audio_t*)data;
   if (psp)
      psp->nonblocking = toggle;
}

static bool psp_audio_use_float(void *data)
{
   (void)data;
   return false;
}

static size_t psp_write_avail(void *data)
{
   psp_audio_t* psp = (psp_audio_t*)data;
#ifdef VITA
   size_t val;

   sceKernelLockLwMutex((struct SceKernelLwMutexWork*)&psp->lock, 1, 0);
   val = AUDIO_BUFFER_SIZE - ((uint16_t)
         (psp->write_pos - psp->read_pos) & AUDIO_BUFFER_SIZE_MASK);
   sceKernelUnlockLwMutex((struct SceKernelLwMutexWork*)&psp->lock, 1);
   return val;
#else
   /* TODO */
   return AUDIO_BUFFER_SIZE - ((uint16_t)
         (psp->write_pos - psp->read_pos) & AUDIO_BUFFER_SIZE_MASK);
#endif
}

static size_t psp_buffer_size(void *data)
{
   /* TODO */
   return AUDIO_BUFFER_SIZE /** sizeof(uint32_t)*/;
}


audio_driver_t audio_psp = {
   psp_audio_init,
   psp_audio_write,
   psp_audio_stop,
   psp_audio_start,
   psp_audio_alive,
   psp_audio_set_nonblock_state,
   psp_audio_free,
   psp_audio_use_float,
#ifdef VITA
   "vita",
#else
   "psp",
#endif
   NULL,
   NULL,
   psp_write_avail,
   psp_buffer_size
};
