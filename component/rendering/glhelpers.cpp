#include "../../Standard.h"
#include "glhelpers.h"

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif

#include <fcntl.h>
#include <memory.h>

GLeffect glhLoadEffect( const char* strVertexShader,
						const char* strGeometryShader,
						const char* strPixelShader,
						const char** aryHeaders,
						int nHeaderCount )
{
	GLeffect effect;
	MEMSET( &effect, 0, sizeof(GLeffect) );
	// @Phil: Hope you don't mind, calloc() does what you were doing with the
	// memsets
	GLchar** arySources = (GLchar**)calloc( sizeof(GLchar*), nHeaderCount + 1 );
	GLint* arySizes = (GLint*)calloc( sizeof(GLint), nHeaderCount + 1 );

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

	glGetProgramiv( effect.program, GL_ACTIVE_UNIFORM_BLOCKS, &effect.blockcount );
	effect.blocks = (GLuint*)MALLOC( sizeof(GLuint) * effect.blockcount * 2 );

	GLsizei nLabelSize;
	GLchar strLabel[256];
	for( int i = 0; i < effect.blockcount; i++ )
	{
		glGetActiveUniformBlockName( effect.program, i, 256, &nLabelSize, strLabel );
		GLuint index = glGetUniformBlockIndex( effect.program, strLabel );
		glUniformBlockBinding( effect.program, index, i + 1 );

		effect.blocks[i*2] = index;
		effect.blocks[i*2+1] = i + 1;
	}

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
	free( effect.blocks );
}

#ifdef _WIN32

#define _OPEN			_open
#define _FILELEN		_filelength
#define _READ			_read
#define _CLOSE			_close
#define GLFILE_FLAGS	_O_RDONLY | _O_BINARY | _O_SEQUENTIAL

#else

#define _OPEN			open
#define _FILELEN		_filelength
#define _READ			read
#define _CLOSE			close
#define GLFILE_FLAGS	O_RDONLY
#endif

int glhReadFile( const char* strFilename, GLchar*& strSource, GLint& nSize )
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
		GLuint index = glGetUniformBlockIndex( program, strLabel );

		DEBUGOUT( "%s\n", strLabel );
	}
	DEBUGOUT( "\n" );
}

int glhCreateBuffer( const GLeffect& effect, const GLchar* strBuffer, GLint nSize, GLbuffer* pBuffer )
{
	MEMSET( pBuffer, 0, sizeof(GLbuffer) );

	if( nSize <= 0 )
		return 0;

	glUseProgram( effect.program );

	pBuffer->data = (GLchar*)MALLOC( nSize );
	if( !pBuffer->data )
		return 0;

	MEMSET( pBuffer->data, 0, nSize );

	pBuffer->location = glGetUniformBlockIndex( effect.program, strBuffer );
	if( pBuffer->location < 0 )															// RETURNING -1
		return 0;

	pBuffer->size = nSize;

	glGenBuffers( 1, &pBuffer->buffer );
	glBindBuffer( GL_UNIFORM_BUFFER, pBuffer->buffer );
	glBufferData( GL_UNIFORM_BUFFER, pBuffer->size, pBuffer->data, GL_DYNAMIC_DRAW );

	for( Int32 i = 0; i < effect.blockcount; i++ )
	{
		if( pBuffer->location == effect.blocks[i*2] )
		{
			pBuffer->base = i*2+1;
			glBindBufferBase( GL_UNIFORM_BUFFER, effect.blocks[i*2+1], pBuffer->buffer );
			break;
		}
	}

	return 1;
}

int glhUpdateBuffer( const GLeffect& effect, const GLbuffer& buffer )
{
	glBindBuffer( GL_UNIFORM_BUFFER, buffer.buffer );
	glBufferData( GL_UNIFORM_BUFFER, buffer.size, buffer.data, GL_DYNAMIC_DRAW );
	glBindBufferBase( GL_UNIFORM_BUFFER, effect.blocks[buffer.base], buffer.buffer );

	return 1;
}

void glhDestroyBuffer( GLbuffer& buffer )
{
	glDeleteBuffers( 1, &buffer.buffer );
	free( buffer.data );
}

int glhCreateMesh( GLmesh& glmesh, const SEG::Mesh& meshdata )
{
	glGenVertexArrays( 1, &glmesh.vao ); 
    glBindVertexArray( glmesh.vao );

	glGenBuffers( 1, &glmesh.vbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, glmesh.vbuffer );
	glBufferData( GL_ARRAY_BUFFER, meshdata.GetVertexDataSize(), meshdata.GetVertexData(), GL_STATIC_DRAW );

	glGenBuffers( 1, &glmesh.ibuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, glmesh.ibuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, meshdata.GetIndexDataSize(), meshdata.GetIndexData(), GL_STATIC_DRAW );

	glmesh.vstride = meshdata.GetVertexSize();
	glmesh.icount = meshdata.GetIndexCount();
	glmesh.type = meshdata.GetType();

	glhPredefinedVertexLayout( glmesh.type );

	glBindVertexArray( 0 );

	return 1;
}

int glhDrawMesh( const GLeffect& gleffect, const GLmesh& glmesh )
{
	glUseProgram( gleffect.program );
	glBindVertexArray( glmesh.vao );
	glDrawElements( GL_TRIANGLES, glmesh.icount, GL_UNSIGNED_SHORT, (void*)0 );
	glBindVertexArray( 0 );

	return 1;
}

void glhDestroyMesh( GLmesh& glmesh )
{
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	glDeleteBuffers( 1, &glmesh.vbuffer );
	glDeleteBuffers( 1, &glmesh.ibuffer );
	glDeleteVertexArrays( 1, &glmesh.vao );

	MEMSET( &glmesh, -1, sizeof(GLmesh) );
}

void glhPredefinedVertexLayout( Int32 nType )
{
	switch( nType )
	{
	case 1:
		glEnableVertexAttribArray( 0 );
		glEnableVertexAttribArray( 1 );
		glEnableVertexAttribArray( 2 );
		glEnableVertexAttribArray( 3 );
		glVertexAttribPointer( 0, 4, GL_SHORT,			GL_FALSE, sizeof(SEG::VertexSS), (void*)0 );
		glVertexAttribPointer( 1, 2, GL_FLOAT,			GL_FALSE, sizeof(SEG::VertexSS), (void*)8 );
		glVertexAttribPointer( 2, 4, GL_UNSIGNED_BYTE,	GL_FALSE, sizeof(SEG::VertexSS), (void*)16 );
		glVertexAttribPointer( 3, 4, GL_SHORT,			GL_FALSE, sizeof(SEG::VertexSS), (void*)20 );
		break;
	};
}
