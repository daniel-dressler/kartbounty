#ifndef __SHADER_STRUCTS__
#define __SHADER_STRUCTS__

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
};

#endif
