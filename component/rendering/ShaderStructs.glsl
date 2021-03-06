#version 330
#define saturate( x ) clamp( x, 0.0, 1.0 )
#define _inst gl_InstanceID

#define MAX_LIGHTS	20

layout (shared,row_major) uniform cstPerMesh
{
	mat4	g_matWorld;
	mat4	g_matWorldViewProj;
	vec4	g_vRenderParams;
	vec4	g_vColor;
};

layout (shared,row_major) uniform cstPerFrame
{
	vec4	g_vEyePos;
	vec4	g_vEyeDir;
	vec4	g_vEyeExtra;

	mat4	g_matView;
	mat4	g_matProj;
	mat4	g_matViewProj;

	vec4	g_vLight[MAX_LIGHTS];
};

