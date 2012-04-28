#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define APP_WIDTH		1280
#define APP_HEIGHT		720

#define FBO_WIDTH		5
#define FBO_HEIGHT		5

class GpuPhysicsApp : public AppBasic {
  public:
	void prepareSettings( Settings *settings );
	void setup();
	void setInitPositions( gl::Fbo &fbo );
	void setInitVelocities( gl::Fbo &fbo );
	void createSphere( gl::VboMesh &mesh, int res );
	void drawSphereTri( Vec3f va, Vec3f vb, Vec3f vc, int div, Color c );
	void keyDown( KeyEvent event );
	void update();
	void drawIntoVelocityFbo();
	void drawIntoPositionFbo();
	void draw();

	// TIME
	float			mTime, mTimePrev;
	float			mTimeDelta;

	// CAMERA
	CameraPersp		mCam;
	
	// SHADERS
	gl::GlslProg	mPosInitShader;		// Used to initialize the Position FBO data
	gl::GlslProg	mVelInitShader;		// Used to initialize the Velocity FBO data
	gl::GlslProg	mVelocityShader;	// Used to calculate the new velocities
	gl::GlslProg	mPositionShader;	// Used to calculate the new positions
	gl::GlslProg	mShader;			// Draws particles to screen

	// SPHERES
	gl::VboMesh			mSphereVbo;		// Stores the data for all the spheres
	std::vector<Vec3f>	mPosCoords;		// Vertex positions
	std::vector<Vec3f>	mNormals;		// Vertex normals
	std::vector<Vec2f>	mTexCoords;		// Sphere texture coords, not currently being used
	std::vector<Colorf>	mColors;		// Holds the U,V data for where to access FBO data
	
	// POSITION/VELOCITY FBOS
	ci::Vec2f		mFboSize;
	ci::Area		mFboBounds;
	gl::Fbo			mPositionFbos[2];
	gl::Fbo			mVelocityFbos[2];
	int				mThisFbo, mPrevFbo;
	
	bool			mIsGravityOn;
	float			mGravity;
	
	float			mCharge;
};

void GpuPhysicsApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
}

void GpuPhysicsApp::setup()
{
	// SPHERES VBO
	createSphere( mSphereVbo, 3 );
	
	// TIME
	mTimePrev	= (float)getElapsedSeconds();
	mTime		= (float)getElapsedSeconds();
	mTimeDelta	= mTime - mTimePrev;
	
	// CAMERA	
	mCam.setPerspective( 65.0f, getWindowAspectRatio(), 5.0f, 3000.0f );
	mCam.lookAt( Vec3f( 0.0f, 0.0f, -200.0f ), Vec3f::zero(), Vec3f::yAxis() );
	
	// SHADERS
	try {
		mVelocityShader = gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "Velocity.frag" ) );
		mPositionShader	= gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "Position.frag" ) );
		mShader			= gl::GlslProg( loadResource( "final.vert" ), loadResource( "final.frag" ) );
		mPosInitShader	= gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "fboPosInit.frag" ) );
		mVelInitShader	= gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "fboVelInit.frag" ) );
	} catch( gl::GlslProgCompileExc e ) {
		std::cout << e.what() << std::endl;
		quit();
	}
	
	// POSITION/VELOCITY FBOS
	gl::Fbo::Format format;
	format.enableColorBuffer( true, 2 );
	format.setColorInternalFormat( GL_RGBA16F_ARB );
	format.setMinFilter( GL_NEAREST );
	format.setMagFilter( GL_NEAREST );
	mFboSize			= Vec2f( FBO_WIDTH, FBO_HEIGHT );
	mFboBounds			= Area( 0, 0, FBO_WIDTH, FBO_HEIGHT );
	mPositionFbos[0]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mPositionFbos[1]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mVelocityFbos[0]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mVelocityFbos[1]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	
	mThisFbo		= 0;
	mPrevFbo		= 1;

	mIsGravityOn	= false;
	mGravity		= 0.0f;
	mCharge			= 1.0f;
	
	// Initialize the position and velocity fbos
	setInitPositions( mPositionFbos[0] );
	setInitPositions( mPositionFbos[1] );
	setInitVelocities( mVelocityFbos[0] );
	setInitVelocities( mVelocityFbos[1] );
}

// createSphere and drawSphereTri creates a recursive subdivided icosahedron
void GpuPhysicsApp::createSphere( gl::VboMesh &vbo, int res )
{
	float X = 0.525731112119f; 
	float Z = 0.850650808352f;
	
	static Vec3f verts[12] = {
		Vec3f( -X, 0.0f, Z ), Vec3f( X, 0.0f, Z ), Vec3f( -X, 0.0f, -Z ), Vec3f( X, 0.0f, -Z ),
		Vec3f( 0.0f, Z, X ), Vec3f( 0.0f, Z, -X ), Vec3f( 0.0f, -Z, X ), Vec3f( 0.0f, -Z, -X ),
		Vec3f( Z, X, 0.0f ), Vec3f( -Z, X, 0.0f ), Vec3f( Z, -X, 0.0f ), Vec3f( -Z, -X, 0.0f ) };
	
	static GLuint triIndices[20][3] = { 
		{0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1}, {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
		{7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };
	
	mPosCoords.clear();
	mNormals.clear();
	mTexCoords.clear();
	mColors.clear();
	
	// Creates a unit sphere located at the origin for each fbo pixel.
	// The verts get moved to the desired location in the final.vert shader.
	float invWidth = 1.0f/(float)FBO_WIDTH;
	float invHeight = 1.0f/(float)FBO_HEIGHT;
	for( int x = 0; x < FBO_WIDTH; ++x ) {
		for( int y = 0; y < FBO_HEIGHT; ++y ) {
			float u = ( (float)x + 0.5f ) * invWidth;
			float v = ( (float)y + 0.5f ) * invHeight;
			Colorf c = Colorf( u, v, 0.0f );
			
			for( int i=0; i<20; i++ ){
				drawSphereTri( verts[triIndices[i][0]], verts[triIndices[i][1]], verts[triIndices[i][2]], res, c );
			}
		}
	}
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticNormals();
	layout.setStaticTexCoords2d();
	layout.setStaticColorsRGB();
	
	vbo = gl::VboMesh( mPosCoords.size(), 0, layout, GL_TRIANGLES );	
	vbo.bufferPositions( mPosCoords );
	vbo.bufferNormals( mNormals );
	vbo.bufferTexCoords2d( 0, mTexCoords );
	vbo.bufferColorsRGB( mColors );
	vbo.unbindBuffers();
}

void GpuPhysicsApp::drawSphereTri( Vec3f va, Vec3f vb, Vec3f vc, int div, Color c )
{
	if( div <= 0 ){
		mColors.push_back( c );
		mColors.push_back( c );
		mColors.push_back( c );
		mPosCoords.push_back( va );
		mPosCoords.push_back( vb );
		mPosCoords.push_back( vc );
		Vec3f vn = ( va + vb + vc ) * 0.3333f;
		mNormals.push_back( vn );
		mNormals.push_back( vn );
		mNormals.push_back( vn );
		
		float u1 = ( (float)atan( va.y/(va.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
		float u2 = ( (float)atan( vb.y/(vb.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
		float u3 = ( (float)atan( vc.y/(vc.x+0.0001f) ) + (float)M_PI * 0.5f ) / (float)M_PI;
		
		if( va.x <= 0.0f ){
			u1 = u1 * 0.5f + 0.5f;
		} else {
			u1 *= 0.5f;
		}
		
		if( vb.x <= 0.0f ){
			u2 = u2 * 0.5f + 0.5f;
		} else {
			u2 *= 0.5f;
		}
		
		if( vc.x <= 0.0f ){
			u3 = u3 * 0.5f + 0.5f;
		} else {
			u3 *= 0.5f;
		}
		
		
		float v1 = (float)acos( va.z ) / (float)M_PI;
		float v2 = (float)acos( vb.z ) / (float)M_PI;
		float v3 = (float)acos( vc.z ) / (float)M_PI;
		
		mTexCoords.push_back( Vec2f( u1, v1 ) );
		mTexCoords.push_back( Vec2f( u2, v2 ) );
		mTexCoords.push_back( Vec2f( u3, v3 ) );
	} else {
		Vec3f vab = ( ( va + vb ) * 0.5f ).normalized();
		Vec3f vac = ( ( va + vc ) * 0.5f ).normalized();
		Vec3f vbc = ( ( vb + vc ) * 0.5f ).normalized();
		drawSphereTri( va, vab, vac, div-1, c );
		drawSphereTri( vb, vbc, vab, div-1, c );
		drawSphereTri( vc, vac, vbc, div-1, c );
		drawSphereTri( vab, vbc, vac, div-1, c );
	}
}

void GpuPhysicsApp::setInitPositions( gl::Fbo &fbo )
{
	// Give each pixel on the surface a randomized rgb value
	// which will represent the particle position.
	// Store the mass of the particle as the alpha value.
	Surface32f posSurface( fbo.getTexture() );
	Surface32f::Iter it = posSurface.getIter();
	while( it.line() ){
		while( it.pixel() ){
			Vec3f initPos	= Rand::randVec3f() * 50.0f;
			float initMass	= 2000.0f;
			it.r() = initPos.x;
			it.g() = initPos.y;
			it.b() = initPos.z;
			it.a() = initMass;
		}
	}
	
	
	gl::Texture posTexture( posSurface );
	posTexture.bind();
	
	// Store the positions and masses in the fbo
	fbo.bindFramebuffer();
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	mPosInitShader.bind();
	mPosInitShader.uniform( "initTex", 0 );
	gl::drawSolidRect( mFboBounds );
	mPosInitShader.unbind();
	fbo.unbindFramebuffer();
}

void GpuPhysicsApp::setInitVelocities( gl::Fbo &fbo )
{
	// Give each pixel on the surface a zero value
	// which will represent the initial velocity of each particle.
	// The alpha value will be used to show if a collision is happening
	// but for now, also set it to zero.
	Surface32f velSurface( fbo.getTexture() );
	Surface32f::Iter it = velSurface.getIter();
	while( it.line() ){
		while( it.pixel() ){
			it.r() = 0.0f;
			it.g() = 0.0f;
			it.b() = 0.0f;
			it.a() = 0.0f;
		}
	}
	
	gl::Texture velTexture( velSurface );
	velTexture.bind();
	
	// Store the velocities in the fbo
	fbo.bindFramebuffer();
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	mVelInitShader.bind();
	mVelInitShader.uniform( "initTex", 0 );
	gl::drawSolidRect( mFboBounds );
	mVelInitShader.unbind();
	fbo.unbindFramebuffer();
}

void GpuPhysicsApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'g' ){
		mIsGravityOn = !mIsGravityOn;
	} else if( event.getChar() == 'c' ){
		mCharge *= -1.0f;
	}
}

void GpuPhysicsApp::update()
{
	mTimePrev		= mTime;
	mTime			= (float)app::getElapsedSeconds();
	mTimeDelta		= ( mTime - mTimePrev ) * 60.0f;
	
	if( mIsGravityOn ) mGravity -= ( mGravity + 0.02f ) * 0.2f;
	else			   mGravity -= ( mGravity - 0.0f ) * 0.2f;

	gl::disableAlphaBlending();
	gl::disableDepthRead();
	gl::disableDepthWrite();
	drawIntoVelocityFbo();
	drawIntoPositionFbo();
}



void GpuPhysicsApp::drawIntoVelocityFbo()
{
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	
	mVelocityFbos[ mThisFbo ].bindFramebuffer();	// Bind the current velocity fbo
	mPositionFbos[ mPrevFbo ].bindTexture( 0, 0 );	// Bind the 0 target of the old position fbo
	mVelocityFbos[ mPrevFbo ].bindTexture( 1, 0 );	// Bind the 0 target of the old velocity fbo

	mVelocityShader.bind();
	mVelocityShader.uniform( "position", 0 );
	mVelocityShader.uniform( "velocity", 1 );
	mVelocityShader.uniform( "fboWidth", FBO_WIDTH );
	mVelocityShader.uniform( "fboHeight", FBO_HEIGHT );
	mVelocityShader.uniform( "invWidth", 1.0f/(float)FBO_WIDTH );
	mVelocityShader.uniform( "invHeight", 1.0f/(float)FBO_HEIGHT );
	mVelocityShader.uniform( "dt", mTimeDelta );
	mVelocityShader.uniform( "gravity", mGravity );
	mVelocityShader.uniform( "charge", mCharge );
	gl::drawSolidRect( mFboBounds );
	mVelocityShader.unbind();
	
	mVelocityFbos[ mThisFbo ].unbindFramebuffer();
}

void GpuPhysicsApp::drawIntoPositionFbo()
{	
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	
	mPositionFbos[ mThisFbo ].bindFramebuffer();	// Bind the current position fbo
	mPositionFbos[ mPrevFbo ].bindTexture( 0, 0 );	// Bind the 0 target of the old position fbo
	mPositionFbos[ mPrevFbo ].bindTexture( 1, 1 );	// Bind the 1 target of the old position fbo
	mVelocityFbos[ mThisFbo ].bindTexture( 2, 0 );	// Bind the 0 target of the new velocity fbo
	mVelocityFbos[ mThisFbo ].bindTexture( 3, 1 );	// Bind the 1 target of the new velocity fbo
	mPositionShader.bind();
	mPositionShader.uniform( "pos0Tex", 0 );
	mPositionShader.uniform( "pos1Tex", 1 );
	mPositionShader.uniform( "vel0Tex", 2 );
	mPositionShader.uniform( "vel1Tex", 3 );
	mPositionShader.uniform( "dt", mTimeDelta );
	gl::drawSolidRect( mFboBounds );
	mPositionShader.unbind();
	
	mPositionFbos[ mThisFbo ].unbindFramebuffer();
}


void GpuPhysicsApp::draw()
{
	gl::clear( ColorA( 0.5f, 0.5f, 0.5f, 0.0f ), true );
	
	gl::setMatrices( mCam );
	gl::setViewport( getWindowBounds() );
	
	gl::enable( GL_TEXTURE_2D );
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::enableAlphaBlending();
	
	gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
	
	// DRAW PARTICLES
	mPositionFbos[mThisFbo].bindTexture( 0 );
	mVelocityFbos[mThisFbo].bindTexture( 1 );
	mShader.bind();
	mShader.uniform( "positions", 0 );
	mShader.uniform( "velocities", 1 );
	gl::draw( mSphereVbo );
	mShader.unbind();
	
	if( getElapsedFrames()%60 == 0 ){
		std::cout << "FPS = " << getAverageFps() << std::endl;
	}
	
	// Ping-pong the Fbo index
	mThisFbo	= ( mThisFbo + 1 ) % 2;
	mPrevFbo	= ( mThisFbo + 1 ) % 2;
}




CINDER_APP_BASIC( GpuPhysicsApp, RendererGl )
