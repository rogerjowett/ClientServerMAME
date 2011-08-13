set PATH=C:\mingw-mame\mingw64-w32\bin;%PATH%
make -j8 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=1 TARGET=mame
make -j8 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=1 TARGET=mame DEBUG=1 SYMBOLS=1
copy vmame.exe Hub\src\
copy vmame.pdb Hub\src\
copy vmame.exe Installer\ClientServerMAME\
copy vmame.pdb Installer\ClientServerMAME\

