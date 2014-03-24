#ifndef __SHADER_STRUCTS__
#define __SHADER_STRUCTS__

#define MAX_LIGHTS	20

struct cstPerMesh
{
	Matrix matWorld;
	Matrix matWorldViewProj;
	Vector4 vRenderParams;
	Vector4 vColor;
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

struct cstGUI
{
	Matrix matWorldViewProj;
};

#endif
