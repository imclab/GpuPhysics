#version 110
uniform sampler2D position;
uniform sampler2D velocity;
uniform float timeDelta;

void main()
{	
	vec4 vPos			= texture2D( position, gl_TexCoord[0].st );
	vec4 vVel			= texture2D( velocity, gl_TexCoord[0].st );
	
	vec3 newPos			= vPos.xyz + ( ( vVel.xyz ) * ( vVel.a * 0.05 ) * timeDelta );
//	if( length( newPos ) > boundingRadius )
//		newPos = normalize( newPos ) * ( boundingRadius );
	
	gl_FragColor.rgb	= newPos;
	gl_FragColor.a		= vPos.a;//vVel.a;
}
