//
//  Bait.cpp
//  Flocking
//
//  Created by Robert Hodgin on 4/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "Bait.h"

using namespace ci;

Bait::Bait( Vec3f pos )
{
	mPos		= pos;
	mRadius		= 0.0f;
	mFallSpeed	= Rand::randFloat( -0.5f, -0.15f );
	mColor		= Color( CM_HSV, Rand::randFloat( 0.0f, 0.1f ), 0.9, 1.0 );
	mIsDead		= false;
	mIsDying	= false;
}

void Bait::update( float yFloor ){
	mPos += Vec3f( 0.0f, mFallSpeed, 0.0f );
	if( mPos.y < yFloor ){
		mIsDying = true;
	}
	
	if( mIsDying ){
		mRadius -= ( mRadius - 0.0f ) * 0.2f;
		if( mRadius < 0.1f )
			mIsDead = true;
	} else {
		mRadius -= ( mRadius - Rand::randFloat( 0.9f, 1.2f ) ) * 0.2f;
	}
}

void Bait::draw(){
	gl::drawSphere( mPos, mRadius, 32 );
}