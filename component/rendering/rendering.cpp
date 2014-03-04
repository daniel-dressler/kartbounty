#include "rendering.h"
#include "../events/events.h"

class Renderer
{
private:
	Int32				m_bInitComplete;
	SDL_Window*			m_Window;
	Events::Mailbox*	m_pMailbox;

	Real				m_fTime;

	GLeffect			m_eftMesh;
	GLbuffer			m_bufPerMesh;
	GLbuffer			m_bufPerFrame;


	// Arena
	GLmesh				m_mshArenaCldr;

	GLmesh				m_mshArenaWalls;
	GLtex				m_difArenaWalls;
	GLtex				m_nrmArenaWalls;

	GLmesh				m_mshArenaFlags;
	GLtex				m_difArenaFlags;
	GLtex				m_nrmArenaFlags;

	GLmesh				m_mshArenaTops;
	GLtex				m_difArenaTops;
	GLtex				m_nrmArenaTops;

	GLmesh				m_mshArenaFloor;
	GLtex				m_difArenaFloor;
	GLtex				m_nrmArenaFloor;

	GLmesh				m_mshKart;

	// Power ups
	GLmesh				m_mshPowerRing1;
	GLmesh				m_mshPowerRing2;
	GLmesh				m_mshPowerSphere;


	Vector3				m_vArenaOfs;

	void _DrawArena();
	void _DrawArenaQuad( Vector3 vColor );

public:
	int Init( SDL_Window* win );
	int Update( float fElapseSec );
	int Render();

	Renderer();
	~Renderer();
};

Renderer* g_pRenderer = 0;

int InitRendering()
{
	SDL_Init( SDL_INIT_EVERYTHING );
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
		return 0;

	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		return 0;

	SDL_GL_MakeCurrent( win, glcontext );

	if( g_pRenderer )
		return 0;

	g_pRenderer = new Renderer();
	return g_pRenderer ? g_pRenderer->Init( win ) : 0;
}

int UpdateRendering( float fElapseSec )
{
	return g_pRenderer ? g_pRenderer->Update( fElapseSec ) : 0;
}

int Render()
{
	return g_pRenderer ? g_pRenderer->Render() : 0;
}

int ShutdownRendering()
{
	if( !g_pRenderer ) return 0;
	delete g_pRenderer;
	return 1;
}

Renderer::Renderer()
{
	m_bInitComplete = 0;
	m_Window = 0;
	m_pMailbox = 0;

	m_fTime = 0;

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
	glhDestroyTexture( m_difArenaTops );
	glhDestroyTexture( m_nrmArenaTops );

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

int Renderer::Init( SDL_Window* win )
{
	m_Window = win;
	m_pMailbox = new Events::Mailbox();
	m_pMailbox->request( Events::EventType::Input );
	m_pMailbox->request( Events::EventType::StateUpdate );

	if( glewInit() != GLEW_OK )
		return 1;

	// OpenGL Test Info
	const GLubyte* strVendor = glGetString( GL_VENDOR );
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
		return 0;

	glhCreateBuffer( m_eftMesh, "cstPerMesh", sizeof(cstPerMesh), &m_bufPerMesh );
	glhCreateBuffer( m_eftMesh, "cstPerFrame", sizeof(cstPerFrame), &m_bufPerFrame );
	
	{
		GLchar* pData;
		Int32 nSize;
		if( !glhReadFile( "assets/arena_cldr.msh", pData, nSize ) )
			return 0;

		SEG::Mesh meshdata;
		GetMutState()->bttmArena = new btTriangleMesh();
		if( !meshdata.ReadData( (Byte*)pData, nSize, 0, GetMutState()->bttmArena, m_vArenaOfs ) )
			return 0;

		free( pData );

		if( !glhCreateMesh( m_mshArenaCldr, meshdata ) )
			return 0;
	}

	if( !LoadMesh( m_mshArenaWalls, "assets/arena_walls.msh" ) )
		return 0;
	if( !glhLoadTexture( m_difArenaWalls, "assets/arena_walls_diff.png" ) )
		return 0;
	if( !glhLoadTexture( m_nrmArenaWalls, "assets/arena_walls_norm.png" ) )
		return 0;

	if( !LoadMesh( m_mshArenaFlags, "assets/arena_flags.msh" ) )
		return 0;
	if( !glhLoadTexture( m_difArenaFlags, "assets/arena_flags_diff.png" ) )
		return 0;
	if( !glhLoadTexture( m_nrmArenaFlags, "assets/arena_flags_norm.png" ) )
		return 0;

	if( !LoadMesh( m_mshArenaFloor, "assets/arena_floor.msh" ) )
		return 0;
	if( !glhLoadTexture( m_difArenaFloor, "assets/arena_floor_diff.png" ) )
		return 0;
	if( !glhLoadTexture( m_nrmArenaFloor, "assets/blank_norm.png" ) )
		return 0;

	if( !LoadMesh( m_mshArenaTops, "assets/arena_tops.msh" ) )
		return 0;
	if( !glhLoadTexture( m_difArenaTops, "assets/blank.png" ) )
		return 0;
	if( !glhLoadTexture( m_nrmArenaTops, "assets/blank_norm.png" ) )
		return 0;


	if( !LoadMesh( m_mshKart, "assets/Kart.msh" ) )
		return 0;
	
	if( !LoadMesh( m_mshPowerSphere, "assets/PowerSphere.msh" ) )
		return 0;
	
	if( !LoadMesh( m_mshPowerRing1, "assets/PowerRing1.msh" ) )
		return 0;
	
	if( !LoadMesh( m_mshPowerRing2, "assets/PowerRing2.msh" ) )
		return 0;
	
	glhMapTexture( m_eftMesh, "g_texDiffuse", 0 );
	glhMapTexture( m_eftMesh, "g_texNormal", 1 );

	m_bInitComplete = 1;

	// TEMP CRAP

	for( Int32 i = 0; i < NUM_KARTS; i++ )
		GetState().Karts[i].vColor = Vector4( 1,1,1,1 );

	GetState().Karts[0].vColor = Vector4( 1,0,0,1 );
	GetState().Karts[1].vColor = Vector4( 0,0,1,1 );
	GetState().Karts[2].vColor = Vector4( 0,1,0,1 );
	//GetState().Karts[3].vColor = Vector4( 1,1,0,1 );

	GetState().Karts[1].vPos = Vector3( 10,1.5,10 );							// These two lines are temporary
	GetState().Karts[1].qOrient.Identity().RotateAxisAngle(Vector3(0,1,0), DEGTORAD(-90));

	
	GetState().Karts[2].vPos = Vector3( -10,1.5,10 );							// These two lines are temporary
	GetState().Karts[2].qOrient.Identity().RotateAxisAngle(Vector3(0,1,0), DEGTORAD(90));
	/*
	GetState().Karts[3].vPos = Vector3( 10, 1.5, -10 );							// These two lines are temporary
	GetState().Karts[3].qOrient.Identity().RotateAxisAngle(Vector3(0,1,0), DEGTORAD(180));
	*/
	return 1;
}

int Renderer::Update( float fElapseSec )
{
	m_fTime += fElapseSec;

	if( !m_bInitComplete )
		return 0;

	if( m_pMailbox )
	{
		const std::vector<Events::Event*> aryEvents = m_pMailbox->checkMail();
		for( unsigned int i = 0; i < aryEvents.size(); i++ )
		{
			switch( aryEvents[i]->type )
			{
			case Events::EventType::StateUpdate:
				{
					//DEBUGOUT( "I GOT HERE!" );
				}
				break;
			case Events::EventType::Input:
				{
					Events::InputEvent* input = (Events::InputEvent*)aryEvents[i];
				
				}
				break;
			}
		}
		m_pMailbox->emptyMail();
	}

	glClearColor( 0, 0, 0, 1 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	Vector3 vFocus = GetState().Camera.vFocus;
	perFrame.vEyePos = GetState().Camera.vPos;
	perFrame.vEyeDir = Vector3( vFocus - perFrame.vEyePos.xyz() ).Normalize();
	perFrame.matProj.Perspective( DEGTORAD( GetState().Camera.fFOV ), (Real)nWinWidth/nWinHeight, 0.1f, 100.0f );
	perFrame.matView.LookAt( perFrame.vEyePos.xyz(), vFocus, Vector3( 0, 1, 0 ) );
	perFrame.matViewProj = perFrame.matView * perFrame.matProj;

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

	glhUpdateBuffer( m_eftMesh, m_bufPerFrame );

	return 1;
}

int Renderer::Render()
{
	if( !m_bInitComplete )
		return 0;

	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	_DrawArena();

	glhEnableTexture( m_difArenaTops );
	glhEnableTexture( m_nrmArenaTops, 1 );

	for( Int32 i = 0; i < NUM_KARTS; i++ )
	{
		perMesh.vColor = GetState().Karts[i].vColor;
		perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
		perMesh.matWorld = Matrix::GetRotateQuaternion( GetState().Karts[i].qOrient ) *
			Matrix::GetTranslate( GetState().Karts[i].vPos );
		perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
		glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
		glhDrawMesh( m_eftMesh, m_mshKart );
	}

	for( Int32 i = 0; i < MAX_POWERUPS; i++ )
	{
		if( GetState().Powerups[i].bEnabled )
		{
			perMesh.vColor = Vector4( 1,0,0,1 );
			perMesh.vRenderParams = Vector4( 1, 1, 0, 0 );
			perMesh.matWorld = Matrix::GetRotateY( m_fTime * 7 ) * Matrix::GetTranslate( GetState().Powerups[i].vPos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerRing1 );

			perMesh.matWorld = Matrix::GetRotateY( -m_fTime * 10 ) * Matrix::GetTranslate( GetState().Powerups[i].vPos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerRing2 );

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			perMesh.vColor = Vector4( 1,1,1,1 );
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerSphere );
			glDisable( GL_BLEND );


		}
	}

	SDL_GL_SwapWindow( m_Window );

	return 1;
}

void Renderer::_DrawArena()
{
	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	// QUAD A
	glCullFace( GL_BACK );
	perMesh.vRenderParams = Vector4( -1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, 1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 0, 0 ) );

	// QUAD B
	perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( 1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 0, 1, 0 ) );
	
	// QUAD C
	glCullFace( GL_FRONT );
	perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 1, 0 ) );

	// QUAD D
	perMesh.vRenderParams = Vector4( -1, 0, 0, 0 );
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

	glhEnableTexture( m_difArenaTops );
	glhEnableTexture( m_nrmArenaTops, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaTops );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glhEnableTexture( m_difArenaFlags );
	glhEnableTexture( m_nrmArenaFlags, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaFlags );

	glDisable( GL_BLEND );
}
