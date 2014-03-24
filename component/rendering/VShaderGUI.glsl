#version 330
#define saturate( x ) clamp( x, 0.0, 1.0 )
#define _inst gl_InstanceID

in vec3 vs_position;
in vec2 vs_texcoord;

out vec2 ps_texcoord;

layout (shared,row_major) uniform cstGUI
{
	mat4	g_matWorldViewProj;
};

void main()
{
	ps_texcoord = vs_texcoord;
	gl_Position = vec4( vs_position, 1 ) * g_matWorldViewProj;
}
