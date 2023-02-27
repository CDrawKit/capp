#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CAPP_IMPLEMENTATION
#include "../CAPP.h"
#include "../os_generic.h"

#if defined( CAPPOGL )
#define TITLESTRING "Fontsize test with OpenGL"
#elif defined( CAPPRASTERIZER )
#define TITLESTRING "Fontsize test with Rasterizer"
#else
#define TITLESTRING "Fontsize test with Unknown"
#endif

void HandleKey( int keycode, int bDown )
{
	if( keycode == CAPP_KEY_ESCAPE ) exit( 0 );
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

int main()
{

	int i;

	CAPPBGColor = 0x800000;
	// Deprecated CAPPDialogColor = 0x444444;
	CAPPSetup( TITLESTRING, 1220, 480 );



	while (CAPPHandleInput()) {
		CAPPClearFrame();
		CAPPColor( 0xffffff );
		for( i = 1; i < 5; i++ )
		{
			int c;
			char tw[2] = { 0, 0 };
			for( c = 0; c < 256; c++ )
			{
				tw[0] = c;

				CAPPPenX = ( c % 16 ) * 16 + 5 + (i - 1) * 320;
				CAPPPenY = ( c / 16 ) * 18 + 5;
				CAPPDrawText( tw, i );
			}
		}

		CAPPPenX = 20;
		CAPPPenY = 300;
		for( i = 1; i < 7; i++) {
			CAPPPenY += i * 6;
			CAPPDrawText("A quick brown fox jumps over the lazy dog.", i);
		}
		CAPPSwapBuffers();
		OGUSleep( (int)( 0.5 * 1000000 ) );
	}
}
