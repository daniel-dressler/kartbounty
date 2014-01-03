#include "SEStdMath.h"

namespace Root
{
	Int32 Linear( Real x, Real k, Real* root )
	{
		if( IsZero( x ) )
			return 0;

		*root = -k / x;
		return 1;
	}

	Int32 Quadratic( Real x2, Real x, Real k, Real roots[2] )
	{
		if( IsZero( x2 ) )
			return Linear( x, k, roots );

		Real d = x * x - 4 * x2 * k;
		if( IsClose( d, 0.0f ) )
		{
			roots[0] = roots[1] = -x / ( 2 * x2 );
			return 1;	// One Root
		}
		else if( d > 0.0f )
		{
			Real fSqrD = SQRT( d );
			Real f2A = 2.0f * x2;
			roots[0] = ( -x + fSqrD ) / f2A;
			roots[1] = ( -x - fSqrD ) / f2A;
			return 2;	// Two Roots
		}
		return 0;
	}

	Int32 Cubic( Real x3, Real x2, Real x, Real k, Real roots[3] )
	{
		if( IsZero( x3 ) )
			return Quadratic( x2, x, k, roots );

		Real a, b, c, d, a2, p, q, p3, s;

		// Normalize
		a = x2 / x3;
		b = x  / x3;
		c = k  / x3;

		// Reduction to a depressed cubic
		a2 = a * a;
		p = 1.0f / 3.0f * ( -1.0f / 3.0f * a2 + b );
		q = 1.0f / 2.0f * ( 2.0f / 27.0f * a * a2 - 1.0f / 3.0f * a * b + c );
		s = 1.0f / 3.0f * a;

		// Cardano's method
		p3 = p * p * p;
		d = q * q + p3;

		if( IsZero( d ) )
		{
			if( IsZero( q ) )
			{
				roots[0] = -s;
				return 1;			
			}
			else
			{
				Real u = CubeRoot( -q );
				roots[0] = 2.0f * u - s;
				roots[1] = -u - s;
				return 2;
			}
		}

		if( d < 0.0f )
		{
			Real phi = 1.0f / 3.0f * ACOS( -q / SQRT( -p3 ) );
			Real t = 2.0f * SQRT( -p );
			roots[0] =  t * COS( phi ) - s;
			roots[1] = -t * COS( phi + PI / 3.0f ) - s;
			roots[2] = -t * COS( phi - PI / 3.0f ) - s;
			return 3;
		}

		Real u = CubeRoot( SQRT( d ) + ABS( q ) );
		roots[0] = ( q > 0.0f ? - u + p / u : u - p / u ) - s;
		return 1;
	}

	Int32 Quartic( Real x4, Real x3, Real x2, Real x, Real k, Real roots[4] )
	{
		if( IsZero( x4 ) )
			return Cubic( x3, x2, x, k, roots );
		
		Int32 n;
		Real a, b, c, d, s, a2, p, q, r, z, u, v;

		// Normalize
		a = x3 / x4;
		b = x2 / x4;
		c = x  / x4;
		d = k  / x4;

		// Reduction to a depressed quartic
		a2 = a * a;
		p = -3.0f / 8.0f * a2 + b;
		q = 1.0f / 8.0f * a2 * a - 1.0f / 2.0f * a * b + c;
		r = -3.0f / 256.0f * a2 * a2 + 1.0f / 16.0f * a2 * b - 1.0f / 4.0f * a * c + d;

		if( IsZero( r ) )
		{
			n = Cubic( 1.0f, 0.0f, p, q, roots );
			roots[n++] = 0.0f;
		}
		else
		{
			Cubic( 1.0f, -1.0f / 2.0f * p, -r, 1.0f / 2.0f * r * p - 1.0f / 8.0f * q * q, roots );

			z = roots[0];
			u = z * z - r;
			v = 2.0f * z - p;

			if( u < -CLOSE ) return 0;
			else u = u > CLOSE ? SQRT( u ) : 0.0f;

			if( v < -CLOSE ) return 0;
			else v = v > CLOSE ? SQRT( v ) : 0.0f;

			n  = Quadratic( 1.0f, q < 0.0f ? -v : v, z - u, roots );
			n += Quadratic( 1.0f, q < 0.0f ? v : -v, z + u, roots + n );
		}

		s = 1.0f / 4.0f * a;
		for( Int32 i = 0; i < n; i++ )
			roots[i] -= s;

		return n;
	}
}