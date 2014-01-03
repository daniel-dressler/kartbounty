//#include "SEGraphics.h"
#include "SELinearMath.h"

Real Matrix2::Determinant()
{
	return ( _11 * _22 * _33 ) + ( _12 * _23 * _31 ) + ( _13 * _21 * _32 )
		 - ( _13 * _22 * _31 ) - ( _11 * _23 * _32 ) - ( _12 * _21 * _33 );
}

Matrix2& Matrix2::Multiply( const Matrix2& a, const Matrix2& b )
{
	_11 = b._11 * a._11 + b._21 * a._12 + b._31 * a._13;
	_12 = b._12 * a._11 + b._22 * a._12 + b._32 * a._13;
	_13 = b._13 * a._11 + b._23 * a._12 + b._33 * a._13;
	_21 = b._11 * a._21 + b._21 * a._22 + b._31 * a._23;
	_22 = b._12 * a._21 + b._22 * a._22 + b._32 * a._23;
	_23 = b._13 * a._21 + b._23 * a._22 + b._33 * a._23;
	_31 = b._11 * a._31 + b._21 * a._32 + b._31 * a._33;
	_32 = b._12 * a._31 + b._22 * a._32 + b._32 * a._33;
	_33 = b._13 * a._31 + b._23 * a._32 + b._33 * a._33;
	return *this;
}

Matrix2& Matrix2::Transpose()
{
	Real fTemp;
	fTemp = _12;		_12 = _21;		_21 = fTemp;
	fTemp = _13;		_13 = _31;		_31 = fTemp;
	fTemp = _23;		_23 = _32;		_32 = fTemp;
	return *this;
}

Matrix2& Matrix2::Rotate( Real a )
{
	Real fCos = COS( a );
	Real fSin = SIN( a );

	Zero();
	_33 = 1.0;
	_11 = _22 = fCos;
	_21 = -fSin;
	_12 = fSin;
	return *this;
}

Matrix2& Matrix2::Translate( const Vector2& v )
{
	Identity();
	_31 = v.x;
	_32 = v.y;
	return *this;
}

Matrix2& Matrix2::Scale( const Vector2& v )
{
	Zero();
	_11 = v.x;
	_22 = v.y;
	_33 = 1.0f;
	return *this;
}

Matrix3& Matrix3::Multiply( const Matrix3& a, const Matrix3& b )
{
#ifdef __USE_D3DX__
	D3DXMatrixMultiply( (D3DXMATRIX*)this, (D3DXMATRIX*)&a, (D3DXMATRIX*)&b );
#else
	e[0] = b.e[0] * a.e[0] + b.e[4] * a.e[1] + b.e[8] * a.e[2] + b.e[12] * a.e[3];
	e[1] = b.e[1] * a.e[0] + b.e[5] * a.e[1] + b.e[9] * a.e[2] + b.e[13] * a.e[3];
	e[2] = b.e[2] * a.e[0] + b.e[6] * a.e[1] + b.e[10] * a.e[2] + b.e[14] * a.e[3];
	e[3] = b.e[3] * a.e[0] + b.e[7] * a.e[1] + b.e[11] * a.e[2] + b.e[15] * a.e[3];
	e[4] = b.e[0] * a.e[4] + b.e[4] * a.e[5] + b.e[8] * a.e[6] + b.e[12] * a.e[7];
	e[5] = b.e[1] * a.e[4] + b.e[5] * a.e[5] + b.e[9] * a.e[6] + b.e[13] * a.e[7];
	e[6] = b.e[2] * a.e[4] + b.e[6] * a.e[5] + b.e[10] * a.e[6] + b.e[14] * a.e[7];
	e[7] = b.e[3] * a.e[4] + b.e[7] * a.e[5] + b.e[11] * a.e[6] + b.e[15] * a.e[7];
	e[8] = b.e[0] * a.e[8] + b.e[4] * a.e[9] + b.e[8] * a.e[10] + b.e[12] * a.e[11];
	e[9] = b.e[1] * a.e[8] + b.e[5] * a.e[9] + b.e[9] * a.e[10] + b.e[13] * a.e[11];
	e[10] = b.e[2] * a.e[8] + b.e[6] * a.e[9] + b.e[10] * a.e[10] + b.e[14] * a.e[11];
	e[11] = b.e[3] * a.e[8] + b.e[7] * a.e[9] + b.e[11] * a.e[10] + b.e[15] * a.e[11];
	e[12] = b.e[0] * a.e[12] + b.e[4] * a.e[13] + b.e[8] * a.e[14] + b.e[12] * a.e[15];
	e[13] = b.e[1] * a.e[12] + b.e[5] * a.e[13] + b.e[9] * a.e[14] + b.e[13] * a.e[15];
	e[14] = b.e[2] * a.e[12] + b.e[6] * a.e[13] + b.e[10] * a.e[14] + b.e[14] * a.e[15];
	e[15] = b.e[3] * a.e[12] + b.e[7] * a.e[13] + b.e[11] * a.e[14] + b.e[15] * a.e[15];
#endif
	return *this;
}

Matrix3& Matrix3::Transpose()
{
#ifdef __USE_D3DX__
	D3DXMatrixTranspose( (D3DXMATRIX*)this, (D3DXMATRIX*)this );
#else
	Real fTemp;
	fTemp = e[1];		e[1] = e[4];		e[4] = fTemp;
	fTemp = e[2];		e[2] = e[8];		e[8] = fTemp;
	fTemp = e[3];		e[3] = e[12];		e[12] = fTemp;
	fTemp = e[6];		e[6] = e[9];		e[9] = fTemp;
	fTemp = e[7];		e[7] = e[13];		e[13] = fTemp;
	fTemp = e[11];		e[11] = e[14];		e[14] = fTemp;
#endif
	return *this;
}

Matrix3& Matrix3::Invert()
{
#ifdef __USE_D3DX__
	Float32 fDet;
	fDet = D3DXMatrixDeterminant( (D3DXMATRIX*)&mat );
	D3DXMatrixInverse( (D3DXMATRIX*)&mat, &fDet, (D3DXMATRIX*)&mat );
#else

	Real inv[16], fDet;

	inv[0]  = e[5] * e[10] * e[15] - e[5] * e[11] * e[14] - e[9] * e[6] * e[15] + e[9] * e[7] * e[14] + e[13] * e[6] * e[11] - e[13] * e[7] * e[10];
	inv[4]  =-e[4] * e[10] * e[15] + e[4] * e[11] * e[14] + e[8] * e[6] * e[15] - e[8] * e[7] * e[14] - e[12] * e[6] * e[11] + e[12] * e[7] * e[10];
	inv[8]  = e[4] * e[9] * e[15] - e[4] * e[11] * e[13] - e[8] * e[5] * e[15] + e[8] * e[7] * e[13] + e[12] * e[5] * e[11] - e[12] * e[7] * e[9];
	inv[12] =-e[4] * e[9] * e[14] + e[4] * e[10] * e[13] + e[8] * e[5] * e[14] - e[8] * e[6] * e[13] - e[12] * e[5] * e[10] + e[12] * e[6] * e[9];
	inv[1]  =-e[1] * e[10] * e[15] + e[1] * e[11] * e[14] + e[9] * e[2] * e[15] - e[9] * e[3] * e[14] - e[13] * e[2] * e[11] + e[13] * e[3] * e[10];
	inv[5]  = e[0] * e[10] * e[15] - e[0] * e[11] * e[14] - e[8] * e[2] * e[15] + e[8] * e[3] * e[14] + e[12] * e[2] * e[11] - e[12] * e[3] * e[10];
	inv[9]  =-e[0] * e[9] * e[15] + e[0] * e[11] * e[13] + e[8] * e[1] * e[15] - e[8] * e[3] * e[13] - e[12] * e[1] * e[11] + e[12] * e[3] * e[9];
	inv[13] = e[0] * e[9] * e[14] - e[0] * e[10] * e[13] - e[8] * e[1] * e[14] + e[8] * e[2] * e[13] + e[12] * e[1] * e[10] - e[12] * e[2] * e[9];
	inv[2]  = e[1] * e[6] * e[15] - e[1] * e[7] * e[14] - e[5] * e[2] * e[15] + e[5] * e[3] * e[14] + e[13] * e[2] * e[7] - e[13] * e[3] * e[6];
	inv[6]  =-e[0] * e[6] * e[15] + e[0] * e[7] * e[14] + e[4] * e[2] * e[15] - e[4] * e[3] * e[14] - e[12] * e[2] * e[7] + e[12] * e[3] * e[6];
	inv[10] = e[0] * e[5] * e[15] - e[0] * e[7] * e[13] - e[4] * e[1] * e[15] + e[4] * e[3] * e[13] + e[12] * e[1] * e[7] - e[12] * e[3] * e[5];
	inv[14] =-e[0] * e[5] * e[14] + e[0] * e[6] * e[13] + e[4] * e[1] * e[14] - e[4] * e[2] * e[13] - e[12] * e[1] * e[6] + e[12] * e[2] * e[5];
	inv[3]  =-e[1] * e[6] * e[11] + e[1] * e[7] * e[10] + e[5] * e[2] * e[11] - e[5] * e[3] * e[10] - e[9] * e[2] * e[7] + e[9] * e[3] * e[6];
	inv[7]  = e[0] * e[6] * e[11] - e[0] * e[7] * e[10] - e[4] * e[2] * e[11] + e[4] * e[3] * e[10] + e[8] * e[2] * e[7] - e[8] * e[3] * e[6];
	inv[11] =-e[0] * e[5] * e[11] + e[0] * e[7] * e[9] + e[4] * e[1] * e[11] - e[4] * e[3] * e[9] - e[8] * e[1] * e[7] + e[8] * e[3] * e[5];
	inv[15] = e[0] * e[5] * e[10] - e[0] * e[6] * e[9] - e[4] * e[1] * e[10] + e[4] * e[2] * e[9] + e[8] * e[1] * e[6] - e[8] * e[2] * e[5];

    fDet = e[0] * inv[0] + e[1] * inv[4] + e[2] * inv[8] + e[3] * inv[12];

    if( fDet != 0 )
	{
		fDet = 1.0f / fDet;

		for( Int32 i = 0; i < 16; i++ )
			(*this)[i] = inv[i] * fDet;
	}

#endif
	return *this;
}

Matrix3& Matrix3::RotateQuaternion( const Quaternion& quat )
{
	e[15] = 1.0f;
	e[3] = e[7] = e[11] = e[12] = e[13] = e[14] = 0.0f;

	Real ww  = quat.w * quat.w;
	Real wx2 = quat.w * quat.x * 2.0f;
	Real wy2 = quat.w * quat.y * 2.0f;
	Real wz2 = quat.w * quat.z * 2.0f;
	Real xx  = quat.x * quat.x;
	Real xy2 = quat.x * quat.y * 2.0f;
	Real xz2 = quat.x * quat.z * 2.0f;
	Real yy  = quat.y * quat.y;
	Real yz2 = quat.y * quat.z * 2.0f;
	Real zz  = quat.z * quat.z;

	e[0]  = ww + xx - yy - zz;
	e[1]  = xy2 + wz2;
	e[2]  = xz2 - wy2;

	e[4]  = xy2 - wz2;
	e[5]  = ww - xx + yy - zz;
	e[6]  = yz2 + wx2;

	e[8]  = xz2 + wy2;
	e[9]  = yz2 - wx2;
	e[10] = ww - xx - yy + zz;

	return *this;
}

Matrix3& Matrix3::RotateAxis( const Vector3& vAxis, const Real fTheta )
{
#ifdef __USE_D3DX__
	D3DXMatrixRotationAxis( (D3DXMATRIX*)this, (D3DXVECTOR3*)&vAxis, fTheta );
	Transpose();
#else
	Real fCosTheta = COS( fTheta );
	Real fSinTheta = SIN( fTheta );
	Real fInvCT = 1.0f - fCosTheta;

	Vector3 vAST = vAxis * fSinTheta;

	Real xy = vAxis.x * vAxis.y * fInvCT;
	Real xz = vAxis.x * vAxis.z * fInvCT;
	Real yz = vAxis.y * vAxis.z * fInvCT;

	e[15] = 1.0f;
	e[3] = e[7] = e[11] = e[12] = e[13] = e[14] = 0.0f;

	e[0]  = fCosTheta + vAxis.x * vAxis.x * fInvCT;
	e[5]  = fCosTheta + vAxis.y * vAxis.y * fInvCT;
	e[10] = fCosTheta + vAxis.z * vAxis.z * fInvCT;

	e[1]  = xy - vAST.z;
	e[2]  = vAST.y + xz;
	e[6]  = yz - vAST.x;

	e[4]  = vAST.z + xy;
	e[8]  = xz - vAST.y;
	e[9]  = vAST.x + yz;
#endif
	return *this;
}

Matrix3& Matrix3::RotateX( const Real x )
{
#ifdef __USE_D3DX__
	D3DXMatrixRotationX( (D3DXMATRIX*)this, x );
#else
	Real fCos = COS( x );
	Real fSin = SIN( x );

	Zero();
	e[0] = e[15] = 1.0;
	e[5] = e[10] = fCos;
	e[6] = fSin;
	e[9] = -fSin;
#endif
	return *this;
}

Matrix3& Matrix3::RotateY( const Real y )
{
#ifdef __USE_D3DX__
	D3DXMatrixRotationY( (D3DXMATRIX*)this, y );
#else
	Real fCos = COS( y );
	Real fSin = SIN( y );

	Zero();
	e[5] = e[15] = 1.0;
	e[0] = e[10] = fCos;
	e[2] = -fSin;
	e[8] = fSin;
#endif
	return *this;
}

Matrix3& Matrix3::RotateZ( const Real z )
{
#ifdef __USE_D3DX__
	D3DXMatrixRotationZ( (D3DXMATRIX*)this, z );
#else
	Real fCos = COS( z );
	Real fSin = SIN( z );

	Zero();
	e[10] = e[15] = 1.0;
	e[0] = e[5] = fCos;
	e[1] = fSin;
	e[4] = -fSin;
#endif
	return *this;
}

Matrix3& Matrix3::Translate( const Vector3& v )
{
#ifdef __USE_D3DX__
	D3DXMatrixTranslation( (D3DXMATRIX*)this, v.x, v.y, v.z );
#else
	Identity();
	e[12] = v.x;
	e[13] = v.y;
	e[14] = v.z;
#endif
	return *this;
}

Matrix3& Matrix3::Scale( const Vector3& v )
{
#ifdef __USE_D3DX__
	D3DXMatrixScaling( (D3DXMATRIX*)this, v.x, v.y, v.z );
#else
	Zero();
	e[0] = v.x;
	e[5] = v.y;
	e[10] = v.z;
	e[15] = 1.0f;
#endif
	return *this;
}

Matrix3& Matrix3::LookAt( const Vector3& eye, const Vector3& at, const Vector3& up )
{
#ifdef __USE_D3DX__
	D3DXMatrixLookAtLH( (D3DXMATRIX*)this, (D3DXVECTOR3*)&eye, (D3DXVECTOR3*)&at, (D3DXVECTOR3*)&up );
#else
	Vector3 zaxis = at - eye;
	zaxis.Normalize();

	Vector3 xaxis = Vector3::Cross( up, zaxis );
	xaxis.Normalize();

	Vector3 yaxis = Vector3::Cross( zaxis, xaxis );
		
	e[0] = xaxis.x; e[4] = xaxis.y; e[8]  = xaxis.z;
	e[1] = yaxis.x; e[5] = yaxis.y; e[9]  = yaxis.z;
	e[2] = zaxis.x; e[6] = zaxis.y; e[10] = zaxis.z;

	e[12] = -Vector3::Dot( xaxis, eye );
	e[13] = -Vector3::Dot( yaxis, eye );
	e[14] = -Vector3::Dot( zaxis, eye );

	e[3] = e[7] = e[11] = 0.0f;
	e[15] = 1.0f;
		
#endif
	return *this;
}

Matrix3& Matrix3::Perspective( const Real fov, const Real aspect, const Real zn, const Real zf )
{
#ifdef __USE_D3DX__
	D3DXMatrixPerspectiveFovLH( (D3DXMATRIX*)this, fov, aspect, zn, zf );
#else
	Real h = 1.0f / TAN( fov * 0.5f );
	Real w = h / aspect;
	Real Q = zf / ( zf - zn );

	Zero();
		
	e[0] = w;
	e[5] = h;
	e[10] = Q;
	e[14] = -Q * zn;
	e[11] = 1.0;
		
#endif
	return *this;
}

Matrix3& Matrix3::Orthographic( const Real l, const Real r, const Real b, const Real t, const Real zn, const Real zf )
{
#ifdef __USE_D3DX__
	D3DXMatrixOrthoOffCenterLH( (D3DXMATRIX*)this, l, r, b, t, zn, zf );
#else
	Identity();
		
	Real d = zf - zn;

	e[0] = 2.0f / ( r - l );
	e[5] = 2.0f / ( t - b );
	e[10] = 1.0f / d;
	e[14] = -zn / d;
		
#endif
	return *this;
}

Matrix3& Matrix3::Viewport( const Int32 x, const Int32 y, const Int32 w, const Int32 h, const Real zn, const Real zf )
{
	Real hw = (Real)( w >> 1 );
	Real hh = (Real)( h >> 1 );

	Zero();
		
	e[0] = hw;
	e[5] = -hh;
	e[10] = (Real)( zf - zn );
	e[12] = hw + x;
	e[13] = hh + y;
	e[14] = (Real)zn;
	e[15] = 1.0;
		
	return *this;
}
