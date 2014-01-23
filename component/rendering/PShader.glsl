
in vec3 ps_worldpos;
in vec3 ps_normal;
in vec4 ps_color;
in vec4 ps_texcoord;
in vec3 ps_tangent;
in vec3 ps_bitangent;

out vec4 out_color;

void main()
{
	float fDot = dot( g_vEyeDir.xyz, ps_normal );
	vec3 color = ps_color.xyz * ( ( 1.0f - fDot ) / 2 );
	out_color  = vec4( color, ps_color.a );
//	out_color  = vec4( 1,1,1,1 );
}
