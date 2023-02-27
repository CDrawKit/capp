//Copyright (c) 2011 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.

#include "os_generic.h"

#define CAPP3D
#define CAPP_IMPLEMENTATION
//#define CAPPRASTERIZER

#include "CAPP.h"

double OGGetAbsoluteTime();
void OGUSleep( int us );
void prints( const char * sdebug );
void print( double idebug );
double sin( double x );
double cos( double x );

unsigned long iframeno = 0;
int lastmousex;
int lastmousey = 100;

void __attribute__((export_name("HandleKey"))) HandleKey( int keycode, int bDown )
{
	print( keycode );
	print( bDown );
}

void __attribute__((export_name("HandleButton"))) HandleButton( int x, int y, int button, int bDown )
{
	print( x );
	print( y );
	print( button );
	print( bDown );
}

void __attribute__((export_name("HandleMotion"))) HandleMotion( int x, int y, int mask )
{
	lastmousex = x;
	lastmousey = y;
}

void HandleDestroy()
{
	//printf( "Destroying\n" );
}

#define HMX 40
#define HMY 40
short screenx, screeny;
float Heightmap[HMX*HMY];
void DrawHeightmap();

unsigned fpsframes = 0;
double fpstime = 0;

int __attribute__((export_name("main"))) main()
{
	int i, x, y;
	fpstime = OGGetAbsoluteTime();

	//Setup colors.
	CAPPBGColor = 0xff800000;

	//Actually sets outside window title, and
	//If not in full-screen mode, will also resize.
	//CAPPSetup( "Test Bench", 640, 480 );
	CAPPSetupFullscreen( "Test Bench", 0 );

	//Configure heightmap.
	for( x = 0; x < HMX; x++ )
	for( y = 0; y < HMY; y++ )
	{
		Heightmap[x+y*HMX] = tdPerlin2D( x, y )*8.;
	}

	prints( "Main started.  This will appear in your browser console." );

#ifdef CAPP_USE_LOOP_FUNCTION
	return 0;
}
int __attribute__((export_name("loop"))) loop()
{
#else
while(CAPPHandleInput())
#endif
	{
		int i, pos;
		float f;
		iframeno++;
		RDPoint pto[3];

		//Normally this would invoke callbacks, but
		//at least currently, we do that out-of-band.
		//CAPPHandleInput();


		CAPPClearFrame();
		CAPPColor( 0xFFFFFFFF );
		CAPPGetDimensions( &screenx, &screeny );
		CAPPPenX = 100;
		CAPPPenY = 100;

//quick minitest
//		CAPPDrawText("+hello!!",4 );
//		CAPPSwapBuffers();
//		continue;

		// Mesh in background
		DrawHeightmap();

		// Square behind text
		CAPPColor( 0xff444444 );
		CAPPSetLineWidth(3);
		CAPPTackSegment( 0, 50, 100, 50 );
		CAPPTackSegment( 0, 50, 0, 150 );
		CAPPTackRectangle( 2, 2+50, 345, 345+50 );

		CAPPSetLineWidth(3);

		// Text stuff in upper left.
		pos = 0;
		CAPPColor( 0xffffffff );
		for( i = 0; i < 1; i++ )
		{
			int c;
			char tw[2] = { 0, 0 };
			for( c = 0; c < 256; c++ )
			{
				tw[0] = c;

				CAPPPenX = ( c % 16 ) * 20+5;
				CAPPPenY = ( c / 16 ) * 20+55;
				CAPPDrawText( tw, 4 );
			}
		}

#if 1
		// Green triangles
		CAPPPenX = 0;
		CAPPPenY = 0;

		RDPoint pp[3];
		for( i = 0; i < 400; i++ )
		{
			CAPPColor( 0xFF00FF00 );
			pp[0].x = (short)(50*sin((float)(i+iframeno)*.01) + (i%20)*30);
			pp[0].y = (short)(50*cos((float)(i+iframeno)*.01) + (i/20)*20)+100;
			pp[1].x = (short)(20*sin((float)(i+iframeno)*.01) + (i%20)*30);
			pp[1].y = (short)(50*cos((float)(i+iframeno)*.01) + (i/20)*20)+100;
			pp[2].x = (short)(10*sin((float)(i+iframeno)*.01) + (i%20)*30);
			pp[2].y = (short)(30*cos((float)(i+iframeno)*.01) + (i/20)*20)+100;
			CAPPTackPoly( pp, 3 );
		}
#endif

#if 0 //Profiling
		extern int Add1( int i );
		double now = OGGetAbsoluteTime();
		double k = 4;
		for( i = 0; i < 1000000; i++ )
		{
			//CAPPTackPoly( pp, 3 ); //3200 ns
			CAPPTackPixel( 0,0  ); //340ns
			//CAPPTackSegment( 0, 0, 10, 10 ); //157ns
			//k = sin(k); //28ns
			//k = Add1(k); //16ns
			//NOTE: actual callback time is ~14ns, so loop overhead is about 10ns. 
		}
		print( (OGGetAbsoluteTime() - now)*1000 );
		print( k );
#endif

#if 1 //Enable alpha objec test
		static uint32_t randomtexturedata[65536];
		int x, y;
		for( y = 0; y < 256; y++ )
		for( x = 0; x < 256; x++ )
			randomtexturedata[x+y*256] = x | ((x*394543L+y*355+iframeno)<<8);
		CAPPBlitImage( randomtexturedata, 100, 300, 256, 256 );
#endif

		fpsframes++;
		CAPPSwapBuffers();

		double Now = OGGetAbsoluteTime();
		if( Now - fpstime >= 1 )
		{
			print( fpsframes );
			fpsframes = 0;
			fpstime += 1;
		}
	}

	return(0);
}


void DrawHeightmap()
{
	int x, y;
	float fdt = ((iframeno++)%(360*10))/100.0 + lastmousex;
	float eye[3] = { (float)sin(fdt*(3.14159/180.0))*1, (float)cos(fdt*(3.14159/180.0))*1, lastmousey*.01 };
	float at[3] = { 0,0, 0 };
	float up[3] = { 0,0, 1 };

	tdSetViewport( -1, -1, 1, 1, screenx, screeny );

	tdMode( tdPROJECTION );
	tdIdentity( gSMatrix );
	tdPerspective( 40, ((float)screenx)/((float)screeny), .1, 200., gSMatrix );

	tdMode( tdMODELVIEW );
	tdIdentity( gSMatrix );
	tdTranslate( gSMatrix, 0, 0, -40 );
	tdLookAt( gSMatrix, eye, at, up );


	for( x = 0; x < HMX-1; x++ )
	for( y = 0; y < HMY-1; y++ )
	{
		float tx = x-HMX/2;
		float ty = y-HMY/2;
		float pta[3];
		float ptb[3];
		float ptc[3];
		float ptd[3];

		float normal[3];
		float lightdir[3] = { 1, -1, 1 };
		float tmp1[3];
		float tmp2[3];

		RDPoint pto[6];

		pta[0] = tx+0; pta[1] = ty+0; pta[2] = Heightmap[(x+0)+(y+0)*HMX];
		ptb[0] = tx+1; ptb[1] = ty+0; ptb[2] = Heightmap[(x+1)+(y+0)*HMX];
		ptc[0] = tx+0; ptc[1] = ty+1; ptc[2] = Heightmap[(x+0)+(y+1)*HMX];
		ptd[0] = tx+1; ptd[1] = ty+1; ptd[2] = Heightmap[(x+1)+(y+1)*HMX];

		tdPSub( pta, ptb, tmp2 );
		tdPSub( ptc, ptb, tmp1 );
		tdCross( tmp1, tmp2, normal );

		tdFinalPoint( pta, pta );
		tdFinalPoint( ptb, ptb );
		tdFinalPoint( ptc, ptc );
		tdFinalPoint( ptd, ptd );

		if( pta[2] >= 1.0 ) continue;
		if( ptb[2] >= 1.0 ) continue;
		if( ptc[2] >= 1.0 ) continue;
		if( ptd[2] >= 1.0 ) continue;

		if( pta[2] < 0 ) continue;
		if( ptb[2] < 0 ) continue;
		if( ptc[2] < 0 ) continue;
		if( ptd[2] < 0 ) continue;

		pto[0].x = pta[0]; pto[0].y = pta[1];
		pto[1].x = ptb[0]; pto[1].y = ptb[1];
		pto[2].x = ptd[0]; pto[2].y = ptd[1];

		pto[3].x = ptc[0]; pto[3].y = ptc[1];
		pto[4].x = ptd[0]; pto[4].y = ptd[1];
		pto[5].x = pta[0]; pto[5].y = pta[1];

		float bright = tdDot( normal, lightdir );
		if( bright < 0 ) bright = 0;
		CAPPColor( 0xff000000 | (int)( bright * 50 ) );

		CAPPTackSegment( pta[0], pta[1], ptb[0], ptb[1] );
		CAPPTackSegment( pta[0], pta[1], ptc[0], ptc[1] );	
	}
}


