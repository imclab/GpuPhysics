#version 110
uniform vec3 eyePos;
uniform float mainPower;

varying float crowd;
varying vec4 vVertex;
varying vec3 vNormal;
varying vec3 lightDir;
void main()
{	
	vec3 eyeDir			= eyePos - vVertex.xyz;
	vec3 eyeDirNorm		= normalize( eyeDir );
	float ppEyeDiff		= max( dot( vNormal, lightDir ), 0.0 );
	float ppSpec		= pow( ppEyeDiff, 20.0 );
	float ppFres		= pow( 1.0 - ppEyeDiff, 2.0 );	
	

	vec3 litRoomColor	= vec3( ppEyeDiff * 0.2 + 0.6 + ppSpec ) * ( vNormal.y * 0.5 + 0.5 );
	vec3 darkRoomColor	= vec3( ( crowd * 0.5 + 0.5 ) * ppEyeDiff * 0.5 ) + crowd * ( vNormal * -0.5 + 0.5 );
	
	gl_FragColor.rgb	= mix( litRoomColor, darkRoomColor, mainPower );
	gl_FragColor.a		= 1.0;
}