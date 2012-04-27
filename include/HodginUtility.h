//
//  HodginUtility.h
//  Sol
//
//  Created by Robert Hodgin on 12/8/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#pragma once
#include "cinder/gl/gl.h"

namespace hodgin
{
	namespace gl
	{
		void drawSphericalBillboard( const ci::Vec3f &camEye, const ci::Vec3f &objPos, const ci::Vec2f &scale, float rotInRadians );
	}
	
	namespace math
	{
		float fastSqrt( float number );
		ci::Vec3f calcSurfaceNormal( const ci::Vec3f &p1, const ci::Vec3f &p2, const ci::Vec3f &p3 );
	}
	
	namespace sim
	{
		bool didParticlesCollide( const ci::Vec3f &dir, const ci::Vec3f &dirNormal, const float dist, const float sumRadii, const float sumRadiiSqrd, ci::Vec3f *moveVec );
		float getRadiusFromMass( float mass );
		float getMassFromRadius( float radius );
		float getStarRadiusFromStarMass( float mass );
		float getStarMassFromStarRadius( float radius );
	}
	
	namespace geom
	{
		class Circle {
		  public:
			Circle(){}
			Circle( ci::Vec2f c, float r ) : center(c), radius(r) {}
			ci::Vec2f center;
			float radius;
		};
		Circle getCircleFromThreePoints( const ci::Vec2f &p1, const ci::Vec2f &p2, const ci::Vec2f &p3 );
		
		
		// IsoSphere is based on BloomSphere, created for Kepler
		class IsoSphereEs {
		  public:
			struct VertexData {
				ci::Vec3f vertex;
				ci::Vec3f normal;
				ci::Vec2f texture;
			};            
			
			IsoSphereEs(): mInited(false) {}
			~IsoSphereEs() {
				if( mInited ) glDeleteBuffers(1, &mVbo);
			}
			
			void setup( int res );
			void drawSphereTri( ci::Vec3f va, ci::Vec3f vb, ci::Vec3f vc, int div );
			void draw();
			static void prepare();

			int						mIndex;
			std::vector<uint32_t>	mIndices;
			std::vector<ci::Vec3f>	mPosCoords;
			std::vector<ci::Vec3f>	mNormals;
			std::vector<ci::Vec2f>	mTexCoords;
			
		  private:
			bool	mInited;
			GLuint  mVbo;
			int		mNumVerts; 
		};
	}
}
