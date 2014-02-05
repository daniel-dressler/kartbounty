#include "SELib.h"

Quaternion& Quaternion::Multiply( const Quaternion& q1, const Quaternion& q2 )
{
#ifdef __USE_D3DX__
	D3DXQuaternionMultiply( (D3DXQUATERNION*)this, (D3DXQUATERNION*)&q1, (D3DXQUATERNION*)&q2 );
#else
	w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
#endif
	return *this;
}

Quaternion& Quaternion::Multiply( const Quaternion& q, const Matrix4x4& m )
{
	Quaternion quatMat;
	quatMat.RotateMatrix( m );
	return Multiply( q, quatMat );
}

Quaternion& Quaternion::RotateMatrix( const Matrix4x4& m )
{
	Real tr = m._11 + m._22 + m._33;
	if( tr > 0.0f )
	{
		Real s = SQRT( tr + 1.0f ) * 2.0f;
		w = 0.25f * s;
		x = ( m._23 - m._32 ) / s;
		y = ( m._31 - m._13 ) / s;
		z = ( m._12 - m._21 ) / s;
	}
	else if( ( m._11 > m._22 ) && ( m._11 > m._33 ) )
	{
		Real s = SQRT( 1.0f + m._11 - m._22 - m._33 ) * 2.0f;
		w = ( m._23 - m._32 ) / s;
		x = 0.25f * s;
		y = ( m._21 + m._12 ) / s;
		z = ( m._31 + m._13 ) / s;
	}
	else if( m._22 > m._33 )
	{
		Real s = SQRT( 1.0f + m._22 - m._11 - m._33 ) * 2.0f;
		w = ( m._31 - m._13 ) / s;
		x = ( m._21 + m._12 ) / s;
		y = 0.25f * s;
		z = ( m._32 + m._23 ) / s;
	}
	else
	{
		Real s = SQRT( 1.0f + m._33 - m._11 - m._22 ) * 2.0f;
		w = ( m._12 - m._21 ) / s;
		x = ( m._31 + m._13 ) / s;
		y = ( m._32 + m._23 ) / s;
		z = 0.25f * s;
	}
	return *this;
}

Quaternion& Quaternion::RotateAxisAngle( const Vector3& vAxis, const Real fAngle )
{
#ifdef __USE_D3DX__
	D3DXQuaternionRotationAxis( (D3DXQUATERNION*)this, (D3DXVECTOR3*)&vAxis, fAngle );
#else

	Real fSin = SIN( fAngle * 0.5f );
	x = vAxis.x * fSin;
	y = vAxis.y * fSin;
	z = vAxis.z * fSin;
	w = COS( fAngle * 0.5f );

#endif
	return *this;
}

Quaternion& Quaternion::Lerp( Quaternion& qA, Quaternion& qB, Real fT )
{
	*this = ( qA * ( 1.0f - fT ) + qB * fT );
	return Normalize();
}

Quaternion& Quaternion::Slerp( Quaternion& qA, Quaternion& qB, Real fT )
{
	if( fT <= 0.0f )
		*this = qA;
	else if( fT >= 1.0f )
		*this = qB;
	else
	{
		Quaternion qC;
		Real fDot = Dot( qA, qB );
		if( fDot >= 0.0f )
			qC = qB;
		else
		{
			fDot = -fDot;
			qC = -qB;
		}

		if( fDot >= 0.95f )
			Lerp( qA, qB, fT );
		else
		{
			Real fAngle = ACOS( fDot );
			*this = ( qA * SIN( fAngle * ( 1 - fT ) ) + qC * SIN( fAngle * fT ) ) / SIN( fAngle );
		}
	}
	return *this;
}

Int32 Quaternion::ToAxisAngle( Vector3* pAxis, Real* pAngle ) const
{
	Real f = SQRT( 1.0f - w * w );
	if( !::IsClose( f, 0.0f ) )
	{
		*pAngle = 2.0f * ACOS(w);
		pAxis->x = x * f;
		pAxis->y = y * f;
		pAxis->z = z * f;
		return 1;
	}
	else
	{
		*pAngle = 0.0f;
		pAxis->x = 0.0f;
		pAxis->y = 0.0f;
		pAxis->z = 0.0f;
		return 0;
	}
}
