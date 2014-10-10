#pragma once

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

#define L16(x)  (USHORT)(x&0x0000ffff)
#define H16(x)  (USHORT)((x&0xffff0000)>>16)
#define L32(x)  (uint32)(x&0x00000000ffffffff)
#define H32(x)  (uint32)((x&0xffffffff00000000)>>32)

