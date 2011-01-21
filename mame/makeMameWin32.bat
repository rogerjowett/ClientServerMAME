set PATH=C:\mingw-mame\mingw64-w32\bin;%PATH%
make -j4 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=0 TARGET=mame
make -j4 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=0 TARGET=mame DEBUG=1 SYMBOLS=1
copy vmame.exe Hub\src\
copy vmame.pdb Hub\src\
copy vmame.exe Installer\ClientServerMAME\
copy vmame.pdb Installer\ClientServerMAME\

