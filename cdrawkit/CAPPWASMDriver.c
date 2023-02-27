//Right now, designed for use with https://github.com/cnlohr/cappwasm/
#include <CAPP.h>
#include <stdint.h>

extern void __attribute__((import_module("bynsyncify"))) CAPPSwapBuffersInternal();
void CAPPBlitImageInternal( uint32_t * data, int x, int y, int w, int h );
void print( double idebug );
void prints( const char* sdebug );


//Forward declarations that we get from either WASM or our javascript code.
void CAPPClearFrameInternal( uint32_t bgcolor );

//The WASM driver handles internal resizing automatically.
#ifndef CAPPRASTERIZER

void	CAPPInternalResize( short x, short y )
{
}

void CAPPFlushRender()
{
	if( !CAPPVertPlace ) return;
	CAPPEmitBackendTriangles( CAPPVertDataV, CAPPVertDataC, CAPPVertPlace );
	CAPPVertPlace = 0;
}
void CAPPClearFrame()
{
	CAPPFlushRender();
	CAPPClearFrameInternal( CAPPBGColor );
}
void CAPPSwapBuffers()
{
	CAPPFlushRender();
	CAPPSwapBuffersInternal( );
}

int CAPPHandleInput()
{
	//Do nothing.
	//Input is handled on swap frame.
	return 1;
}

void CAPPBlitImage( uint32_t * data, int x, int y, int w, int h )
{
	CAPPBlitImageInternal( data, x, y, w, h );
}

#else
	
//Rasterizer - if you want to do this, you will need to enable blitting in the javascript.
//XXX TODO: NEED MEMORY ALLOCATOR
extern unsigned char __heap_base;
unsigned int bump_pointer = (unsigned int)&__heap_base;
void* malloc(unsigned long size) {
	unsigned int ptr = bump_pointer;
	bump_pointer += size;
	return (void *)ptr;
}
void free(void* ptr) {  }

#include "CAPPRasterizer.c"

extern void CAPPUpdateScreenWithBitmapInternal( uint32_t * data, int w, int h );
void CAPPUpdateScreenWithBitmap( uint32_t * data, int w, int h )
{
	CAPPBlitImageInternal( data, 0, 0, w, h );
	CAPPSwapBuffersInternal();
}


void	CAPPSetLineWidth( short width )
{
	//Rasterizer does not support line width.
}

void CAPPHandleInput()
{
	//Do nothing.
	//Input is handled on swap frame.
}

#endif
