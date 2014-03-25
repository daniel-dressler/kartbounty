#version 330
#define saturate( x ) clamp( x, 0.0, 1.0 )
#define _inst gl_InstanceID

in vec3 vs_position;
in vec2 vs_texcoord;
in vec4 vs_color;

out vec2 ps_texcoord;
out vec4 ps_color;

layout (shared,row_major) uniform cstGUI
{
	mat4	g_matWorldViewProj;
};

void main()
{
	gl_Position = vec4( vs_position, 1 ) * g_matWorldViewProj;
	ps_texcoord = vs_texcoord;
	ps_color = vs_color;
}
