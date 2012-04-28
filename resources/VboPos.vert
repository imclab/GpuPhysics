
uniform sampler2D currentPosition;
uniform sampler2D currentVelocity;
uniform float mainPower;

varying vec4 vVertex;
varying vec3 vNormal;
varying vec3 vColor;
varying vec3 lightDir;
varying float collideAmt;

void main()
{
	gl_TexCoord[0]	= gl_MultiTexCoord0;
	vVertex			= vec4( gl_Vertex );
	vNormal			= vec3( gl_Normal );
	vColor			= vec3( gl_Color );
	
	vec4 currentPos	= texture2D( currentPosition, vColor.xy );
	vec4 currentVel = texture2D( currentVelocity, vColor.xy );
	collideAmt		= currentVel.a;
	
	float radius	= currentPos.a;
	lightDir = vec3( 1.0, 1.0, -1.0 ) * 0.5;
	
	vVertex			= vec4( vVertex.xyz * radius + currentPos.xyz, gl_Vertex.w );
	gl_Position		= gl_ModelViewProjectionMatrix * vVertex;
}