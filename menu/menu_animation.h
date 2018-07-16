/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2014-2016 - Jean-André Santoni
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

#ifndef _MENU_ANIMATION_H
#define _MENU_ANIMATION_H

#include <stdint.h>
#include <stdlib.h>

#include <boolean.h>
#include <retro_common_api.h>

RETRO_BEGIN_DECLS

typedef float (*easing_cb) (float, float, float, float);
typedef void  (*tween_cb)  (void);

enum menu_animation_ctl_state
{
   MENU_ANIMATION_CTL_NONE = 0,
   MENU_ANIMATION_CTL_DEINIT,
   MENU_ANIMATION_CTL_CLEAR_ACTIVE,
   MENU_ANIMATION_CTL_SET_ACTIVE,
   MENU_ANIMATION_CTL_DELTA_TIME,
   MENU_ANIMATION_CTL_KILL_BY_TAG,
   MENU_ANIMATION_CTL_KILL_BY_SUBJECT
};

enum menu_animation_easing_type
{
   /* Linear */
   EASING_LINEAR    = 0,
   /* Quad */
   EASING_IN_QUAD,
   EASING_OUT_QUAD,
   EASING_IN_OUT_QUAD,
   EASING_OUT_IN_QUAD,
   /* Cubic */
   EASING_IN_CUBIC,
   EASING_OUT_CUBIC,
   EASING_IN_OUT_CUBIC,
   EASING_OUT_IN_CUBIC,
   /* Quart */
   EASING_IN_QUART,
   EASING_OUT_QUART,
   EASING_IN_OUT_QUART,
   EASING_OUT_IN_QUART,
   /* Quint */
   EASING_IN_QUINT,
   EASING_OUT_QUINT,
   EASING_IN_OUT_QUINT,
   EASING_OUT_IN_QUINT,
   /* Sine */
   EASING_IN_SINE,
   EASING_OUT_SINE,
   EASING_IN_OUT_SINE,
   EASING_OUT_IN_SINE,
   /* Expo */
   EASING_IN_EXPO,
   EASING_OUT_EXPO,
   EASING_IN_OUT_EXPO,
   EASING_OUT_IN_EXPO,
   /* Circ */
   EASING_IN_CIRC,
   EASING_OUT_CIRC,
   EASING_IN_OUT_CIRC,
   EASING_OUT_IN_CIRC,
   /* Bounce */
   EASING_IN_BOUNCE,
   EASING_OUT_BOUNCE,
   EASING_IN_OUT_BOUNCE,
   EASING_OUT_IN_BOUNCE
};

typedef struct menu_animation_ctx_delta
{
   float current;
   float ideal;
} menu_animation_ctx_delta_t;

typedef struct menu_animation_ctx_tag
{
   int id;
} menu_animation_ctx_tag_t;

typedef struct menu_animation_ctx_subject
{
   size_t count;
   const void *data;
} menu_animation_ctx_subject_t;

typedef struct menu_animation_ctx_entry
{
   float duration;
   float target_value;
   float *subject;
   enum menu_animation_easing_type easing_enum;
   int tag;
   tween_cb cb;
} menu_animation_ctx_entry_t;

typedef struct menu_animation_ctx_ticker
{
   char *s;
   size_t len;
   uint64_t idx;
   const char *str;
   bool selected;
} menu_animation_ctx_ticker_t;

bool menu_animation_update(float delta_time);

bool menu_animation_get_ideal_delta_time(menu_animation_ctx_delta_t *delta);

bool menu_animation_ticker(const menu_animation_ctx_ticker_t *ticker);

void menu_animation_update_time(bool timedate_enable);

bool menu_animation_is_active(void);

bool menu_animation_push(menu_animation_ctx_entry_t *entry);

bool menu_animation_ctl(enum menu_animation_ctl_state state, void *data);

RETRO_END_DECLS

#endif
