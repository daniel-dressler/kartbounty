﻿#include "../../Standard.h"
#include "glhelpers.h"

#include "Outsource/lodepng.h"

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
		GLchar strError[99999];
		GLsizei nLogSize;
		glGetShaderInfoLog( shader, 99999, &nLogSize, strError );
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
		glGetUniformBlockIndex( program, strLabel );

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
	glUseProgram( effect.program );
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

int glhUpdateInst( GLmesh& glmesh, const void* pData, Int32 nVertSize, Int32 nCount )
{
	glBindBuffer( GL_ARRAY_BUFFER, glmesh.xbuffer );
	glBufferData( GL_ARRAY_BUFFER, nVertSize * nCount, pData, GL_STREAM_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	glmesh.xcount = nCount;

	return 1;
}

int glhCreateInst( GLmesh& glmesh, const void* pData, Int32 nVertSize, Int32 nCount, Int32 nType )
{
    glBindVertexArray( glmesh.vao );

	glGenBuffers( 1, &glmesh.xbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, glmesh.xbuffer );
	glBufferData( GL_ARRAY_BUFFER, nVertSize * nCount, pData, GL_DYNAMIC_DRAW );

	glmesh.xcount = nCount;
	glhPredefinedVertexLayout( nType );

	glBindVertexArray( 0 );

	return 1;
}

int glhCreateMesh( GLmesh& glmesh, const void* pData, Int32 nVertSize, Int32 nCount, Int32 nType )
{
	glGenVertexArrays( 1, &glmesh.vao ); 
    glBindVertexArray( glmesh.vao );

	glGenBuffers( 1, &glmesh.vbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, glmesh.vbuffer );
	glBufferData( GL_ARRAY_BUFFER, nVertSize * nCount, pData, GL_STATIC_DRAW );

	glmesh.vstride = nVertSize;
	glmesh.icount = nCount;
	glmesh.type = nType;

	glhPredefinedVertexLayout( nType );

	glBindVertexArray( 0 );

	return 1;
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

int glhCreateGUI( GLmesh& glmesh, const GLvertex* aryVertices, const GLint nCount )
{
	glGenVertexArrays( 1, &glmesh.vao ); 
    glBindVertexArray( glmesh.vao );

	glGenBuffers( 1, &glmesh.vbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, glmesh.vbuffer );
	glBufferData( GL_ARRAY_BUFFER, nCount * sizeof(GLvertex), aryVertices, GL_STATIC_DRAW );

	glmesh.vstride = sizeof(GLvertex);
	glmesh.icount = nCount;
	glmesh.type = 10;

	glhPredefinedVertexLayout( 10 );

	glBindVertexArray( 0 );

	return 1;
}

int glhDrawMesh( const GLeffect& gleffect, const GLmesh& glmesh )
{
	glBindVertexArray( glmesh.vao );
	glUseProgram( gleffect.program );
	if( glmesh.type == 10 )
		glDrawArrays( GL_TRIANGLES, 0, glmesh.icount );
	else
		glDrawElements( GL_TRIANGLES, glmesh.icount, GL_UNSIGNED_SHORT, (void*)0 );
	glBindVertexArray( 0 );
	return 1;
}

int glhDrawInst( const GLeffect& gleffect, const GLmesh& glmesh )
{
	glBindVertexArray( glmesh.vao );
	glUseProgram( gleffect.program );
	glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, glmesh.icount, glmesh.xcount );
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
	glDeleteBuffers( 1, &glmesh.xbuffer );
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
	case 20:
		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 2, GL_FLOAT,	GL_FALSE, sizeof(Vector2), (void*)0 );
		glVertexAttribDivisor( 0, 0 );
		break;
	case 30:
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 4, GL_FLOAT,	GL_FALSE, sizeof(Vector4) * 4, (void*)0 );
		glVertexAttribDivisor( 1, 1 );

		glEnableVertexAttribArray( 2 );
		glVertexAttribPointer( 2, 4, GL_FLOAT,	GL_FALSE, sizeof(Vector4) * 4, (void*)16 );
		glVertexAttribDivisor( 2, 1 );

		glEnableVertexAttribArray( 3 );
		glVertexAttribPointer( 3, 4, GL_FLOAT,	GL_FALSE, sizeof(Vector4) * 4, (void*)32 );
		glVertexAttribDivisor( 3, 1 );

		glEnableVertexAttribArray( 4 );
		glVertexAttribPointer( 4, 4, GL_FLOAT,	GL_FALSE, sizeof(Vector4) * 4, (void*)48 );
		glVertexAttribDivisor( 4, 1 );
		break;
	case 10:
		glEnableVertexAttribArray( 0 );
		glEnableVertexAttribArray( 1 );
		glEnableVertexAttribArray( 2 );
		glDisableVertexAttribArray( 3 );
		glVertexAttribPointer( 0, 3, GL_FLOAT,	GL_FALSE, sizeof(GLvertex), (void*)0 );
		glVertexAttribPointer( 1, 2, GL_FLOAT,	GL_FALSE, sizeof(GLvertex), (void*)12 );
		glVertexAttribPointer( 2, 4, GL_FLOAT,	GL_FALSE, sizeof(GLvertex), (void*)20 );
		break;
	};
}

int glhLoadTexture( GLtex& gltex, char* strFilename, int mips )
{
	int nSize;
	char* pData;
	if( !glhReadFile( strFilename, pData, nSize ) )
		return 0;

	unsigned char* pImageData;
	unsigned int nWidth, nHeight;
	int rtn = (int)lodepng_decode32( &pImageData, &nWidth, &nHeight, (unsigned char*)pData, nSize );
	
	free( pData );
	if( rtn )
		return 0;

	rtn = glhCreateTexture( gltex, (int)nWidth, (int)nHeight, (char*)pImageData, mips );
	free( pImageData );
	if( !rtn )
		return 0;

	return 1;
}

int glhCreateTexture( GLtex& gltex, int nWidth, int nHeight, char* pData, int mips )
{
	glGenTextures( 1, &gltex.id );
	glBindTexture( GL_TEXTURE_2D, gltex.id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData );
	glEnable( GL_TEXTURE_2D );
	
	if( mips )
	{
		glGenerateMipmap( GL_TEXTURE_2D );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	}

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glBindTexture( GL_TEXTURE_2D, 0 );
	
	return 1;
}

int glhEnableTexture( const GLtex& gltex, int nIndex )
{
	if( !nIndex )
		glActiveTexture( GL_TEXTURE0 );
	else
		glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, gltex.id );

	return 1;
}

void glhMapTexture( GLeffect& gleft, char* strName, int nIndex )
{
	glUseProgram( gleft.program );
	GLint texLoc;
	texLoc = glGetUniformLocation( gleft.program, strName );
	glUniform1i( texLoc, nIndex );
}

void glhDestroyTexture( GLtex& gltex )
{
	glDeleteTextures( 1, &gltex.id );
}

