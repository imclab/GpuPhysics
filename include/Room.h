//
//  Room.h
//  Stillness
//
//  Created by Robert Hodgin on 12/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#pragma once
#include "cinder/gl/Vbo.h"

class Room
{
  public:
	Room();
	Room( float xb, float yb, float zb );
	void init();
	void update( float xb, float yb, float zb );
	void draw();
	void setBounds( float xb, float yb, float zb ){ mXBounds = xb; mYBounds = yb; mZBounds = zb; };
	ci::Vec3f getBounds(){ return ci::Vec3f( mXBounds, mYBounds, mZBounds ); };
	
	ci::gl::VboMesh mVbo;
	
	float mXBounds;
	float mYBounds;
	float mZBounds;

};