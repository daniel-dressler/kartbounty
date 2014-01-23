
in vec3 vs_position;
in vec2 vs_tangents;
in vec4 vs_color;
in vec4 vs_texcoord;

out vec3 ps_worldpos;
out vec3 ps_normal;
out vec4 ps_color;
out vec4 ps_texcoord;
out vec3 ps_tangent;
out vec3 ps_bitangent;

void main()
{
	vec4 position = vec4( vs_position * 0.0005f, 1.0f );
	vec3 normal   = fract(      vs_tangents.x   * vec3(1,256,65536) ) * 2 - 1;
	vec3 tangent  = fract( abs( vs_tangents.y ) * vec3(1,256,65536) ) * 2 - 1;

	gl_Position   = position * g_matWorldViewProj;
//	gl_Position.y = -gl_Position.y;
//	gl_Position   = g_matWorldViewProj * position;

	ps_worldpos   = vec4( position * g_matWorld ).xyz;
	ps_normal     = normalize( mat3( g_matWorld ) * normal );
	ps_color      = vs_color * ( 1.0f / 256.0f );

	ps_texcoord   = vec4( vs_texcoord.xy * 0.0005f, vs_texcoord.zw );
	ps_tangent    = normalize( mat3( g_matWorld ) * tangent );
	ps_bitangent  = normalize( cross( normal, tangent ) * ( vs_tangents.y > 0 ? 1 : -1 ) );
}
