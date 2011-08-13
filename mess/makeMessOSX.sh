export PATH=~/Libraries/sdl/out/bin:$PATH
nice nice make -j2 MACOSX_USE_LIBSDL=1 PTR64=1 NOWERROR=1 TARGET=mess
cp mess64 ~/mamehub/Mac/src/osxmess64
cp mess64 ../mame/Hub/src/osxmess64
