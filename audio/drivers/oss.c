/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_OSS_BSD
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <retro_endianness.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../audio_driver.h"
#include "../../verbosity.h"

#ifdef HAVE_OSS_BSD
#define DEFAULT_OSS_DEV "/dev/snd/actsnd"//"/dev/audio"
#else
#define DEFAULT_OSS_DEV "/dev/snd/actsnd"//"/dev/dsp"
#endif

static bool oss_is_paused = false;

static void *oss_init(const char *device, unsigned rate, unsigned latency,
      unsigned block_frames,
      unsigned *new_out_rate)
{
   int frags, frag, channels, format, new_rate;
   int              *fd   = (int*)calloc(1, sizeof(int));
   const char *oss_device = device ? device : DEFAULT_OSS_DEV;
  int  mSpeakerOn = 1,mOutMode =0,state=0,vol = 40;
//   RARCH_ERR("%s line %d. %s \n",__FILE__,__LINE__,oss_device);
   if (!fd)
      return NULL;

   if ((*fd = open(oss_device, O_WRONLY)) < 0)
   {
      free(fd);
      perror("open");
      return NULL;
   }
 //RARCH_ERR("%s line %d. %d \n",__FILE__,__LINE__,*fd);
   frags = 10;//(latency * rate * 4) / (1000 * (1 << 10));
   frag  = (frags << 16) | 10;

   if (ioctl(*fd, SNDCTL_DSP_SETFRAGMENT, &frag) < 0)
      RARCH_WARN("Cannot set fragment sizes. Latency might not be as expected ...\n");

   channels = 2;
   format   = is_little_endian() ? AFMT_S16_LE : AFMT_S16_BE;

   if (ioctl(*fd, SNDCTL_DSP_CHANNELS, &channels) < 0)
      goto error;

//   if (ioctl(*fd, SNDCTL_DSP_SETFMT, &format) < 0)
//      goto error;

   new_rate = rate;
//RARCH_ERR("%s line %d. new_rate %d \n",__FILE__,__LINE__,new_rate);
   if (ioctl(*fd, SNDCTL_DSP_SPEED, &new_rate) < 0)
      goto error;
/*
ioctl(*fd, 0xffff0001, &mSpeakerOn);
    			ioctl(*fd, 0xffff0000, &mOutMode);
				ioctl(*fd, 0xffff0003, &state);
				ioctl(*fd, 0x20000000|0x40000000|(4&0x1fff<<16)|('M'<<8)|0, &vol);
				*/
				
   if (new_rate != (int)rate)
   {
      RARCH_WARN("Requested sample rate not supported. Adjusting output rate to %d Hz.\n", new_rate);
      *new_out_rate = new_rate;
   }
 //RARCH_ERR("%s line %d.\n",__FILE__,__LINE__);
   return fd;

error:
   close(*fd);
   free(fd);
   perror("ioctl");
   return NULL;
}
//static unsigned char tempbuf[16*1024];
//static int cursize=0;
//#define ONE_WRITE 1024
static ssize_t oss_write(void *data, const void *buf, size_t size)
{
   ssize_t ret=0;
 //   ssize_t wret;
   int *fd = (int*)data;

  if (size == 0)
      return 0;
/*	memcpy(tempbuf,buf,size);
	cursize +=size;
	if(cursize < ONE_WRITE)
		return 0;
	while(1)
	{
		wret = write(*fd, tempbuf+ret, ONE_WRITE);
		if(wret > 0)
			ret += wret;

		if((cursize-ret) < ONE_WRITE)
			break;
	}*/
   if ((ret = write(*fd, buf, size)) < 0)
   { RARCH_ERR("%s line %d. ret \n",__FILE__,__LINE__);
      if (errno == EAGAIN && (fcntl(*fd, F_GETFL) & O_NONBLOCK))
         return 0;

      return -1;
   }
//if((cursize-ret)>0)memcpy(tempbuf,tempbuf+ret,(cursize-ret));
//cursize -= ret;
   // RARCH_ERR("%s line %d. ret %d \n",__FILE__,__LINE__,ret);

  // ioctl(*fd, SNDCTL_DSP_NONBLOCK, 0);
   return ret;
}

static bool oss_stop(void *data)
{
   int *fd = (int*)data;
RARCH_ERR("%s line %d.\n",__FILE__,__LINE__);
   ioctl(*fd, SNDCTL_DSP_RESET, 0);
   oss_is_paused = true;
   return true;
}

static bool oss_start(void *data)
{
   (void)data;
   oss_is_paused = false;
   return true;
}

static bool oss_alive(void *data)
{
   (void)data;
   return !oss_is_paused;
}

static void oss_set_nonblock_state(void *data, bool state)
{
   int rc;
   int *fd = (int*)data;
RARCH_ERR("%s line %d.\n",__FILE__,__LINE__);
   if (state)
      rc = fcntl(*fd, F_SETFL, fcntl(*fd, F_GETFL) | O_NONBLOCK);
   else
      rc = fcntl(*fd, F_SETFL, fcntl(*fd, F_GETFL) & (~O_NONBLOCK));
   if (rc != 0)
      RARCH_WARN("Could not set nonblocking on OSS file descriptor. Will not be able to fast-forward.\n");
}

static void oss_free(void *data)
{
   int *fd = (int*)data;
RARCH_ERR("%s line %d.\n",__FILE__,__LINE__);
   ioctl(*fd, SNDCTL_DSP_RESET, 0);
   close(*fd);
   free(fd);
}

static size_t oss_write_avail(void *data)
{
   audio_buf_info info;
   int *fd = (int*)data;
//RARCH_ERR("%s line %d.\n",__FILE__,__LINE__);
 //  if (ioctl(*fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
   {
    //  RARCH_ERR("SNDCTL_DSP_GETOSPACE failed ...\n");
      return 2048;//0;
   }

 //  return info.bytes;
}

static size_t oss_buffer_size(void *data)
{
   audio_buf_info info;
   int *fd = (int*)data;
RARCH_ERR("%s line %d.\n",__FILE__,__LINE__);
   //f (ioctl(*fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
   {
      RARCH_ERR("SNDCTL_DSP_GETOSPACE failed ...\n");
      return 2048;//1; /* Return something non-zero to avoid SIGFPE. */
   }

  // return info.fragsize * info.fragstotal;
}

static bool oss_use_float(void *data)
{
   (void)data;
   return false;
}

audio_driver_t audio_oss = {
   oss_init,
   oss_write,
   oss_stop,
   oss_start,
   oss_alive,
   oss_set_nonblock_state,
   oss_free,
   oss_use_float,
   "oss",
   NULL,
   NULL,
   oss_write_avail,
   oss_buffer_size,
};
