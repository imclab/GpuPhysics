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
#include "Controller.h"
#include "Bait.h"
#include "SpringCam.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using std::sort;


#define APP_WIDTH		1280
#define APP_HEIGHT		720
#define FBO_WIDTH		4	// THIS
#define FBO_HEIGHT		4	// TIMES THIS IS THE NUMBER OF FLOCKING AGENTS
#define ROOM_FBO_RES	2
#define MAX_LIGHTS		3

class RepulsionApp : public AppBasic {
  public:
	
	
	void prepareSettings( Settings *settings );
	void setup();
	void setFboPositions( gl::Fbo &fbo );
	void setFboVelocities( gl::Fbo &fbo );
	void createSphere( gl::VboMesh &mesh, int res );
	void drawSphereTri( Vec3f va, Vec3f vb, Vec3f vc, int div, Color c );
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
	void drawToLightsSurface();
	
	// PARAMS
	params::InterfaceGl	mParams;

	// TIME
	float			mTime, mTimePrev;
	float			mTimer;
	float			mTimeDelta;
	float			mTimeAdjusted;
	float			mTimeMulti;
	
	Controller		mController;
	
	// POINT LIGHTS
	int				mNumLights;
	float			mInvNumLights;
	Surface32f		mLightsSurface;
	gl::Texture		mLightsTex;
	
	// CAMERA
	SpringCam		mSpringCam;
	
	// SHADERS
	gl::GlslProg	mVelocityShader;
	gl::GlslProg	mPositionShader;
	gl::GlslProg	mRoomShader;
	gl::GlslProg	mShader;
	gl::GlslProg	mFboInitShader;
	
	// TEXTURES
	gl::Texture		mRoomPanelTex;
	gl::Texture		mGlowTex;
	
	// ROOM
	Room			mRoom;
	gl::Fbo			mRoomFbo;
	ci::Vec2f		mRoomFboSize;
	ci::Area		mRoomFboBounds;
	Rectf			mRoomPanelRect;
	
	gl::VboMesh			mSphereVbo;
	std::vector<Vec3f>	mPosCoords;
	std::vector<Vec3f>	mNormals;
	std::vector<Vec2f>	mTexCoords;
	std::vector<Colorf>	mColors;
	
	// POSITION/VELOCITY FBOS
	ci::Vec2f		mFboSize;
	ci::Area		mFboBounds;
	gl::Fbo			mPositionFbos[2];
	gl::Fbo			mVelocityFbos[2];
	int				mThisFbo, mPrevFbo;
	
	float			mMainPower;
	bool			mIsPowerOn;
	float			mGravity;
	bool			mIsGravityOn;
	
	bool			mShowParams;
	float			mMinThresh, mMaxThresh;
	float			mZoneRadius;
	float			mMinSpeed, mMaxSpeed;
	
	Vec2f			mMousePos;
	float			mMousePressed;
	
	bool			mSaveFrames;
	int				mNumFrames;
};

void RepulsionApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
}

void RepulsionApp::setup()
{
	createSphere( mSphereVbo, 2 );
	
	// TIME
	mTimePrev	= (float)getElapsedSeconds();
	mTime		= (float)getElapsedSeconds();
	mTimeDelta	= mTime - mTimePrev;
	mTimeMulti	= 120.0f;
	
	// CAMERA	
	mSpringCam		= SpringCam( -188.0f, getWindowAspectRatio() );
	
	// POINT LIGHTS
	mNumLights			= MAX_LIGHTS;
	mInvNumLights		= 1.0f/(float)mNumLights;
	gl::Texture::Format hdTexFormat;
	hdTexFormat.setInternalFormat( GL_RGB32F_ARB );
	mLightsTex			= gl::Texture( mNumLights, 2, hdTexFormat );
	mLightsSurface		= Surface32f( mNumLights, 2, false );
	
	// POSITION/VELOCITY FBOS
	gl::Fbo::Format format;
	format.enableColorBuffer( true, 2 );
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

	// SHADERS
	try {
		mVelocityShader = gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "Velocity.frag" ) );
		mPositionShader	= gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "Position.frag" ) );
		mShader			= gl::GlslProg( loadResource( "VboPos.vert" ), loadResource( "VboPos.frag" ) );
		mRoomShader		= gl::GlslProg( loadResource( "room.vert" ), loadResource( "room.frag" ) );
		mFboInitShader	= gl::GlslProg( loadResource( "passThru.vert" ), loadResource( "fboInit.frag" ) );
	} catch( gl::GlslProgCompileExc e ) {
		std::cout << e.what() << std::endl;
		quit();
	}
	
	// TEXTURES
	mRoomPanelTex	= gl::Texture( loadImage( loadResource( "roomPanel.png" ) ) );
	mGlowTex		= gl::Texture( loadImage( loadResource( "glow.png" ) ) );
	
	// ROOM
	gl::Fbo::Format roomFormat;
	format.setColorInternalFormat( GL_RGB );
	int fboXRes		= APP_WIDTH/ROOM_FBO_RES;
	int fboYRes		= APP_HEIGHT/ROOM_FBO_RES;
	mRoomFboSize	= Vec2f( fboXRes, fboYRes );
	mRoomFboBounds	= Area( 0, 0, fboXRes, fboYRes );
	mRoomFbo		= gl::Fbo( fboXRes, fboYRes, roomFormat );
	mRoomPanelRect	= Rectf( 32.0f, 35.0f, 32.0f + mRoomPanelTex.getWidth(), 35.0f - mRoomPanelTex.getHeight() );
	mRoom			= Room( Vec3f( 155.0f, 88.0f, 50.0f ) );	
	
	mShowParams = false;
	mZoneRadius = 20.0f;
	mMinThresh	= 0.14f;
	mMaxThresh	= 0.90f;
	mMinSpeed	= 0.4f;
	mMaxSpeed	= 10.1f;
	
	mMainPower	= 0.0f;
	mIsPowerOn	= false;
	
	mGravity	= 0.0f;
	mIsGravityOn= false;
	
	mMousePos	= Vec2f::zero();
	mMousePressed = 0.0f;

	
	setFboPositions( mPositionFbos[0] );
	setFboPositions( mPositionFbos[1] );
	setFboVelocities( mVelocityFbos[0] );
	setFboVelocities( mVelocityFbos[1] );

	mParams = params::InterfaceGl( "Flocking", Vec2i( 200, 150 ) );
	mParams.addParam( "Time Multi", &mTimeMulti, "min=0.0 max=180.0 step=1.0 keyIncr=t keyDecr=T" );
	mParams.addParam( "Zone Radius", &mZoneRadius, "min=2.0 max=30.0 step=0.1 keyIncr=z keyDecr=Z" );
	mParams.addParam( "Min Thresh", &mMinThresh, "min=0.025 max=1.0 step=0.01 keyIncr=l keyDecr=L" );
	mParams.addParam( "Max Thresh", &mMaxThresh, "min=0.025 max=1.0 step=0.01 keyIncr=h keyDecr=H" );
	mParams.addParam( "Min Speed", &mMinSpeed, "min=0.1 max=1.0 step=0.025" );
	mParams.addParam( "Max Speed", &mMaxSpeed, "min=0.5 max=4.0 step=0.025" );
	
	mSaveFrames	= false;
	mNumFrames	= 0;
	
	
	mRoom.init();
}

void RepulsionApp::createSphere( gl::VboMesh &vbo, int res )
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
	
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticNormals();
	layout.setStaticTexCoords2d();
	layout.setStaticColorsRGB();
	
	mPosCoords.clear();
	mNormals.clear();
	mTexCoords.clear();
	mColors.clear();
	
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
	vbo = gl::VboMesh( mPosCoords.size(), 0, layout, GL_TRIANGLES );	
	vbo.bufferPositions( mPosCoords );
	vbo.bufferNormals( mNormals );
	vbo.bufferTexCoords2d( 0, mTexCoords );
	vbo.bufferColorsRGB( mColors );
	vbo.unbindBuffers();
}

void RepulsionApp::drawSphereTri( Vec3f va, Vec3f vb, Vec3f vc, int div, Color c )
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

void RepulsionApp::setFboPositions( gl::Fbo &fbo )
{
	Surface32f pos( fbo.getTexture() );
	Surface32f::Iter it = pos.getIter();
	while( it.line() ){
		while( it.pixel() ){
			Vec3f r = Vec3f( Rand::randFloat( -mRoom.getBounds().x, mRoom.getBounds().x ), 
							 Rand::randFloat( -mRoom.getBounds().y, mRoom.getBounds().y ),
							 Rand::randFloat( -mRoom.getBounds().z, mRoom.getBounds().z ) ) * 0.9f;
			float radius = Rand::randFloat( 1.5f, 4.0f );
			if( Rand::randFloat() < 0.05f ) 
				radius = Rand::randFloat( 6.0f, 12.0f );
			it.r() = r.x;
			it.g() = r.y;
			it.b() = r.z;
			it.a() = radius;
		}
	}
	
	gl::Texture posTexture( pos );
	posTexture.bind();
	
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	
	fbo.bindFramebuffer();
	mFboInitShader.bind();
	mFboInitShader.uniform( "initTex", 0 );
	gl::drawSolidRect( mFboBounds );
	mFboInitShader.unbind();
	fbo.unbindFramebuffer();
}

void RepulsionApp::setFboVelocities( gl::Fbo &fbo )
{
	Surface32f vel( fbo.getTexture() );
	Surface32f::Iter it = vel.getIter();
	while( it.line() ){
		while( it.pixel() ){
			Vec3f r = Rand::randVec3f() * 0.1f;
			it.r() = 0.0f;//r.x;
			it.g() = 0.0f;//r.y;
			it.b() = 0.0f;//r.z;
			it.a() = 1.0f;
		}
	}
	
	gl::Texture velTexture( vel );
	velTexture.bind();
	
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	
	fbo.bindFramebuffer();
	mFboInitShader.bind();
	mFboInitShader.uniform( "initTex", 0 );
	gl::drawSolidRect( mFboBounds );
	mFboInitShader.unbind();
	fbo.unbindFramebuffer();
}

void RepulsionApp::mouseDown( MouseEvent event )
{
	mMousePressed = 1.0f;
}

void RepulsionApp::mouseUp( MouseEvent event )
{
	mMousePressed = 0.0f;
}

void RepulsionApp::mouseDrag( MouseEvent event )
{
	mMousePos = event.getPos();
}

void RepulsionApp::keyDown( KeyEvent event )
{
	if( event.getChar() == '/' ){
		mShowParams = !mShowParams;
	} else if( event.getChar() == ' ' ){
		mSaveFrames = !mSaveFrames;
	} else if( event.getChar() == 'p' ){
		mIsPowerOn = !mIsPowerOn;
	} else if( event.getChar() == 'g' ){
		mIsGravityOn = !mIsGravityOn;
	} else if( event.getChar() == 'b' ){
		mController.addBait( mRoom.getRandCeilingPos() );
	}
}

void RepulsionApp::updateTime()
{
	mTimePrev		= mTime;
	mTime			= (float)app::getElapsedSeconds();
	mTimeDelta		= mTime - mTimePrev;
	
	// ONLY FOR SAVING VIDEO
	mTimeDelta		= 1.0f/60.0f;
	//
	
	mTimeAdjusted	= mTimeDelta * mTimeMulti;
	mTimer			+= mTimeAdjusted;
}

void RepulsionApp::update()
{
	updateTime();
	mController.update( mRoom.getFloorLevel() );
	
//	if( Rand::randFloat() < 0.007f && mController.mBaits.size() < MAX_LIGHTS && mIsPowerOn ){
//		mController.addBait( mRoom.getRandCeilingPos() );
//	}
	
	
	if( mIsPowerOn ) mMainPower -= ( mMainPower - 1.0f ) * 0.2f;
	else			 mMainPower -= ( mMainPower - 0.0f ) * 0.2f;
	
	if( mIsGravityOn ) mGravity -= ( mGravity + 0.02f ) * 0.2f;
	else			   mGravity -= ( mGravity - 0.0f ) * 0.2f;
	
	mNumLights		= math<int>::min( (int)mController.mBaits.size(), MAX_LIGHTS );
	
	mSpringCam.update( mTimeAdjusted );
	
	drawToLightsSurface();
	drawIntoVelocityFbo();
	drawIntoPositionFbo();
	drawIntoRoomFbo();
}



void RepulsionApp::drawIntoVelocityFbo()
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
	mVelocityShader.uniform( "gravity", mGravity );
	gl::drawSolidRect( mFboBounds );
	mVelocityShader.unbind();
	
	mVelocityFbos[ mThisFbo ].unbindFramebuffer();
}

void RepulsionApp::drawIntoPositionFbo()
{	
	gl::setMatricesWindow( mFboSize, false );
	gl::setViewport( mFboBounds );
	
	mPositionFbos[ mThisFbo ].bindFramebuffer();
	mPositionFbos[ mPrevFbo ].bindTexture( 0, 0 );
	mPositionFbos[ mPrevFbo ].bindTexture( 1, 1 );
	mVelocityFbos[ mThisFbo ].bindTexture( 2, 0 );
	mVelocityFbos[ mThisFbo ].bindTexture( 3, 1 );	
	mPositionShader.bind();
	mPositionShader.uniform( "pos0Tex", 0 );
	mPositionShader.uniform( "pos1Tex", 1 );
	mPositionShader.uniform( "vel0Tex", 2 );
	mPositionShader.uniform( "vel1Tex", 3 );
	mPositionShader.uniform( "timeDelta", mTimeAdjusted );
	gl::drawSolidRect( mFboBounds );
	mPositionShader.unbind();
	
	mPositionFbos[ mThisFbo ].unbindFramebuffer();
}

void RepulsionApp::drawIntoRoomFbo()
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
	
	mRoomShader.bind();
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

void RepulsionApp::draw()
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
	
	
	// DRAW PANEL
	gl::color( ColorA( mMainPower, mMainPower, mMainPower, mMainPower * 0.1f + 0.9f ) );
	mRoomPanelTex.bind();
	mRoom.drawPanel( mRoomPanelTex );
	
	gl::enableDepthWrite();
	
	// DRAW PARTICLES
	mPositionFbos[mThisFbo].bindTexture( 0 );
	mVelocityFbos[mThisFbo].bindTexture( 1 );
	mShader.bind();
	mShader.uniform( "currentPosition", 0 );
	mShader.uniform( "currentVelocity", 1 );
	mShader.uniform( "eyePos", mSpringCam.mEye );
	mShader.uniform( "mainPower", mMainPower );
	gl::draw( mSphereVbo );
	mShader.unbind();
	
	gl::disableDepthWrite();
	gl::enableAdditiveBlending();
	gl::color( Color( 1.0f, 1.0f, 1.0f ) );
	mGlowTex.bind();
	mController.drawBaitGlows();
	
//	gl::enableDepthWrite();
//	gl::enableAlphaBlending();
//	gl::disable( GL_TEXTURE_2D );
//	gl::color( Color( 1.0f, 1.0f, 1.0f ) );
//	mController.drawBaits();
	
	
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

void RepulsionApp::drawToLightsSurface()
{
	Surface32f::Iter iter = mLightsSurface.getIter();
	while( iter.line() ) {
		while( iter.pixel() ) {
			int index = iter.x();
			
			if( iter.y() == 0 ){ // set light position
				if( index < (int)mController.mBaits.size() ){
					iter.r() = mController.mBaits[index].mPos.x;
					iter.g() = mController.mBaits[index].mPos.y;
					iter.b() = mController.mBaits[index].mPos.z;
				} else { // if the light shouldnt exist, put it way out there
					iter.r() = 0.0f;
					iter.g() = 0.0f;
					iter.b() = 0.0f;
				}
			} else {	// set light color
				if( index < (int)mController.mBaits.size() ){
					float radius = mController.mBaits[index].mRadius;
					iter.r() = mController.mBaits[index].mColor.r * radius;
					iter.g() = mController.mBaits[index].mColor.g * radius;
					iter.b() = mController.mBaits[index].mColor.b * radius;
				} else { 
					iter.r() = 0.0f;
					iter.g() = 0.0f;
					iter.b() = 0.0f;
				}
			}
		}
	}
	
	mLightsTex = gl::Texture( mLightsSurface );
}


CINDER_APP_BASIC( RepulsionApp, RendererGl )
