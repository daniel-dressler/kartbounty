
in vec3 ps_worldpos;
in vec3 ps_normal;
in vec4 ps_color;
in vec4 ps_texcoord;
in vec3 ps_tangent;
in vec3 ps_bitangent;

out vec4 out_color;

uniform sampler2D g_texDiffuse;
uniform sampler2D g_texNormal;

void main()
{
	vec4 texDiffuse = texture2D( g_texDiffuse, ps_texcoord.xy );
	vec4 texNormal = texture2D( g_texNormal, ps_texcoord.xy );

	vec3 vNormal;

	float fPower = 0;
	if( g_vRenderParams.z > 0.5f )
	{
		fPower = 1;
		vNormal = vec3( 0, 0, 0 );
	}
	else
	{
		texNormal.xyz = ( texNormal.xyz * 2.0f ) - 1.0f;
		vNormal = normalize( mat3( ps_tangent, ps_bitangent, ps_normal ) * texNormal.xyz );

		int light = 0;
		while( light < MAX_LIGHTS )
		{
			vec3 vLightDiff = g_vLight[light].xyz - ps_worldpos.xyz;
			float fLightDist = length( vLightDiff );
			vec3 vLightDir = vLightDiff / fLightDist;

			float fLdN = dot( vLightDir.xyz, vNormal );

			fPower += fLdN * saturate( 1.0f - ( fLightDist / g_vLight[light].w ) );

			light++;
		}
		fPower = saturate( fPower );

		// Temp light
		fPower = clamp( fPower, 0.2f, 1.0f );
	}

	vec3 color = g_vColor.rgb * ps_color.rgb * texDiffuse.rgb * fPower;
	out_color  = vec4( color.xyz, texDiffuse.a * ps_color.a * g_vColor.a );
//	out_color  = vec4( ( vNormal.xyz ), 1 );

	// Lighting Test
//	out_color = vec4( fPower, fPower, fPower, 1 );
}
