#version 330
#define saturate( x ) clamp( x, 0.0, 1.0 )
#define _inst gl_InstanceID

layout(location = 0) in vec2 vs_corner;		// vertex corner
layout(location = 1) in vec4 vs_pos;		// instance pos,angle
layout(location = 2) in vec4 vs_color;		// instance color
layout(location = 3) in vec4 vs_tex;		// instance tex params
layout(location = 4) in vec4 vs_quater;		// instance quaternion

out vec4 ps_color;
out vec2 ps_tex;

layout (shared,row_major) uniform cstParticles
{
	vec4 g_vEyeUp;
	vec4 g_vEyeSide;
	mat4 g_matWVP;
};

vec3 QuatRot( vec3 v, vec4 q )
{
	return 2.0f * dot( q.xyz, v ) * q.xyz + 
		   ( q.w * q.w - dot( q.xyz, q.xyz ) ) * v + 
		   2.0f * q.w * cross( q.xyz, v );
}

void main()
{
	ps_color = vs_color;
	ps_tex = vs_tex.xy + vs_corner * vs_tex.zw;
//	ps_tex = vs_corner;

	vec3 vPos = g_vEyeSide.xyz * ( vs_corner.x - 0.5f ) + g_vEyeUp.xyz * ( vs_corner.y - 0.5f );

	vec4 qRot;	
	qRot.xyz = cross( g_vEyeSide.xyz, g_vEyeUp.xyz ) * sin( vs_quater.w * 0.5f );
	qRot.w = cos( vs_quater.w * 0.5f );
	vPos = QuatRot( vPos, qRot );

	vPos = vs_pos.xyz + vs_pos.w * vPos;
	gl_Position = vec4( vPos, 1 ) * g_matWVP;
}
