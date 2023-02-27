//Copyright (c) 2011 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CAPP.h"
#include "os_generic.h"

unsigned frames = 0;
unsigned long iframeno = 0;

void HandleKey( int keycode, int bDown )
{
	if( keycode == 65307 ) exit( 0 );
	printf( "Key: %d -> %d\n", keycode, bDown );
}

void HandleButton( int x, int y, int button, int bDown )
{
	printf( "Button: %d,%d (%d) -> %d\n", x, y, button, bDown );
}

void HandleMotion( int x, int y, int mask )
{
//	printf( "Motion: %d,%d (%d)\n", x, y, mask );
}


void HandleDestroy()
{
	printf( "Destroying\n" );
	exit(10);
}


short screenx, screeny;

int main()
{
	int i, x, y;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();
	double LastFrameTime = OGGetAbsoluteTime();
	double SecToWait;
	int linesegs = 0;

	CAPPBGColor = 0x800000;
	//Deprecated CAPPDialogColor = 0x444444;
	CAPPPrepareForTransparency();
	CAPPSetupFullscreen( "Test Bench", 0 );

	while(CAPPHandleInput())
	{
		int i, pos;
		float f;
		iframeno++;
		RDPoint pto[3];

		CAPPClearFrame();
		CAPPColor( 0xFFFFFF );
		CAPPGetDimensions( &screenx, &screeny );

//		DrawFrame();

/*

		pto[0].x = 100;
		pto[0].y = 100;
		pto[1].x = 200;
		pto[1].y = 100;
		pto[2].x = 100;
		pto[2].y = 200;
		CAPPTackPoly( &pto[0], 3 );

		CAPPColor( 0xFF00FF );
*/

/*		CAPPTackSegment( pto[0].x, pto[0].y, pto[1].x, pto[1].y );
		CAPPTackSegment( pto[1].x, pto[1].y, pto[2].x, pto[2].y );
		CAPPTackSegment( pto[2].x, pto[2].y, pto[0].x, pto[0].y );
*/

		CAPPClearTransparencyLevel();
		CAPPDrawToTransparencyMode(1);
		CAPPTackRectangle( 0, 0, 260, 260 );
		CAPPTackRectangle( 400, 400, 600, 600 );
		CAPPPenX = 300;
		CAPPPenY = 200;
		CAPPSetLineWidth( 7 + sin(iframeno)*5 );
		CAPPDrawText( "HELLO!", 5 );
		CAPPDrawToTransparencyMode(0);	


		CAPPSetLineWidth( 1 );
		CAPPDrawText( "HELLO!", 5 );

		CAPPDrawBox( 0, 0, 260, 260 );

		CAPPPenX = 10; CAPPPenY = 10;

		pos = 0;
		CAPPColor( 0xffffff );
		for( i = 0; i < 1; i++ )
		{
			int c;
			char tw[2] = { 0, 0 };
			for( c = 0; c < 256; c++ )
			{
				tw[0] = c;

				CAPPPenX = ( c % 16 ) * 16+5;
				CAPPPenY = ( c / 16 ) * 16+5;
				CAPPDrawText( tw, 2 );
			}
		}

		CAPPPenX = 0;
		CAPPPenY = 0;


		RDPoint pp[3];
		CAPPColor( 0x00FF00 );
		pp[0].x = (short)(-200*sin((float)(iframeno)*.01) + 500);
		pp[0].y = (short)(-200*cos((float)(iframeno)*.01) + 500);
		pp[1].x = (short)(200*sin((float)(iframeno)*.01) + 500);
		pp[1].y = (short)(00*cos((float)(iframeno)*.01) + 500);
		pp[2].x = (short)(200*sin((float)(iframeno)*.01) + 500);
		pp[2].y = (short)(200*cos((float)(iframeno)*.01) + 500);
		CAPPTackPoly( pp, 3 );


		frames++;
		CAPPSwapBuffers();

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			linesegs = 0;
			LastFPSTime+=1;
		}

		SecToWait = .016 - ( ThisTime - LastFrameTime );
		LastFrameTime += .016;
		if( SecToWait > 0 )
			OGUSleep( (int)( SecToWait * 1000000 ) );
	}

	return(0);
}

