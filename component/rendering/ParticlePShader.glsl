#version 330
#define saturate( x ) clamp( x, 0.0, 1.0 )
#define _inst gl_InstanceID

// Standard Particle Shader
// Make as minimal as possible
// alpha as texture

in vec4 ps_color;
in vec2 ps_tex;

out vec4 out_color;

uniform sampler2D g_texDiffuse;

void main()
{
	vec4 texParticle = texture2D( g_texDiffuse, ps_tex.xy );
	out_color  = ps_color * texParticle;
//	out_color  = ps_color;
}
