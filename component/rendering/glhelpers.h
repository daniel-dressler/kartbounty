#ifndef __GL_HELPERS__
#define __GL_HELPERS__

struct GLeffect
{
	GLuint program;
	GLuint vertex;
	GLuint geometry;
	GLuint pixel;

	GLint blockcount;
	GLuint* blocks;
};

struct GLbuffer
{
	GLint	location;
	GLuint	buffer;
	GLchar* data;
	GLint	size;
	GLuint  base;
};

struct GLmesh
{
	GLuint  vao;
	GLuint	vbuffer;
	GLuint	ibuffer;
	GLuint  vstride;
	GLuint	icount;
	GLint	type;
};

GLeffect glhLoadEffect( const char* strVertexShader,
						const char* strGeometryShader,
						const char* strPixelShader,
						const char** aryHeaders,
						int nHeaderCount );
void glhUnloadEffect( GLeffect effect );

int glhReadFile( const char* strFilename, GLchar*& strSource, GLint& nSize );
GLuint glhCompileShader( GLuint uType, GLsizei nSourceCount, GLchar** arySourceData, GLint* aryDataSizes );
GLuint glhCombineProgram( GLuint uVertex, GLuint uGeometry, GLuint uPixel );

void glhCheckUniformNames( GLuint program );

int glhCreateBuffer( const GLeffect& effect, const GLchar* strBuffer, GLint nSize, GLbuffer* pBuffer );
int glhUpdateBuffer( const GLeffect& effect, const GLbuffer& buffer );
void glhDestroyBuffer( GLbuffer& buffer );

int glhCreateMesh( GLmesh& glmesh, const SEG::Mesh& meshdata );
int glhDrawMesh( const GLeffect& gleffect, const GLmesh& glmesh );
void glhDestroyMesh( GLmesh& glmesh );

void glhPredefinedVertexLayout( Int32 nType );

#endif
