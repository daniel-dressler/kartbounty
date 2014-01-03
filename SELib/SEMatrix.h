#ifndef __SE_MATRIX__
#define __SE_MATRIX__

typedef struct Matrix3x3
{
	union
	{
		Real e[9];
		struct {
#ifdef SE_MATRIX_COLMAJOR
			Real _11, _21, _31;
			Real _12, _22, _32;
			Real _13, _23, _33;
#else
			Real _11, _12, _13;
			Real _21, _22, _23;
			Real _31, _32, _33;
#endif
		};
	};
	
	inline const void Zero() { MEMSET( (Real*)this, 0, sizeof(Real) * 9 ); }
	inline Matrix3x3& Identity() { Zero(); _11 = _22 = _33 = 1.0; return *this; }

	Real Determinant();

	Matrix3x3& Multiply( const Matrix3x3& a, const Matrix3x3& b );
	Matrix3x3& Transpose();
	Matrix3x3& Rotate( Real a );
	Matrix3x3& Translate( const Vector2& v );
	Matrix3x3& Scale( const Vector2& v );

	inline Matrix3x3& operator*=( const Matrix3x3& rhs )
	{
		Matrix3x3 m = *this;
		Multiply( m, rhs );
		return *this;
	}

	inline operator		  Real* ()			{ return (Real*)this; }
	inline operator const Real* () const 	{ return (Real*)this; }
	inline Real& operator[]( const int nIndex ) { return ((Real*)this)[nIndex]; }
	Matrix3x3& operator=( const Matrix4x4& rhs );
} Real3x3, Matrix2;

typedef struct Matrix4x4
{
	union
	{
		Real e[16];
		struct {
#ifdef SE_MATRIX_COLMAJOR
			Real _11, _21, _31, _41;
			Real _12, _22, _32, _42;
			Real _13, _23, _33, _43;
			Real _14, _24, _34, _44;
#else
			Real _11, _12, _13, _14;
			Real _21, _22, _23, _24;
			Real _31, _32, _33, _34;
			Real _41, _42, _43, _44;
#endif
		};
	};

	inline void Zero()	{ MEMSET( (Real*)this, 0, sizeof(Real) << 4 ); }
	Matrix4x4& Identity() { Zero(); _11 = _22 = _33 = _44 = 1.0; return *this; }

	Matrix4x4& Multiply( const Matrix4x4& a, const Matrix4x4& b );
	Matrix4x4& Transpose();
	Matrix4x4& Invert();

	Matrix4x4& RotateQuaternion( const Quaternion& quat );
	Matrix4x4& RotateAxis( const Vector3& vAxis, const Real fTheta );
	Matrix4x4& RotateX( const Real x );
	Matrix4x4& RotateY( const Real y );
	Matrix4x4& RotateZ( const Real z );
	Matrix4x4& Translate( const Vector3& v );
	Matrix4x4& Scale( const Vector3& v );

	Matrix4x4& LookAt( const Vector3& eye, const Vector3& at, const Vector3& up );
	Matrix4x4& Perspective( const Real fov, const Real aspect, const Real zn, const Real zf );
	Matrix4x4& Orthographic( const Real l, const Real r, const Real b, const Real t, const Real zn, const Real zf );
	Matrix4x4& Viewport( const Int32 x, const Int32 y, const Int32 w, const Int32 h, const Real zn, const Real zf );

	static inline const Matrix4x4 GetIdentity()
	{
		Matrix4x4 mat;
		return mat.Identity();
	}

	static inline const Matrix4x4 GetRotateQuaternion( const Quaternion& quat )
	{
		Matrix4x4 mat;
		return mat.RotateQuaternion( quat );
	}

	static inline const Matrix4x4 GetRotateAxis( const Vector3& vAxis, const Real fTheta )
	{
		Matrix4x4 mat;
		return mat.RotateAxis( vAxis, fTheta );
	}

	static inline const Matrix4x4 GetRotateX( const Real x )
	{
		Matrix4x4 mat;
		return mat.RotateX( x );
	}

	static inline const Matrix4x4 GetRotateY( const Real y )
	{
		Matrix4x4 mat;
		return mat.RotateY( y );
	}

	static inline const Matrix4x4 GetRotateZ( const Real z )
	{
		Matrix4x4 mat;
		return mat.RotateZ( z );
	}


	static inline const Matrix4x4 GetTranslate( const Vector3& v )
	{
		Matrix4x4 mat;
		return mat.Translate( v );
	}

	static inline const Matrix4x4 GetTranslate( Real fX, Real fY, Real fZ )
	{
		Matrix4x4 mat;
		return mat.Translate( Vector3( fX, fY, fZ ) );
	}

	static inline const Matrix4x4 GetScale( const Vector3& v )
	{
		Matrix4x4 mat;
		return mat.Scale( v );
	}

	static inline const Matrix4x4 GetScale( Real fScale )
	{
		Matrix4x4 mat;
		return mat.Scale( Vector3( fScale, fScale, fScale ) );
	}

	static inline const Matrix4x4 GetScale( Real fX, Real fY, Real fZ )
	{
		Matrix4x4 mat;
		return mat.Scale( Vector3( fX, fY, fZ ) );
	}

	inline Matrix4x4& operator*=( const Matrix4x4& rhs )
	{
		Matrix4x4 m = *this;
		Multiply( m, rhs );
		return *this;
	}

	const int operator==( const Matrix4x4& rhs ) const
	{
		for( int i = 0; i < 16; i++ )
		{
			if( !IsClose( (*this)[i], rhs[i] ) )
				return 0;
		}
		return 1;
	}

	Matrix4x4& operator+=( const Matrix4x4& rhs )
	{
		for( Int32 i = 0; i < 16; i++ )
			(*this)[i] += rhs[i];
		return *this;
	}

	Matrix4x4& operator-=( const Matrix4x4& rhs )
	{
		for( Int32 i = 0; i < 16; i++ )
			(*this)[i] -= rhs[i];
		return *this;
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

} Real4x4, Matrix, Matrix3;

typedef struct Matrix4x3
{
	Vector4	a, b, c;

	Matrix4x3& operator=( const Matrix& rhs )
	{
		Real div = rhs._44 == 0 ? 0.0f : 1.0f / rhs._44;
		a = Vector4( rhs._11, rhs._12, rhs._13, rhs._41 * div );
		b = Vector4( rhs._21, rhs._22, rhs._23, rhs._42 * div );
		c = Vector4( rhs._31, rhs._32, rhs._33, rhs._43 * div );
		return *this;
	}

	inline Matrix4x3() {}
	inline Matrix4x3(  const Matrix& mat )
	{
		*this = mat;
	}

} Matrix4x3;

inline Matrix2 operator*( const Matrix2& lhs, const Matrix2& rhs )
{
	Matrix3x3 m;
	return m.Multiply( lhs, rhs );
}

inline Matrix2& Matrix2::operator=( const Matrix3& rhs )
{
	_11 = rhs._11; _12 = rhs._12; _13 = rhs._13;
	_21 = rhs._21; _22 = rhs._22; _23 = rhs._23;
	_31 = rhs._31; _32 = rhs._32; _33 = rhs._33;
	return *this;
}

inline Matrix4x4 operator*( const Matrix4x4& lhs, const Matrix4x4& rhs )
{
	Matrix4x4 m;
	return m.Multiply( lhs, rhs );
}

inline Vector3 operator*( const Matrix3& lhs, const Vector3& rhs )
{
	Vector3 v = rhs;
	return v.Transform( lhs );
}

inline Vector3 operator*( const Vector3& lhs, const Matrix4x4& rhs )
{
	Vector3 v = lhs;
	return v.Transform( rhs );
}

inline Matrix4x4 operator+( const Matrix4x4& lhs, const Matrix4x4& rhs )
{
	Matrix4x4 m = lhs;
	m += rhs;
	return m;
}

inline Matrix4x4 operator-( const Matrix4x4& lhs, const Matrix4x4& rhs )
{
	Matrix4x4 m = lhs;
	m -= rhs;
	return m;
}

#endif
