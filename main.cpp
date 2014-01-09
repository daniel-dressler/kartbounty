#include "Standard.h"
#include "glhelpers.h"
#include <SDL.h>

#include "ShaderStructs.h"
#include "component/events/events.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#define GAMENAME "KartBounty"

int main( int argc, char** argv )
{
#ifdef _WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 ); // Test this with 4.0, 3.3, 3.2
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_Init( SDL_INIT_EVERYTHING );

	SDL_Window *win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			800, 450, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if( !win )
		return 1;

#ifdef _WIN32
	if( !glhCreateContext( win ) )
		return 1;
#else
	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		return 1;

	SDL_GL_MakeCurrent( win, glcontext );
#endif

	if( glewInit() != GLEW_OK )
		return 1;

	const GLubyte* strVendor = glGetString( GL_VENDOR );

	GLint major, minor;
	glGetIntegerv( GL_MAJOR_VERSION, &major );
	glGetIntegerv( GL_MINOR_VERSION, &minor );

	const GLchar* aryHeaders[] = { "ShaderStructs.glsl" };
	GLeffect effect = glhLoadEffect( "VShader.glsl", NULL, "PShader.glsl", aryHeaders, 1 );
	if( !effect.program )
		return 1;

	glhCheckUniformNames( effect.program ); // This is just a test (not needed)

	GLbuffer gl_permesh, gl_perframe;
	glhCreateBuffer( effect, "cstPerMesh", sizeof(cstPerMesh), &gl_permesh );
	glhCreateBuffer( effect, "cstPerFrame", sizeof(cstPerFrame), &gl_perframe );

	cstPerMesh& perMesh = *(cstPerMesh*)gl_permesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)gl_perframe.data;

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
//	glFrontFace( GL_CCW );
	glCullFace( GL_FRONT );

	GLchar* pData;
	Int32 nSize;
	if( !glhReadFile( "assets/Wheel.msh", pData, nSize ) )
		return 1;

	SEG::Mesh meshdata;
	if( !meshdata.ReadData( (Byte*)pData, nSize ) )
		return 1;

	free( pData );

	GLmesh glmesh;
	if( !glhCreateMesh( glmesh, meshdata ) )
		return 1;


	Timer timer;
	int bRunning = 1;
	SDL_Event event;
	while( bRunning )
	{
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

		
		Real fTime = (Real)timer.CalcSeconds();
		static Real fLastTime = 0;
		Real fElapse = fTime - fLastTime;
		while( fElapse < 0.015f )
		{
			fTime = (Real)timer.CalcSeconds();
			fElapse = fTime - fLastTime;
		}

		static Int32 nFPS = 0;
		nFPS++;
		if( (Int32)fTime != (Int32)fLastTime )
		{
			DEBUGOUT( "FPS: %d\n", nFPS );
			nFPS = 0;
		}

		fLastTime = fTime;

		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		Int32 nWinWidth, nWinHeight;
		SDL_GetWindowSize( win, &nWinWidth, &nWinHeight );

		Vector3 vFocus = Vector3( 0, 0, 0 );
		Vector3 vEyePos = Vector3( 1, 2, 1 ).RotateXZ( fTime );
		perFrame.vEyePos = vEyePos;
		perFrame.vEyeDir = Vector3( vFocus - perFrame.vEyePos.xyz() ).Normalize();
		perFrame.matProj.Perspective( DEGTORAD( 60.0f ), (Real)nWinWidth/nWinHeight, 0.1f, 100.0f );
		perFrame.matView.LookAt( vEyePos, vFocus, Vector3( 0, 1, 0 ) );
		perFrame.matViewProj = perFrame.matView * perFrame.matProj;
		glhUpdateBuffer( effect, gl_perframe );

		perMesh.matWorld = Matrix::GetRotateX( PI );// * Matrix::GetScale( 10.0f );
		perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
		glhUpdateBuffer( effect, gl_permesh );

		glhDrawMesh( effect, glmesh );


		glFlush();
#ifdef _WIN32
		SwapBuffers( glhGetHDC() );
#else
		SDL_GL_SwapWindow(win);
#endif
	}

	glhDestroyBuffer( gl_permesh );
	glhDestroyBuffer( gl_perframe );
	glhUnloadEffect( effect );

#ifdef _WIN32
	glhDestroyContext();
#endif

	SDL_DestroyWindow( win );
	SDL_Quit();

	return 0;
}
