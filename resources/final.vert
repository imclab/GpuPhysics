
uniform sampler2D positions;
uniform sampler2D velocities;

varying vec4 vVertex;
varying vec3 vNormal;
varying vec3 vColor;

varying float collideAmt;


const float M_4_PI = 12.566370614359172;

float getRadiusFromMass( float m ){
	float r = pow( (3.0 * m )/M_4_PI, 0.3333333 );
	return r;
}

float getMassFromRadius( float r ){
	float m = ( ( r * r * r ) * M_4_PI ) * 0.33333;
	return m;
}

void main()
{
	gl_TexCoord[0]	= gl_MultiTexCoord0;
	vVertex			= vec4( gl_Vertex );	// Vertices for unit sphere at origin
	vNormal			= vec3( gl_Normal );	// Normals for unit sphere at origin
	vColor			= vec3( gl_Color );		// Using red and green to store index
	
	vec4 posData	= texture2D( positions, vColor.xy );	// RGB = position : A = mass
	vec4 velData	= texture2D( velocities, vColor.xy );	// RGB = velocity : A = collision amount
	collideAmt		= velData.a;
	
	vec3 pos		= posData.rgb;
	float mass		= posData.a;
	float radius	= getRadiusFromMass( mass );

	// new vertex is original unit sphere vertex * sphere radius,
	// then translated out to correct position
	vVertex			= vec4( vVertex.xyz * radius + pos, gl_Vertex.w );
	gl_Position		= gl_ModelViewProjectionMatrix * vVertex;
}