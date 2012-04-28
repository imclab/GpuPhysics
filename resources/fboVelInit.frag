uniform sampler2D initTex;

void main()
{
	vec4 target0Col = texture2D( initTex, gl_TexCoord[0].st );
	vec4 target1Col = vec4( 0.0 );

	gl_FragData[0] = target0Col;
	gl_FragData[1] = target1Col;
}