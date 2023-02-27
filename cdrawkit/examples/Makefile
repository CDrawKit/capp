all : fontsize fontsize_ogl

#for X11 consider:             xorg-dev
#for X11, you will need:       libx-dev
#for full screen you'll need:  libxinerama-dev libxext-dev
#for OGL You'll need:          mesa-common-dev libglu1-mesa-dev

#-DCAPPRASTERIZER
#  and
#-CAPPOGL
#  are incompatible.


MINGW32:=/usr/bin/i686-w64-mingw32-

fontsize : fontsize.c
	gcc -o $@ $^ -lX11 -lm -lpthread -lXinerama -lXext -lGL -g -DCAPPRASTERIZER -Wall

fontsize_ogl : fontsize.c
	gcc -o $@ $^ -lX11 -lm -lpthread -lXinerama -lXext -lGL -g -lX11 -lXinerama -lGL -DCAPPOGL -Wall

fontsize.exe : fontsize.c
	$(MINGW32)gcc -m32 -o $@ $^  -lgdi32 $(CFLAGS)

clean :
	rm -rf *.o *~ capp.exe capp ontop capp_mac capp_mac_soft capp_mac_cg capp_mac_ogl ogltest ogltest.exe capp_egl fontsize fontsize_ogl fontsize.exe

