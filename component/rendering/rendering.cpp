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

	GLmesh				m_glmArena;

	GLmesh				m_glmKart;

public:
	int Init( SDL_Window* win );
	int Update( float fElapseSec );
	int Render();

	Renderer();
	~Renderer();
};

Renderer* g_pRenderer = 0;

int InitRendering( SDL_Window* win )
{
	if( g_pRenderer ) return 0;
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
}

Renderer::~Renderer()
{
	glhDestroyBuffer( m_bufPerMesh );
	glhDestroyBuffer( m_bufPerFrame );
	glhUnloadEffect( m_eftMesh );
}

int Renderer::Init( SDL_Window* win )
{
	m_Window = win;
	m_pMailbox = new Events::Mailbox();

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
		if( !glhReadFile( "assets/Arena.msh", pData, nSize ) )
			return 0;

		SEG::Mesh meshdata;
		GetMutState()->bttmArena = new btTriangleMesh();
		if( !meshdata.ReadData( (Byte*)pData, nSize, 0, GetMutState()->bttmArena ) )
			return 0;

		free( pData );

		if( !glhCreateMesh( m_glmArena, meshdata ) )
			return 0;
	}

	{
		GLchar* pData;
		Int32 nSize;
		if( !glhReadFile( "assets/RedBox.msh", pData, nSize ) )
			return 0;

		SEG::Mesh meshdata;
		if( !meshdata.ReadData( (Byte*)pData, nSize ) )
			return 0;

		free( pData );

		if( !glhCreateMesh( m_glmKart, meshdata ) )
			return 0;
	}

	m_pMailbox->request( Events::EventType::StateUpdate );

	m_bInitComplete = 1;

	return 1;
}

int Renderer::Update( float fElapseSec )
{
	if( !m_bInitComplete )
		return 0;

	const std::vector<Events::Event*> aryEvents = m_pMailbox->checkMail();
	for( int i = 0; i < aryEvents.size(); i++ )
	{
		switch( aryEvents[i]->type )
		{
		case Events::EventType::StateUpdate:
			{
				//DEBUGOUT( "I GOT HERE!" );
			}
			break;
		}
	}

	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	Vector3 vFocus = GetState().Camera.vFocus;
	perFrame.vEyePos = GetState().Camera.vPos;
	perFrame.vEyeDir = Vector3( vFocus - perFrame.vEyePos.xyz() ).Normalize();
	perFrame.matProj.Perspective( DEGTORAD( 60.0f ), (Real)nWinWidth/nWinHeight, 0.1f, 100.0f );
	perFrame.matView.LookAt( perFrame.vEyePos.xyz(), vFocus, Vector3( 0, 1, 0 ) );
	perFrame.matViewProj = perFrame.matView * perFrame.matProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerFrame );

	return 1;
}

int Renderer::Render()
{
	if( !m_bInitComplete )
		return 0;

	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	perMesh.matWorld.Identity();
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_glmArena );

	perMesh.matWorld = Matrix::GetScale( 0.3f, 0.3f, 0.5f ) * 
		Matrix::GetRotateQuaternion( GetState().Karts[0].qOrient ) *
		Matrix::GetTranslate( GetState().Karts[0].vPos );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
	glhDrawMesh( m_eftMesh, m_glmKart );

	SDL_GL_SwapWindow( m_Window );

	return 1;
}
