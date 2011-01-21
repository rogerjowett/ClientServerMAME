set PATH=C:\mingw-mame\mingw64-w32\bin;%PATH%
make -j4 MESSUI=0 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=0 TARGET=mess
make -j4 MESSUI=0 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=0 TARGET=mess DEBUG=1 SYMBOLS=1
copy vmess.exe ..\mame\Hub\src\
copy vmess.pdb ..\mame\Hub\src\
copy vmess.exe Installer\ClientServerMESS\
copy vmess.pdb Installer\ClientServerMESS\

