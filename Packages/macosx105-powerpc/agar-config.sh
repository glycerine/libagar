#!/bin/sh
# Public domain

PREFIX="%PREFIX%"

if [ "$1" = "--version" ]; then echo "%INSTALLED_VERSION%"; fi
if [ "$1" = "--prefix" ]; then echo "${PREFIX}"; fi
if [ "$1" = "--exec-prefix" ]; then echo "${PREFIX}"; fi

if [ "$1" = "--cflags" ]; then
	echo "-D_GNU_SOURCE=1 -D_THREAD_SAFE -I${PREFIX}/include/agar -I${PREFIX}/include/SDL -I${PREFIX}/include/freetype2 -I${PREFIX}/include -I/usr/local/include/SDL -I/usr/local/include/freetype2 -I/usr/local/include -I/opt/local/include/SDL -I/opt/local/include/freetype2 -I/opt/local/include -I/usr/X11R6/include/SDL -I/usr/X11R6/include/freetype2 -I/usr/X11R6/include"
fi
if [ "$1" = "--libs" ]; then
	echo "-L${PREFIX}/lib -L/opt/local/lib -L/usr/local/lib -L/usr/X11R6/lib -lag_gui -lag_core -lSDLmain -lSDL -Wl,-framework,Cocoa -lfreetype -lz -Wl,-framework,CoreServices,-framework,ApplicationServices -framework OpenGL -lm -lpthread -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib"
fi
