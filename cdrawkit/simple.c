//Make it so we don't need to include any other C files in our build.
#define CAPP_IMPLEMENTATION

//Optional: Use OpenGL to do rendering on appropriate systems.
#define CAPPOGL

#include "CAPP.h"

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }
int main()
{
	CAPPSetup( "Example App", 1024, 768 );

	while(CAPPHandleInput())
	{
		CAPPBGColor = 0x000080ff; //Dark Blue Background

		short w, h;
		CAPPClearFrame();
		CAPPGetDimensions( &w, &h );

		//Change color to white.
		CAPPColor( 0xffffffff ); 

		CAPPPenX = 1; CAPPPenY = 1;
		CAPPDrawText( "Hello, World", 2 );
		//Draw a white pixel at 3,0 30 
		CAPPTackPixel( 30, 30 );         

		//Draw a line from 50,50 to 100,50
		CAPPTackSegment( 50, 50, 100, 50 );

		//Dark Red Color Select
		CAPPColor( 0x800000ff ); 

		//Draw 50x50 box starting at 100,50
		CAPPTackRectangle( 100, 50, 150, 100 ); 

		//Draw a triangle
		RDPoint points[3] = { { 30, 36 }, { 20, 50 }, { 40, 50 } };
		CAPPTackPoly( points, 3 );

		// Blit random pixel data
		{
			static uint32_t data[64*64];
			int x, y;

			for( y = 0; y < 64; y++ ) for( x = 0; x < 64; x++ )
				data[x+y*64] = 0xff | (rand()<<8);

			CAPPBlitImage( data, 120, 190, 64, 64 );
		}

		//Display the image and wait for time to display next frame.
		CAPPSwapBuffers();		
	}

	printf("exiting\n");
}
