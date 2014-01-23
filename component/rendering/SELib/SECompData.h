#ifndef __SE_COMP_DATA__
#define __SE_COMP_DATA__

/******************************************************************************/
// Comp2x16
/******************************************************************************/

typedef struct Comp2x16
{
	Int16		x;
	Int16		y;

	void Comp( const Vector2& vIn )
	{
		x = (Int16)( vIn.x * 2000 );
		y = (Int16)( vIn.y * 2000 );
	}

	Vector2 Decomp()
	{
		Vector2 vOut;
		vOut.x = (Real)( x * 0.0005f );
		vOut.y = (Real)( y * 0.0005f );
		return vOut;
	}
} Comp2x16;

inline bool operator==( const Comp2x16& lhs, const Comp2x16& rhs )
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

/******************************************************************************/
// Comp3x16
/******************************************************************************/

typedef struct Comp3x16
{
	Int16		x;
	Int16		y;
	Int16		z;

	void Comp( const Vector3& vIn )
	{
		x = (Int16)( vIn.x * 2000 );
		y = (Int16)( vIn.y * 2000 );
		z = (Int16)( vIn.z * 2000 );
	}

	Vector3 Decomp()
	{
		Vector3 vOut;
		vOut.x = (Real)( x * 0.0005f );
		vOut.y = (Real)( y * 0.0005f );
		vOut.z = (Real)( z * 0.0005f );
		return vOut;
	}
} Comp3x16;

inline bool operator==( const Comp3x16& lhs, const Comp3x16& rhs )
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/******************************************************************************/
// Comp4x16
/******************************************************************************/

typedef struct Comp4x16
{
	Int16		x;
	Int16		y;
	Int16		z;
	Int16		w;

	Vector4 Decomp()
	{
		Vector4 vOut;
		vOut.x = (Real)( x * 0.0005f );
		vOut.y = (Real)( y * 0.0005f );
		vOut.z = (Real)( z * 0.0005f );
		vOut.w = (Real)( w * 0.0005f );
		return vOut;
	}

	Comp4x16(){}
	Comp4x16( const Comp3x16& in )
	{
		x = in.x;
		y = in.y;
		z = in.z;
		w = 0;
	}

	Comp4x16( const Comp2x16& in, Int16 imat )
	{
		x = in.x;
		y = in.y;
		z = imat;
		w = imat;
	}

} Comp4x16;

/******************************************************************************/
// Comp4x8
/******************************************************************************/

typedef struct Comp4x8
{
	UInt8		x;
	UInt8		y;
	UInt8		z;
	UInt8		w;

	void Comp( const Vector4& vIn )
	{
		x = (UInt8)( vIn.x * 255 );
		y = (UInt8)( vIn.y * 255 );
		z = (UInt8)( vIn.z * 255 );
		w = (UInt8)( vIn.w * 255 );
	}

	Vector4 Decomp()
	{
		Vector4 vOut;
		vOut.x = (Real)( x * ( 1.0f / 255.0f ) );
		vOut.y = (Real)( y * ( 1.0f / 255.0f ) );
		vOut.z = (Real)( z * ( 1.0f / 255.0f ) );
		vOut.w = (Real)( w * ( 1.0f / 255.0f ) );
		return vOut;
	}

	Comp4x8() {}
	Comp4x8( UInt32 i )
	{
		*(UInt32*)this = i;
	}
} Comp4x8;

inline bool operator==( const Comp4x8& lhs, const Comp4x8& rhs )
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/******************************************************************************/
// CompTan
/******************************************************************************/

typedef struct CompTan
{
	UInt8		x1;
	UInt8		y1;
	UInt8		z1;
	UInt8		x2;
	UInt8		y2;
	UInt8		z2;

	CompTan( const Vector3& vNormal, const Vector3& vTangent, const Vector3& vBitangent )
	{
		x1 = (UInt8)( ( vNormal.x + 1.0f ) * 127.5f );
		y1 = (UInt8)( ( vNormal.y + 1.0f ) * 127.5f );
		z1 = (UInt8)( ( vNormal.z + 1.0f ) * 127.5f );

		x2 = (UInt8)( ( vTangent.x + 1.0f ) * 127.5f );
		y2 = (UInt8)( ( vTangent.y + 1.0f ) * 127.5f );
		z2 = (UInt8)( ( vTangent.z + 1.0f ) * 127.5f );

		Real fSign = SIGN( Vector3::Dot( vBitangent, Vector3::Cross( vNormal, vTangent ) ) );
		z2 = ( z2 & 0xFE ) | ( fSign > 0.0f ? 0 : 1 );
	}

	Vector2 Pack()
	{
		Vector2 rtn;
		UInt32 packed;

		packed = ( x1 << 16 ) | ( y1 << 8 ) | z1;
		rtn.x = (Float32) ( ( (Float64)packed ) / ( (Float64)( 1 << 24 ) ) );

		packed = ( x2 << 16 ) | ( y2 << 8 ) | z2;
		rtn.y = (Float32) ( ( (Float64)packed ) / ( (Float64)( 1 << 24 ) ) );

		if( z2 & 0x01 )
			rtn.y = -rtn.y;

		return rtn;
	}

} CompTan;

inline void DecompTangents( const Vector2& vIn, Vector3& vOutNorm, Vector3& vOutTan, Vector3& vOutBitan )
{
	vOutNorm = Vector3( 1, 256, 65536 ) * vIn.x;
	vOutTan  = Vector3( 1, 256, 65536 ) * ABS( vIn.y );

	vOutNorm.x = FRAC( vOutNorm.x );
	vOutNorm.y = FRAC( vOutNorm.y );
	vOutNorm.z = FRAC( vOutNorm.z );

	vOutNorm.x = FRAC( vOutNorm.x ) * 2.0f - 1.0f;
	vOutNorm.y = FRAC( vOutNorm.y ) * 2.0f - 1.0f;
	vOutNorm.z = FRAC( vOutNorm.z ) * 2.0f - 1.0f;

	vOutTan.x = FRAC( vOutTan.x ) * 2.0f - 1.0f;
	vOutTan.y = FRAC( vOutTan.y ) * 2.0f - 1.0f;
	vOutTan.z = FRAC( vOutTan.z ) * 2.0f - 1.0f;

	vOutBitan = Vector3::Cross( vOutNorm, vOutTan );
	vOutBitan = vIn.y < 0.0f ? -vOutBitan : vOutBitan;
}

inline bool operator==( const CompTan& lhs, const CompTan& rhs )
{
	return lhs.x1 == rhs.x1 && lhs.y1 == rhs.y1 && lhs.z1 == rhs.z1 &&
		   lhs.x2 == rhs.x2 && lhs.y2 == rhs.y2 && lhs.z2 == rhs.z2;
}

/******************************************************************************/
// CompBone
/******************************************************************************/

typedef struct CompBone
{
	Int8		type;
	Int8		parent;
	Comp3x16	ofs;

} CompBone;

inline bool operator==( const CompBone& lhs, const CompBone& rhs )
{
	return lhs.type == rhs.type && lhs.parent == rhs.parent && lhs.ofs == rhs.ofs;
}

/******************************************************************************/
// CompWeight
/******************************************************************************/

typedef struct CompWeight
{
	UInt8		bone;
	UInt8		weight;

	void Comp( Int32 nBone, Float32 fWeight )
	{
		bone = nBone + 1;
		weight = (UInt8)( fWeight * 255 );
	}

	void Decomp( Int32& out_bone, Float32& out_weight )
	{
		out_bone = bone - 1;
		out_weight = (Float32)( weight * ( 1.0f / 255.0f ) );
	}

	Float32 PackWeight()
	{
		return ( (Float32)bone - 1.0f ) + ( (Float32)weight * ( 1.0f / 2550.0f ) );
	}

	CompWeight() {}
	CompWeight( UInt16 in )
	{
		*(UInt16*)this = in;
	}
} CompWeight;

inline bool operator==( const CompWeight& lhs, const CompWeight& rhs )
{
	return lhs.weight == rhs.weight && lhs.bone == rhs.bone;
}

inline void UnpackWeight( Float32 fIn, Int8& bone, Float32& weight )
{
	bone = (Int8)fIn;
	weight = (Float32)ABS( FRAC( fIn ) * 10.0f );
}

/******************************************************************************/
// CompVertex
/******************************************************************************/

typedef struct CompVertex
{
	UInt16			point;
	Comp4x8			color;

	UInt16			tangents;
	UInt16			coord1;
	UInt8			mat1;

	UInt16			coord2;
	UInt8			mat2;

	CompWeight		weight1;
	CompWeight		weight2;
	CompWeight		weight3;
	CompWeight		weight4;

} CompVertex;

inline bool operator==( const CompVertex& lhs, const CompVertex& rhs )
{
	return lhs.point == rhs.point && 
		   lhs.color == rhs.color &&

		   lhs.tangents == rhs.tangents &&
		   lhs.coord1 == rhs.coord1 &&
		   lhs.mat1 == rhs.mat1 &&

		   lhs.coord2 == rhs.coord2 &&
		   lhs.mat2 == rhs.mat2 &&

		   lhs.weight1 == rhs.weight1 &&
		   lhs.weight2 == rhs.weight2 &&
		   lhs.weight3 == rhs.weight3 &&
		   lhs.weight4 == rhs.weight4;
}


#endif
