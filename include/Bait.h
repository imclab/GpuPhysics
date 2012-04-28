//
//  Bait.h
//  Flocking
//
//  Created by Robert Hodgin on 4/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once
#include "cinder/Vector.h"
#include "cinder/Color.h"

class Bait {
  public:
	Bait();
	Bait( ci::Vec3f pos );
	void update( float yFloor );
	void draw();
	
	ci::Vec3f	mPos;
	float		mRadius;
	float		mFallSpeed;
	ci::Color	mColor;
	bool		mIsDead;
	bool		mIsDying;
};