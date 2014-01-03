#ifndef __SE_STD_DATATYPES__
#define __SE_STD_DATATYPES__

typedef			 __int8		Byte;
typedef	signed	 __int8		Int8;
typedef unsigned __int8		UInt8;
typedef	signed	 __int16	Int16;
typedef unsigned __int16	UInt16;
typedef	signed	 __int32	Int32;
typedef unsigned __int32	UInt32;
typedef	signed	 __int64	Int64;
typedef unsigned __int64	UInt64;
typedef			 float		Float32;
typedef			 double		Float64;

#ifdef SE_DOUBLE_PRECISION
typedef			 double		Real;
#else
typedef			 float		Real;
#endif

#endif
