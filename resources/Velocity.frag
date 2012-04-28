#version 110
uniform sampler2D position;
uniform sampler2D velocity;
uniform vec2 mousePos;
uniform float mousePressed;
uniform float zoneRadius;
uniform float zoneRadiusSqrd;
uniform float minSpeed;
uniform float maxSpeed;
uniform int w;
uniform int h;
uniform float invWidth;
uniform float invHeight;
uniform vec3 roomBounds;
uniform float timeDelta;
uniform float mainPower;
uniform float gravity;


const float M_4_PI = 12.566370614359172;

struct Sphere
{
	vec3	pos;
	vec3	vel;
	float	radius;
	float	mass;
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

//
// from http://wp.freya.no/3d-math-and-physics/simple-sphere-sphere-collision-detection-and-collision-response/
//
bool simpleSphereSphere( const Sphere a, const Sphere b )
{
	vec3 dir		= a.pos - b.pos;
	float dist		= length( dir );
	float sumRadii	= a.radius + b.radius;
	
	if( dist <= sumRadii ){
		return true;
	}
	
	return false;
}

bool advancedSphereSphere( Sphere a, Sphere b, inout float t )
{
	vec3 s		= a.pos - b.pos;
	vec3 v		= a.vel - b.vel;
	float r		= a.radius + b.radius;
	
	float c1	= dot( s, s ) - r*r;
	
	if( c1 < 0.0 ){
		t		= 0.0;
		return true;
	}
	
	float a1 = dot( s, v );
	if( a1 < 0.00001 )
		return false;
	
	float b1 = dot( v, s );
	if( b1 >= 0.0 )
		return false;
	
	float d1 = b1*b1 - a1*c1;
	if( d1 < 0.0 )
		return false;
	
	t = ( -b1 - sqrt(d1) )/a1;
	
	return true;
}

void sphereCollisionResponse( inout Sphere a, inout Sphere b )
{
    vec3 U1x,U1y,U2x,U2y,V1x,V1y,V2x,V2y;
	
	
    float x1, x2;
    vec3 v1temp, v1, v2, v1x, v2x, v1y, v2y;
	vec3 x = a.pos - b.pos;
	
    normalize( x );
    v1	= a.vel;
    x1	= dot( x, v1 );
    v1x = x * x1;
    v1y = v1 - v1x;
	
    x	*= -1.0;
    v2	= b.vel;
    x2	= dot( x, v2 );
    v2x = x * x2;
    v2y = v2 - v2x;
	
	float m1	= a.mass;
    float m2	= b.mass;
	float sumMass = a.mass + b.mass;
	a.vel *= 0.99;
	a.vel += vec3( v1x * (m1-m2)/sumMass + v2x * (2.0*m2)/sumMass + v1y ) * 0.0005;
//	b.vel += vec3( v1x * (2.0*m1)/sumMass + v2x*(m2-m1)/sumMass + v2y );
}


void main()
{	
	vec2 texCoord		= gl_TexCoord[0].st;
	vec4 vPos			= texture2D( position, texCoord );	// my current position
	vec4 vVel			= texture2D( velocity, texCoord );	// my current velocity
	
	Sphere s1			= Sphere( vPos.xyz, vVel.xyz * 0.995, vPos.a, getMassFromRadius( vPos.a ) ); 	

	vec3 acc			= vec3( 0.0, 0.0, 0.0 );
	float wOffset		= invWidth * 0.5;
	float hOffset		= invHeight * 0.5;
	
	int myX = int( ( texCoord.s - wOffset ) * float(w) );
	int myY = int( ( texCoord.t - hOffset ) * float(h) );

	vec3 posOffset = vec3( 0.0 );
	float radiusOffset = 0.0;
	float massTrans = 0.0;
	
	float collideAmt = 0.0;
	// APPLY THE REPULSIVE FORCES
	for( int y=0; y<h; y++ ){
		for( int x=0; x<w; x++ ){
			if( x == myX && y == myY ){

			} else {
				vec2 tc			= vec2( float(x) * invWidth + wOffset, float(y) * invHeight + hOffset );
				vec4 P			= texture2D( position, tc );
				vec4 V			= texture2D( velocity, tc );
				
				Sphere s2		= Sphere( P.xyz, V.xyz, P.a, getMassFromRadius( P.a ) );
				
				vec3 dir = s1.pos - s2.pos;
				float dist = length( dir );
				float timeToCollision;
                if( advancedSphereSphere( s1, s2, timeToCollision ) )
                {
					collideAmt += 0.05;
					sphereCollisionResponse( s1, s2 );
						
					float overlap = length( dir ) - s1.radius - s2.radius;
					if( overlap <= 0.0 ){
						posOffset += normalize( dir ) * -overlap * 0.1;
						
//						if( s1.radius > s2.radius ){
//							float new2Radius = s2.radius + overlap;
//							float new2Mass = getMassFromRadius( new2Radius );
//							float massLost = s2.mass - new2Mass;
//							
//							float new1Mass = s1.mass + massLost;
//							float new1Radius = getRadiusFromMass( new1Mass );
//							
//						}

						
//						massTrans += getMassFromRadius( ( s1.radius - s2.radius ) * 0.01 );
//						radiusOffset += getRadiusFromMass( massTrans );
						
					}
                }
				
				float F = ( s1.mass * s2.mass )/( dist * dist );
				acc -= F * normalize( dir ) * 0.00000003;
			}
		}
	}
	
	acc += vec3( 0.0, gravity, 0.0 );
	
	s1.vel += acc * timeDelta;
	
	
	vec3 tempNewPos		= s1.pos + s1.vel * timeDelta;		// NEXT POSITION
	
	// AVOID WALLS
	if( mainPower > 0.5 ){
		float xPull	= tempNewPos.x/( roomBounds.x );
		float yPull	= tempNewPos.y/( roomBounds.y );
		float zPull	= tempNewPos.z/( roomBounds.z );
		s1.vel -= vec3( xPull * xPull * xPull * xPull * xPull * xPull * xPull * xPull * xPull, 
					    yPull * yPull * yPull * yPull * yPull * yPull * yPull * yPull * yPull, 
					    zPull * zPull * zPull * zPull * zPull * zPull * zPull * zPull * zPull ) * 0.01;
	}
	
	float velLength = length( s1.vel );						// GET READY TO IMPOSE SPEED LIMIT
	if( velLength > maxSpeed ){							// SPEED LIMIT FOR FAST
		s1.vel = normalize( s1.vel ) * maxSpeed;
	}
	
	
	bool hitWall = false;
	vec3 wallNormal = vec3( 0.0 );
	
	if( tempNewPos.y < -roomBounds.y ){
		hitWall = true;
		wallNormal += vec3( 0.0, 1.0, 0.0 );
	} else if( tempNewPos.y > roomBounds.y ){
		hitWall = true;
		wallNormal += vec3( 0.0,-1.0, 0.0 );
	}

	if( tempNewPos.x < -roomBounds.x ){
		hitWall = true;
		wallNormal += vec3( 1.0, 0.0, 0.0 );
	} else if( tempNewPos.x > roomBounds.x ){
		hitWall = true;
		wallNormal += vec3(-1.0, 0.0, 0.0 );
	}

	if( tempNewPos.z < -roomBounds.z ){
		hitWall = true;
		wallNormal += vec3( 0.0, 0.0, 1.0 );
	} else if( tempNewPos.z > roomBounds.z ){
		hitWall = true;
		wallNormal += vec3( 0.0, 0.0,-1.0 );
	}
	
	if( hitWall ){
		normalize( wallNormal );
		vec3 reflect = 2.0 * wallNormal * ( wallNormal * s1.vel );
		s1.vel -= reflect * 0.5;
	}
	
	gl_FragData[0]		= vec4( s1.vel, collideAmt );
	gl_FragData[1]		= vec4( posOffset, radiusOffset );
}
