
in vec3 ps_worldpos;
in vec3 ps_normal;
in vec4 ps_color;
in vec4 ps_texcoord;
in vec3 ps_tangent;
in vec3 ps_bitangent;

out vec4 out_color;

void main()
{
	float fPower = 0;

	int light = 0;
	while( light < MAX_LIGHTS )
	{
		vec3 vLightDiff = g_vLight[light].xyz - ps_worldpos.xyz;
		float fLightDist = length( vLightDiff );
		vec3 vLightDir = vLightDiff / fLightDist;

		float fLdN = dot( vLightDir.xyz, ps_normal );
		fPower += fLdN * saturate( 1.0f - ( fLightDist / g_vLight[light].w ) );

		light++;
	}

	vec3 color = ps_color.xyz * saturate( fPower );
	out_color  = vec4( color, ps_color.a );
//	out_color  = vec4( 1,1,1,1 );
}
