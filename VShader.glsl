
in vec3 vs_position;

layout (std140,row_major) uniform cstPerMesh
{
	mat4	g_matWorld;
	mat4	g_matWorldViewProj;
};

layout (std140,row_major) uniform cstPerFrame
{
	vec4	g_vEyePos;
	vec4	g_vEyeDir;

	mat4	g_matView;
	mat4	g_matProj;
	mat4	g_matViewProj;
};

void main()
{
	gl_Position = vec4( vs_position, 1.0f ) * g_matWorld * g_matViewProj;
}
