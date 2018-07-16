/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2015 - Hans-Kristian Arntzen
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
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/epoll.h>

#include <libudev.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kd.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include <file/file_path.h>
#include <compat/strl.h>
#include <string/stdstring.h>

#include "../input_config.h"
#include "../input_driver.h"
#include "../input_joypad_driver.h"
#include "../input_keymaps.h"

#include "../drivers_keyboard/keyboard_event_udev.h"
#include "../../gfx/video_driver.h"
#include "../common/linux_common.h"
#include "../common/udev_common.h"
#include "../common/epoll_common.h"

#include "../../verbosity.h"

typedef struct udev_input udev_input_t;

typedef void (*device_handle_cb)(void *data,
      const struct input_event *event, udev_input_device_t *dev);

struct udev_input_device
{
   int fd;
   dev_t dev;
   device_handle_cb handle_cb;
   char devnode[PATH_MAX_LENGTH];

   union
   {
      /*
       * keyboard
       * mouse
       */
      struct
      {
         float x, y;
         float mod_x, mod_y;
         struct input_absinfo info_x;
         struct input_absinfo info_y;
         bool touch;
      } touchpad;
   } state;
};

struct udev_input
{
   bool blocked;
   struct udev *udev;
   struct udev_monitor *monitor;


   const input_device_driver_t *joypad;

   int epfd;
   udev_input_device_t **devices;
   unsigned num_devices;

   int16_t mouse_x;
   int16_t mouse_y;
   bool mouse_l, mouse_r, mouse_m, mouse_wu, mouse_wd, mouse_whu, mouse_whd;
};

#ifdef HAVE_XKBCOMMON
int init_xkb(int fd, size_t size);
#endif

static void udev_handle_touchpad(void *data,
      const struct input_event *event, udev_input_device_t *dev)
{
   udev_input_t *udev = (udev_input_t*)data;

   switch (event->type)
   {
      case EV_ABS:
         switch (event->code)
         {
            case ABS_X:
            {
               int x        = event->value - dev->state.touchpad.info_x.minimum;
               int range    = dev->state.touchpad.info_x.maximum - 
                  dev->state.touchpad.info_x.minimum;
               float x_norm = (float)x / range;
               float rel_x  = x_norm - dev->state.touchpad.x;

               if (dev->state.touchpad.touch)
                  udev->mouse_x += (int16_t)
                     roundf(dev->state.touchpad.mod_x * rel_x);

               dev->state.touchpad.x = x_norm;
               /* Some factor, not sure what's good to do here ... */
               dev->state.touchpad.mod_x = 500.0f;
               break;
            }

            case ABS_Y:
            {
               int y        = event->value - dev->state.touchpad.info_y.minimum;
               int range    = dev->state.touchpad.info_y.maximum - 
                  dev->state.touchpad.info_y.minimum;
               float y_norm = (float)y / range;
               float rel_y  = y_norm - dev->state.touchpad.y;

               if (dev->state.touchpad.touch)
                  udev->mouse_y += (int16_t)roundf(dev->state.touchpad.mod_y * rel_y);

               dev->state.touchpad.y = y_norm;

               /* Some factor, not sure what's good to do here ... */
               dev->state.touchpad.mod_y = 500.0f;
               break;
            }

            default:
               break;
         }
         break;

      case EV_KEY:
         switch (event->code)
         {
            case BTN_TOUCH:
               dev->state.touchpad.touch = event->value;
               dev->state.touchpad.mod_x = 0.0f; /* First ABS event is not a relative one. */
               dev->state.touchpad.mod_y = 0.0f;
               break;

            default:
               break;
         }
   }
}

static void udev_handle_mouse(void *data,
      const struct input_event *event, udev_input_device_t *dev)
{
   udev_input_t *udev = (udev_input_t*)data;

   switch (event->type)
   {
      case EV_KEY:
         /* TODO: mouse wheel up/down doesn't work */
         switch (event->code)
         {
            case BTN_LEFT:
               udev->mouse_l = event->value;
               break;

            case BTN_RIGHT:
               udev->mouse_r = event->value;
               break;

            case BTN_MIDDLE:
               udev->mouse_m = event->value;
               break;
            default:
               break;
         }
         break;

      case EV_REL:
         switch (event->code)
         {
            case REL_X:
               udev->mouse_x += event->value;
               break;

            case REL_Y:
               udev->mouse_y += event->value;
               break;
            case REL_WHEEL:
               if (event->value == 1)
                  udev->mouse_wu = 1;
               else if (event->value == -1)
                  udev->mouse_wd = 1;
               break;
            case REL_HWHEEL:
               if (event->value == 1)
                  udev->mouse_whu = 1;
               else if (event->value == -1)
                  udev->mouse_whd = 1;
               break;
            default:
               break;
         }
         break;

      default:
         break;
   }
}

static bool udev_input_add_device(udev_input_t *udev,
      const char *devnode, device_handle_cb cb)
{
   int fd;
   udev_input_device_t **tmp;
   udev_input_device_t *device = NULL;
   struct stat st              = {0};

   if (stat(devnode, &st) < 0)
      return false;

   fd = open(devnode, O_RDONLY | O_NONBLOCK);
   if (fd < 0)
      return false;

   device = (udev_input_device_t*)calloc(1, sizeof(*device));
   if (!device)
      goto error;

   device->fd        = fd;
   device->dev       = st.st_dev;
   device->handle_cb = cb;

   strlcpy(device->devnode, devnode, sizeof(device->devnode));

   /* Touchpads report in absolute coords. */
   if (cb == udev_handle_touchpad &&
         (ioctl(fd, EVIOCGABS(ABS_X), &device->state.touchpad.info_x) < 0 ||
          ioctl(fd, EVIOCGABS(ABS_Y), &device->state.touchpad.info_y) < 0))
      goto error;

   tmp = ( udev_input_device_t**)realloc(udev->devices,
         (udev->num_devices + 1) * sizeof(*udev->devices));

   if (!tmp)
      goto error;

   tmp[udev->num_devices++] = device;
   udev->devices            = tmp;

   epoll_add(&udev->epfd, fd, device);

   return true;

error:
   close(fd);
   if (device)
      free(device);

   return false;
}

static void udev_input_remove_device(udev_input_t *udev, const char *devnode)
{
   unsigned i;

   for (i = 0; i < udev->num_devices; i++)
   {
      if (!string_is_equal(devnode, udev->devices[i]->devnode))
         continue;

      close(udev->devices[i]->fd);
      free(udev->devices[i]);
      memmove(udev->devices + i, udev->devices + i + 1,
            (udev->num_devices - (i + 1)) * sizeof(*udev->devices));
      udev->num_devices--;
   }
}

static void udev_input_handle_hotplug(udev_input_t *udev)
{
   const char *devtype      = NULL;
   const char *val_keyboard = NULL;
   const char *val_mouse    = NULL;
   const char *val_touchpad = NULL;
   const char *action       = NULL;
   const char *devnode      = NULL;
   struct udev_device *dev  = udev_monitor_receive_device(udev->monitor);

   if (!dev)
      return;

   val_keyboard  = udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");
   val_mouse     = udev_device_get_property_value(dev, "ID_INPUT_MOUSE");
   val_touchpad  = udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD");
   action        = udev_device_get_action(dev);
   devnode       = udev_device_get_devnode(dev);

   if (val_keyboard && string_is_equal(val_keyboard, "1") && devnode)
      devtype = "keyboard";

   if (val_mouse && string_is_equal(val_mouse, "1") && devnode)
      devtype = "mouse";

   if (val_touchpad && string_is_equal(val_touchpad, "1") && devnode)
      devtype = "touchpad";

   if (!devtype)
      goto end;

   if (string_is_equal(action, "add"))
   {
      device_handle_cb cb      = NULL;
      if (string_is_equal(devtype, "keyboard"))
         cb = udev_handle_keyboard;
      else if (string_is_equal(devtype, "touchpad"))
         cb = udev_handle_touchpad;
      else if (string_is_equal(devtype, "mouse"))
         cb = udev_handle_mouse;

      RARCH_LOG("[udev]: Hotplug add %s: %s.\n", devtype, devnode);
      udev_input_add_device(udev, devnode, cb);
   }
   else if (string_is_equal(action, "remove"))
   {
      RARCH_LOG("[udev]: Hotplug remove %s: %s.\n", devtype, devnode);
      udev_input_remove_device(udev, devnode);
   }

end:
   udev_device_unref(dev);
}

static void udev_input_poll(void *data)
{
   int i, ret;
   struct epoll_event events[32];
   udev_input_t *udev = (udev_input_t*)data;

   if (!udev)
      return;

   udev->mouse_x   = udev->mouse_y   = 0;
   udev->mouse_wu  = udev->mouse_wd  = 0;
   udev->mouse_whu = udev->mouse_whd = 0;

   while (udev->monitor && udev_hotplug_available(udev->monitor))
      udev_input_handle_hotplug(udev);

   ret = epoll_waiting(&udev->epfd, events, ARRAY_SIZE(events), 0);

   for (i = 0; i < ret; i++)
   {
      if (events[i].events & EPOLLIN)
      {
         int j, len;
         struct input_event input_events[32];
         udev_input_device_t *device = (udev_input_device_t*)events[i].data.ptr;

         while ((len = read(device->fd, input_events, sizeof(input_events))) > 0)
         {
            len /= sizeof(*input_events);
            for (j = 0; j < len; j++)
               device->handle_cb(udev, &input_events[j], device);
         }
      }
   }

   if (udev->joypad)
      udev->joypad->poll();
}

static int16_t udev_mouse_state(udev_input_t *udev, unsigned id)
{
   switch (id)
   {
      case RETRO_DEVICE_ID_MOUSE_X:
         return udev->mouse_x;
      case RETRO_DEVICE_ID_MOUSE_Y:
         return udev->mouse_y;
      case RETRO_DEVICE_ID_MOUSE_LEFT:
         return udev->mouse_l;
      case RETRO_DEVICE_ID_MOUSE_RIGHT:
         return udev->mouse_r;
      case RETRO_DEVICE_ID_MOUSE_MIDDLE:
         return udev->mouse_m;
      case RETRO_DEVICE_ID_MOUSE_WHEELUP:
         return udev->mouse_wu;
      case RETRO_DEVICE_ID_MOUSE_WHEELDOWN:
         return udev->mouse_wd;
      case RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP:
         return udev->mouse_whu;
      case RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN:
         return udev->mouse_whd;
   }

   return 0;
}

static int16_t udev_lightgun_state(udev_input_t *udev, unsigned id)
{
   switch (id)
   {
      case RETRO_DEVICE_ID_LIGHTGUN_X:
         return udev->mouse_x;
      case RETRO_DEVICE_ID_LIGHTGUN_Y:
         return udev->mouse_y;
      case RETRO_DEVICE_ID_LIGHTGUN_TRIGGER:
         return udev->mouse_l;
      case RETRO_DEVICE_ID_LIGHTGUN_CURSOR:
         return udev->mouse_m;
      case RETRO_DEVICE_ID_LIGHTGUN_TURBO:
         return udev->mouse_r;
      case RETRO_DEVICE_ID_LIGHTGUN_START:
         return udev->mouse_m && udev->mouse_r; 
      case RETRO_DEVICE_ID_LIGHTGUN_PAUSE:
         return udev->mouse_m && udev->mouse_l; 
   }

   return 0;
}

static int16_t udev_analog_pressed(const struct retro_keybind *binds, unsigned idx, unsigned id)
{
   unsigned id_minus     = 0;
   unsigned id_plus      = 0;
   int16_t pressed_minus = 0;
   int16_t pressed_plus  = 0;

   input_conv_analog_id_to_bind_id(idx, id, &id_minus, &id_plus);

   if (binds && binds[id_minus].valid && udev_input_is_pressed(binds, id_minus))
      pressed_minus = -0x7fff;
   if (binds && binds[id_plus].valid && udev_input_is_pressed(binds, id_plus))
      pressed_plus = 0x7fff;

   return pressed_plus + pressed_minus;
}

static int16_t udev_pointer_state(udev_input_t *udev,
      unsigned idx, unsigned id, bool screen)
{
   bool inside              = false;
   struct video_viewport vp = {0};
   int16_t res_x = 0, res_y = 0, res_screen_x = 0, res_screen_y = 0;

   if (!(video_driver_translate_coord_viewport_wrap(&vp, udev->mouse_x, udev->mouse_y,
         &res_x, &res_y, &res_screen_x, &res_screen_y)))
      return 0;

   if (screen)
   {
      res_x = res_screen_x;
      res_y = res_screen_y;
   }

   inside = (res_x >= -0x7fff) && (res_y >= -0x7fff);

   if (!inside)
      return 0;

   switch (id)
   {
      case RETRO_DEVICE_ID_POINTER_X:
         return res_x;
      case RETRO_DEVICE_ID_POINTER_Y:
         return res_y;
      case RETRO_DEVICE_ID_POINTER_PRESSED:
         return udev->mouse_l;
   }

   return 0;
}

static int16_t udev_input_state(void *data,
      rarch_joypad_info_t joypad_info,
      const struct retro_keybind **binds,
      unsigned port, unsigned device, unsigned idx, unsigned id)
{
   int16_t ret;
   udev_input_t *udev         = (udev_input_t*)data;

   if (!udev)
      return 0;

   switch (device)
   {
      case RETRO_DEVICE_JOYPAD:
         return udev_input_is_pressed(binds[port], id) ||
            input_joypad_pressed(udev->joypad, joypad_info, port, binds[port], id);
      case RETRO_DEVICE_ANALOG:
         ret = udev_analog_pressed(binds[port], idx, id);
         if (!ret && binds[port])
            ret = input_joypad_analog(udev->joypad, joypad_info, port, idx, id, binds[port]);
         return ret;

      case RETRO_DEVICE_KEYBOARD:
         return udev_input_state_kb(data, binds, port, device, idx, id);
      case RETRO_DEVICE_MOUSE:
         return udev_mouse_state(udev, id);

      case RETRO_DEVICE_POINTER:
      case RARCH_DEVICE_POINTER_SCREEN:
         if (idx == 0)
            return udev_pointer_state(udev, idx, id,
                  device == RARCH_DEVICE_POINTER_SCREEN);
         break;
      case RETRO_DEVICE_LIGHTGUN:
         return udev_lightgun_state(udev, id);
   }

   return 0;
}

static bool udev_input_meta_key_pressed(void *data, int key)
{
   return false;
}

static void udev_input_free(void *data)
{
   unsigned i;
   udev_input_t *udev = (udev_input_t*)data;

   if (!data || !udev)
      return;

   if (udev->joypad)
      udev->joypad->destroy();

   epoll_free(&udev->epfd);

   for (i = 0; i < udev->num_devices; i++)
   {
      close(udev->devices[i]->fd);
      free(udev->devices[i]);
   }
   free(udev->devices);

   if (udev->monitor)
      udev_monitor_unref(udev->monitor);
   if (udev->udev)
      udev_unref(udev->udev);

   udev_input_kb_free();

   free(udev);
}

static bool open_devices(udev_input_t *udev, const char *type, device_handle_cb cb)
{
   struct udev_list_entry     *devs = NULL;
   struct udev_list_entry     *item = NULL;
   struct udev_enumerate *enumerate = udev_enumerate_new(udev->udev);

   if (!enumerate)
      return false;

   udev_enumerate_add_match_property(enumerate, type, "1");
   udev_enumerate_scan_devices(enumerate);
   devs = udev_enumerate_get_list_entry(enumerate);

   for (item = devs; item; item = udev_list_entry_get_next(item))
   {
      const char *name        = udev_list_entry_get_name(item);

      /* Get the filename of the /sys entry for the device
       * and create a udev_device object (dev) representing it. */
      struct udev_device *dev = udev_device_new_from_syspath(udev->udev, name);
      const char *devnode     = udev_device_get_devnode(dev);

      if (devnode)
      {
         int fd = open(devnode, O_RDONLY | O_NONBLOCK);

         RARCH_LOG("[udev] Adding device %s as type %s.\n", devnode, type);
         if (!udev_input_add_device(udev, devnode, cb))
            RARCH_ERR("[udev] Failed to open device: %s (%s).\n", devnode, strerror(errno));
         close(fd);
      }

      udev_device_unref(dev);
   }

   udev_enumerate_unref(enumerate);
   return true;
}

static void *udev_input_init(const char *joypad_driver)
{
   udev_input_t *udev   = (udev_input_t*)calloc(1, sizeof(*udev));

   if (!udev)
      return NULL;

   udev->udev = udev_new();
   if (!udev->udev)
   {
      RARCH_ERR("Failed to create udev handle.\n");
      goto error;
   }

   udev->monitor = udev_monitor_new_from_netlink(udev->udev, "udev");
   if (udev->monitor)
   {
      udev_monitor_filter_add_match_subsystem_devtype(udev->monitor, "input", NULL);
      udev_monitor_enable_receiving(udev->monitor);
   }

#ifdef HAVE_XKBCOMMON
   if (init_xkb(-1, 0) == -1)
      goto error;
#endif

   if (!epoll_new(&udev->epfd))
   {
      RARCH_ERR("Failed to create epoll FD.\n");
      goto error;
   }

   if (!open_devices(udev, "ID_INPUT_KEYBOARD", udev_handle_keyboard))
   {
      RARCH_ERR("Failed to open keyboard.\n");
      goto error;
   }

   if (!open_devices(udev, "ID_INPUT_MOUSE", udev_handle_mouse))
   {
      RARCH_ERR("Failed to open mouse.\n");
      goto error;
   }

   if (!open_devices(udev, "ID_INPUT_TOUCHPAD", udev_handle_touchpad))
   {
      RARCH_ERR("Failed to open touchpads.\n");
      goto error;
   }

   /* If using KMS and we forgot this, 
    * we could lock ourselves out completely. */
   if (!udev->num_devices)
      RARCH_WARN("[udev]: Couldn't open any keyboard, mouse or touchpad. Are permissions set correctly for /dev/input/event*?\n");

   udev->joypad = input_joypad_init_driver(joypad_driver, udev);
   input_keymaps_init_keyboard_lut(rarch_key_map_linux);

   linux_terminal_disable_input();

   return udev;

error:
   udev_input_free(udev);
   return NULL;
}

static uint64_t udev_input_get_capabilities(void *data)
{
   (void)data;

   return
      (1 << RETRO_DEVICE_JOYPAD)   |
      (1 << RETRO_DEVICE_ANALOG)   |
      (1 << RETRO_DEVICE_KEYBOARD) |
      (1 << RETRO_DEVICE_MOUSE)    |
      (1 << RETRO_DEVICE_LIGHTGUN);
}

static void udev_input_grab_mouse(void *data, bool state)
{
   /* Dummy for now. Might be useful in the future. */
   (void)data;
   (void)state;
}

static bool udev_input_set_rumble(void *data, unsigned port, enum retro_rumble_effect effect, uint16_t strength)
{
   udev_input_t *udev = (udev_input_t*)data;
   if (udev && udev->joypad)
      return input_joypad_set_rumble(udev->joypad, port, effect, strength);
   return false;
}

static const input_device_driver_t *udev_input_get_joypad_driver(void *data)
{
   udev_input_t *udev = (udev_input_t*)data;
   if (!udev)
      return NULL;
   return udev->joypad;
}

static bool udev_input_keyboard_mapping_is_blocked(void *data)
{
   udev_input_t *udev = (udev_input_t*)data;
   if (!udev)
      return false;
   return udev->blocked;
}

static void udev_input_keyboard_mapping_set_block(void *data, bool value)
{
   udev_input_t *udev = (udev_input_t*)data;
   if (!udev)
      return;
   udev->blocked = value;
}

input_driver_t input_udev = {
   udev_input_init,
   udev_input_poll,
   udev_input_state,
   udev_input_meta_key_pressed,
   udev_input_free,
   NULL,
   NULL,
   udev_input_get_capabilities,
   "udev",
   udev_input_grab_mouse,
   linux_terminal_grab_stdin,
   udev_input_set_rumble,
   udev_input_get_joypad_driver,
   NULL,
   udev_input_keyboard_mapping_is_blocked,
   udev_input_keyboard_mapping_set_block,
};
