#ifndef __SE_QUATERNION__
#define __SE_QUATERNION__

typedef struct Quaternion
{
	Real x;
	Real y;
	Real z;
	Real w;

	inline const void Zero()			{ x = y = z = w = 0.0f; }
	inline Quaternion& Identity()	{ w = 1.0f; x = y = z = 0.0f; return *this; }

	inline const Int32 IsIdentity() const
	{
		return ::IsClose( w, 1.0f ) && ::IsClose( x, 0.0f ) && ::IsClose( y, 0.0f ) && ::IsClose( z, 0.0f ) ? 1 : 0;
	}

	inline static const Quaternion GetIdentity()
	{
		Quaternion quat;
		return quat.Identity();
	}

	inline static const Real Dot( const Quaternion& fst, const Quaternion& snd )
	{
		return fst.x * snd.x + fst.y * snd.y + fst.z * snd.z + fst.w * snd.w;
	}

	inline static const Real Angle( const Quaternion& fst, const Quaternion& snd )
	{
		Real fDot = Dot( fst, snd );
		return ACOS( ( 2 * ( fDot * fDot ) ) - 1 );
	}

	inline static const Real EstimateAngle( const Quaternion& fst, const Quaternion& snd )
	{
		Real fDot = Dot( fst, snd );
		return 1 - ( fDot * fDot );
	}

	inline const Real Length() const		{ return SQRT( x * x + y * y + z * z + w * w );	}
	inline const Real SquaredLength() const	{ return x * x + y * y + z * z + w * w;	}

	inline const Int32 IsClose( const Quaternion& other, const Real fClose = CLOSE ) const
	{
		return Quaternion( *this - other ).SquaredLength() <= fClose * fClose;
	}

	inline Quaternion& Normalize()
	{
		Real fLength = SquaredLength();
		if( fLength != 0.0f ) 
			*this /= SQRT( fLength );
		return *this;
	}

	inline Quaternion& Conjugate()
	{
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}

	inline Quaternion& Invert()
	{
		Conjugate();
		*this /= SquaredLength();
		return *this;
	}

	Quaternion& Multiply( const Quaternion& q1, const Quaternion& q2 );
	Quaternion& Multiply( const Quaternion& q, const Matrix4x4& m );

	Quaternion& RotateMatrix( const Matrix4x4& m );
	Quaternion& RotateAxisAngle( const Vector3& vAxis, const Real fAngle );

	Quaternion& Lerp( Quaternion& qA, Quaternion& qB, Real fT );
	Quaternion& Slerp( Quaternion& qA, Quaternion& qB, Real fT );
	Int32 ToAxisAngle( Vector3* pAxis, Real* pAngle ) const;

	static Quaternion GetRotateMatrix( const Matrix4x4& m )
	{
		Quaternion q;
		return q.RotateMatrix( m );
	}

	static Quaternion GetRotateAxisAngle( const Vector3& vAxis, const Real fAngle )
	{
		Quaternion q;
		return q.RotateAxisAngle( vAxis, fAngle );
	}

	inline Quaternion& operator*=( const Quaternion& rhs )
	{
		Quaternion m = *this;
		Multiply( m, rhs );
		return *this;
	}

	inline const Quaternion operator*( const Quaternion& rhs ) const
	{
		Quaternion result = *this;
		result *= rhs;
		return result;
	}

	inline Quaternion& operator*=( const Matrix4x4& rhs )
	{
		Quaternion m = *this;
		Multiply( m, rhs );
		return *this;
	}

	inline Quaternion& operator*=( const Real scaler ) 
	{
		x *= scaler;
		y *= scaler;
		z *= scaler;
		w *= scaler;
		return *this;
	}

	inline Quaternion& operator/=( const Real scaler ) 
	{
		x /= scaler;
		y /= scaler;
		z /= scaler;
		w /= scaler;
		return *this;
	}

	inline const Quaternion operator/( const Real scaler ) const
	{
		Quaternion result = *this;
		result /= scaler;
		return result;
	}

	inline Quaternion& operator+=( const Quaternion& other ) 
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	inline const Quaternion operator+( const Quaternion& other ) const
	{
		Quaternion result = *this;
		result += other;
		return result;
	}

	inline Quaternion& operator-=( const Quaternion& other ) 
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}

	inline const Quaternion operator-( const Quaternion& other ) const
	{
		Quaternion result = *this;
		result -= other;
		return result;
	}

	inline const Quaternion operator-() const
	{
		Quaternion result;
		result.x = -x;
		result.y = -y;
		result.z = -z;
		result.w = -w;
		return result;
	}

#ifdef __D3DX10_H__
	inline D3DXQUATERNION& AsD3DX() 
	{
		return *(D3DXQUATERNION*)this;
	}
#endif

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

	inline Quaternion() {}
	Quaternion( const Real ix, const Real iy, const Real iz, const Real iw )
	{
		x = ix;
		y = iy;
		z = iz;
		w = iw;
	}

} Quaternion;

inline Quaternion operator*( const Quaternion& lhs, const Matrix4x4& rhs )
{
	Quaternion result = lhs;
	result *= rhs;
	return result;
}

inline Quaternion operator*( const Matrix4x4& lhs, const Quaternion& rhs )
{
	Quaternion result = rhs;
	result *= lhs;
	return result;
}

inline Quaternion operator*( const Quaternion& lhs, const Real scaler )
{
	Quaternion result = lhs;
	result *= scaler;
	return result;
}

inline Quaternion operator*( const Real scaler, const Quaternion& rhs )
{
	Quaternion result = rhs;
	result *= scaler;
	return result;
}

#endif
