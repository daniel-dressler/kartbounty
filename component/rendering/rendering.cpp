#include "rendering.h"
#include "../events/events.h"

class Renderer
{
private:
	Int32				m_bInitComplete;
	SDL_Window*			m_Window;
	Events::Mailbox*	m_pMailbox;

	GLeffect			m_eftMesh;
	GLbuffer			m_bufPerMesh;
	GLbuffer			m_bufPerFrame;

	GLmesh				m_mshArena;
	GLmesh				m_mshKart;

	GLtex				m_texDiffuse;
	GLtex				m_texNormal;

	Vector3				m_vArenaOfs;

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

	m_vArenaOfs = Vector3( 10,0,-10 );
}

Renderer::~Renderer()
{
	glhDestroyTexture( m_texDiffuse );
	glhDestroyTexture( m_texNormal );
	glhDestroyMesh( m_mshArena );
	glhDestroyMesh( m_mshKart );
	glhDestroyBuffer( m_bufPerMesh );
	glhDestroyBuffer( m_bufPerFrame );
	glhUnloadEffect( m_eftMesh );
	SDL_DestroyWindow( m_Window );

	if( m_pMailbox )
		delete m_pMailbox;
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
		if( !glhReadFile( "assets/Arena5.msh", pData, nSize ) )
			return 0;

		SEG::Mesh meshdata;
		GetMutState()->bttmArena = new btTriangleMesh();
		if( !meshdata.ReadData( (Byte*)pData, nSize, 0, GetMutState()->bttmArena, m_vArenaOfs ) )
			return 0;

		free( pData );

		if( !glhCreateMesh( m_mshArena, meshdata ) )
			return 0;
	}

	{
		GLchar* pData;
		Int32 nSize;
		if( !glhReadFile( "assets/Kart.msh", pData, nSize ) )
			return 0;

		SEG::Mesh meshdata;
		if( !meshdata.ReadData( (Byte*)pData, nSize ) )
			return 0;

		free( pData );

		if( !glhCreateMesh( m_mshKart, meshdata ) )
			return 0;
	}
	
	glhMapTexture( m_eftMesh, "g_texDiffuse", 0 );
	glhMapTexture( m_eftMesh, "g_texNormal", 1 );

	if( !glhLoadTexture( m_texDiffuse, "assets/brick_diff.png" ) )
		return 0;

	if( !glhLoadTexture( m_texNormal, "assets/brick_norm.png" ) )
		return 0;

	m_bInitComplete = 1;

	return 1;
}

int Renderer::Update( float fElapseSec )
{
	static Real fTime = 0;
	fTime += fElapseSec;

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

	perFrame.vLight[0] = Vector4( SIN( fTime ) * 3, 2, COS( fTime ) * 3, 10 );
	perFrame.vLight[1] = Vector4( 10, 5, 0, 10 );
	perFrame.vLight[2] = Vector4( 0, 5, 10, 10 );
	perFrame.vLight[3] = Vector4( -10, 5, 0, 10 );
	perFrame.vLight[4] = Vector4( 0, 5, -10, 10 );
	
	glhUpdateBuffer( m_eftMesh, m_bufPerFrame );

	return 1;
}

int Renderer::Render()
{
	if( !m_bInitComplete )
		return 0;

	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	glhEnableTexture( m_texDiffuse );
	glhEnableTexture( m_texNormal, 1 );

	glCullFace( GL_BACK );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, 1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_mshArena );

	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( 1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_mshArena );

	glCullFace( GL_FRONT );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_mshArena );

	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_mshArena );

	perMesh.matWorld = Matrix::GetRotateQuaternion( GetState().Karts[0].qOrient ) *
		Matrix::GetTranslate( GetState().Karts[0].vPos );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_mshKart );

	SDL_GL_SwapWindow( m_Window );

	return 1;
}
