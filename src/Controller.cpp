//
//  Controller.cpp
//  Flocking
//
//  Created by Robert Hodgin on 4/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "cinder/app/AppBasic.h"
#include "cinder/gl/Gl.h"
#include "cinder/Rand.h"
#include "Controller.h"

using namespace ci;
using std::vector;

Controller::Controller()
{
	
}

void Controller::update( float floorLevel )
{
	for( std::vector<Bait>::iterator it = mBaits.begin(); it != mBaits.end(); ){
		if( it->mIsDead ){
			it = mBaits.erase( it );
		} else {
			it->update( floorLevel );
			++it;
		}
	}
	
	sort( mBaits.begin(), mBaits.end(), depthSortFunc );
}

void Controller::addBait( const Vec3f &pos )
{
	mBaits.push_back( Bait( pos ) );
}

void Controller::drawBaits()
{
	for( std::vector<Bait>::iterator it = mBaits.begin(); it != mBaits.end(); ++it ){
		it->draw();
	}
}

void Controller::drawBaitGlows()
{
	for( std::vector<Bait>::iterator it = mBaits.begin(); it != mBaits.end(); ++it ){
		float radius = it->mRadius * 100.0f;
		gl::color( Color( it->mColor ) );
		gl::drawBillboard( it->mPos, Vec2f( radius, radius ), 0.0f, Vec3f::xAxis(), Vec3f::yAxis() );
		gl::color( Color::white() );
		gl::drawBillboard( it->mPos, Vec2f( radius, radius ) * 0.35f, 0.0f, Vec3f::xAxis(), Vec3f::yAxis() );
	}
}


bool depthSortFunc( Bait a, Bait b ){
	return a.mPos.z > b.mPos.z;
}