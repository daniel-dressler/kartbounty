#include "Standard.h"
#include "glhelpers.h"
#include <SDL.h>

#include <thread>
#include <chrono>

#include "ShaderStructs.h"
#include "component/entities/entities.h"
#include "component/events/events.h"
#include "component/physics/physics.h"

int main( int argc, char** argv )
{
#ifdef _WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	SDL_Init( SDL_INIT_EVERYTHING );
//	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );		// This breaks shit...
//	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	SDL_Window *win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if( !win )
		return 1;

	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		return 1;

	SDL_GL_MakeCurrent( win, glcontext );

	if( glewInit() != GLEW_OK )
		return 1;

	const GLubyte* strVendor = glGetString( GL_VENDOR );

	GLint major, minor;
	glGetIntegerv( GL_MAJOR_VERSION, &major );
	glGetIntegerv( GL_MINOR_VERSION, &minor );

	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glEnable( GL_MULTISAMPLE );

	const GLchar* aryHeaders[] = { "ShaderStructs.glsl" };
	GLeffect effect = glhLoadEffect( "VShader.glsl", NULL, "PShader.glsl", aryHeaders, 1 );
	if( !effect.program )
		return 1;

	GLbuffer gl_permesh, gl_perframe;
	glhCreateBuffer( effect, "cstPerMesh", sizeof(cstPerMesh), &gl_permesh );
	glhCreateBuffer( effect, "cstPerFrame", sizeof(cstPerFrame), &gl_perframe );

	cstPerMesh& perMesh = *(cstPerMesh*)gl_permesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)gl_perframe.data;

	GLchar* pData;
	Int32 nSize;
	if( !glhReadFile( "assets/Wheel.msh", pData, nSize ) )
		return 1;

	SEG::Mesh meshdata;
	if( !meshdata.ReadData( (Byte*)pData, nSize ) )
		return 1;

	free( pData );

	GLmesh glmesh, glmesh2;
	if( !glhCreateMesh( glmesh, meshdata ) )
		return 1;

	if( !glhCreateMesh( glmesh2, meshdata ) )
		return 1;

	glhDestroyMesh( glmesh );

	/* FULLSCREEN MODE

	SDL_DestroyWindow( win );
	win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN );
	if( !win )
		return 1;
	*/

	Timer timer;
	int bRunning = 1;
	SDL_Event event;

	// Init components
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();

	while( bRunning )
	{
		static Real fLastTime = 0;
		Real fTime, fElapse;

		// Daniel: Caused infinite loop on linux
		// @Phil: Does windows need multiple pollings?
		// The SDL_PollEvent should retreive
		// the queued input so mutliple checks 
		// of the poll might just complicate our
		// input architecture.
		// do {
			fTime = (Real)timer.CalcSeconds();
			fElapse = fTime - fLastTime;

			while( SDL_PollEvent( &event ) )
			{
				switch( event.type )
				{
				case SDL_WINDOWEVENT:
					switch( event.window.event )
					{
					case SDL_WINDOWEVENT_RESIZED:
						{
							Int32 nWinWidth, nWinHeight;
							SDL_GetWindowSize( win, &nWinWidth, &nWinHeight );
							glViewport( 0, 0, nWinWidth, nWinHeight );
						}
						break;
					}
					break;
				case SDL_QUIT:
					bRunning = 0;
					break;
				}
			}

		//} while( fElapse < 0.008f );
		
		static Int32 nFPS = 0;
		nFPS++;
		if( (Int32)fTime != (Int32)fLastTime )
		{
			DEBUGOUT( "FPS: %d\n", nFPS );
			nFPS = 0;
		}


		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		// Components
		simulation->step(fElapse);


		Int32 nWinWidth, nWinHeight;
		SDL_GetWindowSize( win, &nWinWidth, &nWinHeight );

		Vector3 vFocus = Vector3( 0, 0, 0 );
		perFrame.vEyePos = Vector3( 1, 2, 1 ).RotateXZ( fTime );
		perFrame.vEyeDir = Vector3( vFocus - perFrame.vEyePos.xyz() ).Normalize();
		perFrame.matProj.Perspective( DEGTORAD( 60.0f ), (Real)nWinWidth/nWinHeight, 0.1f, 100.0f );
		perFrame.matView.LookAt( perFrame.vEyePos.xyz(), vFocus, Vector3( 0, 1, 0 ) );
		perFrame.matViewProj = perFrame.matView * perFrame.matProj;
		glhUpdateBuffer( effect, gl_perframe );

		perMesh.matWorld = Matrix::GetRotateX( PI );
		perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
		glhUpdateBuffer( effect, gl_permesh );

		glhDrawMesh( effect, glmesh );
		glhDrawMesh( effect, glmesh2 );

		SDL_GL_SwapWindow(win);

		fLastTime = fTime;
		std::chrono::milliseconds timespan(10); // oswhatever
		std::this_thread::sleep_for(timespan);
	}

	glhDestroyBuffer( gl_permesh );
	glhDestroyBuffer( gl_perframe );
	glhUnloadEffect( effect );

	SDL_DestroyWindow( win );
	SDL_Quit();

	return 0;
}
