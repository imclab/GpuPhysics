#version 110
uniform sampler2D pos0Tex;
uniform sampler2D pos1Tex;
uniform sampler2D vel0Tex;
uniform sampler2D vel1Tex;
uniform float dt;

void main()
{	
	// vPos0.rgb	= position
	// vPos0.a		= mass
	vec4 vPos0		= texture2D( pos0Tex, gl_TexCoord[0].st );
	
	// vPos1.rgb	= unused
	// vPos1.a		= unused
	vec4 vPos1		= texture2D( pos1Tex, gl_TexCoord[0].st );
	
	// vVel0.rgb	= velocity
	// vVel0.a		= collision amount
	vec4 vVel0		= texture2D( vel0Tex, gl_TexCoord[0].st );
	
	// vVel1.rgb	= unused
	// vVel1.a		= unused
	vec4 vVel1		= texture2D( vel1Tex, gl_TexCoord[0].st );
	
	// Add the velocity vector to the position vector
	vec3 pos		= vPos0.xyz + ( vVel0.xyz * dt );
	
	// Do nothing to the mass
	float mass		= vPos0.a;
	
	// Store new position and mass back into fbo target 0
	gl_FragData[0]		= vec4( pos, mass );
	
	// Store the unused data back into fbo target 1
	gl_FragData[1]		= vPos1;
}
