/* 7zBuf2.c -- uint8_t Buffer
   2008-10-04 : Igor Pavlov : Public domain */

#include <stdint.h>
#include <string.h>
#include "7zBuf.h"

void DynBuf_Construct(CDynBuf *p)
{
   p->data = 0;
   p->size = 0;
   p->pos = 0;
}

void DynBuf_SeekToBeg(CDynBuf *p)
{
   p->pos = 0;
}

int DynBuf_Write(CDynBuf *p, const uint8_t *buf, size_t size, ISzAlloc *alloc)
{
   if (size > p->size - p->pos)
   {
      size_t current_size = p->pos + size;
      size_t newSize = (current_size) + (current_size / 4);
      uint8_t *data = (uint8_t *)alloc->Alloc(alloc, newSize);

      if (data == 0)
         return 0;

      p->size = newSize;
      memcpy(data, p->data, p->pos);
      alloc->Free(alloc, p->data);
      p->data = data;
   }
   memcpy(p->data + p->pos, buf, size);
   p->pos += size;
   return 1;
}

void DynBuf_Free(CDynBuf *p, ISzAlloc *alloc)
{
   alloc->Free(alloc, p->data);
   p->data = 0;
   p->size = 0;
   p->pos = 0;
}
