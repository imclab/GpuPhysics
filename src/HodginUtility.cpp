//
//  HodginUtility.cpp
//  Sol
//
//  Created by Robert Hodgin on 12/8/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "cinder/CinderMath.h"
#include "cinder/Vector.h"
#include "cinder/Ray.h"
#include "HodginUtility.h"

#define M_4_PI 12.566370614359172

using namespace ci;
using namespace std;

namespace hodgin 
{
	namespace gl
	{
		void drawSphericalBillboard( const Vec3f &camEye, const Vec3f &objPos, const Vec2f &scale, float rotInRadians )
		{	
			glPushMatrix();
			glTranslatef( objPos.x, objPos.y, objPos.z );
			
			Vec3f lookAt = Vec3f::zAxis();
			Vec3f upAux;
			float angleCosine;
			
			Vec3f objToCam = ( camEye - objPos ).normalized();
			Vec3f objToCamProj = Vec3f( objToCam.x, 0.0f, objToCam.z );
			objToCamProj.normalize();
			
			upAux = lookAt.cross( objToCamProj );
			
			// Cylindrical billboarding
			angleCosine = constrain( lookAt.dot( objToCamProj ), -1.0f, 1.0f );
			glRotatef( toDegrees( acos(angleCosine) ), upAux.x, upAux.y, upAux.z );	
			
			// Spherical billboarding
			angleCosine = constrain( objToCamProj.dot( objToCam ), -1.0f, 1.0f );
			if( objToCam.y < 0 )	glRotatef( toDegrees( acos(angleCosine) ), 1.0f, 0.0f, 0.0f );	
			else					glRotatef( toDegrees( acos(angleCosine) ),-1.0f, 0.0f, 0.0f );
			
			
			Vec3f verts[4];
			GLfloat texCoords[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };
			
			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glVertexPointer( 3, GL_FLOAT, 0, &verts[0].x );
			glTexCoordPointer( 2, GL_FLOAT, 0, texCoords );
			
			float sinA = ci::math<float>::sin( rotInRadians );
			float cosA = ci::math<float>::cos( rotInRadians );
			
			float scaleXCosA = 0.5f * scale.x * cosA;
			float scaleXSinA = 0.5f * scale.x * sinA;
			float scaleYSinA = 0.5f * scale.y * sinA;
			float scaleYCosA = 0.5f * scale.y * cosA;
			
			verts[0] = Vec3f( ( -scaleXCosA - scaleYSinA ), ( -scaleXSinA + scaleYCosA ), 0.0f );
			verts[1] = Vec3f( ( -scaleXCosA + scaleYSinA ), ( -scaleXSinA - scaleYCosA ), 0.0f );
			verts[2] = Vec3f( (  scaleXCosA - scaleYSinA ), (  scaleXSinA + scaleYCosA ), 0.0f );
			verts[3] = Vec3f( (  scaleXCosA + scaleYSinA ), (  scaleXSinA - scaleYCosA ), 0.0f );
			
			glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
			
			glDisableClientState( GL_VERTEX_ARRAY );
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
			
			glPopMatrix();
		}
		
	}
	
	namespace math 
	{
	
		float fastSqrt( float number )
		{
			int i;
			float x, y;
			x = number * 0.5f;
			y  = number;
			i  = * (int * ) &y;
			i  = 0x5f3759df - (i >> 1);
			y  = * ( float * ) &i;
			y  = y * (1.5f - (x * y * y));
			y  = y * (1.5f - (x * y * y));
			return number * y;
		}
		
		ci::Vec3f calcSurfaceNormal( const ci::Vec3f &p1, const ci::Vec3f &p2, const ci::Vec3f &p3 )
		{
			ci::Vec3f normal;
			ci::Vec3f u = p1 - p3;
			ci::Vec3f v = p1 - p2;
			
			normal.x = u.y * v.z - u.z * v.y;
			normal.y = u.z * v.x - u.x * v.z;
			normal.z = u.x * v.y - u.y * v.x;
			return normal.normalized();
		}
	}
	
	namespace sim 
	{
		float getRadiusFromMass( float m )
		{
			float r = powf( (3.0f * m )/(float)M_4_PI, 0.3333333f );
			return r;
		}
		
		float getMassFromRadius( float r )
		{
			float m = ( ( r * r * r ) * (float)M_4_PI ) * 0.33333f;
			return m;
		}
		
		float getStarRadiusFromStarMass( float m )
		{
//			float r = powf( ( 3.0f * ( pow( m*0.01f, 2.0f ) + 1.0f ) )/M_4_PI, 0.3333333f );
//			return r;
			return getRadiusFromMass( m );
		}
		
		float getStarMassFromStarRadius( float r )
		{
//			float m = ci::math<float>::max( sqrtf( ( ( r * r * r ) * M_4_PI ) * 0.33333f - 1.0f ), 1.0f ) * 100.0f;
//			return m;
			return getMassFromRadius( r );
		}
		
		bool didParticlesCollide( const ci::Vec3f &dir, const ci::Vec3f &dirNormal, const float dist, const float sumRadii, const float sumRadiiSqrd, ci::Vec3f *moveVec )
		{
			float moveVecLength = sqrtf( moveVec->x * moveVec->x + moveVec->y * moveVec->y + moveVec->z * moveVec->z );
			float newDist = dist - sumRadii;
			
			if( newDist < 0 )
				return true;
			
			if( moveVecLength < dist )
				return false;
			
			moveVec->normalize();
			float D	= moveVec->dot(dir);
			
			if( D <= 0 )
				return false;
			
			float F	= ( newDist * newDist ) - ( D * D );
			
			if( F >= sumRadiiSqrd )
				return false;
			
			float T = sumRadiiSqrd - F;
			
			if( T < 0 )
				return false;
			
			float distance = D - sqrtf(T);
			
			if( moveVecLength < distance )
				return false;
			
			*moveVec *= distance;
//			moveVec->set( moveVec * distance );
//			moveVec->set( moveVec->normalized() * distance );
			
			return true;
		}
	}
	
	namespace geom
	{
		Circle getCircleFromThreePoints( const Vec2f &p1, const Vec2f &p2, const Vec2f &p3 )
		{
			Circle c;
			
			float ax = p1.x;
			float ay = p1.y;
			float bx = p2.x;
			float by = p2.y;
			float cx = p3.x;
			float cy = p3.y;
			
			// Get the perpendicular bisector of (x1, y1) and (x2, y2).
			float x1 = ( bx + ax ) / 2.0f;
			float y1 = ( by + ay ) / 2.0f;
			float dy1 = ( bx - ax );
			float dx1 = -( by - ay );
			
			// Get the perpendicular bisector of (x2, y2) and (x3, y3).
			float x2 = ( cx + bx ) / 2.0f;
			float y2 = ( cy + by ) / 2.0f;
			float dy2 = cx - bx;
			float dx2 = -( cy - by );
			
			float ox = (y1 * dx1 * dx2 + x2 * dx1 * dy2 - x1 * dy1 * dx2 - y2 * dx1 * dx2) / (dx1 * dy2 - dy1 * dx2);
			float oy = ( (ox - x1) * dy1 ) / dx1 + y1;
			
			if( ox != ox ){
				std::cout << "OX NAN ERROR!!!!!!!!!!!" << std::endl;
				ox = 0.001f;
			}
			
			if( oy != oy ){
				std::cout << "OY NAN ERROR!!!!!!!!!!!" << std::endl;
				oy = 0.001f;
			}
			
			float dx = ox - ax;
			float dy = oy - ay;
			float r = sqrt( dx * dx + dy * dy );
			
			c.center = Vec2f( ox, oy );
			c.radius = r;
			
			return c;
		}
		
		void IsoSphereEs::setup( int res )
		{	
			if (mInited) {
				glDeleteBuffers(1, &mVbo);
			}
			
			float X = 0.525731112119f; 
			float Z = 0.850650808352f;
			
			static Vec3f isoVerts[12] = {
				Vec3f( -X, 0.0f, Z ), Vec3f( X, 0.0f, Z ), Vec3f( -X, 0.0f, -Z ), Vec3f( X, 0.0f, -Z ),
				Vec3f( 0.0f, Z, X ), Vec3f( 0.0f, Z, -X ), Vec3f( 0.0f, -Z, X ), Vec3f( 0.0f, -Z, -X ),
				Vec3f( Z, X, 0.0f ), Vec3f( -Z, X, 0.0f ), Vec3f( Z, -X, 0.0f ), Vec3f( -Z, -X, 0.0f ) };
			
			static GLuint triIndices[20][3] = { 
				{0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1}, {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
				{7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };
			
			
			mIndex = 0;
			mPosCoords.clear();
			mNormals.clear();
			mIndices.clear();
			mTexCoords.clear();
			for( int i=0; i<20; i++ ){
				drawSphereTri( isoVerts[triIndices[i][0]], isoVerts[triIndices[i][1]], isoVerts[triIndices[i][2]], res );
			}
			
			mNumVerts = mIndices.size();
			VertexData *verts = new VertexData[ mNumVerts ];
			
			for( int i=0; i<mNumVerts; i++ ){
				verts[i].vertex = mPosCoords[i];
				verts[i].normal = mNormals[i];
				verts[i].texture = mTexCoords[i];
			}

			// do VBO (there are more complex ways, let's try this first)
			// (other things to try include VAOs, with glGenVertexArraysOES?)
			glGenBuffers(1, &mVbo);
			glBindBuffer(GL_ARRAY_BUFFER, mVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * mNumVerts, verts, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0); // Leave no VBO bound.        
			
			delete[] verts;
			
			if( !mInited ){
				glBindBuffer(GL_ARRAY_BUFFER, mVbo );
				glVertexPointer( 3, GL_FLOAT, sizeof(VertexData), 0 ); // last arg becomes an offset instead of an address
				glNormalPointer( GL_FLOAT, sizeof(VertexData), (GLvoid*)sizeof(Vec3f) );
				glTexCoordPointer( 2, GL_FLOAT, sizeof(VertexData), (GLvoid*)( sizeof(Vec3f) * 2 ) );        
				glBindBuffer(GL_ARRAY_BUFFER, 0); // Leave no VBO bound.   
			}
			
			mInited = true;
		}
		
		void IsoSphereEs::drawSphereTri( Vec3f va, Vec3f vb, Vec3f vc, int div )
		{
			if( div <= 0 ){
				mPosCoords.push_back( va );
				mPosCoords.push_back( vb );
				mPosCoords.push_back( vc );
				mIndices.push_back( mIndex++ );
				mIndices.push_back( mIndex++ );
				mIndices.push_back( mIndex++ );
				Vec3f vn = ( va + vb + vc ) * 0.3333f;
				mNormals.push_back( va );
				mNormals.push_back( vb );
				mNormals.push_back( vc );
				
				float u1 = ( atan( va.y/(va.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
				float u2 = ( atan( vb.y/(vb.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
				float u3 = ( atan( vc.y/(vc.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
				if( va.x < 0.0f || vb.x < 0.0f || vc.x < 0.0f ){
					u1 = ( atan( va.y/abs(va.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
					u2 = ( atan( vb.y/abs(vb.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
					u3 = ( atan( vc.y/abs(vc.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
				}
				
				float v1 = acos( va.z ) / (float)M_PI;
				float v2 = acos( vb.z ) / (float)M_PI;
				float v3 = acos( vc.z ) / (float)M_PI;

				mTexCoords.push_back( Vec2f( u1, v1 ) );
				mTexCoords.push_back( Vec2f( u2, v2 ) );
				mTexCoords.push_back( Vec2f( u3, v3 ) );
			} else {
				Vec3f vab = ( ( va + vb ) * 0.5f ).normalized();
				Vec3f vac = ( ( va + vc ) * 0.5f ).normalized();
				Vec3f vbc = ( ( vb + vc ) * 0.5f ).normalized();
				drawSphereTri( va, vab, vac, div-1 );
				drawSphereTri( vb, vbc, vab, div-1 );
				drawSphereTri( vc, vac, vbc, div-1 );
				drawSphereTri( vab, vbc, vac, div-1 );
			}
		}

		void IsoSphereEs::draw()
		{
			glDrawArrays( GL_TRIANGLES, 0, mNumVerts ); 
		}
	}
} 