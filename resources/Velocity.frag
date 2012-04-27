#version 110
uniform sampler2D position;
uniform sampler2D velocity;
uniform vec2 mousePos;
uniform float mousePressed;
uniform float minThresh;
uniform float maxThresh;
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

void main()
{	
	vec2 texCoord		= gl_TexCoord[0].st;
	vec4 vPos			= texture2D( position, texCoord );
	vec3 myPos			= vPos.xyz;
	float leadership	= vPos.a;
	
	vec4 vVel			= texture2D( velocity, texCoord );
	vec3 myVel			= vVel.xyz;
	float myCrowd		= vVel.a;
	
//	float zoneRadSqrd	= pow( zoneRadius - myCrowd * 0.1, 2.0 );
	float zoneRadSqrd	= zoneRadius * zoneRadius;
	vec3 acc			= vec3( 0.0, 0.0, 0.0 );
	float offset		= invWidth * 0.5;
	
	int myX = int(texCoord.s) * w;
	int myY = int(texCoord.t) * h;
	float crowded = 5.0;
	

	// APPLY THE ATTRACTIVE, ALIGNING, AND REPULSIVE FORCES
	for( int y=0; y<h; y++ ){
		for( int x=0; x<w; x++ ){
			if( x != myX && y != myY ){
				vec2 tc			= vec2( float(x) * invWidth + offset, float(y) * invHeight + offset );
				vec3 pos		= texture2D( position, tc ).xyz;
				vec3 dir		= myPos - pos;
				
				float dist		= length( dir );
				float distSqrd	= dist * dist;
				
				vec3 dirNorm	= normalize( dir );
				
				if( distSqrd < zoneRadSqrd ){
					
					float percent = distSqrd/zoneRadSqrd + 0.0000001;
					crowded += ( 1.0 - percent ) * 2.0;

					if( percent < minThresh ){
						float F  = ( minThresh/percent - 1.0 );
						acc		+= dirNorm * F * 0.1 * timeDelta * leadership;
						
					} else if( percent < maxThresh && mainPower > 0.1 ){
						float threshDelta		= maxThresh - minThresh;
						float adjustedPercent	= ( percent - minThresh )/( threshDelta + 0.0000001 );
						float F					= ( 1.0 - ( cos( adjustedPercent * 6.28318 ) * -0.5 + 0.5 ) );
						
						acc += normalize( texture2D( velocity, tc ).xyz ) * F * 0.1 * timeDelta * leadership;
						
					} else if( dist < zoneRadius && mainPower > 0.1 ){
						
						float threshDelta		= 1.0 - maxThresh;
						float adjustedPercent	= ( percent - maxThresh )/( threshDelta + 0.0000001 );
						float F					= ( 1.0 - ( cos( adjustedPercent * 6.28318 ) * -0.5 + 0.5 ) ) * 0.1 * timeDelta;

						acc -= dirNorm * F * leadership;
						
					}
				}
			}
		}
	}

	
	if( mousePressed > 0.5 ){
		vec3 dirToMouse			= myPos - vec3( mousePos.x * -100.0, mousePos.y * -60.0, 0.0 );
		float distToMouse		= length( dirToMouse );
		float distToMouseSqrd	= distToMouse * distToMouse;
		
		if( distToMouseSqrd > 625.0 && distToMouseSqrd < 15000.0 ){
			acc -= normalize( dirToMouse ) * ( 15000.0 / distToMouseSqrd ) * 0.00125 * timeDelta;
		}
	}
	

	
	myCrowd -= ( myCrowd - crowded ) * ( 0.1 * timeDelta );

	// MODULATE MYCROWD MULTIPLIER AND GRAVITY FOR INTERESTING DERIVATIONS
		
//	if( mainPower > 0.5 ){
//		myVel += acc * timeDelta;
//		float newMaxSpeed = maxSpeed + myCrowd * 0.02;			// CROWDING MAKES EM FASTER
//		
//		float velLength = length( myVel );						// GET READY TO IMPOSE SPEED LIMIT
//		//	if( velLength < minSpeed ){								// SPEED LIMIT FOR SLOW
//		//		myVel = normalize( myVel ) * minSpeed;
//		//	} else 
//		if( velLength > newMaxSpeed ){							// SPEED LIMIT FOR FAST
//			myVel = normalize( myVel ) * newMaxSpeed;
//		}
//	} else {
//		myVel += vec3( 0.0, -0.05, 0.0 );
//	}
	
	
	myVel += acc * timeDelta;
	float newMaxSpeed = maxSpeed + myCrowd * 0.02;			// CROWDING MAKES EM FASTER
	
	float velLength = length( myVel );						// GET READY TO IMPOSE SPEED LIMIT
	//	if( velLength < minSpeed ){								// SPEED LIMIT FOR SLOW
	//		myVel = normalize( myVel ) * minSpeed;
	//	} else 
	if( velLength > newMaxSpeed ){							// SPEED LIMIT FOR FAST
		myVel = normalize( myVel ) * newMaxSpeed;
	}
	
	
//	myVel += vec3( 0.0, -0.05, 0.0 );
	
	
	
	
	
	// MAIN GRAVITY TO MAKE THEM FALL
	myVel += vec3( 0.0, -0.0025, 0.0 );
	
	
	vec3 tempNewPos		= myPos + myVel * timeDelta;		// NEXT POSITION
	
	// AVOID WALLS
	if( mainPower > 0.5 ){
		float xPull	= tempNewPos.x/( roomBounds.x );
		float yPull	= tempNewPos.y/( roomBounds.y );
		float zPull	= tempNewPos.z/( roomBounds.z );
		myVel -= vec3( xPull * xPull * xPull * xPull * xPull, 
					   yPull * yPull * yPull * yPull * yPull, 
					   zPull * zPull * zPull * zPull * zPull ) * 0.02;
	} else {
		float decay = -0.25;
		if( tempNewPos.y < -roomBounds.y ){
			myVel.y *= decay;
		} else if( tempNewPos.y > roomBounds.y ){
			myVel.y *= decay;
		}

		if( tempNewPos.x < -roomBounds.x ){
			myVel.x *= decay;
		} else if( tempNewPos.x > roomBounds.x ){
			myVel.x *= decay;
		}

		if( tempNewPos.z < -roomBounds.z ){
			myVel.z *= decay;
		} else if( tempNewPos.z > roomBounds.z ){
			myVel.z *= decay;
		}
	}
	
	gl_FragColor.rgb	= myVel;
	gl_FragColor.a		= myCrowd;
}
