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
	GLuint	xbuffer;
	GLuint  vstride;
	GLuint	icount;
	GLuint	xcount;
	GLint	type;
};

struct GLtex
{
	GLuint	id;
};

struct GLvertex
{
	Vector3 vPos;
	Vector2 vTex;
	Vector4 vColor;

	GLvertex() {}
	GLvertex( const Vector3& vPosI, const Vector2& vTexI, const Vector4& vColorI = Vector4( 1,1,1,1 ) )
	{
		vPos = vPosI;
		vTex = vTexI;
		vColor = vColorI;
	}
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

int glhUpdateInst( GLmesh& glmesh, const void* pData, Int32 nVertSize, Int32 nCount );
int glhCreateInst( GLmesh& glmesh, const void* pData, Int32 nVertSize, Int32 nCount, Int32 nType );
int glhCreateMesh( GLmesh& glmesh, const void* pData, Int32 nVertSize, Int32 nCount, Int32 nType );
int glhCreateMesh( GLmesh& glmesh, const SEG::Mesh& meshdata );
int glhCreateGUI( GLmesh& glmesh, const GLvertex* aryVertices, const GLint nCount );
int glhDrawMesh( const GLeffect& gleffect, const GLmesh& glmesh );
int glhDrawInst( const GLeffect& gleffect, const GLmesh& glmesh );
void glhDestroyMesh( GLmesh& glmesh );

void glhPredefinedVertexLayout( Int32 nType );

int glhLoadTexture( GLtex& gltex, char* strFilename, int mips = 0 );
int glhCreateTexture( GLtex& gltex, int nWidth, int nHeight, char* pData, int mips );
int glhEnableTexture( const GLtex& gltex, int nIndex = 0 );
void glhMapTexture( GLeffect& gleft, char* strName, int nIndex );
void glhDestroyTexture( GLtex& gltex );

#endif
