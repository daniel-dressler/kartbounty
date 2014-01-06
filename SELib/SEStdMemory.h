#ifndef __SE_STD_MEMORY__
#define __SE_STD_MEMORY__

// Memory Includes
#ifdef _WIN32
	#include <memory.h>
#else
	#include <string.h> // for memset
#endif

// Memory Macros
#define THREADDATA					__declspec(thread)
#define MEMCPY( dest, src, size )	memcpy( dest, src, size )
#define MEMSET( x, v, s )			memset( x, v, s )
#define MALLOC( size )				malloc( size )
#define REALLOC( ptr, size )		realloc( ptr, size )
#define OFFSETOF(type, field)		((unsigned long) &(((type *) 0)->field))

#endif
