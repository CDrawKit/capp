//Don't call this file yourself.  It is intended to be included in any drivers which want to support the rasterizer plugin.

#ifdef CAPPRASTERIZER
#include "CAPP.h"
//#include <stdlib.h>
#include <stdint.h>

uint32_t * CAPPBuffer = 0;
short CAPPBufferx;
short CAPPBuffery;

#ifdef CAPPOGL
void CAPPFlushRender()
{
}
#endif

void CAPPInternalResize( short x, short y )
{
	CAPPBufferx = x;
	CAPPBuffery = y;
	if( CAPPBuffer ) free( CAPPBuffer );
	CAPPBuffer = malloc( CAPPBufferx * CAPPBuffery * 4 );
#ifdef CAPPOGL
	void CAPPInternalResizeOGLBACKEND( short w, short h );
	CAPPInternalResizeOGLBACKEND( x, y );
#endif
}

#ifdef __wasm__
static uint32_t SWAPS( uint32_t r )
{
	uint32_t ret = (r&0xFF)<<24;
	r>>=8;
	ret |= (r&0xff)<<16;
	r>>=8;
	ret |= (r&0xff)<<8;
	r>>=8;
	ret |= (r&0xff)<<0;
	return ret;
}
#elif !defined(CAPPOGL)
#define SWAPS(x) (x>>8)
#else
static uint32_t SWAPS( uint32_t r )
{
	uint32_t ret = (r&0xFF)<<16;
	r>>=8;
	ret |= (r&0xff)<<8;
	r>>=8;
	ret |= (r&0xff);
	r>>=8;
	ret |= (r&0xff)<<24;
	return ret;
}
#endif
uint32_t CAPPColor( uint32_t RGB )
{
	CAPPLastColor = SWAPS(RGB);
	return CAPPLastColor;
}

void CAPPTackSegment( short x1, short y1, short x2, short y2 )
{
	short tx, ty;
	//float slope, lp;
	float slope;

	short dx = x2 - x1;
	short dy = y2 - y1;

	if( !CAPPBuffer ) return;

	if( dx < 0 ) dx = -dx;
	if( dy < 0 ) dy = -dy;

	if( dx > dy )
	{
		short minx = (x1 < x2)?x1:x2;
		short maxx = (x1 < x2)?x2:x1;
		short miny = (x1 < x2)?y1:y2;
		short maxy = (x1 < x2)?y2:y1;
		float thisy = miny;
		slope = (float)(maxy-miny) / (float)(maxx-minx);

		for( tx = minx; tx <= maxx; tx++ )
		{
			ty = thisy;
			if( tx < 0 || ty < 0 || ty >= CAPPBuffery ) continue;
			if( tx >= CAPPBufferx ) break;
			CAPPBuffer[ty * CAPPBufferx + tx] = CAPPLastColor;
			thisy += slope;
		}
	}
	else
	{
		short minx = (y1 < y2)?x1:x2;
		short maxx = (y1 < y2)?x2:x1;
		short miny = (y1 < y2)?y1:y2;
		short maxy = (y1 < y2)?y2:y1;
		float thisx = minx;
		slope = (float)(maxx-minx) / (float)(maxy-miny);

		for( ty = miny; ty <= maxy; ty++ )
		{
			tx = thisx;
			if( ty < 0 || tx < 0 || tx >= CAPPBufferx ) continue;
			if( ty >= CAPPBuffery ) break;
			CAPPBuffer[ty * CAPPBufferx + tx] = CAPPLastColor;
			thisx += slope;
		}
	}
}
void CAPPTackRectangle( short x1, short y1, short x2, short y2 )
{
	short minx = (x1<x2)?x1:x2;
	short miny = (y1<y2)?y1:y2;
	short maxx = (x1>=x2)?x1:x2;
	short maxy = (y1>=y2)?y1:y2;

	short x, y;

	if( minx < 0 ) minx = 0;
	if( miny < 0 ) miny = 0;
	if( maxx >= CAPPBufferx ) maxx = CAPPBufferx-1;
	if( maxy >= CAPPBuffery ) maxy = CAPPBuffery-1;

	for( y = miny; y <= maxy; y++ )
	{
		uint32_t * CAPPBufferstart = &CAPPBuffer[y * CAPPBufferx + minx];
		for( x = minx; x <= maxx; x++ )
		{
			(*CAPPBufferstart++) = CAPPLastColor;
		}
	}
}

void CAPPTackPoly( RDPoint * points, int verts )
{
	short minx = 10000, miny = 10000;
	short maxx =-10000, maxy =-10000;
	short i, x, y;

	//Just in case...
	if( verts > 32767 ) return;

	for( i = 0; i < verts; i++ )
	{
		RDPoint * p = &points[i];
		if( p->x < minx ) minx = p->x;
		if( p->y < miny ) miny = p->y;
		if( p->x > maxx ) maxx = p->x;
		if( p->y > maxy ) maxy = p->y;
	}

	if( miny < 0 ) miny = 0;
	if( maxy >= CAPPBuffery ) maxy = CAPPBuffery-1;

	for( y = miny; y <= maxy; y++ )
	{
		short startfillx = maxx;
		short endfillx = minx;

		//Figure out what line segments intersect this line.
		for( i = 0; i < verts; i++ )
		{
			short pl = i + 1;
			if( pl == verts ) pl = 0;

			RDPoint ptop;
			RDPoint pbot;

			ptop.x = points[i].x;
			ptop.y = points[i].y;
			pbot.x = points[pl].x;
			pbot.y = points[pl].y;
//printf( "Poly: %d %d\n", pbot.y, ptop.y );

			if( pbot.y < ptop.y )
			{
				RDPoint ptmp;
				ptmp.x = pbot.x;
				ptmp.y = pbot.y;
				pbot.x = ptop.x;
				pbot.y = ptop.y;
				ptop.x = ptmp.x;
				ptop.y = ptmp.y;
			}

			//Make sure this line segment is within our range.
//printf( "PT: %d %d %d\n", y, ptop.y, pbot.y );
			if( ptop.y <= y && pbot.y >= y )
			{
				short diffy = pbot.y - ptop.y;
				uint32_t placey = (uint32_t)(y - ptop.y)<<16;  //Scale by 16 so we can do integer math.
				short diffx = pbot.x - ptop.x;
				short isectx;

				if( diffy == 0 )
				{
					if( pbot.x < ptop.x )
					{
						if( startfillx > pbot.x ) startfillx = pbot.x;
						if( endfillx < ptop.x ) endfillx = ptop.x;
					}
					else
					{
						if( startfillx > ptop.x ) startfillx = ptop.x;
						if( endfillx < pbot.x ) endfillx = pbot.x;
					}
				}
				else
				{
					//Inner part is scaled by 65536, outer part must be scaled back.
					isectx = (( (placey / diffy) * diffx + 32768 )>>16) + ptop.x;
					if( isectx < startfillx ) startfillx = isectx;
					if( isectx > endfillx ) endfillx = isectx;
				}
//printf( "R: %d %d %d\n", pbot.x, ptop.x, isectx );
			}
		}

//printf( "%d %d %d\n", y, startfillx, endfillx );

		if( endfillx >= CAPPBufferx ) endfillx = CAPPBufferx - 1;
		if( endfillx >= CAPPBufferx ) endfillx = CAPPBuffery - 1;
		if( startfillx < 0 ) startfillx = 0;
		if( startfillx < 0 ) startfillx = 0;

		unsigned int * bufferstart = &CAPPBuffer[y * CAPPBufferx + startfillx];
		for( x = startfillx; x <= endfillx; x++ )
		{
			(*bufferstart++) = CAPPLastColor;
		}
	}
//exit(1);
}


void CAPPClearFrame()
{
	int i, m;
	uint32_t col = 0;
	short x, y;
	CAPPGetDimensions( &x, &y );
	if( x != CAPPBufferx || y != CAPPBuffery || !CAPPBuffer )
	{
		CAPPBufferx = x;
		CAPPBuffery = y;
		CAPPBuffer = malloc( x * y * 8 );
	}

	m = x * y;
	col = CAPPColor( CAPPBGColor );
	for( i = 0; i < m; i++ )
	{
		CAPPBuffer[i] = col;
	}
}

void CAPPTackPixel( short x, short y )
{
	if( x < 0 || y < 0 || x >= CAPPBufferx || y >= CAPPBuffery ) return;
	CAPPBuffer[x+CAPPBufferx*y] = CAPPLastColor;
}


void CAPPBlitImage( uint32_t * data, int x, int y, int w, int h )
{
	int ox = x;
	int stride = w;
	if( w <= 0 || h <= 0 || x >= CAPPBufferx || y >= CAPPBuffery ) return;
	if( x < 0 ) { w += x; x = 0; }
	if( y < 0 ) { h += y; y = 0; }

	//Switch w,h to x2, y2
	h += y;
	w += x;

	if( w >= CAPPBufferx ) { w = CAPPBufferx; }
	if( h >= CAPPBuffery ) { h = CAPPBuffery; }


	for( ; y < h-1; y++ )
	{
		x = ox;
		uint32_t * indat = data;
		uint32_t * outdat = CAPPBuffer + y * CAPPBufferx + x;
		for( ; x < w-1; x++ )
		{
			uint32_t newm = *(indat++);
			uint32_t oldm = *(outdat);
			if( (newm & 0xff) == 0xff )
			{
				*(outdat++) = newm;
			}
			else
			{
				//Alpha blend.
				int alfa = newm&0xff;
				int onemalfa = 255-alfa;
#ifdef __wasm__
				uint32_t newv = 255<<0; //Alpha, then RGB
				newv |= ((((newm>>24)&0xff) * alfa + ((oldm>>24)&0xff) * onemalfa + 128)>>8)<<24;
				newv |= ((((newm>>16)&0xff) * alfa + ((oldm>>16)&0xff) * onemalfa + 128)>>8)<<16;
				newv |= ((((newm>>8)&0xff) * alfa + ((oldm>>8)&0xff) * onemalfa + 128)>>8)<<8;
#elif defined(WINDOWS) || defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
				uint32_t newv = 255UL<<24; //Alpha, then RGB
				newv |= ((((newm>>24)&0xff) * alfa + ((oldm>>16)&0xff) * onemalfa + 128)>>8)<<16;
				newv |= ((((newm>>16)&0xff) * alfa + ((oldm>>8)&0xff) * onemalfa + 128)>>8)<<8;
				newv |= ((((newm>>8)&0xff) * alfa + ((oldm>>0)&0xff) * onemalfa + 128)>>8)<<0;
#elif defined( ANDROID ) || defined( __android__ )
				uint32_t newv = 255<<16; //Alpha, then RGB
				newv |= ((((newm>>24)&0xff) * alfa + ((oldm>>24)&0xff) * onemalfa + 128)>>8)<<24;
				newv |= ((((newm>>16)&0xff) * alfa + ((oldm>>0)&0xff) * onemalfa + 128)>>8)<<0;
				newv |= ((((newm>>8)&0xff) * alfa + ((oldm>>8)&0xff) * onemalfa + 128)>>8)<<8;
#elif defined( CAPPOGL ) //OGL, on X11
				uint32_t newv = 255<<16; //Alpha, then RGB
				newv |= ((((newm>>24)&0xff) * alfa + ((oldm>>24)&0xff) * onemalfa + 128)>>8)<<24;
				newv |= ((((newm>>16)&0xff) * alfa + ((oldm>>0)&0xff) * onemalfa + 128)>>8)<<0;
				newv |= ((((newm>>8)&0xff) * alfa + ((oldm>>8)&0xff) * onemalfa + 128)>>8)<<8;
#else //X11
				uint32_t newv = 255UL<<24; //Alpha, then RGB
				newv |= ((((newm>>24)&0xff) * alfa + ((oldm>>16)&0xff) * onemalfa + 128)>>8)<<16;
				newv |= ((((newm>>16)&0xff) * alfa + ((oldm>>8)&0xff) * onemalfa + 128)>>8)<<8;
				newv |= ((((newm>>8)&0xff) * alfa + ((oldm>>0)&0xff) * onemalfa + 128)>>8)<<0;
#endif
				*(outdat++) = newv;
			}
		}
		data += stride;
	}
}

void CAPPSwapBuffers()
{
	CAPPUpdateScreenWithBitmap( (uint32_t*)CAPPBuffer, CAPPBufferx, CAPPBuffery );
}


#endif
