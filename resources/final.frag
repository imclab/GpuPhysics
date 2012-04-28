#version 110
varying vec4 vVertex;
varying vec3 vNormal;
varying vec3 vColor;
varying float collideAmt;

void main()
{	
	gl_FragColor.rgb	= vec3( collideAmt );
	gl_FragColor.a		= 1.0;
}



