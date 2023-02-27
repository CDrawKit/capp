//Include this file to get all of capp.  You usually will not
//want to include this in your build, but instead, #include "CAPP.h"
//after #define CAPP_IMPLEMENTATION in one of your C files.

#if defined( CAPPHTTP )
#include "CAPPHTTP.c"
#elif defined( __wasm__ )
#include "CAPPWASMDriver.c"
#elif defined(WINDOWS) || defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#include "CAPPWinDriver.c"
#elif defined( EGL_LEAN_AND_MEAN )
#include "CAPPEGLLeanAndMean.c"
#elif defined( __android__ ) || defined( ANDROID )
#include "CAPPEGLDriver.c"
#else
#include "CAPPXDriver.c"
#endif

#include "CAPPFunctions.c"

#ifdef CAPP3D
#include "CAPP3D.c"
#endif

