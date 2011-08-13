export PATH=~/Libraries/sdl/out/bin:$PATH
nice nice make -j2 MACOSX_USE_LIBSDL=1 PTR64=1 NOWERROR=1 TARGET=mame
cp mame64 ~/mamehub/Mac/src/osxmame64
cp mame64 Hub/src/osxmame64

