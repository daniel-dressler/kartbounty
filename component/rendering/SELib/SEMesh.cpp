#include "SELib.h"
#include "SEMesh.h"
#include <stdlib.h>
#include <vector>

#define SafeFree(x)				{free(x);x=0;}

namespace SEG
{
	class Iter
	{
	private:
		Byte*	m_pData;
		Int32	m_nSize;
		Int32	m_idx;

	public:
		Int32	IsEnd()					{ return m_idx >= m_nSize; }
		Int32	SeekSet( Int32 s = 0 )	{ m_idx  = s; return m_idx; }
		Int32	Seek( Int32 s )			{ m_idx += s; return m_idx; }
		Int32	SeekEnd( Int32 s = 0 )	{ m_idx = m_nSize - s; return m_idx; }
		Int32	Tell() const 			{ return m_idx; }

		template <class D>
		D Read()
		{ 
			D& rtn = *(D*)&m_pData[m_idx];
			m_idx += sizeof(D);
			return rtn; 
		}

		Iter( Byte* pData, Int32 nSize )
		{
			m_pData = pData;
			m_nSize = nSize;
			m_idx = 0;
		}

		Iter( Iter& iter, Int32 nIdx )
		{
			m_pData = iter.m_pData;
			m_nSize = iter.m_nSize;
			m_idx = nIdx;
		}
	};

	Mesh::Mesh( Int32 nID )
	{
		m_nID = nID;

		m_nVertexCount = 0;
		m_nVertexSize = 0;
		m_nVertexDataSize = 0;
		m_aryVertices = 0;

		m_nTriangleCount = 0;
		m_nIndexCount = 0;
		m_nIndexDataSize = 0;
		m_aryIndices = 0;

		m_bTexturesUpdated = 0;
		m_nMaterialSet = -1;
		m_nMaterialCount = 0;
		m_aryMaterials = 0;

		m_nBoneCount = 0;
		m_aryBones = 0;
	}

	Mesh::~Mesh()
	{
		SafeFree( m_aryIndices );
		SafeFree( m_aryVertices );
		SafeFree( m_aryMaterials );
		SafeFree( m_aryBones );
	}

	Int32 Mesh::ReadData( Byte* pData, Int32 nSize, UInt32 uFlags, btTriangleMesh* pCollider, Vector3 vOfs )
	{
		uFlags = uFlags; // Hide unused param warning
		SafeFree( m_aryIndices );
		SafeFree( m_aryVertices );
		SafeFree( m_aryMaterials );
		SafeFree( m_aryBones );
		m_bTexturesUpdated = 0;

		Iter scan( pData, nSize );

		scan.Read<UInt32>();
		m_nType = scan.Read<Int32>();

		switch( m_nType )
		{
		case SE_MT_STATICSINGLE:
			m_nVertexSize = sizeof(VertexSS);
			break;
		case SE_MT_STATICDUAL:
			m_nVertexSize = sizeof(VertexSD);
			break;
		case SE_MT_BLENDSINGLE:
			m_nVertexSize = sizeof(VertexBS);
			break;
		case SE_MT_COLLIDER:
			m_nVertexSize = sizeof(VertexCldr);
			break;
		};
	
		m_nMaterialSet		= scan.Read<Int32>();
		Int32 nPointCount	= scan.Read<Int32>();
		Int32 nTangentCount	= scan.Read<Int32>();
		m_nMaterialCount	= scan.Read<Int32>();
		Int32 nCoordCount	= scan.Read<Int32>();
		Int32 nBoneCount	= scan.Read<Int32>();
		m_nVertexCount		= scan.Read<Int32>();
		m_nTriangleCount	= scan.Read<Int32>();
		m_nIndexCount		= m_nTriangleCount * 3;

		Comp3x16*	pPoints		= (Comp3x16*)  &pData[scan.Read<Int32>()];
		CompTan*	pTangents	= (CompTan*)   &pData[scan.Read<Int32>()];
		Int16*		pMaterials  = (Int16*)	   &pData[scan.Read<Int32>()];
		Comp2x16*	pCoords		= (Comp2x16*)  &pData[scan.Read<Int32>()];
		CompBone*	pBones		= (CompBone*)  &pData[scan.Read<Int32>()];
		Iter		scnVertices	= Iter( scan, scan.Read<Int32>() );
		Comp3x16*	pTriangles	= (Comp3x16*)  &pData[scan.Read<Int32>()];

		Int32 nFilesize = scan.Read<Int32>();

		// Unused vars
		// We need to keep the reads
		// to match the file format
		nPointCount = nPointCount;
		nTangentCount = nTangentCount;
		nCoordCount = nCoordCount;
		nFilesize = nFilesize;

		m_nVertexDataSize = m_nVertexCount * m_nVertexSize;
		m_nIndexDataSize = m_nIndexCount * sizeof(UInt16);

		m_aryVertices	= (Byte*)  MALLOC( m_nVertexDataSize );
		m_aryIndices	= (VIndex*)MALLOC( m_nIndexCount * sizeof(VIndex) );
		m_aryMaterials	= (Int16*) MALLOC( m_nMaterialCount * sizeof(Int16) );
		m_aryBones		= (Bone*)  MALLOC( m_nBoneCount * sizeof(Bone) );
		if( !m_aryVertices || 
			!m_aryIndices || 
			( m_nMaterialCount > 0 && !m_aryMaterials ) || 
			( m_nBoneCount > 0 && !m_aryBones ) )
			return 0;

		MEMCPY( m_aryIndices,   pTriangles, m_nTriangleCount * sizeof(Comp3x16) );
		MEMCPY( m_aryMaterials, pMaterials, m_nMaterialCount * sizeof(Int16) );

		for( Int32 i = 0; i < nBoneCount; i++ )
		{
			m_aryBones[i].nType   = pBones[i].type;
			m_aryBones[i].nParent = pBones[i].parent;
			m_aryBones[i].vOffset = pBones[i].ofs.Decomp();
		}

		std::vector<Vector3> aryVertices;


		switch( m_nType )
		{
		case SE_MT_STATICSINGLE:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexSS& vtx = ((VertexSS*)m_aryVertices)[i];
				vtx.Pos = pPoints[scnVertices.Read<UInt16>()];
				vtx.Color = scnVertices.Read<UInt32>();
				vtx.Tan = pTangents[scnVertices.Read<UInt16>()].Pack();
				
				aryVertices.push_back( vtx.Pos.Decomp().xyz() );

				Comp2x16 tex = pCoords[scnVertices.Read<UInt16>()];
				UInt8 mat = scnVertices.Read<UInt8>();
				vtx.Tex = Comp4x16( tex, mat );
			}
			if( pCollider )
			{
				for( Int32 t = 0; t < m_nTriangleCount; t++ )
				{
					Int32 i = t * 3;
					Int32 ia = m_aryIndices[i];
					Int32 ib = m_aryIndices[i+1];
					Int32 ic = m_aryIndices[i+2];
					btVector3 a = btVector3( aryVertices[ia].x + vOfs.x, aryVertices[ia].y + vOfs.y, aryVertices[ia].z + vOfs.z );
					btVector3 b = btVector3( aryVertices[ib].x + vOfs.x, aryVertices[ib].y + vOfs.y, aryVertices[ib].z + vOfs.z );
					btVector3 c = btVector3( aryVertices[ic].x + vOfs.x, aryVertices[ic].y + vOfs.y, aryVertices[ic].z + vOfs.z );

					pCollider->addTriangle( a, b, c, 1 );
					
					a.setX( -a.x() ); b.setX( -b.x() ); c.setX( -c.x() );
					pCollider->addTriangle( c, b, a, 1 );
					a.setZ( -a.z() ); b.setZ( -b.z() ); c.setZ( -c.z() );
					pCollider->addTriangle( a, b, c, 1 );
					a.setX( -a.x() ); b.setX( -b.x() ); c.setX( -c.x() );
					pCollider->addTriangle( c, b, a, 1 );
					
				}
			}
			break;
		case SE_MT_STATICDUAL:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexSD& vtx = ((VertexSD*)m_aryVertices)[i];
				vtx.Pos = pPoints[scnVertices.Read<UInt16>()];
				vtx.Color = scnVertices.Read<UInt32>();
				vtx.Tan = pTangents[scnVertices.Read<UInt16>()].Pack();

				Comp2x16 tex = pCoords[scnVertices.Read<UInt16>()];
				UInt8 mat = scnVertices.Read<UInt8>();
				vtx.Tex1 = Comp4x16( tex, mat );

				tex = pCoords[scnVertices.Read<UInt16>()];
				mat = scnVertices.Read<UInt8>();
				vtx.Tex2 = Comp4x16( tex, mat );
			}
			break;
		case SE_MT_BLENDSINGLE:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexBS& vtx = ((VertexBS*)m_aryVertices)[i];
				vtx.Pos = pPoints[scnVertices.Read<UInt16>()];
				vtx.Color = scnVertices.Read<UInt32>();
				vtx.Tan = pTangents[scnVertices.Read<UInt16>()].Pack();

				Comp2x16 tex = pCoords[scnVertices.Read<UInt16>()];
				UInt8 mat = scnVertices.Read<UInt8>();
				vtx.Tex = Comp4x16( tex, mat );

				CompWeight w1 = scnVertices.Read<UInt16>();
				CompWeight w2 = scnVertices.Read<UInt16>();
				CompWeight w3 = scnVertices.Read<UInt16>();
				CompWeight w4 = scnVertices.Read<UInt16>();
				vtx.Weights = Vector4( w1.PackWeight(), w2.PackWeight(), w3.PackWeight(), w4.PackWeight() );
			}
			break;
		case SE_MT_COLLIDER:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexCldr& vtx = ((VertexCldr*)m_aryVertices)[i];
				vtx.Pos = pPoints[scnVertices.Read<UInt16>()];
				vtx.Color = scnVertices.Read<UInt32>();
			}
			break;
		}

		return 1;
	}

	Int32 Mesh::UpdateTextures( Int16* aryDiffuse, Int16* aryEffects, Int32 nSize )
	{
		if( m_bTexturesUpdated )
			return 0;

		if( m_nMaterialCount != nSize )
			return 0;

		switch( m_nType )
		{
		case SE_MT_STATICSINGLE:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexSS& vtx = ((VertexSS*)m_aryVertices)[i];
				vtx.Tex.z = aryDiffuse[vtx.Tex.z];
				vtx.Tex.w = aryEffects[vtx.Tex.w];
			}
			break;
		case SE_MT_STATICDUAL:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexSD& vtx = ((VertexSD*)m_aryVertices)[i];
				vtx.Tex1.z = aryDiffuse[vtx.Tex1.z];
				vtx.Tex1.w = aryEffects[vtx.Tex1.w];
				vtx.Tex2.z = aryDiffuse[vtx.Tex2.z];
				vtx.Tex2.w = aryEffects[vtx.Tex2.w];
			}
			break;
		case SE_MT_BLENDSINGLE:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexBS& vtx = ((VertexBS*)m_aryVertices)[i];
				vtx.Tex.z = aryDiffuse[vtx.Tex.z];
				vtx.Tex.w = aryEffects[vtx.Tex.w];
			}
			break;
		}

		m_bTexturesUpdated = 1;

		return 1;
	}

	Int32 Mesh::SetTextures( Int16 nDiffuse, Int16 nEffect, Int32 nSet )
	{
		switch( m_nType )
		{
		case SE_MT_STATICSINGLE:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexSS& vtx = ((VertexSS*)m_aryVertices)[i];
				vtx.Tex.z = nDiffuse;
				vtx.Tex.w = nEffect;
			}
			break;
		case SE_MT_STATICDUAL:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexSD& vtx = ((VertexSD*)m_aryVertices)[i];
				if( nSet == 0 )
				{
					vtx.Tex1.z = nDiffuse;
					vtx.Tex1.w = nEffect;
				}
				else
				{
					vtx.Tex2.z = nDiffuse;
					vtx.Tex2.w = nEffect;
				}
			}
			break;
		case SE_MT_BLENDSINGLE:
			for( Int32 i = 0; i < m_nVertexCount; i++ )
			{
				VertexBS& vtx = ((VertexBS*)m_aryVertices)[i];
				vtx.Tex.z = nDiffuse;
				vtx.Tex.w = nEffect;
			}
			break;
		}

		m_bTexturesUpdated = 1;

		return 1;
	}
}
