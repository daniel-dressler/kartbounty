#ifndef __SE_MATH__
#define __SE_MATH__

#include "SEStdDataTypes.h"

// Math Includes
#include <math.h>

#define CLOSE		0.0001f

// Math Macros
#ifdef SE_DOUBLE_PRECISION

	#define FLTMAX		1E+37
	#define PI			3.14159265358979

	#define MOD( x )		fmod( x )
	#define ABS( x )		abs( x )
	#define SQRT( x )		sqrt( x )
	#define POW( x, y )		pow( (Float64)x, (Float64)y )
	#define FLOOR( x )		floor( x )
	#define CEIL( x )		ceil( x )
	#define SIN( x )		sin( x )
	#define COS( x )		cos( x )
	#define TAN( x )		tan( x )
	#define ASIN( x )		asin( x )
	#define ACOS( x )		acos( x )
	#define ATAN( x )		atan( x )
	#define ATAN2( y, x )	atan2( y, x )

	#define FRAC( x )		( x - (Int64)x )

#else

	#define FLTMAX		1E+37f
	#define PI			3.141592f

	#define MOD( x )		fmod( x )
	#define ABS( x )		fabs( x )
	#define SQRT( x )		sqrtf( x )
	#define POW( x, y )		powf( x, y )
	#define FLOOR( x )		floorf( x )
	#define CEIL( x )		ceilf( x )
	#define SIN( x )		sinf( x )
	#define COS( x )		cosf( x )
	#define TAN( x )		tanf( x )
	#define ASIN( x )		asinf( x )
	#define ACOS( x )		acosf( x )
	#define ATAN( x )		atanf( x )
	#define ATAN2( y, x )	atan2f( y, x )

	#define FRAC( x )		( x - (Int32)x )

#endif

#define DEGTORAD( x ) ( x * ( PI / 180.0f ) )
#define RADTODEG( x ) ( x * ( 180.0f / PI ) )
#define SIGN( x )	( x < 0.0f ? -1.0f : 1.0f )

#define IABS( x )			abs( x )
#define MAX( a, b )			(a>b?a:b)
#define MIN( a, b )			(a<b?a:b)
#define TRIMAX( a, b, c )	(a>b?a>c?a:c:b>c?b:c)
#define TRIMIN( a, b, c )	(a<b?a<c?a:c:b<c?b:c)

inline Int32 IsZero( Real a, Real fClose = CLOSE )				{ return ABS( a ) <= fClose; }
inline Int32 IsClose( Real a, Real b, Real fClose = CLOSE )		{ return ABS( a - b ) <= fClose; }
inline Real Round( Real r )			{ return ( r > 0.0f ) ? FLOOR( r + 0.5f ) : CEIL( r - 0.5f ); }
inline Real RoundUp( Real r )		{ return ( r > 0.0f ) ? CEIL( r ) : FLOOR( r ); }
inline Real RoundDown( Real r )		{ return ( r > 0.0f ) ? FLOOR( r ) : CEIL( r ); }

inline Real Lerp( Real a, Real b, Real t )				{ return a * ( 1.0f - t ) + b * t; }
inline Real Clamp( Real x, Real min = 0, Real max = 1 )	{ if( x > max ) return max; if( x < min ) return min; return x; }

inline Real SquareRoot( Real x )	{ return SQRT( x ); }
inline Real CubeRoot( Real x )		{ return x < 0.0f ? -POW( -x, 1.0f / 3.0f ) : POW( x, 1.0f / 3.0f ); }

inline UInt32 NextPower2( UInt32 v )
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline Byte HexCharToInt( Byte hex )
{
	if( hex >= 97 && hex <= 102 )
		return hex - 87;
	else if( hex >= 65 && hex <= 70 )
		return hex - 55;
	else if( hex >= 48 && hex <= 57 )
		return hex - 48;
	return -1;
}

namespace Root
{
	Int32 Linear( Real x, Real k, Real* root );
	Int32 Quadratic( Real x2, Real x, Real k, Real roots[2] );
	Int32 Cubic( Real x3, Real x2, Real x, Real k, Real roots[3] );
	Int32 Quartic( Real x4, Real x3, Real x2, Real x, Real k, Real roots[4] );
}

#endif
