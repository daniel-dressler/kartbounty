#ifndef __SHADER_STRUCTS__
#define __SHADER_STRUCTS__

#define MAX_LIGHTS	5

struct cstPerMesh
{
	Matrix matWorld;
	Matrix matWorldViewProj;
};

struct cstPerFrame
{
	Vector4 vEyePos;
	Vector4 vEyeDir;
	Vector4 vEyeExtra;

	Matrix matView;
	Matrix matProj;
	Matrix matViewProj;

	Vector4 vLight[MAX_LIGHTS];
};

#endif
