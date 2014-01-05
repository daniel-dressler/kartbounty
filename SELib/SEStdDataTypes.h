#ifndef __SE_STD_DATATYPES__
#define __SE_STD_DATATYPES__

#include <stdint.h>
typedef	int8_t   Byte;
typedef	int8_t   Int8;
typedef uint8_t	 UInt8;
typedef	int16_t  Int16;
typedef uint16_t UInt16;
typedef	int32_t  Int32;
typedef uint32_t UInt32;
typedef	int64_t  Int64;
typedef uint64_t UInt64;
// IEEE 754 defines the size of floats
// so there are no float32_t's
typedef	float    Float32;
typedef	double   Float64;

#ifdef SE_DOUBLE_PRECISION
typedef			 double		Real;
#else
typedef			 float		Real;
#endif

#endif
