#version 110
uniform sampler2D position;
uniform sampler2D velocity;
uniform int fboWidth;
uniform int fboHeight;
uniform float invWidth;
uniform float invHeight;
uniform float dt;
uniform float gravity;
uniform float charge;

const float M_4_PI = 12.566370614359172;

struct Sphere
{
	vec3	pos;
	vec3	vel;
	vec3	acc;
	float	mass;
	float	radius;
};


float getRadiusFromMass( float m )
{
	float r = pow( (3.0 * m )/M_4_PI, 0.3333333 );
	return r;
}

float getMassFromRadius( float r )
{
	float m = ( ( r * r * r ) * M_4_PI ) * 0.33333;
	return m;
}


void main()
{	
	vec2 texCoord	= gl_TexCoord[0].st;
	vec4 vPos		= texture2D( position, texCoord );	// my current position
	vec4 vVel		= texture2D( velocity, texCoord );	// my current velocity
	
	// Create my current sphere
	vec3 pos		= vPos.xyz;
	vec3 vel		= vVel.xyz;
	vec3 acc		= vec3( 0.0 );
	float mass		= vPos.a;
	float radius	= getRadiusFromMass( mass );
	Sphere s1		= Sphere( pos, vel, acc, mass, radius ); 	

	// Calculate the x and y index of my current position in the fbo.
	// Subtract wOffset and hOffset to account for the fact that
	// the texCoords reference the center of a pixel. So if we were
	// dealing with a 2x2 pixel fbo, the texCoord of the pixel at [0,0]
	// is 0.25,0.25 instead of 0.0,0.0.
	float wOffset	= invWidth * 0.5;
	float hOffset	= invHeight * 0.5;
	int myX = int( ( gl_TexCoord[0].s - wOffset ) * float(fboWidth) );
	int myY = int( ( gl_TexCoord[0].t - hOffset ) * float(fboHeight) );
	
	
	float collideAmt	= 0.0;
	// Nested for loop to compare current Sphere against all other spheres.
	for( int y=0; y<fboHeight; y++ ){
		for( int x=0; x<fboWidth; x++ ){
			if( x == myX && y == myY ){
				// Avoid comparing my sphere against my sphere
			} else {
				vec2 tc			= vec2( float(x) * invWidth + wOffset, float(y) * invHeight + hOffset );
				vec4 P			= texture2D( position, tc );
				vec4 V			= texture2D( velocity, tc );
				
				// Create other sphere
				vec3 pos		= P.xyz;
				vec3 vel		= V.xyz;
				vec3 acc		= vec3( 0.0 );
				float mass		= P.a;
				float radius	= getRadiusFromMass( mass );
				Sphere s2		= Sphere( pos, vel, acc, mass, radius );
				
				// Compare distance to total radii
				vec3 dir		= s1.pos - s2.pos;
				float dist		= length( dir );
				float sumRadii	= s1.radius + s2.radius;
				
				// If distance between spheres is less than total radii
				if( dist <= sumRadii ){
					// A collision (overlap) is occuring
					collideAmt += 0.33;
				}
				
				// Push away from other sphere
				float F			= ( s1.mass * s2.mass )/( dist * dist );
				dir				= normalize( dir ) * F * 0.0001 * charge;
				
				s1.acc			+= dir/s1.mass;
			}
		}
	}
	
	// Add gravity (keypress 'g' turns on and off gravity)
	s1.acc += vec3( 0.0, gravity, 0.0 );
	
	// Add acceleration vector to velocity vector
	s1.vel += s1.acc * dt;
	
	// Store the new velocity as rgb.
	// Store the collision amount as alpha;
	gl_FragData[0]		= vec4( s1.vel, collideAmt );
	
	// Not currently used.
	gl_FragData[1]		= vec4( 0.0 );
}
