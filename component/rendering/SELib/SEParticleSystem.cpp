#include "../../../Standard.h"
#include "SEParticleSystem.h"

using namespace SE;

Int32 ParticleEmitter::c_nNextID = 1;

ParticleSystem::ParticleSystem()
{
}

ParticleSystem::~ParticleSystem()
{
	for( Int32 i = 0; i < m_aryEmitters.size(); i++ )
	{
		SafeDelete( m_aryEmitters[i] );
	}
}

Int32 ParticleSystem::Init()
{
	m_eftParticle = glhLoadEffect( "component/rendering/ParticleVShader.glsl", 0, "component/rendering/ParticlePShader.glsl", 0, 0 );
	if( !m_eftParticle.program )
		return _ERR;

	glhMapTexture( m_eftParticle, "g_texParticle", 0 );

	if( !glhCreateBuffer( m_eftParticle, "cstParticles", sizeof(cstParticles), &m_bufParticle ) )
		return _ERR;

	return _OK;
}

Int32 ParticleSystem::AddEffect( Int32 nMaxParticles, const Matrix& matWorld, Int32 nEmitterCount, const EMITTER* EmitterSettings, const GLtex& glTexture )
{
	ParticleEmitter* pEmitter = new ParticleEmitter( nMaxParticles, matWorld, nEmitterCount, EmitterSettings, glTexture );
	pEmitter->Init();
	m_aryEmitters.push_back( pEmitter );
	return pEmitter->GetID();
}

Int32 ParticleSystem::RemoveEffect( Int32 nID )
{
	for( Int32 i = 0; i < m_aryEmitters.size(); i++ )
	{
		if( m_aryEmitters[i]->GetID() == nID )
		{
			SafeDelete( m_aryEmitters[i] );
			m_aryEmitters.erase( m_aryEmitters.begin() + i );
			return _OK;
		}
	}
	return _ERR;
}

Int32 ParticleSystem::Draw( const Quaternion& qViewAngle, const Matrix& matViewProj )
{
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	cstParticles& p = *(cstParticles*)m_bufParticle.data;
	p.vEyeUp = Vector3( 0,1,0 ).Transform( qViewAngle );
	p.vEyeSide = Vector3( 1,0,0 ).Transform( qViewAngle );

	for( Int32 i = 0; i < m_aryEmitters.size(); i++ )
	{
		p.matWVP = m_aryEmitters[i]->GetWorldMatrix() * matViewProj;
		glhUpdateBuffer( m_eftParticle, m_bufParticle );

		glhEnableTexture( m_aryEmitters[i]->GetTexture(), 0 );
		glhDrawInst( m_eftParticle, m_aryEmitters[i]->GetMesh() );
	}

	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );

	return _OK;
}

Int32 ParticleSystem::Update( Real fElapse, const Vector3& vEyePos )
{
	for( Int32 i = m_aryEmitters.size()-1; i >= 0; i-- )
	{
		if( m_aryEmitters[i]->Expired() )
		{
			SafeDelete( m_aryEmitters[i] );
			m_aryEmitters.erase( m_aryEmitters.begin() + i );
		}
	}

	for( Int32 i = 0; i < m_aryEmitters.size(); i++ )
	{
		m_aryEmitters[i]->Process( fElapse, vEyePos );
	}

	return _OK;
}


ParticleEmitter::ParticleEmitter( Int32 nMaxParticles, const Matrix& matWorld, Int32 nEmitterCount, const EMITTER* EmitterSettings, const GLtex& glTexture )
	: m_texParticle( glTexture )
{
	// Init Emitter Settings
	m_nID = c_nNextID++;
	m_bExpired = 0;
	m_nParticleCount = 0;
	m_nLastParticle = 0;
	m_fLifeTime = 0;
	m_vGlobalAccel.Zero();

	// Init Emitter Data
	m_matWorld = matWorld;
	m_nEmitterCount = nEmitterCount;
	m_EmitterSettings = new EMITTER[nEmitterCount];
	m_EmitterData = new E_DATA[nEmitterCount];
	MEMCPY( m_EmitterSettings, EmitterSettings, sizeof(EMITTER) * nEmitterCount );
	for( Int32 i = 0; i < m_nEmitterCount; i++ )
	{
		m_EmitterData[i].fEmitGap = 1.0f / m_EmitterSettings[i].fEmitRate;
		m_EmitterData[i].fLastEmitTime = 0;
		m_EmitterData[i].nAddParticles = 0;
	}

	// Init Particles
	m_nMaxParticles = nMaxParticles;
	m_aryParticles = new PARTICLE[m_nMaxParticles];
	m_arySortedList = new S_ENTRY[m_nMaxParticles];
	m_aryParticleInsts = new PARTICLEINST[nMaxParticles];
	MEMSET( m_aryParticles, 0, sizeof(PARTICLE) * m_nMaxParticles );
	for( Int32 i = 0; i < m_nMaxParticles; i++ )
	{
		m_arySortedList[i].fDist = 0;
		m_arySortedList[i].nIndex = i;
	}
}

ParticleEmitter::~ParticleEmitter()
{
	glhDestroyMesh( m_mshParticle );
	SafeDelete( m_aryParticles );
	SafeDelete( m_arySortedList );
}

Int32 ParticleEmitter::Init()
{
	Vector2 aryCorners[] = { Vector2( 0, 0 ), Vector2( 0, 1 ), Vector2( 1, 0 ), Vector2( 1, 1 ) };
	glhCreateMesh( m_mshParticle, aryCorners, sizeof(Vector2), 4, 20 );
	glhCreateInst( m_mshParticle, 0, sizeof(PARTICLEINST), 0, 30 );

	return _OK;
}

Int32 ParticleEmitter::Process( Real fElapse, const Vector3& vEyePos )
{
	m_fLifeTime += fElapse;

	static Int32 nTempAdd = 0;

	Int32 bActive = 0;
	Int32 nAddEmitter = 0;
	for( Int32 i = 0; i < m_nEmitterCount; i++ )
	{
		m_EmitterData[i].nAddParticles = m_fLifeTime > m_EmitterSettings[i].fEmitLife && m_EmitterSettings[i].fEmitLife >= 0 ?
			0 : (Int32)( ( m_fLifeTime - m_EmitterData[i].fLastEmitTime ) * m_EmitterSettings[i].fEmitRate );

		if( nAddEmitter == i && m_EmitterData[i].nAddParticles == 0 )
			nAddEmitter++;

		bActive |= m_EmitterSettings[i].fEmitLife < 0 || m_fLifeTime < m_EmitterSettings[i].fEmitLife;

		nTempAdd += m_EmitterData[i].nAddParticles;
	}

	Int32 nFound = 0;
	Int32 nCount = 0;
	Int32 bFilled = 0;
	Int32 bDegen = 0;
	Int32 nStartCount = m_nParticleCount;
	m_nLastParticle = 0;
	while( m_nLastParticle < m_nMaxParticles && ( nFound < nStartCount || nAddEmitter < m_nEmitterCount ) )
	{
		PARTICLE& p = m_aryParticles[m_arySortedList[m_nLastParticle].nIndex];

		bDegen = 0;
		bFilled = 0;
		if( p.fLife > 0.001f )
		{
			nFound++;
			Real fNewLife = p.fLife - fElapse;
			if( fNewLife > 0.001f )
			{
				// Update Particle
				p.fLife = fNewLife;
				p.vVel = p.vVel * ( 1 - p.fResistance * fElapse ) + ( p.vAccel + m_vGlobalAccel ) * fElapse;
				p.vPos += p.vVel * fElapse;
				p.fSize += p.fScale * fElapse;
				p.fRotate += p.fAngleVel * fElapse;

				// Update Distance
				m_arySortedList[m_nLastParticle].fDist = ( p.vPos - vEyePos ).SquaredLength();
			
				bFilled = 1;
			}
			else
			{
				m_nParticleCount--;
				p.fLife = 0;
				m_arySortedList[m_nLastParticle].fDist = 0;
			}
		}
		
		if( !bFilled && nAddEmitter < m_nEmitterCount )
		{
			const Real fRandAdj = 0.0000305185094f;

			EMITTER& settings = m_EmitterSettings[nAddEmitter];
			E_DATA& data = m_EmitterData[nAddEmitter];

			// Fill with new particle
			Real fShade = settings.fShadeRand * fRandAdj * ( rand() % 32767 );
			p.nEmitter = nAddEmitter;
			p.vColorStart = settings.vColorMin + Vector4( fShade, fShade, fShade, 0 )
				+ settings.vColorRand * Vector4( fRandAdj * ( rand() % 32767 ), 
												 fRandAdj * ( rand() % 32767 ), 
												 fRandAdj * ( rand() % 32767 ), 
												 fRandAdj * ( rand() % 32767 ) );
			p.vColorEnd = settings.vColorEnd;
			p.vPos.x = settings.fPosOfs + settings.fPosRand * fRandAdj * ( rand() % 32767 );
			Real fXZAngle = ( 6.28f * fRandAdj * ( rand() % 32767 ) );
			p.vPos.RotateXZ( fXZAngle );
			p.vPos += settings.vPos;

			p.vVel = settings.vVelDir * ( settings.fVelMin + fRandAdj * settings.fVelRand * ( rand() % 32767 ) );

			Real fAngle = fRandAdj * ( rand() % 32767 );
			fAngle = fAngle*(fAngle*(fAngle*6-15)+10)*0.5f;
			fAngle = DEGTORAD( settings.fVelAngleRand ) * -fAngle;
			fAngle += DEGTORAD( settings.fVelAngleMin );
			if( !settings.bShareVelPosAngle )
				fXZAngle = ( 6.28f * fRandAdj * ( rand() % 32767 ) );

			p.vVel.RotateXY( fAngle );
			p.vVel.RotateXZ( fXZAngle );
			p.vAccel = settings.vAccel;
			p.fStartLife = p.fLife = settings.fLifeMin + ( settings.fLifeRand * fRandAdj * ( rand() % 32767 ) );
			p.fFade = settings.fFadeMin + ( settings.fFadeRand * fRandAdj * ( rand() % 32767 ) );
			p.fSize = settings.fSizeMin + ( settings.fSizeRand * fRandAdj * ( rand() % 32767 ) );
			p.fScale = settings.fScaleMin + ( settings.fScaleRand * fRandAdj * ( rand() % 32767 ) );
			p.fResistance = settings.fResistance;
			p.fRotate = settings.fRotateRand * ( 6.28f * fRandAdj * ( rand() % 32767 ) );
			p.fAngleVel = settings.fAngleVelMin + settings.fAngleVelRand * ( ( 1.57f * fRandAdj * ( rand() % 32767 ) ) - 0.785f );
			m_nParticleCount++;

			p.vPos += p.vVel * fElapse * fRandAdj * ( rand() % 32767 );

			// Update Distance
			m_arySortedList[m_nLastParticle].fDist = ( p.vPos - vEyePos ).SquaredLength();

			bDegen = 1;
			bFilled = 1;

			data.fLastEmitTime += data.fEmitGap;
			data.nAddParticles--;
			
			while( m_EmitterData[nAddEmitter].nAddParticles == 0 && nAddEmitter < m_nEmitterCount )
				nAddEmitter++;
		}

		if( bFilled )
		{
			m_aryParticleInsts[nCount].vPos = Vector4( p.vPos, bDegen ? 0 : p.fSize );
			m_aryParticleInsts[nCount].qRot = Quaternion( 0,0,0, p.fRotate );
			m_aryParticleInsts[nCount].vTex = m_EmitterSettings[p.nEmitter].vTexParams;
			m_aryParticleInsts[nCount].vColor = Vector4::Lerp( p.vColorEnd, p.vColorStart, p.fLife / p.fStartLife );
			if( p.fFade > p.fLife )
				m_aryParticleInsts[nCount].vColor.w = p.fLife / p.fFade * p.vColorStart.w;
			nCount++;
		}

		m_nLastParticle++;
	}

	m_nParticleCount = nCount;
	m_bExpired = !( bActive || m_nParticleCount > 0 );

	for( Int32 i = 0; i < m_nEmitterCount; i++ )
		m_EmitterData[nAddEmitter].fLastEmitTime += m_EmitterData[nAddEmitter].fEmitGap * m_EmitterData[nAddEmitter].nAddParticles;

	std::sort( m_arySortedList, m_arySortedList + m_nLastParticle, S_ENTRY() );

	glhUpdateInst( m_mshParticle, m_aryParticleInsts, sizeof(PARTICLEINST), m_nParticleCount );

	return _OK;
}
