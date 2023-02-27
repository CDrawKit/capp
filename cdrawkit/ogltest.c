#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>

#include "CAPP.h"

void HandleKey( int keycode, int bDown )
{
	printf( "Key: %d %d\n", keycode, bDown );
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}

void HandleDestroy()
{
	exit(10);
}

int main()
{
	short w, h;
	int frameno = 0;
	CAPPSetup( "Test", 100, 100 );

	while(CAPPHandleInput())
	{
		CAPPClearFrame();

		glRotatef( frameno/100.0, 0, 0, 1);

		glColor4f( 0, 0, .5, 1 );

		glBegin( GL_TRIANGLES );
		glVertex3f( -40, -40, 0 );
		glVertex3f( 40, 0, 0 );
		glVertex3f( 0, 40, 0 );
		glEnd();

		char stmp[100];
		CAPPColor( 0xFFFFFF );
		sprintf( stmp, "%d\n", frameno );
		CAPPPenX = 10; CAPPPenY = 10;
		CAPPDrawText( stmp, 3 );
		CAPPTackPixel( 100, 100 );

		CAPPSwapBuffers();
		frameno++;
	}
}
