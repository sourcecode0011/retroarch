/* CpuArch.h -- CPU specific code
2010-10-26: Igor Pavlov : Public domain */

#ifndef __CPU_ARCH_H
#define __CPU_ARCH_H

#include "Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
MY_CPU_LE_UNALIGN means that CPU is LITTLE ENDIAN and CPU supports unaligned memory accesses.
If MY_CPU_LE_UNALIGN is not defined, we don't know about these properties of platform.
*/

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
#define MY_CPU_AMD64
#endif

#if defined(MY_CPU_AMD64) || defined(_M_IA64)
#define MY_CPU_64BIT
#endif

#if defined(_M_IX86) || defined(__i386__)
#define MY_CPU_X86
#endif

#if defined(MY_CPU_X86) || defined(MY_CPU_AMD64)
#define MY_CPU_X86_OR_AMD64
#endif

#if defined(MY_CPU_X86) || defined(_M_ARM)
#define MY_CPU_32BIT
#endif

#if defined(_WIN32) && defined(_M_ARM)
#define MY_CPU_ARM_LE
#endif

#if defined(_WIN32) && defined(_M_IA64)
#define MY_CPU_IA64_LE
#endif

#if defined(MY_CPU_X86_OR_AMD64)
#define MY_CPU_LE_UNALIGN
#endif

#ifdef MY_CPU_LE_UNALIGN

#define GetUi16(p) (*(const uint16_t *)(p))
#define GetUi32(p) (*(const uint32_t *)(p))
#define GetUi64(p) (*(const uint64_t *)(p))
#define SetUi16(p, d) *(uint16_t *)(p) = (d);
#define SetUi32(p, d) *(uint32_t *)(p) = (d);
#define SetUi64(p, d) *(uint64_t *)(p) = (d);

#else

#define GetUi16(p) (((const uint8_t *)(p))[0] | ((uint16_t)((const uint8_t *)(p))[1] << 8))

#define GetUi32(p) ( \
             ((const uint8_t *)(p))[0]        | \
    ((uint32_t)((const uint8_t *)(p))[1] <<  8) | \
    ((uint32_t)((const uint8_t *)(p))[2] << 16) | \
    ((uint32_t)((const uint8_t *)(p))[3] << 24))

#define GetUi64(p) (GetUi32(p) | ((uint64_t)GetUi32(((const uint8_t *)(p)) + 4) << 32))

#define SetUi16(p, d) { uint32_t _x_ = (d); \
    ((uint8_t *)(p))[0] = (uint8_t)_x_; \
    ((uint8_t *)(p))[1] = (uint8_t)(_x_ >> 8); }

#define SetUi32(p, d) { uint32_t _x_ = (d); \
    ((uint8_t *)(p))[0] = (uint8_t)_x_; \
    ((uint8_t *)(p))[1] = (uint8_t)(_x_ >> 8); \
    ((uint8_t *)(p))[2] = (uint8_t)(_x_ >> 16); \
    ((uint8_t *)(p))[3] = (uint8_t)(_x_ >> 24); }

#define SetUi64(p, d) { uint64_t _x64_ = (d); \
    SetUi32(p, (uint32_t)_x64_); \
    SetUi32(((uint8_t *)(p)) + 4, (uint32_t)(_x64_ >> 32)); }

#endif

#if defined(MY_CPU_LE_UNALIGN) && defined(_WIN64) && (_MSC_VER >= 1300)

#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)
#define GetBe32(p) _byteswap_ulong(*(const uint32_t *)(const uint8_t *)(p))
#define GetBe64(p) _byteswap_uint64(*(const uint64_t *)(const uint8_t *)(p))

#else

#define GetBe32(p) ( \
    ((uint32_t)((const uint8_t *)(p))[0] << 24) | \
    ((uint32_t)((const uint8_t *)(p))[1] << 16) | \
    ((uint32_t)((const uint8_t *)(p))[2] <<  8) | \
             ((const uint8_t *)(p))[3] )

#define GetBe64(p) (((uint64_t)GetBe32(p) << 32) | GetBe32(((const uint8_t *)(p)) + 4))

#endif

#define GetBe16(p) (((uint16_t)((const uint8_t *)(p))[0] << 8) | ((const uint8_t *)(p))[1])

#ifdef __cplusplus
}
#endif

#endif
