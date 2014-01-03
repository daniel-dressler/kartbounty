//#include "SEGraphics.h"
#include "SELinearMath.h"

Vector2& Vector2::Transform( const Matrix3x3& mat )
{
	Real tx, ty, tz;
	tx  = mat._11 * x;
	tx += mat._21 * y;
	tx += mat._31;

	ty  = mat._12 * x;
	ty += mat._22 * y;
	ty += mat._32;

	tz  = mat._13 * x;
	tz += mat._23 * y;
	tz += mat._33;

	if( tz == 0.0f )
		Zero();
	else
	{
		x = tx / tz;
		y = ty / tz;
	}

	return *this;
}

Vector3& Vector3::Transform( const Matrix3x3& mat )
{
	Real tx, ty, tz;
	tx  = mat._11 * x;
	tx += mat._21 * y;
	tx += mat._31 * z;

	ty  = mat._12 * x;
	ty += mat._22 * y;
	ty += mat._32 * z;

	tz  = mat._13 * x;
	tz += mat._23 * y;
	tz += mat._33 * z;

	x = tx;
	y = ty;
	z = tz;

	return *this;
}

Vector3& Vector3::Transform( const Matrix4x4& mat )
{
#ifdef __USE_D3DX__
	D3DXVec3TransformCoord( (D3DXVECTOR3*)this, (D3DXVECTOR3*)this, (D3DXMATRIX*)&mat );
#else
	Real tx, ty, tz, tw;
	
	tx  = mat._11 * x;
	tx += mat._21 * y;
	tx += mat._31 * z;
	tx += mat._41;

	ty  = mat._12 * x;
	ty += mat._22 * y;
	ty += mat._32 * z;
	ty += mat._42;

	tz  = mat._13 * x;
	tz += mat._23 * y;
	tz += mat._33 * z;
	tz += mat._43;

	tw  = mat._14 * x;
	tw += mat._24 * y;
	tw += mat._34 * z;
	tw += mat._44;

	if( tw == 0.0f )
		Zero();
	else
	{
		x = tx / tw;
		y = ty / tw;
		z = tz / tw;
	}
#endif
	return *this;
}

