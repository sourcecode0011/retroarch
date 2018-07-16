#ifndef CTR_COMMON_H__
#define CTR_COMMON_H__

#include <retro_inline.h>

#define COLOR_ABGR(r, g, b, a) (((unsigned)(a) << 24) | ((b) << 16) | ((g) << 8) | ((r) << 0))

#define CTR_TOP_FRAMEBUFFER_WIDTH   400
#define CTR_TOP_FRAMEBUFFER_HEIGHT  240

extern const u8 ctr_sprite_shbin[];
extern const u32 ctr_sprite_shbin_size;

typedef struct
{
   float v;
   float u;
   float y;
   float x;
} ctr_scale_vector_t;

typedef struct
{
   s16 x0, y0, x1, y1;
   s16 u0, v0, u1, v1;
} ctr_vertex_t;

typedef enum
{
   CTR_VIDEO_MODE_NORMAL,
   CTR_VIDEO_MODE_800x240,
   CTR_VIDEO_MODE_400x240,
   CTR_VIDEO_MODE_3D
}ctr_video_mode_enum;

typedef struct ctr_video
{
   struct
   {
      struct
      {
         void* left;
         void* right;
      }top;
   }drawbuffers;
   void* depthbuffer;

   struct
   {
      uint32_t* display_list;
      int display_list_size;
      void* texture_linear;
      void* texture_swizzled;
      int texture_width;
      int texture_height;
      ctr_scale_vector_t scale_vector;
      ctr_vertex_t* frame_coords;
   }menu;

   uint32_t* display_list;
   int display_list_size;
   void* texture_linear;
   void* texture_swizzled;
   int texture_width;
   int texture_height;

   ctr_scale_vector_t scale_vector;
   ctr_vertex_t* frame_coords;

   DVLB_s*         dvlb;
   shaderProgram_s shader;

   video_viewport_t vp;

   bool rgb32;
   bool vsync;
   bool smooth;
   bool menu_texture_enable;
   bool menu_texture_frame_enable;
   unsigned rotation;
   bool keep_aspect;
   bool should_resize;
   bool lcd_buttom_on;
   bool msg_rendering_enabled;

   void* empty_framebuffer;

   aptHookCookie lcd_aptHook;
   ctr_video_mode_enum video_mode;
   int current_buffer_top;

   bool p3d_event_pending;
   bool ppf_event_pending;
   volatile bool vsync_event_pending;

   struct
   {
      ctr_vertex_t* buffer;
      ctr_vertex_t* current;
      int size;
   }vertex_cache;

} ctr_video_t;

typedef struct ctr_texture
{
   int width;
   int height;
   int active_width;
   int active_height;

   enum texture_filter_type type;
   void* data;
} ctr_texture_t;

static INLINE void ctr_set_scale_vector(ctr_scale_vector_t* vec,
      int viewport_width, int viewport_height,
      int texture_width, int texture_height)
{
   vec->x = -2.0 / viewport_width;
   vec->y = -2.0 / viewport_height;
   vec->u =  1.0 / texture_width;
   vec->v = -1.0 / texture_height;
}


#endif // CTR_COMMON_H__
