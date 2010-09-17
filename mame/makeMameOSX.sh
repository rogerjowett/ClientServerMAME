export PATH=~/Libraries/sdl/out/bin:$PATH
make -j2 MACOSX_USE_LIBSDL=1 NO_OPENGL=1 PTR64=1 TARGET=mame
