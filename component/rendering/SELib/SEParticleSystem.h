#ifndef __SE_PARTICLE_SYSTEM__
#define __SE_PARTICLE_SYSTEM__

#include "SELib.h"
#include "../glhelpers.h"
#include <vector>

namespace SE
{
	struct EMITTER
	{
		Real fEmitRate;
		Real fEmitLife;

		Vector4 vTexParams;

		Vector4 vColorMin;
		Vector4 vColorRand;
		Vector4 vColorEnd;
		Real fShadeRand;

		Vector3 vPos;
		Real fPosOfs;
		Real fPosRand;

		Vector3 vVelDir;
		Real fVelMin;
		Real fVelRand;
		Real fVelAngleMin;
		Real fVelAngleRand;

		Int32 bShareVelPosAngle;

		Vector3 vAccel;

		Real fLifeMin;
		Real fLifeRand;

		Real fFadeMin;
		Real fFadeRand;

		Real fSizeMin;
		Real fSizeRand;

		Real fScaleMin;
		Real fScaleRand;

		Real fRotateRand;
		Real fAngleVelMin;
		Real fAngleVelRand;

		Real fResistance;
	};

	struct PARTICLE
	{
		Int32 nEmitter;

		Real fStartLife;
		Real fLife;
		Real fFade;
		Real fSize;
		Real fScale;
		Real fResistance;
		Real fAngleVel;
		Real fRotate;

		Vector3 vPos;
		Vector3 vVel;
		Vector3 vAccel;

		Vector4 vColorStart;
		Vector4 vColorEnd;
	};

	struct PARTICLEINST
	{
		Vector4	vPos;
		Vector4 vColor;
		Vector4 vTex;
		Quaternion qRot;
	};

	struct cstParticles
	{
		Vector4 vEyeUp;
		Vector4 vEyeSide;
		Matrix  matWVP;
	};

	class ParticleEmitter
	{
	private:
		struct S_ENTRY
		{
			Real  fDist;
			Int32 nIndex;

			inline bool operator()( S_ENTRY const& a, S_ENTRY const& b ) const
			{
				return a.fDist > b.fDist;
			}
		};

		struct E_DATA
		{
			Real  fEmitGap;
			Real  fLastEmitTime;
			Int32 nAddParticles;
		};

		static Int32	c_nNextID;
		Int32			m_nID;
		Int32			m_bExpired;
		Int32			m_nParticleCount;
		Int32			m_nLastParticle;
		Real			m_fLifeTime;
		Vector3			m_vGlobalAccel;

		Matrix			m_matWorld;
		Int32			m_nEmitterCount;
		EMITTER*		m_EmitterSettings;
		E_DATA*			m_EmitterData;

		Int32			m_nMaxParticles;
		PARTICLE*		m_aryParticles;
		S_ENTRY*		m_arySortedList;
		GLtex			m_texParticle;
		PARTICLEINST*	m_aryParticleInsts;
		GLmesh			m_mshParticle;

	public:
		Int32 GetID() { return m_nID; }
		const Matrix& GetWorldMatrix() { return m_matWorld; }
		const GLtex& GetTexture() { return m_texParticle; }
		const GLmesh& GetMesh() { return m_mshParticle; }
		const EMITTER& GetSettings( Int32 emitter ) { return m_EmitterSettings[emitter]; }
		Int32 Expired() { return m_bExpired; }

		Int32 Init();
		Int32 Draw( const Matrix& matViewProj );
		Int32 Process( Real fElapse, const Vector3& vEyePos );

		ParticleEmitter( Int32 nMaxParticles, const Matrix& matWorld, Int32 nEmitterCount, const EMITTER* EmitterSettings, const GLtex& glTexture );
		~ParticleEmitter();
	};

	class ParticleSystem
	{
	private:
		std::vector<ParticleEmitter*>	m_aryEmitters;

		GLeffect	m_eftParticle;
		GLbuffer	m_bufParticle;

	public:
		Int32 Init();

		Int32 AddEffect( Int32 nMaxParticles, const Matrix& matWorld, Int32 nEmitterCount, const EMITTER* EmitterSettings, const GLtex& glTexture );
		Int32 RemoveEffect( Int32 nID );

		Int32 Draw( const Quaternion& qViewAngle, const Matrix& matViewProj );
		Int32 Update( Real fElapse, const Vector3& vEyePos );

		ParticleSystem();
		~ParticleSystem();
	};
}

#endif
