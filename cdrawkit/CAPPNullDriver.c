//Copyright (c) 2017 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.

#include "DrawFunctions.h"

static int w, h;
void CAPPGetDimensions( short * x, short * y )
{
	*x = w;
	*y = h;
}

static void InternalLinkScreenAndGo( const char * WindowName )
{
}

void CAPPSetupFullscreen( const char * WindowName, int screen_no )
{
	CAPPSetup( WindowName, 640, 480 );
}


void CAPPTearDown()
{
}

int CAPPSetup( const char * WindowName, int sw, int sh )
{
	w = sw;
	h = sh;
	return 0;
}

void CAPPHandleInput()
{
}


void CAPPUpdateScreenWithBitmap( uint32_t * data, int w, int h )
{
}


uint32_t CAPPColor( uint32_t RGB )
{
}

void CAPPClearFrame()
{
}

void CAPPSwapBuffers()
{
}

void CAPPTackSegment( short x1, short y1, short x2, short y2 )
{
}

void CAPPTackPixel( short x1, short y1 )
{
}

void CAPPTackRectangle( short x1, short y1, short x2, short y2 )
{
}

void CAPPTackPoly( RDPoint * points, int verts )
{
}

