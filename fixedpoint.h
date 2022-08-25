#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// https://chummersone.github.io/qformat.html


// 0000 0000 0000 0000. 0000 0000 0000 0000
// 16:16 fixed number, +/- 32768
typedef int32_t q16_t;
#define int_to_q16(x) ((q16_t)((x) << 16))
#define float_to_q16(x) ((q16_t)((x) * (float)(1 << 16)))
#define q16_to_float(x) ((x) / (float)(1 << 16))
#define q16_round(x) ((x + 0x00008000) & 0xffff0000)
#define q16_to_int(x) ((x) >> 16)


// All NES APU mathematics are dealing with float number within range 0.0-1.0. Use 3:29 fixed point
// 000.0 0000 0000 0000 0000 0000 0000 0000
// 3:29 fixed number, +/- 4
typedef int32_t q29_t;
#define float_to_q29(x) ((q29_t)((x) * (float)(1 << 29)))
#define q29_to_float(x) ((x) / (float)(1 << 29))
#define q29_mul(x, y)   (((x)>>15)*((y)>>14))
// Map q29 formatted (0-1) sample to signed 16 bit interger sound sample
#define q29_to_sample(x)   (int16_t)(((x) - 268435456) >> 13)


#ifdef __cplusplus
}
#endif