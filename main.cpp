#include "Standard.h"
#include "glhelpers.h"
#include <SDL.h>

#include "ShaderStructs.h"

#define GAMENAME "KartBounty"

int main( int argc, char** argv )
{
#ifdef _WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	SDL_Init( SDL_INIT_EVERYTHING );
	/*
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );		// Test this with 4.0, 3.3, 3.2
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // SDL's default
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetSwapInterval( 1 );
	*/

	SDL_Window *win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			800, 450, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if( !win )
		return 1;

#ifdef _WIN32
	// @Phil: Does SDL_GL_CreateContext not work under windows?
	// Seems like this should be something SDL is meant to fix
	if( !glhCreateContext( win ) )
		return 1;
#else
	// @Phil: good job! This worked without any edits which
	// is impressive since I assume you didn't have a chance to
	// compile it.
	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		return 1;

	SDL_GL_MakeCurrent( win, glcontext );
#endif

	if( glewInit() != GLEW_OK )
		return 1;

	GLint major, minor;
	glGetIntegerv( GL_MAJOR_VERSION, &major );
	glGetIntegerv( GL_MINOR_VERSION, &minor );

	const GLchar* aryHeaders[] = { "ShaderStructs.glsl" };
	GLeffect effect = glhLoadEffect( "VShader.glsl", NULL, "PShader.glsl", aryHeaders, 1 );
	if( !effect.program )
		return 1;

	GLbuffer permesh, perframe;
	if( !glhCreateBuffer( effect, "cstPerMesh", sizeof(cstPerMesh), &permesh ) )
		return 1;

	if( !glhCreateBuffer( effect, "cstPerFrame", sizeof(cstPerFrame), &perframe ) )
		return 1;

	/* @Phil: Are these used? I'm not sure what they are supposed to be doing
	cstPerMesh& perMesh = *(cstPerMesh*)permesh.buffer;
	cstPerFrame& perFrame = *(cstPerFrame*)perframe.buffer;
	*/

	glhCheckUniformNames( effect.program );

	int bRunning = 1;
	SDL_Event event;
	while( bRunning )
	{
		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
				case SDL_QUIT:
					bRunning = 0;
					break;
			}
		}

		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT );
/* @Phil: Tell me if this does not work
#ifdef _WIN32
		SwapBuffers( glhGetHDC() );
#else
		SwapBuffers();
#endif
*/
		SDL_GL_SwapWindow(win);
	}

	glhDestroyBuffer( permesh );
	glhDestroyBuffer( perframe );
	glhUnloadEffect( effect );

	SDL_DestroyWindow( win );
	SDL_Quit();

	return 0;
}
