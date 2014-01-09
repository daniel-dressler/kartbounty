
in vec3 ps_worldpos;
in vec3 ps_normal;
in vec4 ps_color;

in vec4 ps_texcoord;
in vec3 ps_tangent;
in vec3 ps_bitangent;

out vec4 out_color;

void main()
{
	vec3 color = ps_color.xyz * saturate( -dot( g_vEyeDir.xyz, ps_normal ) );
	out_color = vec4( color, ps_color.a ); //vec4( ps_eyedir, 1 );
}
