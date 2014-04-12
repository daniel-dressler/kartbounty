#ifndef __SE_VECTOR__
#define __SE_VECTOR__

typedef struct Vector2
{
	Real x;
	Real y;

	inline		 void  Zero()					{ x = y = 0.0f; }
	inline const Int32 IsZero()					{ return x == 0.0f && y == 0.0f ? 1 : 0; }
	inline const Real  Length() const			{ return SQRT( x * x + y * y ); }
	inline const Real  SquaredLength() const	{ return x * x + y * y; }

	Vector2& Transform( const Matrix3x3& mat );

	Vector2& Rotate( const Real fAngle )
	{
		Real fSinTheta = SIN( fAngle );
		Real fCosTheta = COS( fAngle );
		Real fXTemp = x;
		x = ( x * fCosTheta ) - ( y * fSinTheta );
		y = ( fXTemp * fSinTheta ) + ( y * fCosTheta );
		return *this;
	}

	inline static const Real Dot( const Vector2& fst,  const Vector2& snd )
	{
		return ( fst.x * snd.x ) + ( fst.y * snd.y );
	}

	inline static const Real Det( const Vector2& fst,  const Vector2& snd )
	{
		return ( fst.x * snd.y ) - ( fst.y * snd.x );
	}

	inline const Int32 IsClose( const Vector2& other, const Real fClose = CLOSE ) const
	{
		return ( Vector2( *this - other ).Length() <= fClose );
	}

	inline Vector2& Normalize()
	{
		Real fLength = SquaredLength();
		if( fLength != 0.0f ) 
			*this /= SQRT( fLength );
		return *this;
	}

	inline const Vector2 GetNormal() const
	{
		Vector2 v = *this;
		return v.Normalize();
	}

	inline const Int32 Between( const Vector2& vA, const Vector2& vB ) const
	{
		return ( ( x >= vA.x && x <= vB.x ) || ( x >= vB.x && x <= vA.x ) ) &&
			   ( ( y >= vA.y && y <= vB.y ) || ( y >= vB.y && y <= vA.y ) ) ?
			   1 : 0;
	}

	inline Vector2& operator*=( const Real scaler ) 
	{
		x *= scaler;
		y *= scaler;
		return *this;
	}

	inline Vector2& operator/=( const Real scaler ) 
	{
		x /= scaler;
		y /= scaler;
		return *this;
	}

	inline Vector2& operator+=( const Vector2 &other ) 
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	inline const Vector2 operator+( const Vector2 &other ) const
	{
		Vector2 result = *this;
		result += other;
		return result;
	}

	inline Vector2& operator-=( const Vector2 &other ) 
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	inline const Vector2 operator-( const Vector2 &other ) const
	{
		Vector2 result = *this;
		result -= other;
		return result;
	}

	inline const Vector2 operator-() const
	{
		Vector2 result;
		result.x = -x;
		result.y = -y;
		return result;
	}

	inline operator Real* () 					{ return (Real*)this; }
	inline operator const Real* () const		{ return (Real*)this; }
	inline Real& operator[]( const int nIndex )	{ return ((Real*)this)[nIndex]; }

	inline Vector2 xy() const	{ return Vector2( x, y ); }
	inline Vector2 yx() const	{ return Vector2( y, x ); }

	#ifdef __D3DX10_H__
	inline operator D3DXVECTOR2&() const		{ return *(D3DXVECTOR2*)this; }
	#endif

	inline Vector2() { x = y = 0; }
	Vector2( const Real ix, const Real iy )
	{
		x = ix;
		y = iy;
	}

} Real2, Vector2, Point2;

typedef struct Vector3
{
	Real x;
	Real y;
	Real z;

	inline void Zero()
	{
		x = y = z = 0.0f;
	}

	Vector3& Transform( const Quaternion& q );
	Vector3& Transform( const Matrix3x3& mat );
	Vector3& Transform( const Matrix4x4& mat );

	Vector3& RotateXY( const Real fAngle )
	{
		Real fSinTheta = SIN( fAngle );
		Real fCosTheta = COS( fAngle );
		Real fXTemp = x;
		x = ( x * fCosTheta ) - ( y * fSinTheta );
		y = ( fXTemp * fSinTheta ) + ( y * fCosTheta );
		return *this;
	}

	Vector3& RotateXZ( const Real fAngle )
	{
		Real fSinTheta = SIN( fAngle );
		Real fCosTheta = COS( fAngle );
		Real fXTemp = x;
		x = ( x * fCosTheta ) - ( z * fSinTheta );
		z = ( fXTemp * fSinTheta ) + ( z * fCosTheta );
		return *this;
	}

	Vector3& RotateYZ( const Real fAngle )
	{
		Real fSinTheta = SIN( fAngle );
		Real fCosTheta = COS( fAngle );
		Real fYTemp = y;
		y = ( y * fCosTheta ) - ( z * fSinTheta );
		z = ( fYTemp * fSinTheta ) + ( z * fCosTheta );
		return *this;
	}

	static const Vector3 Cross( const Vector3& fst,  const Vector3& snd )
	{
		Vector3 result;
		result.x = fst.y * snd.z - fst.z * snd.y;
		result.y = fst.z * snd.x - fst.x * snd.z;
		result.z = fst.x * snd.y - fst.y * snd.x;
		return result;
	}

	inline static const Real Dot( const Vector3& fst,  const Vector3& snd )
	{
		return ( fst.x * snd.x ) + ( fst.y * snd.y ) + ( fst.z * snd.z );
	}

	static const Vector3 Lerp( const Vector3& fst, const Vector3& snd, const Real s )
	{
		Vector3 result;
		result.x = ::Lerp( fst.x, snd.x, s );
		result.y = ::Lerp( fst.y, snd.y, s );
		result.z = ::Lerp( fst.z, snd.z, s );
		return result;
	}

	inline const Real Length() const
	{
		return SQRT( x * x + y * y + z * z );
	}

	Vector3& SetLength( const Real fLength )
	{
		Real fAdjust = Length();
		if( fAdjust != 0.0f )
		{
			fAdjust = fLength / fAdjust;
			*this *= fAdjust;
		}
		return *this;
	}

	Vector3& MaxLength( const Real fLength )
	{
		Real fAdjust = Length();
		if( fAdjust > fLength )
		{
			fAdjust = fLength / fAdjust;
			*this *= fAdjust;
		}
		return *this;
	}

	inline const Real SquaredLength() const
	{
		return x * x + y * y + z * z;
	}

	inline const Int32 IsClose( const Vector3& other, const Real fClose = CLOSE ) const
	{
		return ( Vector3( *this - other ).SquaredLength() <= fClose * fClose );
	}

	inline const Int32 IsZero() const
	{
		return x == 0.0f && y == 0.0f && z == 0.0f ? 1 : 0;
	}

	inline const Int32 IsCloseZero( const Real fClose = CLOSE ) const
	{
		return SquaredLength() <= fClose * fClose;
	}

	inline Vector3& Normalize()
	{
		Real fLength = SquaredLength();
		if( fLength != 0.0f ) 
			*this /= SQRT( fLength );
		return *this;
	}

	inline const Vector3 GetNormal() const
	{
		Vector3 v = *this;
		return v.Normalize();
	}

	#ifdef __D3DX10_H__
	inline operator D3DXVECTOR3&() const		{ return *(D3DXVECTOR3*)this; }
	#endif

	inline Vector3& operator*=( const Real scaler ) 
	{
		x *= scaler;
		y *= scaler;
		z *= scaler;
		return *this;
	}

	inline Vector3& operator*=( const Matrix4x4& rhs )
	{
		return Transform( rhs );
	}

	inline const Vector3 operator*( const Matrix4x4& rhs ) const
	{
		Vector3 result = *this;
		result *= rhs;
		return result;
	}

	inline Vector3& operator/=( const Real scaler ) 
	{
		x /= scaler;
		y /= scaler;
		z /= scaler;
		return *this;
	}

	inline Vector3& operator+=( const Vector3& other ) 
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	inline const Vector3 operator+( const Vector3& other ) const
	{
		Vector3 result = *this;
		result += other;
		return result;
	}

	inline Vector3& operator-=( const Vector3& other ) 
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	inline const Vector3 operator-( const Vector3& other ) const
	{
		Vector3 result = *this;
		result -= other;
		return result;
	}

	inline const Vector3 operator-() const
	{
		Vector3 result;
		result.x = -x;
		result.y = -y;
		result.z = -z;
		return result;
	}

	inline const int operator==( const Vector3& other ) const
	{
		return x == other.x && y == other.y && z == other.z ? 1 : 0;
	}

	inline operator Real* () 
	{
		return (Real*)this; 
	}

	inline operator const Real* () const 
	{
		return (Real*)this; 
	}

	inline Real& operator[]( const int nIndex )
	{
		return ((Real*)this)[nIndex];
	}

	inline Vector2 xy()	const { return Vector2( x, y ); }
	inline Vector2 xz()	const { return Vector2( x, z ); }
	inline Vector2 yz()	const { return Vector2( y, z ); }

	inline Vector3() { x = y = z = 0; }
	Vector3( const Real ix, const Real iy, const Real iz )
	{
		x = ix;
		y = iy;
		z = iz;
	}
	Vector3( const Vector2& xy, const Real iz = 0 )
	{
		x = xy.x;
		y = xy.y;
		z = iz;
	}
	Vector3( const Real ix, const Vector2& yz )
	{
		x = ix;
		y = yz.x;
		z = yz.y;
	}
} Real3, Vector3, Point3;

typedef struct Vector4
{
	Real x;
	Real y;
	Real z;
	Real w;

	inline void Zero()
	{
		x = y = z = w = 0.0f;
	}

	inline static const Real Dot( const Vector4& fst,  const Vector4& snd )
	{
		return ( fst.x * snd.x ) + ( fst.y * snd.y ) + ( fst.z * snd.z ) + ( fst.w * snd.w );
	}

	inline const Real Length() const
	{
		return SQRT( x * x + y * y + z * z + w * w );
	}

	inline const Real SquaredLength() const
	{
		return x * x + y * y + z * z + w * w;
	}

	inline const Int32 IsClose( const Vector4& other, Real fClose = CLOSE ) const
	{
		return Vector4( *this - other ).SquaredLength() <= fClose * fClose;
	}

	const Int32 IsEachClose( const Vector4& other, Real fClose = CLOSE ) const
	{
		return ( ABS( x - other.x ) <= fClose ) && 
			   ( ABS( y - other.y ) <= fClose ) &&
			   ( ABS( z - other.z ) <= fClose ) &&
			   ( ABS( w - other.w ) <= fClose );
	}

	inline const Int32 IsZero() const
	{
		return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f ? 1 : 0;
	}

	inline Vector4& Normalize()
	{
		Real fLength = SquaredLength();
		if( fLength != 0.0f ) 
			*this /= SQRT( fLength );
		return *this;
	}

	inline const Vector4 GetNormal() const
	{
		Vector4 v = *this;
		return v.Normalize();
	}

	static const Vector4 Lerp( const Vector4& fst, const Vector4& snd, const Real s )
	{
		Vector4 result;
		result.x = ::Lerp( fst.x, snd.x, s );
		result.y = ::Lerp( fst.y, snd.y, s );
		result.z = ::Lerp( fst.z, snd.z, s );
		result.w = ::Lerp( fst.w, snd.w, s );
		return result;
	}

	#ifdef __D3DX10_H__
	inline operator D3DXVECTOR4&() const		{ return *(D3DXVECTOR4*)this; }
	#endif

	inline Vector4& operator*=( const Real scaler ) 
	{
		x *= scaler;
		y *= scaler;
		z *= scaler;
		w *= scaler;
		return *this;
	}

	inline Vector4& operator/=( const Real scaler ) 
	{
		x /= scaler;
		y /= scaler;
		z /= scaler;
		w /= scaler;
		return *this;
	}

	inline Vector4& operator+=( const Vector4& other ) 
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	inline const Vector4 operator+( const Vector4& other ) const
	{
		Vector4 result = *this;
		result += other;
		return result;
	}

	inline Vector4& operator-=( const Vector4& other ) 
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}

	inline const Vector4 operator-( const Vector4& other ) const
	{
		Vector4 result = *this;
		result -= other;
		return result;
	}

	inline const Vector4 operator-() const
	{
		Vector4 result;
		result.x = -x;
		result.y = -y;
		result.z = -z;
		result.w = -w;
		return result;
	}

	inline void operator=( const Vector3& other )
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = 0.0f;
	}

	inline operator Real* () 
	{
		return (Real*)this; 
	}

	inline operator const Real* () const 
	{
		return (Real*)this; 
	}

	inline Real& operator[]( const int nIndex )
	{
		return ((Real*)this)[nIndex];
	}

	inline Vector2 xy() const	{ return Vector2( x, y ); }
	inline Vector2 xz() const	{ return Vector2( x, z ); }
	inline Vector2 xw() const	{ return Vector2( x, w ); }
	inline Vector2 yz() const	{ return Vector2( y, z ); }
	inline Vector2 yw() const	{ return Vector2( y, w ); }
	inline Vector2 zw() const	{ return Vector2( z, w ); }

	inline Vector3 xyz() const  { return Vector3( x, y, z ); }
	inline Vector3 xyw() const  { return Vector3( x, y, w ); }
	inline Vector3 xzw() const  { return Vector3( x, z, w ); }
	inline Vector3 yzw() const  { return Vector3( y, z, w ); }

	inline Vector4() {}
	Vector4( const Real ix, const Real iy, const Real iz, const Real iw )
	{
		x = ix;
		y = iy;
		z = iz;
		w = iw;
	}
	Vector4( const Vector3& xyz, const Real iw = 0 )
	{
		x = xyz.x;
		y = xyz.y;
		z = xyz.z;
		w = iw;
	}
	Vector4( const Real ix, const Vector3& yzw )
	{
		x = ix;
		y = yzw.x;
		z = yzw.y;
		w = yzw.z;
	}
} Real4, Vector4, Point4;

inline const Vector2 operator*( const Vector2& lhs, const Vector2& rhs )
{
	Vector2 result;
	result.x = lhs.x * rhs.x;
	result.y = lhs.y * rhs.y;
	return result;
}

inline const Vector2 operator*( const Real scaler, const Vector2& rhs )
{
	Vector2 result = rhs;
	result *= scaler;
	return result;
}

inline const Vector2 operator*( const Vector2& lhs, const Real scaler )
{
	Vector2 result = lhs;
	result *= scaler;
	return result;
}

inline const Vector3 operator*( const Vector3& lhs, const Vector3& rhs )
{
	Vector3 result;
	result.x = lhs.x * rhs.x;
	result.y = lhs.y * rhs.y;
	result.z = lhs.z * rhs.z;
	return result;
}

inline const Vector3 operator*( const Real scaler, const Vector3& rhs )
{
	Vector3 result = rhs;
	result *= scaler;
	return result;
}

inline const Vector3 operator*( const Vector3& lhs, const Real scaler )
{
	Vector3 result = lhs;
	result *= scaler;
	return result;
}

inline const Vector4 operator*( const Vector4& lhs, const Vector4& rhs )
{
	Vector4 result;
	result.x = lhs.x * rhs.x;
	result.y = lhs.y * rhs.y;
	result.z = lhs.z * rhs.z;
	result.w = lhs.w * rhs.w;
	return result;
}

inline const Vector4 operator*( const Real scaler, const Vector4& rhs )
{
	Vector4 result = rhs;
	result *= scaler;
	return result;
}

inline const Vector4 operator*( const Vector4& lhs, const Real scaler )
{
	Vector4 result = lhs;
	result *= scaler;
	return result;
}

inline const Vector2 operator/( const Vector2& lhs, const Vector2& rhs )
{
	Vector2 result;
	result.x = lhs.x / rhs.x;
	result.y = lhs.y / rhs.y;
	return result;
}

inline const Vector2 operator/( const Real scaler, const Vector2& rhs )
{
	Vector2 result;
	result.x = scaler / rhs.x;
	result.y = scaler / rhs.y;
	return result;
}

inline const Vector2 operator/( const Vector2& lhs, const Real scaler )
{
	Vector2 result = lhs;
	result /= scaler;
	return result;
}

inline const Vector3 operator/( const Vector3& lhs, const Vector3& rhs )
{
	Vector3 result;
	result.x = lhs.x / rhs.x;
	result.y = lhs.y / rhs.y;
	result.z = lhs.z / rhs.z;
	return result;
}

inline const Vector3 operator/( const Real scaler, const Vector3& rhs )
{
	Vector3 result;
	result.x = scaler / rhs.x;
	result.y = scaler / rhs.y;
	result.z = scaler / rhs.z;
	return result;
}

inline const Vector3 operator/( const Vector3& lhs, const Real scaler )
{
	Vector3 result = lhs;
	result /= scaler;
	return result;
}

inline const Vector4 operator/( const Vector4& lhs, const Vector4& rhs )
{
	Vector4 result;
	result.x = lhs.x / rhs.x;
	result.y = lhs.y / rhs.y;
	result.z = lhs.z / rhs.z;
	result.w = lhs.w / rhs.w;
	return result;
}

inline const Vector4 operator/( const Real scaler, const Vector4& rhs )
{
	Vector4 result;
	result.x = scaler / rhs.x;
	result.y = scaler / rhs.y;
	result.z = scaler / rhs.z;
	result.w = scaler / rhs.w;
	return result;
}

inline const Vector4 operator/( const Vector4& lhs, const Real scaler )
{
	Vector4 result = lhs;
	result /= scaler;
	return result;
}

#endif
