//Copyright (c) 2011-2019 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
//Portion from: http://en.wikibooks.org/wiki/Windows_Programming/Window_Creation

#ifndef _CAPPWINDRIVER_C
#define _CAPPWINDRIVER_C

#include "CAPP.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h> //for alloca
#include <ctype.h>

HBITMAP CAPPlsBitmap;
HWND CAPPlsHWND;
HDC CAPPlsWindowHDC;
HDC CAPPlsHDC;
HDC CAPPlsHDCBlit;

int ShouldClose = 0;

//Queue up lines and points for a faster render.
#ifndef CAPP_WINDOWS_DISABLE_BATCH
#define BATCH_ELEMENTS
#endif

#define COLORSWAPS( RGB ) \
		((((RGB )& 0xFF000000)>>24) | ( ((RGB )& 0xFF0000 ) >> 8 ) | ( ((RGB )& 0xFF00 )<<8 ))


void CAPPChangeWindowTitle( const char * windowtitle )
{
	SetWindowTextA( CAPPlsHWND, windowtitle );
}

#ifdef CAPPRASTERIZER
#include "CAPPRasterizer.c"

void InternalHandleResize()
{
	if( CAPPlsBitmap ) DeleteObject( CAPPlsBitmap );

	CAPPInternalResize( CAPPBufferx, CAPPBuffery );
	CAPPlsBitmap = CreateBitmap( CAPPBufferx, CAPPBuffery, 1, 32, CAPPBuffer );
	SelectObject( CAPPlsHDC, CAPPlsBitmap );
	CAPPInternalResize( CAPPBufferx, CAPPBuffery);
}
#else
static short CAPPBufferx, CAPPBuffery;
static void InternalHandleResize();
#endif


#ifdef CAPPOGL
#include <GL/gl.h>
static HGLRC           hRC=NULL; 
static void InternalHandleResize() { }
void CAPPSwapBuffers()
{
#ifdef CAPP_BATCH
#ifndef CAPPCONTEXTONLY
	CAPPFlushRender();
#endif
#endif

	SwapBuffers(CAPPlsWindowHDC);
}
#endif

void CAPPGetDimensions( short * x, short * y )
{
	static short lastx, lasty;
	RECT window;
	GetClientRect( CAPPlsHWND, &window );
	CAPPBufferx = (short)( window.right - window.left);
	CAPPBuffery = (short)( window.bottom - window.top);
	if( CAPPBufferx != lastx || CAPPBuffery != lasty )
	{
		lastx = CAPPBufferx;
		lasty = CAPPBuffery;
		#ifndef CAPPCONTEXTONLY
		CAPPInternalResize( lastx, lasty );
		#endif
		InternalHandleResize();
	}
	*x = CAPPBufferx;
	*y = CAPPBuffery;
}

#ifndef CAPPOGL
void CAPPUpdateScreenWithBitmap( uint32_t * data, int w, int h )
{
	RECT r;

	SelectObject( CAPPlsHDC, CAPPlsBitmap );
	SetBitmapBits(CAPPlsBitmap,w*h*4,data);
	BitBlt(CAPPlsWindowHDC, 0, 0, w, h, CAPPlsHDC, 0, 0, SRCCOPY);
	UpdateWindow( CAPPlsHWND );

	short thisw, thish;

	//Check to see if the window is closed.
	if( !IsWindow( CAPPlsHWND ) )
	{
		exit( 0 );
	}

	GetClientRect( CAPPlsHWND, &r );
	thisw = (short)(r.right - r.left);
	thish = (short)(r.bottom - r.top);
	if( thisw != CAPPBufferx || thish != CAPPBuffery )
	{
		CAPPBufferx = thisw;
		CAPPBuffery = thish;
		InternalHandleResize();
	}
}
#endif

void CAPPTearDown()
{
	PostQuitMessage(0);
	ShouldClose = 1;
}

//This was from the article
LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
#ifndef CAPPOGL
	case WM_SYSCOMMAND:  //Not sure why, if deactivated, the dc gets unassociated?
		if( wParam == SC_RESTORE || wParam == SC_MAXIMIZE || wParam == SC_SCREENSAVE )
		{
			SelectObject( CAPPlsHDC, CAPPlsBitmap );
			SelectObject( CAPPlsWindowHDC, CAPPlsBitmap );
		}
		break;
#endif
	case WM_DESTROY:
		HandleDestroy();
		CAPPTearDown();
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int CAPPSetupWinInternal( const char * name_of_window, int width, int height, int isFullscreen );

void CAPPSetupFullscreen( const char * WindowName, int screen_number )
{
	// Get primary monitor dimensions, but default to 1920x1080
	int monitorW = GetSystemMetrics(SM_CXSCREEN);
	if(0 == monitorW)
	{
		monitorW = 1920;
	}

	int monitorH = GetSystemMetrics(SM_CYSCREEN);
	if(0 == monitorH)
	{
		monitorH = 1080;
	}

	CAPPSetupWinInternal(WindowName, monitorW, monitorH, 1);
}

int CAPPSetup( const char * name_of_window, int width, int height )
{
	return CAPPSetupWinInternal(name_of_window, width, height, 0);
}

//This was from the article, too... well, mostly.
int CAPPSetupWinInternal( const char * name_of_window, int width, int height, int isFullscreen )
{
	static LPCSTR szClassName = "MyClass";
	RECT client, window;
	WNDCLASSA wnd;
	int w, h, wd, hd;
	int show_window = 1;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	if( width < 0 ) 
	{
		show_window = 0;
		width = -width;
	}
	if( height < 0 ) 
	{
		show_window = 0;
		height = -height;
	}

	CAPPBufferx = (short)width;
	CAPPBuffery = (short)height;

	wnd.style = CS_HREDRAW | CS_VREDRAW; //we will explain this later
	wnd.lpfnWndProc = MyWndProc;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.hInstance = hInstance;
	wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION); //default icon
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);   //default arrow mouse cursor
	wnd.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wnd.lpszMenuName = NULL;                     //no menu
	wnd.lpszClassName = szClassName;

	if(!RegisterClassA(&wnd))                     //register the WNDCLASS
	{
		MessageBoxA(NULL, "This Program Requires Windows NT", "Error", MB_OK);
	}
	
#ifdef UNICODE
	// CreateWindowA **requires** unicode window name even in non-unicode mode.
	int wlen = strlen( name_of_window );
	char * unicodeao = (char*)alloca( wlen * 2 + 2 );
	int i;
	for( i = 0; i <= wlen; i++ )
	{
		unicodeao[i * 2 + 1] = 0;
		unicodeao[i * 2 + 0] = name_of_window[i];
	}
	name_of_window = unicodeao;
#endif


	CAPPlsHWND = CreateWindowA(szClassName,
		name_of_window,      //name_of_window, but must always be 
		isFullscreen ? (WS_MAXIMIZE | WS_POPUP) : (WS_OVERLAPPEDWINDOW), //basic window style
		CW_USEDEFAULT,
		CW_USEDEFAULT,       //set starting point to default value
		CAPPBufferx,
		CAPPBuffery,        //set all the dimensions to default value
		NULL,                //no parent window
		NULL,                //no menu
		hInstance,
		NULL);               //no parameters to pass

	CAPPlsWindowHDC = GetDC( CAPPlsHWND );

#ifdef CAPPOGL
	//From NeHe
	static  PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		8, 0, 8, 8, 8, 16, 
		8,
		24,
		32,
		8, 8, 8, 8,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	GLuint      PixelFormat = ChoosePixelFormat( CAPPlsWindowHDC, &pfd );
	if( !SetPixelFormat( CAPPlsWindowHDC, PixelFormat, &pfd ) )
	{
		MessageBoxA( 0, "Could not create PFD for OpenGL Context\n", 0, 0 );
		exit( -1 );
	}
	if (!(hRC=wglCreateContext(CAPPlsWindowHDC)))                   // Are We Able To Get A Rendering Context?
	{
		MessageBoxA( 0, "Could not create OpenGL Context\n", 0, 0 );
		exit( -1 );
	}
	if(!wglMakeCurrent(CAPPlsWindowHDC,hRC))                        // Try To Activate The Rendering Context
	{
		MessageBoxA( 0, "Could not current OpenGL Context\n", 0, 0 );
		exit( -1 );
	}
#endif

	CAPPlsHDC = CreateCompatibleDC( CAPPlsWindowHDC );
	CAPPlsHDCBlit = CreateCompatibleDC( CAPPlsWindowHDC );
	CAPPlsBitmap = CreateCompatibleBitmap( CAPPlsWindowHDC, CAPPBufferx, CAPPBuffery );
	SelectObject( CAPPlsHDC, CAPPlsBitmap );

	//lsClearBrush = CreateSolidBrush( CAPPBGColor );
	//lsHBR = CreateSolidBrush( 0xFFFFFF );
	//lsHPEN = CreatePen( PS_SOLID, 0, 0xFFFFFF );

	if( show_window )
		ShowWindow(CAPPlsHWND, isFullscreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);              //display the window on the screen

	//Once set up... we have to change the window's borders so we get the client size right.
	GetClientRect( CAPPlsHWND, &client );
	GetWindowRect( CAPPlsHWND, &window );
	w = ( window.right - window.left);
	h = ( window.bottom - window.top);
	wd = w - client.right;
	hd = h - client.bottom;
	MoveWindow( CAPPlsHWND, window.left, window.top, CAPPBufferx + wd, CAPPBuffery + hd, 1 );

	InternalHandleResize();

#ifdef CAPP_BATCH
#ifndef CAPPCONTEXTONLY
	CAPPSetupBatchInternal();
#endif
#endif

	return 0;
}



int CAPPHandleInput()
{
#ifdef CAPPOGL
	if (ShouldClose)
		exit(0);
#endif

	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0xFFFF, 1 ) )
	{
		TranslateMessage(&msg);

		switch( msg.message )
		{
		case WM_MOUSEMOVE:
			HandleMotion( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, ( (msg.wParam & 0x01)?1:0) | ((msg.wParam & 0x02)?2:0) | ((msg.wParam & 0x10)?4:0) );
			break;
		case WM_LBUTTONDOWN:	HandleButton( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, 1, 1 ); break;
		case WM_RBUTTONDOWN:	HandleButton( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, 2, 1 ); break;
		case WM_MBUTTONDOWN:	HandleButton( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, 3, 1 ); break;
		case WM_LBUTTONUP:		HandleButton( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, 1, 0 ); break;
		case WM_RBUTTONUP:		HandleButton( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, 2, 0 ); break;
		case WM_MBUTTONUP:		HandleButton( (msg.lParam & 0xFFFF), (msg.lParam>>16) & 0xFFFF, 3, 0 ); break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (msg.lParam & 0x01000000) HandleKey( (int) msg.wParam + 0x7C , (msg.message==WM_KEYDOWN) );
			else HandleKey( (int) msg.wParam, (msg.message==WM_KEYDOWN) );
			break;
		case WM_MOUSEWHEEL:
		{
			POINT p = { 0 };
			p.x = LOWORD( msg.lParam );
			p.y = HIWORD( msg.lParam );
			ScreenToClient(CAPPlsHWND, &p);
			HandleButton(p.x, p.y, GET_WHEEL_DELTA_WPARAM(msg.wParam) > 0 ? 0x0E : 0x0F, 1);
		} break;
		default:
			DispatchMessage(&msg);
			break;
		}
	}

	return !ShouldClose;
}

#ifndef CAPPOGL

#ifndef CAPPRASTERIZER

static HBITMAP lsBackBitmap;
static HBRUSH lsHBR;
static HPEN lsHPEN;
static HBRUSH lsClearBrush;

static void InternalHandleResize()
{
	DeleteObject( lsBackBitmap );
	lsBackBitmap = CreateCompatibleBitmap( CAPPlsHDC, CAPPBufferx, CAPPBuffery );
	SelectObject( CAPPlsHDC, lsBackBitmap );
}

#ifdef BATCH_ELEMENTS

static int linelisthead;
static int pointlisthead;
static int polylisthead;
static int polylistindex;
static POINT linelist[4096*3];
static DWORD twoarray[4096];
static POINT pointlist[4096];
static POINT polylist[8192];
static INT   polylistcutoffs[8192];


static int last_linex;
static int last_liney;
static int possible_lastline;

void FlushTacking()
{
	int i;

	if( twoarray[0] != 2 )
		for( i = 0; i < 4096; i++ ) twoarray[i] = 2;

	if( linelisthead )
	{
		PolyPolyline( CAPPlsHDC, linelist, twoarray, linelisthead );
		linelisthead = 0;
	}

	if( polylistindex )
	{
		PolyPolygon( CAPPlsHDC, polylist, polylistcutoffs, polylistindex );
		polylistindex = 0;
		polylisthead = 0;
	}

	if( possible_lastline )
		CAPPTackPixel( last_linex, last_liney );
	possible_lastline = 0;

	//XXX TODO: Consider locking the bitmap, and manually drawing the pixels.
	if( pointlisthead )
	{
		for( i = 0; i < pointlisthead; i++ )
		{
			SetPixel( CAPPlsHDC, pointlist[i].x, pointlist[i].y, CAPPLastColor );
		}
		pointlisthead = 0;
	}
}
#endif

uint32_t CAPPColor( uint32_t RGB )
{
	RGB = COLORSWAPS( RGB );
	if( CAPPLastColor == RGB ) return RGB;

#ifdef BATCH_ELEMENTS
	FlushTacking();
#endif

	CAPPLastColor = RGB;

	DeleteObject( lsHBR );
	lsHBR = CreateSolidBrush( RGB );
	SelectObject( CAPPlsHDC, lsHBR );

	DeleteObject( lsHPEN );
	lsHPEN = CreatePen( PS_SOLID, 0, RGB );
	SelectObject( CAPPlsHDC, lsHPEN );

	return RGB;
}

void CAPPBlitImage( uint32_t * data, int x, int y, int w, int h )
{
	static int pbw, pbh;
	static HBITMAP pbb;
	if( !pbb || pbw != w || pbh !=h )
	{
		if( pbb ) DeleteObject( pbb );
		pbb = CreateBitmap( w, h, 1, 32, 0 );
		pbh = h;
		pbw = w;
	}
	SetBitmapBits(pbb,w*h*4,data);
	SelectObject( CAPPlsHDCBlit, pbb );
	BitBlt(CAPPlsHDC, x, y, w, h, CAPPlsHDCBlit, 0, 0, SRCCOPY);
}

void CAPPTackSegment( short x1, short y1, short x2, short y2 )
{
#ifdef BATCH_ELEMENTS

	if( ( x1 != last_linex || y1 != last_liney ) && possible_lastline )
	{
		CAPPTackPixel( last_linex, last_liney );
	}

	if( x1 == x2 && y1 == y2 )
	{
		CAPPTackPixel( x1, y1 );
		possible_lastline = 0;
		return;
	}

	last_linex = x2;
	last_liney = y2;
	possible_lastline = 1;

	if( x1 != x2 || y1 != y2 )
	{
		linelist[linelisthead*2+0].x = x1;
		linelist[linelisthead*2+0].y = y1;
		linelist[linelisthead*2+1].x = x2;
		linelist[linelisthead*2+1].y = y2;
		linelisthead++;
		if( linelisthead >= 2048 ) FlushTacking();
	}
#else
	POINT pt[2] = { {x1, y1}, {x2, y2} };
	Polyline( CAPPlsHDC, pt, 2 );
	SetPixel( CAPPlsHDC, x1, y1, CAPPLastColor );
	SetPixel( CAPPlsHDC, x2, y2, CAPPLastColor );
#endif
}

void CAPPTackRectangle( short x1, short y1, short x2, short y2 )
{
#ifdef BATCH_ELEMENTS
	FlushTacking();
#endif
	RECT r;
	if( x1 < x2 ) { r.left = x1; r.right = x2; }
	else          { r.left = x2; r.right = x1; }
	if( y1 < y2 ) { r.top = y1; r.bottom = y2; }
	else          { r.top = y2; r.bottom = y1; }
	FillRect( CAPPlsHDC, &r, lsHBR );
}

void CAPPClearFrame()
{
#ifdef BATCH_ELEMENTS
	FlushTacking();
#endif
	RECT r = { 0, 0, CAPPBufferx, CAPPBuffery };
	DeleteObject( lsClearBrush  );
	lsClearBrush = CreateSolidBrush( COLORSWAPS(CAPPBGColor) );
	HBRUSH prevBrush = SelectObject( CAPPlsHDC, lsClearBrush );
	FillRect( CAPPlsHDC, &r, lsClearBrush);
	SelectObject( CAPPlsHDC, prevBrush );
}

void CAPPTackPoly( RDPoint * points, int verts )
{
#ifdef BATCH_ELEMENTS
	if( verts > 8192 )
	{
		FlushTacking();
		//Fall-through
	}
	else
	{
		if( polylistindex >= 8191 || polylisthead + verts >= 8191 )
		{
			FlushTacking();
		}
		int i;
		for( i = 0; i < verts; i++ )
		{
			polylist[polylisthead].x = points[i].x;
			polylist[polylisthead].y = points[i].y;
			polylisthead++;
		}
		polylistcutoffs[polylistindex++] = verts;
		return;
	}
#endif
	{
		int i;
		POINT * t = (POINT*)alloca( sizeof( POINT ) * verts );
		for( i = 0; i < verts; i++ )
		{
			t[i].x = points[i].x;
			t[i].y = points[i].y;
		}
		Polygon( CAPPlsHDC, t, verts );
	}
}


void CAPPTackPixel( short x1, short y1 )
{
#ifdef BATCH_ELEMENTS
	pointlist[pointlisthead+0].x = x1;
	pointlist[pointlisthead+0].y = y1;
	pointlisthead++;

	if( pointlisthead >=4096 ) FlushTacking();
#else
	SetPixel( CAPPlsHDC, x1, y1, CAPPLastColor );
#endif

}

void CAPPSwapBuffers()
{
#ifdef BATCH_ELEMENTS
	FlushTacking();
#endif
	int thisw, thish;

	RECT r;
	BitBlt( CAPPlsWindowHDC, 0, 0, CAPPBufferx, CAPPBuffery, CAPPlsHDC, 0, 0, SRCCOPY );
	UpdateWindow( CAPPlsHWND );
	//Check to see if the window is closed.
	if( !IsWindow( CAPPlsHWND ) )
	{
		exit( 0 );
	}

	GetClientRect( CAPPlsHWND, &r );
	thisw = r.right - r.left;
	thish = r.bottom - r.top;

	if( thisw != CAPPBufferx || thish != CAPPBuffery )
	{
		CAPPBufferx = (short)thisw;
		CAPPBuffery = (short)thish;
		InternalHandleResize();
	}
}

void CAPPInternalResize( short bfx, short bfy ) { }
#endif

#endif

#endif // _CAPPWINDRIVER_C

