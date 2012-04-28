#version 110
uniform sampler2D pos0Tex;
uniform sampler2D pos1Tex;
uniform sampler2D vel0Tex;
uniform sampler2D vel1Tex;
uniform float timeDelta;

// POSITION RGB
// RADIUS	A

void main()
{	
	vec4 vPos0			= texture2D( pos0Tex, gl_TexCoord[0].st );
	vec4 vPos1			= texture2D( pos1Tex, gl_TexCoord[0].st );
	vec4 vVel0			= texture2D( vel0Tex, gl_TexCoord[0].st );
	vec4 vVel1			= texture2D( vel1Tex, gl_TexCoord[0].st );
	
	vec3 newPos			= vPos0.xyz + ( vVel0.xyz * timeDelta );
	newPos += vVel1.xyz;
	
	float radius		= vPos0.a;
	radius += vVel1.a;
	
	gl_FragData[0]		= vec4( newPos, radius );
	gl_FragData[1]		= vPos1;
}
