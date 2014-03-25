#version 330
#define saturate( x ) clamp( x, 0.0, 1.0 )
#define _inst gl_InstanceID

uniform sampler2D g_texDiffuse;

in vec2 ps_texcoord;
in vec4 ps_color;

out vec4 out_color;

void main()
{
	out_color = texture2D( g_texDiffuse, ps_texcoord.xy ) * ps_color;
}
