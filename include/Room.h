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
	Room( const ci::Vec3f &bounds );
	void init();
	void update( const ci::Vec3f &bounds );
	void draw();
	void drawPanel( const ci::gl::Texture &tex );
	void setBounds( const ci::Vec3f &bounds ){ mBounds = bounds; };
	ci::Vec3f getBounds(){ return mBounds; };
	ci::Vec3f getRandCeilingPos();
	float getFloorLevel();
	
	ci::gl::VboMesh mVbo;
	ci::Vec3f mBounds;

};