//
//  Controller.h
//  Flocking
//
//  Created by Robert Hodgin on 4/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once
#include "Bait.h"
#include <vector>

class Controller {
  public:
	Controller();
	void update( float floorLevel );
	void addBait( const ci::Vec3f &pos );
	void drawBaits();
	void drawBaitGlows();
	
	int				mNumBaits;
	std::vector<Bait>	mBaits;
};

bool depthSortFunc( Bait a, Bait b );


