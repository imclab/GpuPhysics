//
//  SpringCam.cpp
//  Matter
//
//  Created by Robert Hodgin on 3/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SpringCam.h"

using namespace ci;

SpringCam::SpringCam()
{
}

SpringCam::SpringCam( float camDist, float aspectRatio )
{
	mCamDist		= camDist;
	
	mEye			= Vec3f( 0.0f, 0.0f, mCamDist );
	mCenter			= Vec3f( 0.0f, 0.0f, mCamDist * 0.8f );
	mUp				= Vec3f::yAxis();
	
	mEyeNode		= SpringNode( mEye );
	mCenNode		= SpringNode( mCenter );
	mUpNode			= SpringNode( mUp );
	
	mCam.setPerspective( 65.0f, aspectRatio, 5.0f, 3000.0f );
}

void SpringCam::update( float timeDelta )
{	
	mEyeNode.apply();
	mCenNode.apply();
	
	mEyeNode.update( timeDelta );
	mCenNode.update( timeDelta );
	
	mCam.lookAt( mEyeNode.mPos, mCenNode.mPos, mUpNode.mPos );
	mMvpMatrix		= mCam.getProjectionMatrix() * mCam.getModelViewMatrix();
}