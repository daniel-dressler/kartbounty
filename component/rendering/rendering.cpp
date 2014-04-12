#include <vector>
#include <functional>

#include "rendering.h"
#include "../physics/physics.h"

struct GuiBox
{
	GLvertex box[6];

	void Make( Real x, Real y, Real w, Real h, const Vector4& color )
	{
		Real hw = w * 0.5f;
		Real hh = h * 0.5f;
		box[0] = GLvertex( Vector3( -hw + x, -hh + y, 0 ), Vector2( 0, 1 ), color );
		box[1] = GLvertex( Vector3( -hw + x,  hh + y, 0 ), Vector2( 0, 0 ), color );
		box[2] = GLvertex( Vector3(  hw + x, -hh + y, 0 ), Vector2( 1, 1 ), color );
		box[3] = GLvertex( Vector3( -hw + x,  hh + y, 0 ), Vector2( 0, 0 ), color );
		box[4] = GLvertex( Vector3(  hw + x,  hh + y, 0 ), Vector2( 1, 0 ), color );
		box[5] = GLvertex( Vector3(  hw + x, -hh + y, 0 ), Vector2( 1, 1 ), color );
	}

	void Num( Int32 n )
	{
		const Real fSpread = 1.0f / 10.0f;
		box[0].vTex.x = box[1].vTex.x = box[3].vTex.x = n * fSpread;
		box[2].vTex.x = box[4].vTex.x = box[5].vTex.x = ( n + 1 ) * fSpread;
	}

	inline operator const GLvertex* () const { return (GLvertex*)this; }
	inline GLvertex& operator[]( const int i ) { return box[i]; }
	GuiBox( Real x, Real y, Real w, Real h, const Vector4& color = Vector4(1,1,1,1) ) { Make( x, y, w, h, color ); }
	GuiBox() {}
};

std::map<int, struct Physics::Simulation::bullet *> list_of_bullets;
std::map<int, struct Physics::Simulation::rocket *> list_of_rockets;
std::vector<entity_id> kartsByScore;

Renderer::Renderer()
{
	m_bInitComplete = 0;
	m_Window = 0;
	m_nSplitScreen = 1;
	m_nScreenState = RS_START;
	m_fTime = 0;

	m_pMailbox = new Events::Mailbox();
	m_pMailbox->request( Events::EventType::Input );
	m_pMailbox->request( Events::EventType::StateUpdate );
	m_pMailbox->request( Events::EventType::PlayerKart );
	m_pMailbox->request( Events::EventType::AiKart );
	m_pMailbox->request( Events::EventType::KartCreated );
	m_pMailbox->request( Events::EventType::KartDestroyed );
	m_pMailbox->request( Events::EventType::PowerupPlacement );
	m_pMailbox->request( Events::EventType::PowerupDestroyed );
	m_pMailbox->request( Events::EventType::PowerupActivated );
	m_pMailbox->request( Events::EventType::PowerupPickup );
	m_pMailbox->request( Events::EventType::BulletList );
	m_pMailbox->request( Events::EventType::ScoreBoardUpdate );
	m_pMailbox->request( Events::EventType::RoundStart );
	m_pMailbox->request( Events::EventType::RoundEnd );
	m_pMailbox->request( Events::EventType::Explosion );

	m_vArenaOfs = Vector3( -10,0,10 );
}

Renderer::~Renderer()
{
	glhDestroyMesh( m_mshArenaCldr );

	glhDestroyMesh( m_mshArenaWalls );
	glhDestroyTexture( m_difArenaWalls );
	glhDestroyTexture( m_nrmArenaWalls );

	glhDestroyMesh( m_mshArenaFlags );
	glhDestroyTexture( m_difArenaFlags );
	glhDestroyTexture( m_nrmArenaFlags );

	glhDestroyMesh( m_mshArenaTops );

	glhDestroyMesh( m_mshArenaFloor );
	glhDestroyTexture( m_difArenaFloor );
	glhDestroyTexture( m_nrmArenaFloor );

	glhDestroyMesh( m_mshKart );

	glhDestroyMesh( m_mshPowerRing1 );
	glhDestroyMesh( m_mshPowerRing2 );
	glhDestroyMesh( m_mshPowerSphere );

	glhDestroyBuffer( m_bufPerMesh );
	glhDestroyBuffer( m_bufPerFrame );
	glhUnloadEffect( m_eftMesh );
	SDL_DestroyWindow( m_Window );
	SDL_Quit();

	if( m_pMailbox )
		delete m_pMailbox;
}

int LoadMesh( GLmesh& mesh, char* strFilename )
{
	GLchar* pData;
	Int32 nSize;
	if( !glhReadFile( strFilename, pData, nSize ) )
		return 0;

	SEG::Mesh meshdata;
	if( !meshdata.ReadData( (Byte*)pData, nSize ) )
		return 0;

	free( pData );

	if( !glhCreateMesh( mesh, meshdata ) )
		return 0;

	return 1;
}

int Renderer::setup()
{
	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	SDL_Window *win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
			);
	if( !win )
		exit(1);
	m_Window = win;

	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		exit(2);

	SDL_GL_MakeCurrent( win, glcontext );

	// Note: default glew only inits 2.0 and lower apis
	glewExperimental = GL_TRUE;
	if( glewInit() != GLEW_OK )
		exit(4);

	// OpenGL Test Info
	//unused const GLubyte* strVendor = glGetString( GL_VENDOR );
	GLint major, minor;
	glGetIntegerv( GL_MAJOR_VERSION, &major );
	glGetIntegerv( GL_MINOR_VERSION, &minor );

	// OpenGL State Init
	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glEnable( GL_MULTISAMPLE );
	
	// Load Standard Effects
	const GLchar* aryHeaders[] = { "component/rendering/ShaderStructs.glsl" };
	m_eftMesh = glhLoadEffect( "component/rendering/VShader.glsl", NULL, "component/rendering/PShader.glsl", aryHeaders, 1 );
	if( !m_eftMesh.program )
		exit(5);

	m_eftGUI = glhLoadEffect( "component/rendering/VShaderGUI.glsl", NULL, "component/rendering/PShaderGUI.glsl", 0, 0 );
	if( !m_eftGUI.program )
		exit(5);

	glhCreateBuffer( m_eftMesh, "cstPerMesh", sizeof(cstPerMesh), &m_bufPerMesh );
	glhCreateBuffer( m_eftMesh, "cstPerFrame", sizeof(cstPerFrame), &m_bufPerFrame );
	glhCreateBuffer( m_eftGUI, "cstGUI", sizeof(cstGUI), &m_bufGUI );
	
	{
		GLchar* pData;
		Int32 nSize;
		if( !glhReadFile( "assets/arena_cldr.msh", pData, nSize ) )
			exit(6);

		SEG::Mesh meshdata;
		btTriangleMesh *arena_mesh = new btTriangleMesh();
		if( !meshdata.ReadData( (Byte*)pData, nSize, 0, arena_mesh, m_vArenaOfs ) )
			exit(7);

		// Send Arena to phsyics
		std::vector<Events::Event *> events;
		auto arena_event = NEWEVENT(ArenaMeshCreated);
		arena_event->arena = arena_mesh;
		events.push_back(arena_event);
		m_pMailbox->sendMail(events);

		free( pData );

		if( !glhCreateMesh( m_mshArenaCldr, meshdata ) )
			exit(8);
	}

	if( !LoadMesh( m_mshSkybox, "assets/skybox.msh" ) )
		exit(9);
	if( !glhLoadTexture( m_texSkybox, "assets/skybox.png" ) )
		exit(10);

	if( !LoadMesh( m_mshArenaWalls, "assets/arena_walls.msh" ) )
		exit(9);
	if( !glhLoadTexture( m_difArenaWalls, "assets/arena_walls_diff.png" ) )
		exit(10);
	if( !glhLoadTexture( m_nrmArenaWalls, "assets/arena_walls_norm.png" ) )
		exit(11);

	if( !LoadMesh( m_mshArenaFlags, "assets/arena_flags.msh" ) )
		exit(12);
	if( !glhLoadTexture( m_difArenaFlags, "assets/arena_flags_diff.png" ) )
		exit(13);
	if( !glhLoadTexture( m_nrmArenaFlags, "assets/arena_flags_norm.png" ) )
		exit(14);

	if( !LoadMesh( m_mshArenaFloor, "assets/arena_floor.msh" ) )
		exit(15);
	if( !glhLoadTexture( m_difArenaFloor, "assets/arena_floor_diff.png" ) )
		exit(16);
	if( !glhLoadTexture( m_nrmArenaFloor, "assets/arena_floor_norm.png" ) )
		exit(17);

	if( !LoadMesh( m_mshArenaTops, "assets/arena_tops.msh" ) )
		exit(18);

	if( !LoadMesh( m_mshGold, "assets/Gold.msh" ) )
		exit(21);

	if( !LoadMesh( m_mshBullet, "assets/Bullet.msh" ) )
		exit(21);
	
	if( !LoadMesh( m_mshKart, "assets/Kart.msh" ) )
		exit(21);
	if( !LoadMesh( m_mshKartTire, "assets/Kart_tire.msh" ) )
		exit(21);
	if( !LoadMesh( m_mshKartShadow, "assets/Kart_shadow.msh" ) )
		exit(21);
	if( !glhLoadTexture( m_difKartShadow, "assets/kart_shadow.png" ) )
		exit(19);

	if( !LoadMesh( m_mshPowerSphere, "assets/PowerSphere.msh" ) )
		exit(22);
	if( !LoadMesh( m_mshPowerRing1, "assets/PowerRing1.msh" ) )
		exit(23);
	if( !LoadMesh( m_mshPowerRing2, "assets/PowerRing2.msh" ) )
		exit(24);

	if( !glhLoadTexture( m_difBlank, "assets/blank.png" ) )
		exit(19);
	if( !glhLoadTexture( m_nrmBlank, "assets/blank_norm.png" ) )
		exit(20);

	if( !glhCreateGUI( m_mshGUIStart, GuiBox( 0, 0, 600, 400 ), 6 ) )
		exit(25);
	if( !glhLoadTexture( m_texGUIStart, "assets/KartStartScreen.png" ) )
		exit(14);
	if( !glhLoadTexture( m_texGUIScore, "assets/KartScoreScreen.png" ) )
		exit(14);
	if( !glhLoadTexture( m_texGUINumbers, "assets/gui_numbers.png" ) )
		exit(14);
	if( !glhLoadTexture( m_texGUIPlayer, "assets/gui_player.png" ) )
		exit(14);

	if( !glhLoadTexture( m_texParticle, "assets/Explode.png", 0 ) )
		exit(15);

	
	glhMapTexture( m_eftMesh, "g_texDiffuse", 0 );
	glhMapTexture( m_eftMesh, "g_texNormal", 1 );
	glhMapTexture( m_eftGUI, "g_texDiffuse", 0 );
	
	if( Failed( m_ps.Init() ) )
		exit(16);
	/*
	Int32 build = 0;
	Int32 nEmitterCount = 0;
	SE::EMITTER es[5];
	MEMSET( es, 0, sizeof(SE::EMITTER) * 5 );

	switch( build )
	{
	case 0:
	case 1:
		nEmitterCount = 1;
		glhLoadTexture( m_texParticle, "assets/Particle2.png", 0 );
		glClearColor( 0.0f, 0.0f, 0.0f, 1 );
		es[0].fEmitRate = build ? 4000 : 500;
		es[0].fEmitLife = -1.0f;

		es[0].vTexParams = Vector4( 0, 0, 1, 1 );

		es[0].vColorEnd = es[0].vColorMin = Vector4( 1.0f, 0.0f, 0.0f, 1.0f );
		es[0].vColorRand = Vector4( 0.0f, 1.0f, 0.0f, 0.0f );
		es[0].fShadeRand = 0.0f;

		es[0].vPos = Vector3( 0, -50, 0 );
		es[0].fPosOfs = 0.0f;
		es[0].fPosRand = 1.0f;

		es[0].vVelDir = Vector3( 0, 1, 0 );
		es[0].fVelMin = 80.0f;
		es[0].fVelRand = 20.0f;
		es[0].fVelAngle = 30.0f;

		es[0].bShareVelPosAngle = 0;

		es[0].vAccel = Vector3( 50,0,0 );

		es[0].fLifeMin = 2.0f;
		es[0].fLifeRand = 1.0f;
		es[0].fFadeMin = 2.0f;
		es[0].fFadeRand = 1.0f;
		es[0].fSizeMin = 0.25f;
		es[0].fSizeRand = 0.0f;
		es[0].fScaleMin = 0.25f;
		es[0].fScaleRand = 0.0f;
		es[0].fResistance = 0.0f;
		es[0].fRotateRand = 0;
		es[0].fAngleVelMin = 0;
		es[0].fAngleVelRand = 0;
		break;

	case 2:
		nEmitterCount = 1;
		glhLoadTexture( m_texParticle, "assets/Particle.png", 0 );
		glClearColor( 0.8f, 0.8f, 0.8f, 1 );
		es[0].fEmitRate = 500;
		es[0].fEmitLife = -1.0f;

		es[0].vTexParams = Vector4( 0, 0, 1, 1 );

		es[0].vColorEnd = es[0].vColorMin = Vector4( 0.2f, 0.2f, 0.2f, 0.0f );
		es[0].vColorRand = Vector4( 0, 0, 0, 0.2f );
		es[0].fShadeRand = 0.1f;

		es[0].vPos = Vector3( 0, -50, 0 );
		es[0].fPosOfs = 5.0f;
		es[0].fPosRand = 5.0f;

		es[0].vVelDir = Vector3( 0, 1, 0 );
		es[0].fVelMin = 20.0f;
		es[0].fVelRand = 20.0f;
		es[0].fVelAngle = 70.0f;

		es[0].bShareVelPosAngle = 0;

		es[0].vAccel = Vector3( 0,10,0 );

		es[0].fLifeMin = 2.0f;
		es[0].fLifeRand = 1.0f;
		es[0].fFadeMin = 2.0f;
		es[0].fFadeRand = 1.0f;
		es[0].fSizeMin = 10.0f;
		es[0].fSizeRand = 5.0f;
		es[0].fScaleMin = 10.0f;
		es[0].fScaleRand = 5.0f;
		es[0].fResistance = 0.1f;

		es[0].fRotateRand = 1;
		es[0].fAngleVelMin = 0;
		es[0].fAngleVelRand = 1;
		break;

	case 3:
		nEmitterCount = 1;
		glhLoadTexture( m_texParticle, "assets/FireParticle.png", 0 );
		glClearColor( 0, 0, 0, 1 );
		es[0].fEmitRate = 1000;
		es[0].fEmitLife = -1.0f;

		es[0].vTexParams = Vector4( 0, 0, 1, 1 );

		es[0].vColorEnd = es[0].vColorMin = Vector4( 1, 1, 1, 0.5f );
		es[0].vColorRand = Vector4( 1, 0, 0, 0.3f );
		es[0].fShadeRand = 0;

		es[0].vPos = Vector3( 0, -50, 0 );
		es[0].fPosOfs = 5.0f;
		es[0].fPosRand = 15.0f;

		es[0].vVelDir = Vector3( 0, 1, 0 ).GetNormal();
		es[0].fVelMin = 80.0f;
		es[0].fVelRand = 20.0f;
		es[0].fVelAngle = -5.0f;

		es[0].bShareVelPosAngle = 1;

		es[0].vAccel = Vector3( 0,10,0 );

		es[0].fLifeMin = 0.2f;
		es[0].fLifeRand = 0.8f;
		es[0].fFadeMin = 0.0f;
		es[0].fFadeRand = 0.5f;
		es[0].fSizeMin = 10.0f;
		es[0].fSizeRand = 2.0f;
		es[0].fScaleMin = -5.0f;
		es[0].fScaleRand = -5.0f;
		es[0].fResistance = 0.2f;

		es[0].fRotateRand = 0;
		es[0].fAngleVelMin = 0;
		es[0].fAngleVelRand = 0.25;
		break;
	case 4:
		nEmitterCount = 3;
		glhLoadTexture( m_texParticle, "assets/Explode.png", 0 );
		glClearColor( 0.2f, 0.2f, 0.2f, 1 );

		es[0].fEmitRate = 500;
		es[0].fEmitLife = 0.2f;
		es[0].vTexParams = Vector4( 0, 1 - 0.75f, 0.25f, 0.25f );
		es[0].vColorEnd = Vector4( 0.2f, 0.2f, 0.2f, 0.0f );
		es[0].vColorMin = Vector4( 1, 0.65f, 0.25f, 0.0f );
		es[0].vColorRand = Vector4( 0, 0, 0, 0.2f );
		es[0].fShadeRand = 0.1f;
		es[0].vPos = Vector3( 0, -10, 0 );
		es[0].fPosOfs = 5.0f;
		es[0].fPosRand = 5.0f;
		es[0].vVelDir = Vector3( 0, 1, 0 );
		es[0].fVelMin = 100.0f;
		es[0].fVelRand = 20.0f;
		es[0].fVelAngle = 90.0f;
		es[0].bShareVelPosAngle = 0;
		es[0].vAccel = Vector3( 0,-10,0 );
		es[0].fLifeMin = 1;
		es[0].fLifeRand = 1;
		es[0].fFadeMin = 1;
		es[0].fFadeRand = 0;
		es[0].fSizeMin = 10.0f;
		es[0].fSizeRand = 5.0f;
		es[0].fScaleMin = 20.0f;
		es[0].fScaleRand = 5.0f;
		es[0].fResistance = 1;
		es[0].fRotateRand = 1;
		es[0].fAngleVelMin = 0;
		es[0].fAngleVelRand = 1;

		es[1].fEmitRate = 5000;
		es[1].fEmitLife = 0.2f;
		es[1].vTexParams = Vector4( 0, 0, 1, 1 );
		es[1].vColorEnd = es[1].vColorMin = Vector4( 1.0f, 1.0f, 0.0f, 1.0f );
		es[1].vColorRand = Vector4( 0.0f, 1.0f, 0.0f, 0.0f );
		es[1].fShadeRand = 0.0f;
		es[1].vPos = Vector3( 0, -15, 0 );
		es[1].fPosOfs = 0.0f;
		es[1].fPosRand = 1.0f;
		es[1].vVelDir = Vector3( 0, 1, 0 );
		es[1].fVelMin = 80.0f;
		es[1].fVelRand = 20.0f;
		es[1].fVelAngle = 80.0f;
		es[1].bShareVelPosAngle = 0;
		es[1].vAccel = Vector3( 0,-10,0 );
		es[1].fLifeMin = 0.4f;
		es[1].fLifeRand = 0.4f;
		es[1].fFadeMin = 0.0f;
		es[1].fFadeRand = 0.0f;
		es[1].fSizeMin = 0.25f;
		es[1].fSizeRand = 0.0f;
		es[1].fScaleMin = 0.25f;
		es[1].fScaleRand = 0.0f;
		es[1].fResistance = 0.1f;
		es[1].fRotateRand = 0;
		es[1].fAngleVelMin = 0;
		es[1].fAngleVelRand = 0;

		es[2].fEmitRate = 100;
		es[2].fEmitLife = 0.2f;
		es[2].vTexParams = Vector4( 0.5f, 1 - ( 0.25f + 0.083f * 2 ), 0.083f, 0.083f );
		es[2].vColorEnd = Vector4( 0.3f, 0.3f, 0.3f, 1 );
		es[2].vColorMin = Vector4( 0.3f, 0.3f, 0.3f, 1 );
		es[2].vColorRand = Vector4( 0, 0, 0, 0 );
		es[2].fShadeRand = 0.0f;
		es[2].vPos = Vector3( 0, -10, 0 );
		es[2].fPosOfs = 5.0f;
		es[2].fPosRand = 5.0f;
		es[2].vVelDir = Vector3( 0, 1, 0 );
		es[2].fVelMin = 100.0f;
		es[2].fVelRand = 20.0f;
		es[2].fVelAngle = 90.0f;
		es[2].bShareVelPosAngle = 0;
		es[2].vAccel = Vector3( 0,-50,0 );
		es[2].fLifeMin = 1.0f;
		es[2].fLifeRand = 0;
		es[2].fFadeMin = 0.1f;
		es[2].fFadeRand = 0;
		es[2].fSizeMin = 2.0f;
		es[2].fSizeRand = 1.0f;
		es[2].fScaleMin = 2.0f;
		es[2].fScaleRand = 1.0f;
		es[2].fResistance = 1;
		es[2].fRotateRand = 1;
		es[2].fAngleVelMin = 0;
		es[2].fAngleVelRand = 1;

		break;
	};

	m_ps.AddEffect( 10000, Matrix::GetIdentity(), nEmitterCount, es, m_texParticle );
	*/
	m_bInitComplete = 1;
	
	return 1;
}

Vector4 GetKartColor( Int32 player )
{
	Vector4 vColor;

	switch( player )
	{
	case 1:	vColor = Vector4( 1, 0, 0, 1 ); break;
	case 2:	vColor = Vector4( 0, 1, 0, 1 ); break;
	case 3:	vColor = Vector4( 0, 0, 1, 1 ); break;
	case 4:	vColor = Vector4( 1, 1, 0, 1 ); break;
	case 5:	vColor = Vector4( 0, 1, 1, 1 ); break;
	case 6:	vColor = Vector4( 1, 0, 1, 1 ); break;
	case 7:	vColor = Vector4( 0, 0, 0, 1 ); break;
	case 8:	vColor = Vector4( 1, 1, 1, 1 ); break;
	default: vColor = Vector4( 0.01f * ( rand() % 101 ), 0.01f * ( rand() % 101 ), 0.01f * ( rand() % 101 ), 1 ); break;
	};

	return vColor;
}

std::vector<Int32> camplayerid;
std::vector<Entities::CarEntity::Camera> cameras;

int Renderer::render( float fElapseSec )
{
	static Int32 bIgnore = 1;
	if( bIgnore )
	{
		bIgnore = 0;
		return 0;
	}

	m_fTime += fElapseSec;

	if( !m_bInitComplete )
		return 0;

	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	_CheckMail();

	glClearColor( 0.2f, 0.2f, 0.2f, 1 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	Real fLightDist = 17.0f;
	Real fLightHeight = 5.0f;
	Real fLightPower = 25.0f;
	Real fSpinRadius = 4.0f;
	MEMSET( perFrame.vLight, 0, sizeof(Vector4) * MAX_LIGHTS );
	perFrame.vLight[0] = Vector4( SIN( m_fTime ) * fSpinRadius, 2, COS( m_fTime ) * fSpinRadius, fLightPower );
	perFrame.vLight[1] = Vector4( fLightDist, fLightHeight, 0, fLightPower );
	perFrame.vLight[2] = Vector4( 0, fLightHeight, fLightDist, fLightPower );
	perFrame.vLight[3] = Vector4( -fLightDist, fLightHeight, 0, fLightPower );
	perFrame.vLight[4] = Vector4( 0, fLightHeight, -fLightDist, fLightPower );
	perFrame.vLight[5] = Vector4( fLightDist, fLightHeight, fLightDist, fLightPower );
	perFrame.vLight[6] = Vector4( -fLightDist, fLightHeight, fLightDist, fLightPower );
	perFrame.vLight[7] = Vector4( fLightDist, fLightHeight, -fLightDist, fLightPower );
	perFrame.vLight[8] = Vector4( -fLightDist, fLightHeight, -fLightDist, fLightPower );
	perFrame.vLight[9] = Vector4( -SIN( m_fTime ) * fSpinRadius, 2, -COS( m_fTime ) * fSpinRadius, fLightPower );

	std::vector<RCAMERA> aryCameras;
	_CalcCameras( aryCameras );
	
	static Real fFPSLastTime = 0;
	static Int32 nFPSCount = 0;
	nFPSCount++;
	if( m_fTime - fFPSLastTime > 1.0f )
	{
		_Pulse( Vector3( 0, 1, 0 ) );
		fFPSLastTime += 1.0f;
		nFPSCount = 0;
	}
	
	m_ps.Update( fElapseSec, aryCameras[0].eyepos );

	for( Int32 i = 0; i < aryCameras.size(); i++ )
	{	
		glViewport( aryCameras[i].x, aryCameras[i].y, aryCameras[i].w, aryCameras[i].h );

		perFrame.vEyePos = aryCameras[i].eyepos;
		perFrame.vEyeDir = Vector3( aryCameras[i].eyefocus - aryCameras[i].eyepos ).Normalize();
		perFrame.matProj.Perspective( aryCameras[i].fov, (Real)aryCameras[i].w/aryCameras[i].h, 0.1f, 1000.0f );
		perFrame.matView.LookAt( perFrame.vEyePos.xyz(), aryCameras[i].eyefocus, Vector3( 0, 1, 0 ) );
		perFrame.matViewProj = perFrame.matView * perFrame.matProj;
		glhUpdateBuffer( m_eftMesh, m_bufPerFrame );
		
		_DrawArena();
		
		for (std::pair<entity_id, struct kart>kart_id_pair: m_mKarts) 
		{
			auto kart_entity = GETENTITY(kart_id_pair.first, CarEntity);

			if( !kart_entity->isExploding )
			{
				// DRAW SHADOW
				Vector3 vAxis = Vector3::Cross( kart_entity->groundNormal, Vector3( 0, 1, 0 ) );
				Real fAngle = ACOS( Vector3::Dot( kart_entity->groundNormal, Vector3( 0, 1, 0 ) ) );

				perMesh.vColor = Vector4( 0,0,0,0.3f );
				perMesh.matWorld = Matrix::GetRotateAxis( vAxis, fAngle ) *
					Matrix::GetTranslate( kart_entity->groundHit + Vector3( 0, 0.01f, 0 ) );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

				glhEnableTexture( m_difKartShadow );
				glhEnableTexture( m_nrmBlank, 1 );

				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glhDrawMesh( m_eftMesh, m_mshKartShadow );
				glDisable( GL_BLEND );

				// DRAW KART
				glhEnableTexture( m_difBlank );
				glhEnableTexture( m_nrmBlank, 1 );

				perMesh.vColor = kart_id_pair.second.vColor;
				perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
				perMesh.matWorld = Matrix::GetRotateQuaternion( kart_entity->Orient ) *
					Matrix::GetTranslate( kart_entity->Pos );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
				glhDrawMesh( m_eftMesh, m_mshKart );

				for( int i = 0; i < 4; i++ )
				{
					perMesh.matWorld = Matrix::GetRotateQuaternion( kart_entity->tireOrient[i] ) *
						Matrix::GetTranslate( kart_entity->tirePos[i] );
					perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
					glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
					glhDrawMesh( m_eftMesh, m_mshKartTire );
				}
			}
		}

		glhEnableTexture( m_difBlank );
		glhEnableTexture( m_nrmBlank, 1 );
		for (auto id_powerup_pair : m_powerups) {
			auto powerup = id_powerup_pair.second;
			Vector3 pos = powerup.vPos;

			if( powerup.type == Entities::GoldCasePowerup )
			{
				perMesh.vRenderParams = Vector4( 1, 0, 1, 0 );
				perMesh.vColor = Vector4(1,1,1,1);
				perMesh.matWorld = Matrix::GetTranslate( pos );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glhDrawMesh( m_eftMesh, m_mshGold );
				glDisable( GL_BLEND );
			}
			else if( powerup.type == Entities::FloatingGoldPowerup)
			{
				perMesh.vRenderParams = Vector4( 1, 0, 1, 0 );
				perMesh.vColor = Vector4(0,0,1,1);
				perMesh.matWorld = Matrix::GetTranslate( pos );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glhDrawMesh( m_eftMesh, m_mshGold );
				glDisable( GL_BLEND );
			}
			else
			{
				Vector4 color1;
				Vector4 color2;
				switch (powerup.type) 
				{
					case Entities::HealthPowerup:
						color1 = Vector4(1, 0, 0, 1);
						color2 = Vector4(1, 1, 1, 1);
					break;

					case Entities::SpeedPowerup:
						color1 = Vector4(1, 1, 1, 1);
						color2 = Vector4(0, 0, 0, 0);
					break;

					case Entities::RocketPowerup:
						color1 = Vector4(0, 0, 0, 0);
						color2 = Vector4(0, 0, 0, 0);
					break;

					case Entities::PulsePowerup:
						color1 = Vector4(0.01, 1, 0, 1);
						color2 = Vector4(0.0001, 0.01, 1, 1);
					break;

					default:
						color1 = Vector4(1, 1, 0, 1);
						color2 = Vector4(0, 0, 1, 1);
					break;
				}

				perMesh.vColor = color1;
				perMesh.vRenderParams = Vector4( 1, 1, 1, 0 );
				perMesh.matWorld = Matrix::GetRotateY( m_fTime * 7 ) * Matrix::GetTranslate( pos );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
				glhDrawMesh( m_eftMesh, m_mshPowerRing1 );

				perMesh.matWorld = Matrix::GetRotateY( -m_fTime * 10 ) * Matrix::GetTranslate( pos );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
				glhDrawMesh( m_eftMesh, m_mshPowerRing2 );

				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				perMesh.vColor = color2;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
				glhDrawMesh( m_eftMesh, m_mshPowerSphere );
				glDisable( GL_BLEND );
			}
		}

		// Render rockets hack
		for (auto rocket_pair : list_of_rockets) 
		{
			auto rocket = rocket_pair.second;
			Vector3 pos;
			pos.x = rocket->position.getX();
			pos.y = rocket->position.getY();
			pos.z = rocket->position.getZ();

			Vector3 vDir = Vector3( rocket->direction.x(), rocket->direction.y(), rocket->direction.z() );

			Vector4 color1 = Vector4(0, 0, 0, 0);
			Vector4 color2 = Vector4(0, 0, 0, 0);

			perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
			perMesh.vColor = color1;
			perMesh.vRenderParams = Vector4( 1, 1, 0, 0 );
			perMesh.matWorld = Matrix::GetRotateY( m_fTime * 7 ) * Matrix::GetTranslate( pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerRing1 );

			perMesh.matWorld = Matrix::GetRotateY( -m_fTime * 10 ) * Matrix::GetTranslate( pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerRing2 );

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			perMesh.vColor = color2;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerSphere );
			glDisable( GL_BLEND );
		}

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		perMesh.vColor = Vector4(1, 1, 1, 1);
		perMesh.vRenderParams = Vector4( 1, 1, 1, 0 );
		for (auto bullet_pair : list_of_bullets) 
		{
			
			Vector4 color1;
			Vector4 color2;
			auto bullet = bullet_pair.second;
			Vector3 pos;
			pos.x = bullet->position.getX();
			pos.y = bullet->position.getY();
			pos.z = bullet->position.getZ();

			Vector3 vDir = Vector3( bullet->direction.x(), bullet->direction.y(), bullet->direction.z() );
			Vector3 vAxis = Vector3::Cross( vDir, Vector3( 0, 0, 1 ) );
			Real fAngle = ACOS( Vector3::Dot( vDir, Vector3( 0, 0, 1 ) ) );
			
			perMesh.matWorld = Matrix::GetRotateAxis( vAxis, fAngle ) * Matrix::GetTranslate( pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshBullet );
		}
		glDisable( GL_BLEND );
		
		Quaternion q;
		q.RotateMatrix( perFrame.matView );
		m_ps.Draw( q, perFrame.matViewProj );
	}

	// GUI
	glClear( GL_DEPTH_BUFFER_BIT );

	cstGUI& guidata = *(cstGUI*)m_bufGUI.data;
	guidata.matWorldViewProj.Orthographic( 0, nWinWidth, 0, nWinHeight, -1, 1 );
	glhUpdateBuffer( m_eftGUI, m_bufGUI );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	switch( m_nScreenState )
	{
	case RS_START:
		guidata.matWorldViewProj.Orthographic( 0, nWinWidth, 0, nWinHeight, -1, 1 );
		glhUpdateBuffer( m_eftGUI, m_bufGUI );
		glhEnableTexture( m_texGUIStart );
		glhDrawMesh( m_eftGUI, m_mshGUIStart );
		break;
	case RS_END:
		guidata.matWorldViewProj.Orthographic( 0, nWinWidth, 0, nWinHeight, -1, 1 );
		glhUpdateBuffer( m_eftGUI, m_bufGUI );
		glhEnableTexture( m_texGUIScore );
		glhDrawMesh( m_eftGUI, m_mshGUIStart );
		_DrawScoreBoard( 0, 100, 0 );
		break;
	case RS_DRIVING:
		for( Int32 i = 0; i < aryCameras.size(); i++ )
		{	
			glViewport( aryCameras[i].x, aryCameras[i].y, aryCameras[i].w, aryCameras[i].h );
			guidata.matWorldViewProj.Orthographic( 0, nWinWidth, 0, nWinWidth * ( (Real)aryCameras[i].h / aryCameras[i].w ), -1, 1 );
			glhUpdateBuffer( m_eftGUI, m_bufGUI );

			_DrawScoreBoard( -(nWinWidth>>1) + 125, (nWinHeight>>1) - 40, camplayerid[i] );
		}
		break;
	}

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	
	SDL_GL_SwapWindow( m_Window );

	return 1;
}

void Renderer::_DrawArena()
{
	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	perMesh.vRenderParams = Vector4( 1, 1, 1, 0 );
	perMesh.matWorld.Scale( Vector3( 1000, 1000, 1000 ) );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhEnableTexture( m_texSkybox );
	glhEnableTexture( m_nrmBlank, 1 );
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_mshSkybox );
	
	// QUAD A
	glCullFace( GL_BACK );
	perMesh.vRenderParams = Vector4( -1, 1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, 1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 0, 0 ) );

	// QUAD B
	perMesh.vRenderParams = Vector4( 1, -1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( 1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 0, 1, 0 ) );
	
	// QUAD C
	glCullFace( GL_FRONT );
	perMesh.vRenderParams = Vector4( 1, 1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 1, 0 ) );
	
	// QUAD D
	perMesh.vRenderParams = Vector4( -1, -1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 0, 0, 1 ) );
}

void Renderer::_DrawArenaQuad( Vector3 vColor )
{
	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	perMesh.vColor = Vector4( 1,1,1,1 );
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

	glhEnableTexture( m_difArenaWalls );
	glhEnableTexture( m_nrmArenaWalls, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaWalls );

	glhEnableTexture( m_difArenaFloor );
	glhEnableTexture( m_nrmArenaFloor, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaFloor );

	perMesh.vColor = Vector4( vColor,1 );
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

	glhEnableTexture( m_difBlank );
	glhEnableTexture( m_nrmBlank, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaTops );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glhEnableTexture( m_difArenaFlags );
	glhEnableTexture( m_nrmArenaFlags, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaFlags );

	glDisable( GL_BLEND );
}

void Renderer::_DrawScoreBoard( Int32 x, Int32 y, Int32 player ) // 125, 40
{
	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	GLmesh temp_mesh;

	if( player )
	{
		Int32 draw[3];
		draw[0] = 0;
		draw[1] = 1;
		draw[2] = 2;

		for( Int32 i = 3; i < kartsByScore.size(); i++ )
		{
			auto kart_entity = GETENTITY(kartsByScore[i], CarEntity);
			if( player == kart_entity->playerNumber )
			{
				draw[1] = i - 1;
				draw[2] = i;
				break;
			}
		}

		for( Int32 i = 0; i < 3; i++ )
		{
			Int32 ofs = i * 30;
			_DrawScore( draw[i], x, y - ofs );
		}
	}
	else
	{
		for( Int32 i = 0; i < kartsByScore.size(); i++ )
		{
			Int32 ofs = i * 30;
			_DrawScore( i, x, y - ofs );
		}
	}
}

void Renderer::_DrawScore( Int32 kart, Int32 x, Int32 y )
{
	auto kart_entity = GETENTITY(kartsByScore[kart], CarEntity);

	Vector4 color = m_mKarts[kartsByScore[kart]].vColor;
	color.w = 0.5f;

	GLmesh temp_mesh;
	glhCreateGUI( temp_mesh, GuiBox( GuiBox( x, y, 200, 30, color ) ), 6 );
	glhEnableTexture( m_difBlank );
	glhDrawMesh( m_eftGUI, temp_mesh );
	glhDestroyMesh( temp_mesh );

	glhCreateGUI( temp_mesh, GuiBox( GuiBox( 10 + x, y, 110, 30, Vector4( 1,1,1,1 ) ) ), 6 );
	glhEnableTexture( m_texGUIPlayer );
	glhDrawMesh( m_eftGUI, temp_mesh );
	glhDestroyMesh( temp_mesh );

	GuiBox pnum_box = GuiBox( 80 + x, y, 20, 30, Vector4( 1,1,1,1 ) );
	pnum_box.Num( kart_entity->playerNumber );
	glhCreateGUI( temp_mesh, pnum_box, 6 );
	glhEnableTexture( m_texGUINumbers );
	glhDrawMesh( m_eftGUI, temp_mesh );
	glhDestroyMesh( temp_mesh );

	GuiBox snum_box = GuiBox( -85 + x, y, 20, 30, Vector4( 1,1,1,1 ) );
	snum_box.Num( kart_entity->gold );
	glhCreateGUI( temp_mesh, snum_box, 6 );
	glhEnableTexture( m_texGUINumbers );
	glhDrawMesh( m_eftGUI, temp_mesh );
	glhDestroyMesh( temp_mesh );
}

void Renderer::_CheckMail()
{
	cameras.clear();
	camplayerid.clear();
	for( Events::Event *event : m_pMailbox->checkMail() )
	{
		switch( event->type ) 
		{
		case Events::EventType::RoundStart:
			m_nScreenState = RS_DRIVING;
			break;
		case Events::EventType::RoundEnd:
			m_nScreenState = RS_END;
			break;
		// Each bullet has a position and a direction passed for rendering and an additional value "time to live" that physics uses, no idea if rendering needs it.
		case Events::EventType::BulletList:
		{
			auto bullet_list_event = ((Events::BulletListEvent *)event);
			list_of_bullets = *((std::map<int, struct Physics::Simulation::bullet *> *)(bullet_list_event->list_of_bullets)); // This was passed as a void *, don't forget to cast!
			list_of_rockets = *((std::map<int, struct Physics::Simulation::rocket *> *)(bullet_list_event->list_of_rockets)); // This was passed as a void *, don't forget to cast!

			//DEBUGOUT("LIST OF BULLETS REVIECED!\n")
		}
		break;
		case Events::EventType::KartCreated:
			{
				auto kart_event = ((Events::KartCreatedEvent *)event);
				auto kart = GETENTITY(kart_event->kart_id, CarEntity);
				struct kart kart_local;
				entity_id id = kart_local.idKart = kart_event->kart_id;
				kart_local.vColor = GetKartColor( kart->playerNumber );
				m_mKarts[id] = kart_local;
			}
			break;
		case Events::EventType::KartDestroyed:
			m_mKarts.erase(((Events::KartDestroyedEvent *)event)->kart_id);
			break;
		case Events::EventType::PlayerKart:
			{
				entity_id kart_id = ((Events::PlayerKartEvent *)event)->kart_id;
				auto kart = GETENTITY(kart_id, CarEntity);
				
//				cameras.clear();
				cameras.push_back(kart->camera);
				camplayerid.push_back(kart->playerNumber);
			}
			break;
		case Events::EventType::AiKart:
			{
//				if (!player_kart_found)
				{
					entity_id kart_id = ((Events::AiKartEvent *)event)->kart_id;
					auto kart = GETENTITY(kart_id, CarEntity);

//					cameras.push_back(kart->camera);
//					camplayerid.push_back(kart->playerNumber);
				}
			}
			break;
		case Events::EventType::PowerupPlacement:
			{
				auto powerup_event = ((Events::PowerupPlacementEvent *)event);
				struct powerup powerup_local;
				powerup_local.vPos = powerup_event->pos;
				powerup_local.type = powerup_event->powerup_type;
				powerup_local.idPowerup = powerup_event->powerup_id;
				m_powerups[powerup_local.idPowerup] = powerup_local;

			}
			break;
		case Events::EventType::PowerupPickup:
		case Events::EventType::PowerupDestroyed:
			{
				auto powerup = ((Events::PowerupDestroyedEvent *)event);
				m_powerups.erase(powerup->powerup_id);
			}
			break;
		case Events::EventType::PowerupActivated:
			{
				auto powerup = ((Events::PowerupActivatedEvent *)event);
				if( powerup->powerup_type == Entities::PulsePowerup )
					_Pulse( powerup->pos );
			}
			break;
		case Events::EventType::ScoreBoardUpdate:
			{
				auto scoreboard = ((Events::ScoreBoardUpdateEvent *)event);
				kartsByScore = scoreboard->kartsByScore;
			}
			break;
		case Events::EventType::Explosion:
			{
				auto explody = ((Events::ExplosionEvent *)event);
				_Explode( explody->pos );
			}
			break;
		default:
			break;
		}
	}
	m_pMailbox->emptyMail();
}

void Renderer::_CalcCameras( std::vector<RCAMERA>& aryCameras )
{
	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	switch( m_nScreenState )
	{
	case RS_START:
	case RS_END:
		{
			RCAMERA cam;
			cam.player = 
			cam.x = cam.y = 0;
			cam.w = nWinWidth;
			cam.h = nWinHeight;
			cam.fov = DEGTORAD( 80 );
			cam.eyepos = Vector3(0,15,-5);
			cam.eyefocus = Vector3(0,-0.8,0);
			aryCameras.push_back( cam );
		}
		break;
	case RS_DRIVING:
		{
			switch( cameras.size() )
			{
			case 0:
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth;
					cam.h = nWinHeight;
					cam.fov = DEGTORAD( 80 );
					cam.eyepos = Vector3(0,15,-5);
					cam.eyefocus = Vector3(0,-0.8,0);
					aryCameras.push_back( cam );
				}
				break;
			case 1:
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth;
					cam.h = nWinHeight;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			case 2:
				{
					RCAMERA cam;
					cam.x = 0;
					cam.y = nWinHeight>>1;
					cam.w = nWinWidth;
					cam.h = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );

					cam.y = 0;
					cam.fov = DEGTORAD( cameras[1].fFOV );
					cam.eyepos = cameras[1].vPos;
					cam.eyefocus = cameras[1].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			case 3:
				{
					RCAMERA cam;
					cam.x = 0;
					cam.y = nWinHeight>>1;
					cam.w = nWinWidth;
					cam.h = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );

					cam.y = 0;
					cam.w = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[1].fFOV );
					cam.eyepos = cameras[1].vPos;
					cam.eyefocus = cameras[1].vFocus;
					aryCameras.push_back( cam );

					cam.x = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[2].fFOV );
					cam.eyepos = cameras[2].vPos;
					cam.eyefocus = cameras[2].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			default: // 4
				{
					RCAMERA cam;
					cam.x = 0;
					cam.y = nWinHeight>>1;
					cam.w = nWinWidth>>1;
					cam.h = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );
					
					cam.x = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[1].fFOV );
					cam.eyepos = cameras[1].vPos;
					cam.eyefocus = cameras[1].vFocus;
					aryCameras.push_back( cam );

					cam.y = 0;
					cam.x = 0;
					cam.fov = DEGTORAD( cameras[2].fFOV );
					cam.eyepos = cameras[2].vPos;
					cam.eyefocus = cameras[2].vFocus;
					aryCameras.push_back( cam );

					cam.x = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[3].fFOV );
					cam.eyepos = cameras[3].vPos;
					cam.eyefocus = cameras[3].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			}
		}
		break;
	};
}

void Renderer::_Explode( Vector3 vPos )
{
	SE::EMITTER es[3];

	es[0].fEmitRate = 500;
	es[0].fEmitLife = 0.2f;
	es[0].vTexParams = Vector4( 0, 1 - 0.75f, 0.25f, 0.25f );
	es[0].vColorEnd = Vector4( 0.2f, 0.2f, 0.2f, 0.0f );
	es[0].vColorMin = Vector4( 1, 0.65f, 0.25f, 0.0f );
	es[0].vColorRand = Vector4( 0, 0, 0, 0.2f );
	es[0].fShadeRand = 0.1f;
	es[0].vPos = vPos;
	es[0].fPosOfs = 0.05f;
	es[0].fPosRand = 0.05f;
	es[0].vVelDir = Vector3( 0, 1, 0 );
	es[0].fVelMin = 1.0f;
	es[0].fVelRand = 0.2f;
	es[0].fVelAngleMin = 0.0f;
	es[0].fVelAngleRand = 90.0f;
	es[0].bShareVelPosAngle = 0;
	es[0].vAccel = Vector3( 0,-1,0 );
	es[0].fLifeMin = 1;
	es[0].fLifeRand = 1;
	es[0].fFadeMin = 1;
	es[0].fFadeRand = 0;
	es[0].fSizeMin = 0.1f;
	es[0].fSizeRand = 0.05f;
	es[0].fScaleMin = 0.2f;
	es[0].fScaleRand = 0.05f;
	es[0].fResistance = 1;
	es[0].fRotateRand = 1;
	es[0].fAngleVelMin = 0;
	es[0].fAngleVelRand = 1;

	es[1].fEmitRate = 500;
	es[1].fEmitLife = 0.2f;
	es[1].vTexParams = Vector4( 0, 1 - 0.75f, 0.25f, 0.25f );
	es[1].vColorEnd = es[1].vColorMin = Vector4( 1.0f, 1.0f, 0.0f, 1.0f );
	es[1].vColorRand = Vector4( 0.0f, 1.0f, 0.0f, 0.0f );
	es[1].fShadeRand = 0.0f;
	es[1].vPos = vPos;
	es[1].fPosOfs = 0.0f;
	es[1].fPosRand = 0.1f;
	es[1].vVelDir = Vector3( 0, 1, 0 );
	es[1].fVelMin = 1.0f;
	es[1].fVelRand = 0.25f;
	es[0].fVelAngleMin = 10.0f;
	es[0].fVelAngleRand = 70.0f;
	es[1].bShareVelPosAngle = 0;
	es[1].vAccel = Vector3( 0,-1,0 );
	es[1].fLifeMin = 0.4f;
	es[1].fLifeRand = 0.4f;
	es[1].fFadeMin = 0.0f;
	es[1].fFadeRand = 0.0f;
	es[1].fSizeMin = 0.01f;
	es[1].fSizeRand = 0.0f;
	es[1].fScaleMin = 0.01f;
	es[1].fScaleRand = 0.0f;
	es[1].fResistance = 1;
	es[1].fRotateRand = 0;
	es[1].fAngleVelMin = 0;
	es[1].fAngleVelRand = 0;

	es[2].fEmitRate = 100;
	es[2].fEmitLife = 0.2f;
	es[2].vTexParams = Vector4( 0.5f, 1 - ( 0.25f + 0.083f * 2 ), 0.083f, 0.083f );
	es[2].vColorEnd = Vector4( 0.3f, 0.3f, 0.3f, 1 );
	es[2].vColorMin = Vector4( 0.3f, 0.3f, 0.3f, 1 );
	es[2].vColorRand = Vector4( 0, 0, 0, 0 );
	es[2].fShadeRand = 0.0f;
	es[2].vPos = vPos;
	es[2].fPosOfs = 0.0f;
	es[2].fPosRand = 0.0f;
	es[2].vVelDir = Vector3( 0, 1, 0 );
	es[2].fVelMin = 2.0f;
	es[2].fVelRand = 2.0f;
	es[0].fVelAngleMin = 10.0f;
	es[0].fVelAngleRand = 70.0f;
	es[2].bShareVelPosAngle = 0;
	es[2].vAccel = Vector3( 0,-5,0 );
	es[2].fLifeMin = 1.0f;
	es[2].fLifeRand = 0;
	es[2].fFadeMin = 0.1f;
	es[2].fFadeRand = 0;
	es[2].fSizeMin = 0.01f;
	es[2].fSizeRand = 0.05f;
	es[2].fScaleMin = 0;
	es[2].fScaleRand = 0;
	es[2].fResistance = 1;
	es[2].fRotateRand = 1;
	es[2].fAngleVelMin = 0;
	es[2].fAngleVelRand = 1;

	Real fScale = 2;
	for( Int32 i = 0; i < 3; i++ )
	{
		es[i].fSizeMin *= fScale;
		es[i].fSizeRand *= fScale;
		es[i].fScaleMin *= fScale;
		es[i].fScaleRand *= fScale;
		es[i].fVelMin *= fScale;
	}


	m_ps.AddEffect( 1000, Matrix::GetIdentity(), 3, es, m_texParticle );
}

void Renderer::_Pulse( Vector3 vPos )
{
	SE::EMITTER es[3];

	es[0].fEmitRate = 5000;
	es[0].fEmitLife = 0.2f;
	es[0].vTexParams = Vector4( 0, 1 - 0.75f, 0.25f, 0.25f );
	es[0].vColorEnd = Vector4( 0.2f, 0.2f, 0.2f, 1 );
	es[0].vColorMin = Vector4( 0.2f, 0.65f, 0.25f, 1 );
	es[0].vColorRand = Vector4( 0, 0, 0, 0.2f );
	es[0].fShadeRand = 0.1f;
	es[0].vPos = vPos;
	es[0].fPosOfs = 0.05f;
	es[0].fPosRand = 0.05f;
	es[0].vVelDir = Vector3( 0, 1, 0 );
	es[0].fVelMin = 20.0f;
	es[0].fVelRand = 5.0;
	es[0].fVelAngleMin = 80.0f;
	es[0].fVelAngleRand = 10.0f;
	es[0].bShareVelPosAngle = 0;
	es[0].vAccel = Vector3( 0,-1,0 );
	es[0].fLifeMin = 0.5f;
	es[0].fLifeRand = 0.5f;
	es[0].fFadeMin = 1;
	es[0].fFadeRand = 0;
	es[0].fSizeMin = 0.1f;
	es[0].fSizeRand = 0.05f;
	es[0].fScaleMin = 0.2f;
	es[0].fScaleRand = 0.05f;
	es[0].fResistance = 10;
	es[0].fRotateRand = 1;
	es[0].fAngleVelMin = 0;
	es[0].fAngleVelRand = 1;

	m_ps.AddEffect( 100, Matrix::GetIdentity(), 1, es, m_texParticle );
}
