#ifndef __GL_HELPERS__
#define __GL_HELPERS__

struct GLeffect
{
	GLuint program;
	GLuint vertex;
	GLuint geometry;
	GLuint pixel;
};

struct GLbuffer
{
	GLint	location;
	GLuint	buffer;
	GLchar* data;
	GLint	size;
};

#ifdef _WIN32
int glhCreateContext( SDL_Window* win );
HDC glhGetHDC();
#endif

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
int glhUpdateBuffer( const GLbuffer& buffer );
void glhDestroyBuffer( GLbuffer& buffer );

#endif
