#ifndef __SE_MESH__
#define __SE_MESH__

#define SE_MT_STATICSINGLE			1
#define SE_MT_STATICDUAL			2
#define SE_MT_BLENDSINGLE			3
#define SE_MT_COLLIDER				10

#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

namespace SEG
{
	struct VertexSS
	{
		Comp4x16	Pos;		// Pos x,y,z   ?
		Vector2		Tan;		// Norm, Tan, Bitan
		Comp4x8		Color;		// Color
		Comp4x16	Tex;		// Tex u,v,dif,fx
	};

	struct VertexSD
	{
		Comp4x16	Pos;		// Pos x,y,z   ?
		Vector2		Tan;		// Norm, Tan, Bitan 
		Comp4x8		Color;		// Color
		Comp4x16	Tex1;		// Tex1 u,v,dif,fx
		Comp4x16	Tex2;		// Tex2 u,v,dif,fx
	};

	struct VertexBS
	{
		Comp4x16	Pos;		// Pos x,y,z   ?
		Vector2		Tan;		// Norm, Tan, Bitan
		Comp4x8		Color;		// Color
		Comp4x16	Tex;		// Tex u,v,dif,fx
		Vector4		Weights;	// BoneIndex.BoneWeight/10
	};

	struct VertexCldr
	{
		Comp4x16	Pos;		// Pos x,y,z   ?
		Comp4x8		Color;		// Color
	};

	struct Bone
	{
		Int8	nType;
		Int8	nParent;
		Vector3	vOffset;
	};

	class Mesh
	{
	private:
		Int32		m_nID;
		Int32		m_nType;

		Int32		m_nVertexCount;
		Int32		m_nVertexSize;
		Int32		m_nVertexDataSize;
		Byte*		m_aryVertices;

		Int32		m_nTriangleCount;
		Int32		m_nIndexCount;
		Int32		m_nIndexDataSize;
		VIndex*		m_aryIndices;

		Int32		m_bTexturesUpdated;
		Int32		m_nMaterialSet;
		Int32		m_nMaterialCount;
		Int16*		m_aryMaterials;

		Int32		m_nBoneCount;
		Bone*		m_aryBones;

	public:
		Int32 GetID() const					{ return m_nID; }
		Int32 GetType() const				{ return m_nType; }

		Int32 HasDualTexture() const		{ return m_nType == SE_MT_STATICDUAL; }
		Int32 HasBlending() const			{ return m_nType == SE_MT_BLENDSINGLE; }

		Int32 GetVertexCount() const		{ return m_nVertexCount; }
		Int32 GetVertexSize() const			{ return m_nVertexSize; }
		Int32 GetVertexDataSize() const		{ return m_nVertexDataSize; }
		Byte* GetVertexData() const			{ return (Byte*)m_aryVertices; }

		Int32 GetTriangleCount() const		{ return m_nTriangleCount; }
		Int32 GetIndexCount() const			{ return m_nIndexCount; }
		Int32 GetIndexDataSize() const		{ return m_nIndexDataSize; }
		Byte* GetIndexData() const			{ return (Byte*)m_aryIndices; }

		Int32 AreTexturesUpdated() const	{ return m_bTexturesUpdated; }
		Int32 GetMaterialSet() const		{ return m_nMaterialSet; }
		Int32 GetMaterialCount() const		{ return m_nMaterialCount; }
		Int16* GetMaterialList() const		{ return m_aryMaterials; }

		Int32 GetBoneCount() const			{ return m_nBoneCount; }
		Bone* GetBoneList() const			{ return m_aryBones; }

		Int32 ReadData( Byte* pData, Int32 nSize, UInt32 uFlags = 0, btTriangleMesh* pCollider = 0 );
		Int32 UpdateTextures( Int16* aryDiffuse, Int16* aryEffects, Int32 nSize );
		Int32 SetTextures( Int16 nDiffuse, Int16 nEffect, Int32 nSet = 0 );

		Mesh( Int32 nID = -1 );
		virtual ~Mesh();
	};
}

#endif
