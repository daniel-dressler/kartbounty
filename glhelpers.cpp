#include "Standard.h"
#include "glhelpers.h"

#ifdef _WIN32
#include <io.h>
#else
#include <stdio.h>
#endif

#include <fcntl.h>
#include <memory.h>

#ifdef _WIN32
HDC g_hdc = 0;
HGLRC g_hglrc = 0;
int glhCreateContext( SDL_Window* win )
{
	PIXELFORMATDESCRIPTOR pfd;
	MEMSET( &pfd, 0, sizeof(PIXELFORMATDESCRIPTOR) );
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW |
				  PFD_SUPPORT_OPENGL |
				  PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	SDL_SysWMinfo SysInfo;
	SDL_VERSION( &SysInfo.version );
	SDL_GetWindowWMInfo( win, &SysInfo );

	g_hdc = GetDC( (HWND)SysInfo.info.win.window );

	int PixelFormat;
	if( !( PixelFormat = ChoosePixelFormat( g_hdc, &pfd ) ) )
		return 1;
	
	if( !( g_hglrc = wglCreateContext( g_hdc ) ) )
		return 0;

	if( !wglMakeCurrent( g_hdc, g_hglrc ) )
		return 0;

	if( !SetPixelFormat( g_hdc, PixelFormat, &pfd ) )
		return 0;

	return 1;
}
HDC glhGetHDC()
{
	return g_hdc;
}
#endif

GLeffect glhLoadEffect( char* strVertexShader,
						char* strGeometryShader,
						char* strPixelShader,
						char** aryHeaders,
						int nHeaderCount )
{
	GLeffect effect;
	GLchar** arySources = (GLchar**)malloc( sizeof(GLchar*) * ( nHeaderCount + 1 ) );
	GLint* arySizes = (GLint*)malloc( sizeof(GLint) * ( nHeaderCount + 1 ) );

	memset( &effect, 0, sizeof(GLeffect) );
	memset( arySources, 0, sizeof(GLchar*) * ( nHeaderCount + 1 ) );

	for( int i = 0; i < nHeaderCount; i++ )
	{
		if( !glhReadFile( aryHeaders[i], arySources[i], arySizes[i] ) )
			goto glLoadProgramEnd;
	}

	if( !glhReadFile( strVertexShader, arySources[nHeaderCount], arySizes[nHeaderCount] ) )
		goto glLoadProgramEnd;

	effect.vertex = glhCompileShader( GL_VERTEX_SHADER, nHeaderCount + 1, arySources, arySizes );
	free( arySources[nHeaderCount] );
	if( !effect.vertex )
		goto glLoadProgramEnd;

	if( strGeometryShader )
	{
		if( !glhReadFile( strGeometryShader, arySources[nHeaderCount], arySizes[nHeaderCount] ) )
			goto glLoadProgramEnd;

		effect.geometry = glhCompileShader( GL_GEOMETRY_SHADER, nHeaderCount + 1, arySources, arySizes );
		free( arySources[nHeaderCount] );
		if( !effect.geometry )
			goto glLoadProgramEnd;
	}

	if( !glhReadFile( strPixelShader, arySources[nHeaderCount], arySizes[nHeaderCount] ) )
		goto glLoadProgramEnd;

	effect.pixel = glhCompileShader( GL_FRAGMENT_SHADER, nHeaderCount + 1, arySources, arySizes );
	free( arySources[nHeaderCount] );
	if( !effect.pixel )
		goto glLoadProgramEnd;

	effect.program = glhCombineProgram( effect.vertex, effect.geometry, effect.pixel );

glLoadProgramEnd:
	for( int i = 0; i < nHeaderCount; i++ )
		free( arySources[i] );
	free( arySources );
	free( arySizes );

	if( !effect.program )
		glhUnloadEffect( effect );

	return effect;
}

void glhUnloadEffect( GLeffect effect )
{
	glDeleteShader( effect.vertex );
	glDeleteShader( effect.geometry );
	glDeleteShader( effect.pixel );
	glDeleteProgram( effect.program );
}

#ifdef _WIN32

#define _OPEN			_open
#define _FILELEN		_filelength
#define _READ			_read
#define _CLOSE			_close
#define GLFILE_FLAGS	_O_RDONLY | _O_BINARY | _O_SEQUENTIAL

#else

#define _OPEN			open
#define _FILELEN		filelength
#define _READ			read
#define _CLOSE			close
#define GLFILE_FLAGS	O_RDONLY | O_BINARY | O_SEQUENTIAL

#endif

int glhReadFile( char* strFilename, GLchar*& strSource, GLint& nSize )
{
	int nFile = _OPEN( strFilename, GLFILE_FLAGS );
	if( nFile < 0 )
		return 0;

	nSize = _FILELEN( nFile );
	strSource = (GLchar*)malloc( nSize );

	int nReadAmt = _READ( nFile, strSource, nSize );
	_CLOSE( nFile );

	if( nReadAmt != nSize )
	{
		free( strSource );
		return 0;
	}

	return 1;
}

GLuint glhCompileShader( GLuint uType, GLsizei nSourceCount, GLchar** arySourceData, GLint* aryDataSizes )
{
	GLuint shader = glCreateShader( uType );
	glShaderSource( shader, nSourceCount, (const GLchar**)arySourceData, aryDataSizes );
	glCompileShader( shader );

	GLint result = 0;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &result );
	if( result == GL_FALSE )
	{
		GLchar strError[4096];
		GLsizei nLogSize;
		glGetShaderInfoLog( shader, 4096, &nLogSize, strError );
		DEBUGOUT( "%s\n", strError );
		glDeleteShader( shader );
		return 0;
	}

	return shader;
}

GLuint glhCombineProgram( GLuint uVertex, GLuint uGeometry, GLuint uPixel )
{
	GLuint program = glCreateProgram();
	
	if( uVertex > 0 )
		glAttachShader( program, uVertex );

	if( uGeometry > 0 )
		glAttachShader( program, uGeometry );

	if( uPixel > 0 )
		glAttachShader( program, uPixel );

	glLinkProgram( program );

	GLint result;
	glGetProgramiv( program, GL_LINK_STATUS, &result );
	if( result == GL_FALSE )
	{
		GLchar strError[4096];
		GLsizei nLogSize;
		glGetProgramInfoLog( program, 4096, &nLogSize, strError );
		DEBUGOUT( "%s\n", strError );
		glDeleteProgram( program );
		return 0;
	}

	return program;
}

void glhCheckUniformNames( GLuint program )
{
	GLint nBlocks;
	glGetProgramiv( program, GL_ACTIVE_UNIFORM_BLOCKS, &nBlocks );

	GLchar strLabel[256];
	for( int i = 0; i < nBlocks; i++ )
	{
		GLsizei nSize;
		glGetActiveUniformBlockName( program, i, 256, &nSize, strLabel );
		DEBUGOUT( "%s\n", strLabel );
	}
	DEBUGOUT( "\n" );
}

int glhCreateBuffer( const GLeffect& effect, GLchar* strBuffer, GLint nSize, GLbuffer* pBuffer )
{
	if( nSize <= 0 )
		return 0;

	glUseProgram( effect.program );

	pBuffer->location = glGetUniformBlockIndex( effect.program, strBuffer );
	if( pBuffer->location < 0 )															// RETURNING -1
		return 0;

	pBuffer->data = (GLchar*)malloc( nSize );
	if( !pBuffer->data )
		return 0;

	glGenBuffers( 1, &pBuffer->buffer );
	pBuffer->size = nSize;

	return 1;
}

int glhUpdateBuffer( const GLbuffer& buffer )
{
	glBindBuffer( GL_UNIFORM_BUFFER, buffer.buffer );
	glBufferData( GL_UNIFORM_BUFFER, buffer.size, buffer.data, GL_DYNAMIC_DRAW );
	glBindBufferBase( GL_UNIFORM_BUFFER, buffer.location, buffer.buffer );

	return 1;
}

void glhDestroyBuffer( GLbuffer& buffer )
{
	if( buffer.location >= 0 )
	{
		glDeleteBuffers( 1, &buffer.buffer );
		free( buffer.data );
	}
}
