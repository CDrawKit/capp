//Copyright (c) 2011, 2017, 2018 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
//portions from 
//http://www.xmission.com/~georgeps/documentation/tutorials/Xlib_Beginner.html

//#define HAS_XINERAMA
//#define CAPP_HAS_XSHAPE
//#define FULL_SCREEN_STEAL_FOCUS

#ifndef _CAPPXDRIVER_C
#define _CAPPXDRIVER_C

#include "CAPP.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef HAS_XINERAMA
	#include <X11/extensions/shape.h>
	#include <X11/extensions/Xinerama.h>
#endif
#ifdef CAPP_HAS_XSHAPE
	#include <X11/extensions/shape.h>
	static    XGCValues xsval;
	static    Pixmap xspixmap;
	static    GC xsgc;

	static	int taint_shape;
	static	int prepare_xshape;
	static int was_transp;

#endif

#ifdef CAPP_BATCH
void CAPPSetupBatchInternal();
#endif

XWindowAttributes CAPPWinAtt;
XClassHint *CAPPClassHint;
char * wm_res_name = 0;
char * wm_res_class = 0;
Display *CAPPDisplay;
Window CAPPWindow;
int CAPPWindowInvisible;
Pixmap CAPPPixmap;
GC     CAPPGC;
GC     CAPPWindowGC;
int CAPPDepth;
int CAPPScreen;
Visual * CAPPVisual;
VisualID CAPPVisualID;

#ifdef CAPPOGL
#include <GL/glx.h>
#include <GL/glxext.h>

GLXContext CAPPCtx;
void * CAPPGetExtension( const char * extname ) { return (void*)glXGetProcAddressARB((const GLubyte *) extname); }
GLXFBConfig CAPPGLXFBConfig;


void CAPPGLXSetup( )
{
	int attribs[] = { 
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, True,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DEPTH_SIZE, 1,
		None };
	int elements = 0;
	GLXFBConfig * cfgs = glXChooseFBConfig(	CAPPDisplay, CAPPScreen, attribs, &elements );
	if( elements == 0 )
	{
		fprintf( stderr, "Error: could not get valid GLXFBConfig visual.\n" );
		exit( -1 );
	}
	CAPPGLXFBConfig = cfgs[0];
	XVisualInfo * vis = glXGetVisualFromFBConfig( CAPPDisplay, CAPPGLXFBConfig );
	CAPPVisual = vis->visual;
	CAPPVisualID = vis->visualid;
	CAPPDepth = vis->depth;
	CAPPCtx = glXCreateContext( CAPPDisplay, vis, NULL, True );
}

#endif

int CAPPX11ForceNoDecoration;
XImage *xi;

int g_x_global_key_state;
int g_x_global_shift_key;

void 	CAPPSetWindowIconData( int w, int h, uint32_t * data )
{
	static Atom net_wm_icon;
	static Atom cardinal; 

	if( !net_wm_icon ) net_wm_icon = XInternAtom( CAPPDisplay, "_NET_WM_ICON", False );
	if( !cardinal ) cardinal = XInternAtom( CAPPDisplay, "CARDINAL", False );

	unsigned long outdata[w*h];
	int i;
	for( i = 0; i < w*h; i++ )
	{
		outdata[i+2] = data[i];
	}
	outdata[0] = w;
	outdata[1] = h;
	XChangeProperty(CAPPDisplay, CAPPWindow, net_wm_icon, cardinal,
		32, PropModeReplace, (const unsigned char*)outdata, 2 + w*h);
}


#ifdef CAPP_HAS_XSHAPE
void	CAPPPrepareForTransparency() { prepare_xshape = 1; }
void	CAPPDrawToTransparencyMode( int transp )
{
	static Pixmap BackupCAPPPixmap;
	static GC     BackupCAPPGC;
	if( was_transp && ! transp )
	{
		CAPPGC = BackupCAPPGC;
		CAPPPixmap = BackupCAPPPixmap;
	}
	if( !was_transp && transp )
	{
		BackupCAPPPixmap = CAPPPixmap;
		BackupCAPPGC = CAPPGC;
		taint_shape = 1;
		CAPPGC = xsgc;
		CAPPPixmap = xspixmap;
	}
	was_transp = transp;
}
void	CAPPClearTransparencyLevel()
{
	taint_shape = 1;
	XSetForeground(CAPPDisplay, xsgc, 0);
	XFillRectangle(CAPPDisplay, xspixmap, xsgc, 0, 0, CAPPWinAtt.width, CAPPWinAtt.height);
	XSetForeground(CAPPDisplay, xsgc, 1);
}
#endif

int FullScreen = 0;

void CAPPGetDimensions( short * x, short * y )
{
	static int lastx;
	static int lasty;

	*x = CAPPWinAtt.width;
	*y = CAPPWinAtt.height;

	if( lastx != *x || lasty != *y )
	{
		lastx = *x;
		lasty = *y;
#ifndef CAPPCONTEXTONLY
		CAPPInternalResize( lastx, lasty );
#endif
	}
}

void	CAPPChangeWindowTitle( const char * WindowName )
{
	XSetStandardProperties( CAPPDisplay, CAPPWindow, WindowName, 0, 0, 0, 0, 0 );
}

static void InternalLinkScreenAndGo( const char * WindowName )
{
	XFlush(CAPPDisplay);
	XGetWindowAttributes( CAPPDisplay, CAPPWindow, &CAPPWinAtt );

	if( !wm_res_name ) wm_res_name = strdup( "capp" );
	if( !wm_res_class ) wm_res_class = strdup( "capp" );

	XGetClassHint( CAPPDisplay, CAPPWindow, CAPPClassHint );
	if (!CAPPClassHint) {
		CAPPClassHint = XAllocClassHint();
		if (CAPPClassHint) {
			CAPPClassHint->res_name = wm_res_name;
			CAPPClassHint->res_class = wm_res_class;
			XSetClassHint( CAPPDisplay, CAPPWindow, CAPPClassHint );
		} else {
			fprintf( stderr, "Failed to allocate XClassHint!\n" );
		}
	} else {
		fprintf( stderr, "Pre-existing XClassHint\n" );
	}

	XSelectInput (CAPPDisplay, CAPPWindow, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ExposureMask | PointerMotionMask );


	CAPPWindowGC = XCreateGC(CAPPDisplay, CAPPWindow, 0, 0);


	if( CAPPX11ForceNoDecoration )
	{
		Atom window_type = XInternAtom(CAPPDisplay, "_NET_WM_WINDOW_TYPE", False);
		long value = XInternAtom(CAPPDisplay, "_NET_WM_WINDOW_TYPE_SPLASH", False);
		XChangeProperty(CAPPDisplay, CAPPWindow, window_type,
		   XA_ATOM, 32, PropModeReplace, (unsigned char *) &value,1 );
	}

	CAPPPixmap = XCreatePixmap( CAPPDisplay, CAPPWindow, CAPPWinAtt.width, CAPPWinAtt.height, CAPPWinAtt.depth );
	CAPPGC = XCreateGC(CAPPDisplay, CAPPPixmap, 0, 0);
	XSetLineAttributes(CAPPDisplay, CAPPGC, 1, LineSolid, CapRound, JoinRound);
	CAPPChangeWindowTitle( WindowName );
	if( !CAPPWindowInvisible )
		XMapWindow(CAPPDisplay, CAPPWindow);

#ifdef CAPP_HAS_XSHAPE
	if( prepare_xshape )
	{
	    xsval.foreground = 1;
	    xsval.line_width = 1;
	    xsval.line_style = LineSolid;
	    xspixmap = XCreatePixmap(CAPPDisplay, CAPPWindow, CAPPWinAtt.width, CAPPWinAtt.height, 1);
	    xsgc = XCreateGC(CAPPDisplay, xspixmap, 0, &xsval);
		XSetLineAttributes(CAPPDisplay, xsgc, 1, LineSolid, CapRound, JoinRound);
	}
#endif
}

void CAPPSetupFullscreen( const char * WindowName, int screen_no )
{
#ifdef HAS_XINERAMA
	XineramaScreenInfo *screeninfo = NULL;
	int screens;
	int event_basep, error_basep, a, b;
	CAPPDisplay = XOpenDisplay(NULL);
	CAPPScreen = XDefaultScreen(CAPPDisplay);
	int xpos, ypos;

	if (!XShapeQueryExtension(CAPPDisplay, &event_basep, &error_basep)) {
		fprintf( stderr, "X-Server does not support shape extension\n" );
		exit( 1 );
	}

 	CAPPVisual = DefaultVisual(CAPPDisplay, CAPPScreen);
	CAPPVisualID = 0;
	CAPPWinAtt.depth = DefaultDepth(CAPPDisplay, CAPPScreen);

#ifdef CAPPOGL
	CAPPGLXSetup();
#endif

	if (XineramaQueryExtension(CAPPDisplay, &a, &b ) &&
		(screeninfo = XineramaQueryScreens(CAPPDisplay, &screens)) &&
		XineramaIsActive(CAPPDisplay) && screen_no >= 0 &&
		screen_no < screens ) {

		CAPPWinAtt.width = screeninfo[screen_no].width;
		CAPPWinAtt.height = screeninfo[screen_no].height;
		xpos = screeninfo[screen_no].x_org;
		ypos = screeninfo[screen_no].y_org;
	} else
	{
		CAPPWinAtt.width = XDisplayWidth(CAPPDisplay, CAPPScreen);
		CAPPWinAtt.height = XDisplayHeight(CAPPDisplay, CAPPScreen);
		xpos = 0;
		ypos = 0;
	}
	if (screeninfo)
	XFree(screeninfo);


	XSetWindowAttributes setwinattr;
	setwinattr.override_redirect = 1;
	setwinattr.save_under = 1;
#ifdef CAPP_HAS_XSHAPE

	if (prepare_xshape && !XShapeQueryExtension(CAPPDisplay, &event_basep, &error_basep))
	{
    	fprintf( stderr, "X-Server does not support shape extension" );
		exit( 1 );
	}

	setwinattr.event_mask = 0;
#else
	//This code is probably made irrelevant by the XSetEventMask in InternalLinkScreenAndGo, if this code is not found needed by 2019-12-31, please remove.
	//setwinattr.event_mask = StructureNotifyMask | SubstructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonPressMask | PointerMotionMask | ButtonMotionMask | EnterWindowMask | LeaveWindowMask |KeyPressMask |KeyReleaseMask | SubstructureNotifyMask | FocusChangeMask;
#endif
	setwinattr.border_pixel = 0;
	setwinattr.colormap = XCreateColormap( CAPPDisplay, RootWindow(CAPPDisplay, 0), CAPPVisual, AllocNone);

	CAPPWindow = XCreateWindow(CAPPDisplay, XRootWindow(CAPPDisplay, CAPPScreen),
		xpos, ypos, CAPPWinAtt.width, CAPPWinAtt.height,
		0, CAPPWinAtt.depth, InputOutput, CAPPVisual, 
		CWBorderPixel/* | CWEventMask */ | CWOverrideRedirect | CWSaveUnder | CWColormap, 
		&setwinattr);

	FullScreen = 1;
	InternalLinkScreenAndGo( WindowName );
#ifdef CAPPOGL
	glXMakeCurrent( CAPPDisplay, CAPPWindow, CAPPCtx );
#endif
#ifdef CAPP_BATCH
#ifndef CAPPCONTEXTONLY
	CAPPSetupBatchInternal();
#endif
#endif

#else
	CAPPSetup( WindowName, 640, 480 );
#endif
}


void CAPPTearDown()
{
	HandleDestroy();
	if( xi ) free( xi );
	if ( CAPPClassHint ) XFree( CAPPClassHint );
	if ( CAPPGC ) XFreeGC( CAPPDisplay, CAPPGC );
	if ( CAPPWindowGC ) XFreeGC( CAPPDisplay, CAPPWindowGC );
	if ( CAPPDisplay ) XCloseDisplay( CAPPDisplay );
	CAPPDisplay = NULL;
	CAPPWindowGC = CAPPGC = NULL;
	CAPPClassHint = NULL;
}

int CAPPSetupWMClass( const char * WindowName, int w, int h , char * wm_res_name_ , char * wm_res_class_ )
{
	wm_res_name = wm_res_name_;
	wm_res_class = wm_res_class_;
	return CAPPSetup( WindowName, w, h);
}

int CAPPSetup( const char * WindowName, int w, int h )
{
	CAPPDisplay = XOpenDisplay(NULL);
	if ( !CAPPDisplay ) {
		fprintf( stderr, "Could not get an X Display.\n%s", 
				 "Are you in text mode or using SSH without X11-Forwarding?\n" );
		exit( 1 );
	}
	atexit( CAPPTearDown );

	CAPPScreen = DefaultScreen(CAPPDisplay);
	CAPPDepth = DefaultDepth(CAPPDisplay, CAPPScreen);
 	CAPPVisual = DefaultVisual(CAPPDisplay, CAPPScreen);
	CAPPVisualID = 0;
	Window wnd = DefaultRootWindow( CAPPDisplay );

#ifdef CAPPOGL
	CAPPGLXSetup( );
#endif

	XSetWindowAttributes attr;
	attr.background_pixel = 0;
	attr.colormap = XCreateColormap( CAPPDisplay, wnd, CAPPVisual, AllocNone);
	if( w  > 0 && h > 0 )
		CAPPWindow = XCreateWindow(CAPPDisplay, wnd, 1, 1, w, h, 0, CAPPDepth, InputOutput, CAPPVisual, CWBackPixel | CWColormap, &attr );
	else
	{
		if( w < 0 ) w = -w;
		if( h < 0 ) h = -h;
		CAPPWindow = XCreateWindow(CAPPDisplay, wnd, 1, 1, w, h, 0, CAPPDepth, InputOutput, CAPPVisual, CWBackPixel | CWColormap, &attr );
		CAPPWindowInvisible = 1;
	}

	InternalLinkScreenAndGo( WindowName );

//Not sure of the purpose of this code - if it's still commented out after 2019-12-31 and no one knows why, please delete it.
	Atom WM_DELETE_WINDOW = XInternAtom( CAPPDisplay, "WM_DELETE_WINDOW", False );
	XSetWMProtocols( CAPPDisplay, CAPPWindow, &WM_DELETE_WINDOW, 1 );

#ifdef CAPPOGL
	glXMakeCurrent( CAPPDisplay, CAPPWindow, CAPPCtx );
#endif

#ifdef CAPP_BATCH
#ifndef CAPPCONTEXTONLY
	CAPPSetupBatchInternal();
#endif
#endif

	return 0;
}

int CAPPHandleInput()
{
	if( !CAPPWindow ) return 0;
	static int ButtonsDown;
	XEvent report;

	int bKeyDirection = 1;
	while( XPending( CAPPDisplay ) )
	{
		XNextEvent( CAPPDisplay, &report );

		bKeyDirection = 1;
		switch  (report.type)
		{
		case NoExpose:
			break;
		case Expose:
			XGetWindowAttributes( CAPPDisplay, CAPPWindow, &CAPPWinAtt );
			if( CAPPPixmap ) XFreePixmap( CAPPDisplay, CAPPPixmap );
			CAPPPixmap = XCreatePixmap( CAPPDisplay, CAPPWindow, CAPPWinAtt.width, CAPPWinAtt.height, CAPPWinAtt.depth );
			if( CAPPGC ) XFreeGC( CAPPDisplay, CAPPGC );
			CAPPGC = XCreateGC(CAPPDisplay, CAPPPixmap, 0, 0);
			HandleKey( CAPP_X11_EXPOSE, 0 );
			break;
		case KeyRelease:
		{
			bKeyDirection = 0;
			//Tricky - handle key repeats cleanly.
			if( XPending( CAPPDisplay ) )
			{
				XEvent nev;
				XPeekEvent( CAPPDisplay, &nev );
				if (nev.type == KeyPress && nev.xkey.time == report.xkey.time && nev.xkey.keycode == report.xkey.keycode )
					bKeyDirection = 2;
			}
		}
		case KeyPress:
			g_x_global_key_state = report.xkey.state;
			g_x_global_shift_key = XLookupKeysym(&report.xkey, 1);
			HandleKey( XLookupKeysym(&report.xkey, 0), bKeyDirection );
			break;
		case ButtonRelease:
			bKeyDirection = 0;
		case ButtonPress:
			HandleButton( report.xbutton.x, report.xbutton.y, report.xbutton.button, bKeyDirection );
			ButtonsDown = (ButtonsDown & (~(1<<report.xbutton.button))) | ( bKeyDirection << report.xbutton.button );

			//Intentionall fall through -- we want to send a motion in event of a button as well.
		case MotionNotify:
			HandleMotion( report.xmotion.x, report.xmotion.y, ButtonsDown>>1 );
			break;
		case ClientMessage:
			// Only subscribed to WM_DELETE_WINDOW, so return 0 to let user know of window exit
			return 0;
			break;
		default:
			break;
			//printf( "Event: %d\n", report.type );
		}
	}
	return 1;
}


#ifdef CAPPOGL

void   CAPPSetVSync( int vson )
{
	void (*glfn)( int );
	glfn = (void (*)( int ))CAPPGetExtension( "glXSwapIntervalMESA" );	if( glfn ) glfn( vson );
	glfn = (void (*)( int ))CAPPGetExtension( "glXSwapIntervalSGI" );	if( glfn ) glfn( vson );
	glfn = (void (*)( int ))CAPPGetExtension( "glXSwapIntervalEXT" );	if( glfn ) glfn( vson );
}

#ifdef CAPPRASTERIZER
void CAPPSwapBuffersInternal()
#else
void CAPPSwapBuffers()
#endif
{
	if( CAPPWindowInvisible ) return;

#ifndef CAPPRASTERIZER
#ifndef CAPPCONTEXTONLY
	CAPPFlushRender();
#endif
#endif

#ifdef CAPP_HAS_XSHAPE
	if( taint_shape )
	{
		XShapeCombineMask(CAPPDisplay, CAPPWindow, ShapeBounding, 0, 0, xspixmap, ShapeSet);
		taint_shape = 0;
	}
#endif //CAPP_HAS_XSHAPE
	glXSwapBuffers( CAPPDisplay, CAPPWindow );

#ifdef FULL_SCREEN_STEAL_FOCUS
	if( FullScreen )
		XSetInputFocus( CAPPDisplay, CAPPWindow, RevertToParent, CurrentTime );
#endif //FULL_SCREEN_STEAL_FOCUS
}

#else //CAPPOGL

#ifndef CAPPRASTERIZER
void CAPPBlitImage( uint32_t * data, int x, int y, int w, int h )
{
	static int lw, lh;

	if( lw != w || lh != h || !xi )
	{
		if( xi ) free( xi );
		xi = XCreateImage(CAPPDisplay, CAPPVisual, CAPPDepth, ZPixmap, 0, (char*)data, w, h, 32, w*4 );
		lw = w;
		lh = h;
	}
	//Draw image to pixmap (not a screen flip)
	XPutImage(CAPPDisplay, CAPPPixmap, CAPPGC, xi, 0, 0, x, y, w, h );
}
#endif

void CAPPUpdateScreenWithBitmap( uint32_t * data, int w, int h )
{
	static int lw, lh;

	if( lw != w || lh != h )
	{
		if( xi ) free( xi );
		xi = XCreateImage(CAPPDisplay, CAPPVisual, CAPPDepth, ZPixmap, 0, (char*)data, w, h, 32, w*4 );
		lw = w;
		lh = h;
	}

	//Directly write image to screen (effectively a flip)
	XPutImage(CAPPDisplay, CAPPWindow, CAPPWindowGC, xi, 0, 0, 0, 0, w, h );
}

#endif //CAPPOGL

#if !defined( CAPPOGL)
#define AGLF(x) x
#else
#define AGLF(x) static inline BACKEND_##x
#endif

#if defined( CAPPRASTERIZER ) 
#include "CAPPRasterizer.c"
#undef AGLF
#define AGLF(x) static inline BACKEND_##x
#endif

uint32_t AGLF(CAPPColor)( uint32_t RGB )
{
	CAPPLastColor = RGB;
	unsigned char red = ( RGB >> 24 ) & 0xFF;
	unsigned char grn = ( RGB >> 16 ) & 0xFF;
	unsigned char blu = ( RGB >> 8 ) & 0xFF;
	unsigned long color = (red<<16)|(grn<<8)|(blu);
	XSetForeground(CAPPDisplay, CAPPGC, color);
	return color;
}

void AGLF(CAPPClearFrame)()
{
	XGetWindowAttributes( CAPPDisplay, CAPPWindow, &CAPPWinAtt );
	XSetForeground(CAPPDisplay, CAPPGC, CAPPColor(CAPPBGColor) );	
	XFillRectangle(CAPPDisplay, CAPPPixmap, CAPPGC, 0, 0, CAPPWinAtt.width, CAPPWinAtt.height );
}

void AGLF(CAPPSwapBuffers)()
{
#ifdef CAPP_HAS_XSHAPE
	if( taint_shape )
	{
		XShapeCombineMask(CAPPDisplay, CAPPWindow, ShapeBounding, 0, 0, xspixmap, ShapeSet);
		taint_shape = 0;
	}
#endif
	XCopyArea(CAPPDisplay, CAPPPixmap, CAPPWindow, CAPPWindowGC, 0,0,CAPPWinAtt.width,CAPPWinAtt.height,0,0);
	XFlush(CAPPDisplay);
#ifdef FULL_SCREEN_STEAL_FOCUS
	if( FullScreen )
		XSetInputFocus( CAPPDisplay, CAPPWindow, RevertToParent, CurrentTime );
#endif
}

void AGLF(CAPPTackSegment)( short x1, short y1, short x2, short y2 )
{
	if( x1 == x2 && y1 == y2 )
	{
		//On some targets, zero-length lines will not show up.
		//This is tricky - since this will also cause more draw calls for points on systems like GLAMOR.
		XDrawPoint( CAPPDisplay, CAPPPixmap, CAPPGC, x2, y2 );
	}
	else
	{
		//XXX HACK!  See discussion here: https://github.com/cntools/cnping/issues/68
		XDrawLine( CAPPDisplay, CAPPPixmap, CAPPGC, x1, y1, x2, y2 );
		XDrawLine( CAPPDisplay, CAPPPixmap, CAPPGC, x2, y2, x1, y1 );
	}
}

void AGLF(CAPPTackPixel)( short x1, short y1 )
{
	XDrawPoint( CAPPDisplay, CAPPPixmap, CAPPGC, x1, y1 );
}

void AGLF(CAPPTackRectangle)( short x1, short y1, short x2, short y2 )
{
	XFillRectangle(CAPPDisplay, CAPPPixmap, CAPPGC, x1, y1, x2-x1, y2-y1 );
}

void AGLF(CAPPTackPoly)( RDPoint * points, int verts )
{
	XFillPolygon(CAPPDisplay, CAPPPixmap, CAPPGC, (XPoint *)points, verts, Convex, CoordModeOrigin );
}

void AGLF(CAPPInternalResize)( short x, short y ) { }

void AGLF(CAPPSetLineWidth)( short width )
{
	XSetLineAttributes(CAPPDisplay, CAPPGC, width, LineSolid, CapRound, JoinRound);
}

#endif // _CAPPXDRIVER_C

