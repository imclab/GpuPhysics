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
#include "Room.h"
#include "SpringCam.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define APP_WIDTH		1280
#define APP_HEIGHT		720
#define FBO_WIDTH		264	// THIS
#define FBO_HEIGHT		18	// TIME THIS IS THE NUMBER OF FLOCKING AGENTS
#define ROOM_FBO_RES	4

class FlockingApp : public AppBasic {
  public:
	void prepareSettings( Settings *settings );
	void setup();
	void setFboPositions( gl::Fbo &fbo );
	void setFboVelocities( gl::Fbo &fbo );
	void initVbo();
	void mouseDown( MouseEvent event );
	void mouseUp( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void keyDown( KeyEvent event );
	void updateTime();
	void update();
	void drawIntoRoomFbo();
	void drawIntoVelocityFbo();
	void drawIntoPositionFbo();
	void draw();
	
	// PARAMS
	params::InterfaceGl	mParams;

	// TIME
	float			mTime, mTimePrev;
	float			mTimer;
	float			mTimeDelta;
	float			mTimeAdjusted;
	float			mTimeMulti;
	
	// CAMERA
	SpringCam		mSpringCam;
	
	// SHADERS
	gl::GlslProg	mVelocityShader;
	gl::GlslProg	mPositionShader;
	gl::GlslProg	mRoomShader;
	gl::GlslProg	mShader;
	
	// TEXTURES
	gl::Texture		mRoomPanelTex;
	
	// ROOM
	Room			mRoom;
	gl::Fbo			mRoomFbo;
	ci::Vec2f		mRoomFboSize;
	ci::Area		mRoomFboBounds;
	float			mXBounds, mYBounds, mZBounds;
	Rectf			mRoomPanelRect;
	
	
	
	gl::VboMesh		mVboMesh;
	
	// POSITION/VELOCITY FBOS
	ci::Vec2f		mFboSize;
	ci::Area		mFboBounds;
	gl::Fbo			mPositionFbos[2];
	gl::Fbo			mVelocityFbos[2];
	int				mThisFbo, mPrevFbo;
	
	float			mMainPower;
	bool			mIsPowerOn;
	bool			mShowParams;
	float			mMinThresh, mMaxThresh;
	float			mZoneRadius;
	float			mMinSpeed, mMaxSpeed;
	
	Vec2f			mMousePos;
	float			mMousePressed;
	
	bool			mSaveFrames;
	int				mNumFrames;
};

void FlockingApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
}

void FlockingApp::setup()
{
	// TIME
	mTimePrev	= (float)getElapsedSeconds();
	mTime		= (float)getElapsedSeconds();
	mTimeDelta	= mTime - mTimePrev;
	mTimeMulti	= 120.0f;
	
	// CAMERA	
	mSpringCam		= SpringCam( -188.0f, getWindowAspectRatio() );
	
	
	// POSITION/VELOCITY FBOS
	gl::Fbo::Format format;
	format.setColorInternalFormat( GL_RGBA16F_ARB );
	format.setMinFilter( GL_NEAREST );
	format.setMagFilter( GL_NEAREST );
	mFboSize		= Vec2f( FBO_WIDTH, FBO_HEIGHT );
	mFboBounds		= Area( 0, 0, FBO_WIDTH, FBO_HEIGHT );
	mPositionFbos[0]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mPositionFbos[1]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mVelocityFbos[0]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mVelocityFbos[1]	= gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mThisFbo	= 0;
	mPrevFbo	= 1;
	initVbo();
	
	// SHADERS
	try {
		mVelocityShader = gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "Velocity.frag" ) );
		mPositionShader	= gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "Position.frag" ) );
		mShader			= gl::GlslProg( loadResource( "VboPos.vert" ), loadResource( "VboPos.frag" ) );
		mRoomShader		= gl::GlslProg( loadResource( "room.vert" ), loadResource( "room.frag" ) );
	} catch( gl::GlslProgCompileExc e ) {
		std::cout << e.what() << std::endl;
		quit();
	}
	
	// TEXTURES
	mRoomPanelTex	= gl::Texture( loadImage( loadResource( "roomPanel.png" ) ) );
	
	// ROOM
	gl::Fbo::Format roomFormat;
	format.setColorInternalFormat( GL_RGB );
	int fboXRes		= APP_WIDTH/ROOM_FBO_RES;
	int fboYRes		= APP_HEIGHT/ROOM_FBO_RES;
	mRoomFboSize	= Vec2f( fboXRes, fboYRes );
	mRoomFboBounds	= Area( 0, 0, fboXRes, fboYRes );
	mRoomFbo		= gl::Fbo( fboXRes, fboYRes, roomFormat );
	mRoomPanelRect	= Rectf( 32.0f, 35.0f, 32.0f + mRoomPanelTex.getWidth(), 35.0f - mRoomPanelTex.getHeight() );
	mXBounds		= 155.0f;//85.0f;
	mYBounds		= 88.0f;//47.0f;
	mZBounds		= 50.0f;//25.0f;
	mRoom			= Room( mXBounds, mYBounds, mZBounds );	
	
	mShowParams = false;
	mMinThresh	= 0.68f;
	mMaxThresh	= 0.89f;
	mZoneRadius = 6.5f;
	mMinSpeed	= 0.2f;
	mMaxSpeed	= 0.5f;
	
	mMainPower	= 0.0f;
	mIsPowerOn	= false;
	
	mMousePos	= Vec2f::zero();
	mMousePressed = 0.0f;

	
	setFboPositions( mPositionFbos[0] );
	setFboPositions( mPositionFbos[1] );
	setFboVelocities( mVelocityFbos[0] );
	setFboVelocities( mVelocityFbos[1] );

	mParams = params::InterfaceGl( "Flocking", Vec2i( 200, 150 ) );
	mParams.addParam( "Time Multi", &mTimeMulti, "min=0.0 max=180.0 step=1.0 keyIncr=t keyDecr=T" );
	mParams.addParam( "Zone Radius", &mZoneRadius, "min=2.0 max=8.0 step=0.1 keyIncr=z keyDecr=Z" );
	mParams.addParam( "Min Thresh", &mMinThresh, "min=0.025 max=1.0 step=0.01 keyIncr=l keyDecr=L" );
	mParams.addParam( "Max Thresh", &mMaxThresh, "min=0.025 max=1.0 step=0.01 keyIncr=h keyDecr=H" );
	mParams.addParam( "Min Speed", &mMinSpeed, "min=0.1 max=1.0 step=0.025" );
	mParams.addParam( "Max Speed", &mMaxSpeed, "min=0.5 max=4.0 step=0.025" );
	
	mSaveFrames	= false;
	mNumFrames	= 0;
	
	
	mRoom.init();
}

void FlockingApp::initVbo()
{
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
//	layout.setStaticIndices();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();
	
	int numVertices = FBO_WIDTH * FBO_HEIGHT;
	// 5 points make up the pyramid
	// 8 triangles make up two pyramids
	// 3 points per triangle
	mVboMesh		= gl::VboMesh( numVertices * 8 * 3, 0, layout, GL_TRIANGLES );
	
	float s = 0.5f;
	Vec3f p0( 0.0f, 0.0f, 1.3f );
	Vec3f p1( -s, -s, 0.0f );
	Vec3f p2( -s,  s, 0.0f );
	Vec3f p3(  s,  s, 0.0f );
	Vec3f p4(  s, -s, 0.0f );
	Vec3f p5( 0.0f, 0.0f, -0.75f );
	
	Vec3f n;
	Vec3f n0 = Vec3f( 0.0f, 0.0f, 1.0f );
	Vec3f n1 = Vec3f(-1.0f,-1.0f, 0.0f ).normalized();
	Vec3f n2 = Vec3f(-1.0f, 1.0f, 0.0f ).normalized();
	Vec3f n3 = Vec3f( 1.0f, 1.0f, 0.0f ).normalized();
	Vec3f n4 = Vec3f( 1.0f,-1.0f, 0.0f ).normalized();
	Vec3f n5 = Vec3f( 0.0f, 0.0f,-1.0f );
	
	vector<Vec3f>		positions;
	vector<Vec3f>		normals;
	vector<Vec2f>		texCoords;
	
	for( int x = 0; x < FBO_WIDTH; ++x ) {
		for( int y = 0; y < FBO_HEIGHT; ++y ) {
			float u = (float)x/(float)FBO_WIDTH;
			float v = (float)y/(float)FBO_HEIGHT;
			Vec2f t = Vec2f( u, v );

			positions.push_back( p0 );
			positions.push_back( p1 );
			positions.push_back( p2 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p0 + p1 + p2 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			positions.push_back( p0 );
			positions.push_back( p2 );
			positions.push_back( p3 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p0 + p2 + p3 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			positions.push_back( p0 );
			positions.push_back( p3 );
			positions.push_back( p4 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p0 + p3 + p4 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			positions.push_back( p0 );
			positions.push_back( p4 );
			positions.push_back( p1 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p0 + p4 + p1 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			
			
			
			positions.push_back( p5 );
			positions.push_back( p1 );
			positions.push_back( p4 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p5 + p1 + p4 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			positions.push_back( p5 );
			positions.push_back( p2 );
			positions.push_back( p1 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p5 + p2 + p1 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			positions.push_back( p5 );
			positions.push_back( p3 );
			positions.push_back( p2 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p5 + p3 + p2 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			
			positions.push_back( p5 );
			positions.push_back( p4 );
			positions.push_back( p3 );
			texCoords.push_back( t );
			texCoords.push_back( t );
			texCoords.push_back( t );
			n = ( p5 + p4 + p3 ).normalized();
			normals.push_back( n );
			normals.push_back( n );
			normals.push_back( n );
			

		}
	}
	
	mVboMesh.bufferPositions( positions );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
	mVboMesh.bufferNormals( normals );
	mVboMesh.unbindBuffers();
}

void FlockingApp::setFboPositions( gl::Fbo &fbo )
{
	Surface32f vel( fbo.getTexture() );
	Surface32f::Iter vIter = vel.getIter();
	while( vIter.line() ){
		while( vIter.pixel() ){
			Vec3f r = Vec3f( Rand::randFloat( -mRoom.getBounds().x, mRoom.getBounds().x ), 
							 Rand::randFloat( -mRoom.getBounds().y, 0.0f ),
							 Rand::randFloat( -mRoom.getBounds().z, mRoom.getBounds().z ) ) * 0.9f;
			vIter.r() = r.x;
			vIter.g() = r.y;
			vIter.b() = r.z;
			vIter.a() = Rand::randFloat( 0.7, 1.0 );
		}
	}
	
	gl::Texture velTexture( vel );
	fbo.bindFramebuffer();
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	gl::draw( velTexture );
	fbo.unbindFramebuffer();
}

void FlockingApp::setFboVelocities( gl::Fbo &fbo )
{
	Surface32f vel( fbo.getTexture() );
	Surface32f::Iter vIter = vel.getIter();
	while( vIter.line() ){
		while( vIter.pixel() ){
			Vec3f r = Rand::randVec3f();
			vIter.r() = r.x;
			vIter.g() = r.y;
			vIter.b() = r.z;
			vIter.a() = 1.0f;
		}
	}
	
	gl::Texture velTexture( vel );
	fbo.bindFramebuffer();
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	gl::draw( velTexture );
	fbo.unbindFramebuffer();
}

void FlockingApp::mouseDown( MouseEvent event )
{
	mMousePressed = 1.0f;
}

void FlockingApp::mouseUp( MouseEvent event )
{
	mMousePressed = 0.0f;
}

void FlockingApp::mouseDrag( MouseEvent event )
{
	mMousePos = event.getPos();
}

void FlockingApp::keyDown( KeyEvent event )
{
	if( event.getChar() == '/' ){
		mShowParams = !mShowParams;
	} else if( event.getChar() == ' ' ){
		mSaveFrames = !mSaveFrames;
	} else if( event.getChar() == 'p' ){
		mIsPowerOn = !mIsPowerOn;
	}
}

void FlockingApp::updateTime()
{
	mTimePrev		= mTime;
	mTime			= (float)app::getElapsedSeconds();
	mTimeDelta		= mTime - mTimePrev;
	
	// ONLY FOR SAVING VIDEO
	//	mTimeDelta		= 1.0f/60.0f;
	//
	
	mTimeAdjusted	= mTimeDelta * mTimeMulti;
	mTimer			+= mTimeAdjusted;
}

void FlockingApp::update()
{
	updateTime();
	
	if( mIsPowerOn ) mMainPower -= ( mMainPower - 1.0f ) * 0.2f;
	else			 mMainPower -= ( mMainPower - 0.0f ) * 0.2f;
	
	mRoom.update( mXBounds, mYBounds, mZBounds );
	
	mSpringCam.update( mTimeAdjusted );

	
	drawIntoVelocityFbo();
	drawIntoPositionFbo();
	drawIntoRoomFbo();
}



void FlockingApp::drawIntoVelocityFbo()
{
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	gl::disableAlphaBlending();
	
	mVelocityFbos[ mThisFbo ].bindFramebuffer();
	gl::clear( ColorA( 0, 0, 0, 0 ) );
	
	mPositionFbos[ mPrevFbo ].bindTexture( 0 );
	mVelocityFbos[ mPrevFbo ].bindTexture( 1 );
	
	mVelocityShader.bind();
	mVelocityShader.uniform( "position", 0 );
	mVelocityShader.uniform( "velocity", 1 );
	mVelocityShader.uniform( "mousePos", ( mMousePos - getWindowCenter() ) / getWindowCenter() );
	mVelocityShader.uniform( "mousePressed", mMousePressed );
	mVelocityShader.uniform( "minThresh", mMinThresh );
	mVelocityShader.uniform( "maxThresh", mMaxThresh );
	mVelocityShader.uniform( "zoneRadius", mZoneRadius );
	mVelocityShader.uniform( "zoneRadiusSqrd", mZoneRadius * mZoneRadius );
	mVelocityShader.uniform( "minSpeed", mMinSpeed );
	mVelocityShader.uniform( "maxSpeed", mMaxSpeed );
	mVelocityShader.uniform( "roomBounds", mRoom.getBounds() );
	mVelocityShader.uniform( "w", FBO_WIDTH );
	mVelocityShader.uniform( "h", FBO_HEIGHT );
	mVelocityShader.uniform( "invWidth", 1.0f/(float)FBO_WIDTH );
	mVelocityShader.uniform( "invHeight", 1.0f/(float)FBO_HEIGHT );
	mVelocityShader.uniform( "timeDelta", mTimeAdjusted );
	mVelocityShader.uniform( "mainPower", mMainPower );
	gl::drawSolidRect( mFboBounds );
	mVelocityShader.unbind();
	
	mVelocityFbos[ mThisFbo ].unbindFramebuffer();
}

void FlockingApp::drawIntoPositionFbo()
{	
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	
	mPositionFbos[ mThisFbo ].bindFramebuffer();
	mPositionFbos[ mPrevFbo ].bindTexture( 0 );
	mVelocityFbos[ mThisFbo ].bindTexture( 1 );
	
	mPositionShader.bind();
	mPositionShader.uniform( "position", 0 );
	mPositionShader.uniform( "velocity", 1 );
	mPositionShader.uniform( "timeDelta", mTimeAdjusted );
	gl::drawSolidRect( mFboBounds );
	mPositionShader.unbind();
	
	mPositionFbos[ mThisFbo ].unbindFramebuffer();
}

void FlockingApp::drawIntoRoomFbo()
{
	mRoomFbo.bindFramebuffer();
	gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ), true );
	
	gl::setMatricesWindow( mRoomFboSize, false );
	gl::setViewport( mRoomFboBounds );
	gl::disableAlphaBlending();
	gl::enable( GL_TEXTURE_2D );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	Matrix44f m;
	m.setToIdentity();
	m.scale( mRoom.getBounds() );
	
	//	mLightsTex.bind( 0 );
	mRoomShader.bind();
	//	mRoomShader.uniform( "lightsTex", 0 );
	//	mRoomShader.uniform( "numLights", (float)mNumLights );
	//	mRoomShader.uniform( "invNumLights", mInvNumLights );
	//	mRoomShader.uniform( "invNumLightsHalf", mInvNumLights * 0.5f );
	//	mRoomShader.uniform( "att", 1.015f );
	mRoomShader.uniform( "mvpMatrix", mSpringCam.mMvpMatrix );
	mRoomShader.uniform( "mMatrix", m );
	mRoomShader.uniform( "eyePos", mSpringCam.mEye );
	mRoomShader.uniform( "roomDim", mRoom.getBounds() );
	mRoomShader.uniform( "mainPower", mMainPower );
	mRoom.draw();
	mRoomShader.unbind();
	
	mRoomFbo.unbindFramebuffer();
	glDisable( GL_CULL_FACE );
}

void FlockingApp::draw()
{
	// clear out the window with black
	gl::clear( ColorA( 0.1f, 0.1f, 0.1f, 0.0f ), true );
	
	gl::setMatricesWindow( getWindowSize(), false );
	gl::setViewport( getWindowBounds() );

	gl::disableDepthRead();
	gl::disableDepthWrite();
	gl::enable( GL_TEXTURE_2D );
	gl::enableAlphaBlending();
	
	gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
	
	// DRAW ROOM
	mRoomFbo.bindTexture();
	gl::drawSolidRect( getWindowBounds() );
	
	gl::enableDepthRead();
	gl::setMatrices( mSpringCam.getCam() );
	
	mRoomPanelTex.bind();
	gl::color( ColorA( mMainPower, mMainPower, mMainPower, mMainPower * 0.1f + 0.9f ) );
	gl::drawBillboard( Vec3f( 64.0f, -54.0f, mRoom.getBounds().z ), mRoomPanelTex.getSize() * 0.3f, 0.0f, -Vec3f::xAxis(), Vec3f::yAxis() );
	
	gl::enableDepthWrite();
	
	// DRAW PARTICLES
	mPositionFbos[mPrevFbo].bindTexture( 0 );
	mPositionFbos[mThisFbo].bindTexture( 1 );
	mVelocityFbos[mThisFbo].bindTexture( 2 );
	mShader.bind();
	mShader.uniform( "prevPosition", 0 );
	mShader.uniform( "currentPosition", 1 );
	mShader.uniform( "currentVelocity", 2 );
	mShader.uniform( "eyePos", mSpringCam.mEye );
	mShader.uniform( "mainPower", mMainPower );
	gl::draw( mVboMesh );
	mShader.unbind();
	
	gl::disableDepthRead();
	gl::disableDepthWrite();
	gl::disableAlphaBlending();
	
	
	if( mSaveFrames && mNumFrames < 15000 ){
		writeImage( getHomeDirectory() + "GpuPhysics/" + toString( mNumFrames ) + ".png", copyWindowSurface() );
		mNumFrames ++;
	}
	
	
	if( getElapsedFrames()%60 == 0 ){
		std::cout << "FPS = " << getAverageFps() << std::endl;
	}
	
	// DRAW PARAMS WINDOW
	if( mShowParams ){
		gl::setMatricesWindow( getWindowSize() );
//		gl::draw( mVelocityFbos[mThisFbo].getTexture(), Vec2f( 220.0f, 500.0f ) );
		gl::draw( mPositionFbos[mThisFbo].getTexture(), Vec2f( 392, 443.0f ) );
		params::InterfaceGl::draw();
	}
	
	mThisFbo	= ( mThisFbo + 1 ) % 2;
	mPrevFbo	= ( mThisFbo + 1 ) % 2;
}


CINDER_APP_BASIC( FlockingApp, RendererGl )
